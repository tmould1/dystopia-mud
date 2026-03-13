"""
story_navigator.py - Story quest state machine and area navigation.

Progresses through the 16-node "Echoes of the Sundering" story quest.
Each hub involves talking to NPCs, killing mobs, fetching objects, and
examining room features.  Story areas provide tougher mobs for multi-round
combat, which improves stance/weapon/spell skills.
"""
from __future__ import annotations

import asyncio
import logging
import re
from enum import Enum, auto
from typing import TYPE_CHECKING, Optional

from ..utils.story_data import STORY_NODE_MAP, StoryNode
from ..utils.quest_parser import StoryState

if TYPE_CHECKING:
    from .quest_bot import QuestBot
    from .actions import BotActions
    from .quest_actions import QuestActions

logger = logging.getLogger(__name__)


class StoryPhase(Enum):
    """Sub-states for story navigation."""
    CHECKING = auto()       # Parse story state to determine current node
    NAVIGATING = auto()     # Walking/teleporting to hub area
    TALKING_INTRO = auto()  # Say intro keyword to hub NPC
    KILLING = auto()        # Kill story mobs (multi-round combat)
    FETCHING = auto()       # Navigate to fetch room and pick up object
    EXAMINING = auto()      # Navigate to examine room and examine keyword
    TALKING_NPC = auto()    # Talk to a task NPC (not the hub NPC)
    RETURNING = auto()      # Return to hub NPC, say return keyword
    IDLE = auto()           # No story work — done or not started


class StoryNavigator:
    """
    Navigate the 16-node story quest.

    Usage from QuestBot:
        nav = StoryNavigator(bot)
        next_state = await nav.tick()
        # next_state is a QuestBotState name string or None
    """

    def __init__(self, bot: "QuestBot"):
        self.bot = bot
        self.actions: "BotActions" = bot.actions
        self.quest_actions: "QuestActions" = bot.quest_actions

        self._phase = StoryPhase.CHECKING
        self._story: StoryState = StoryState()

        # Track examine/talk task index (for nodes with multiple)
        self._examine_idx: int = 0
        self._talk_idx: int = 0

        # Combat tracking for story kills
        self._kill_attempts: int = 0
        self._max_kill_attempts: int = 50

    @property
    def current_node(self) -> int:
        return self._story.node

    async def tick(self) -> Optional[str]:
        """
        Advance one step in story progression.

        Returns a QuestBotState name if the bot should switch to that state,
        or None if story is idle/complete.
        """
        if self._phase == StoryPhase.CHECKING:
            return await self._check_story()
        elif self._phase == StoryPhase.NAVIGATING:
            return await self._navigate_to_hub()
        elif self._phase == StoryPhase.TALKING_INTRO:
            return await self._talk_intro()
        elif self._phase == StoryPhase.KILLING:
            return await self._do_kills()
        elif self._phase == StoryPhase.FETCHING:
            return await self._do_fetch()
        elif self._phase == StoryPhase.EXAMINING:
            return await self._do_examine()
        elif self._phase == StoryPhase.TALKING_NPC:
            return await self._do_talk_npc()
        elif self._phase == StoryPhase.RETURNING:
            return await self._do_return()
        elif self._phase == StoryPhase.IDLE:
            return None

        return None

    # ── State handlers ─────────────────────────────────────────────────

    async def _check_story(self) -> Optional[str]:
        """Check story state using storyadmin (immortal) or story (mortal)."""
        # Try storyadmin first for exact numeric state
        state = await self.actions.storyadmin(self.bot.config.name)
        if state.node == 0 and not state.not_started:
            # storyadmin may have failed (mortal char) — fall back
            state = await self.actions.story()

        self._story = state
        logger.info(f"[{self.bot.config.name}] Story: node={state.node}, "
                    f"kills={state.kills}, progress=0x{state.progress:02X}, "
                    f"tasks={state.tasks}")

        if state.not_started:
            logger.info(f"[{self.bot.config.name}] Story not started (need avatar)")
            self._phase = StoryPhase.IDLE
            return None

        if state.completed or state.node > 16:
            logger.info(f"[{self.bot.config.name}] Story complete!")
            self._phase = StoryPhase.IDLE
            return None

        # Check max_story_node config
        if state.node > self.bot.quest_config.max_story_node:
            logger.info(f"[{self.bot.config.name}] Reached max story node "
                        f"{self.bot.quest_config.max_story_node}")
            self._phase = StoryPhase.IDLE
            return None

        node_data = STORY_NODE_MAP.get(state.node)
        if not node_data:
            logger.warning(f"[{self.bot.config.name}] Unknown story node {state.node}")
            self._phase = StoryPhase.IDLE
            return None

        # Reset task indices for new navigation
        self._examine_idx = 0
        self._talk_idx = 0
        self._kill_attempts = 0

        # Navigate to the hub area
        self._phase = StoryPhase.NAVIGATING
        return "STORY_PROGRESSION"  # Stay in story loop

    async def _navigate_to_hub(self) -> Optional[str]:
        """Navigate to the current hub NPC's room."""
        node_data = STORY_NODE_MAP.get(self._story.node)
        if not node_data:
            self._phase = StoryPhase.IDLE
            return None

        npc_vnum = node_data.npc_vnum
        logger.info(f"[{self.bot.config.name}] Navigating to hub NPC "
                    f"{npc_vnum} ({node_data.area})")

        # Use goto (immortal) to teleport directly to the NPC's room
        # NPC vnum == room vnum in story data (NPCs are loaded in their home room)
        success = await self.actions.goto_room(npc_vnum)
        if not success:
            # Fallback: recall and try walking
            logger.warning(f"[{self.bot.config.name}] goto failed, trying recall")
            await self.actions.recall()
            await asyncio.sleep(1.0)

        await asyncio.sleep(0.5)

        # If progress == 0 (haven't talked to NPC yet), say intro keyword
        if self._story.progress == 0 and self._story.kills == 0:
            self._phase = StoryPhase.TALKING_INTRO
        else:
            # Already talked — figure out what task to do next
            self._determine_next_task(node_data)

        return None

    async def _talk_intro(self) -> Optional[str]:
        """Say the intro keyword to the hub NPC to start the node."""
        node_data = STORY_NODE_MAP.get(self._story.node)
        if not node_data:
            self._phase = StoryPhase.IDLE
            return None

        keyword = node_data.intro_keyword
        logger.info(f"[{self.bot.config.name}] Story: saying '{keyword}' "
                    f"to hub NPC (node {self._story.node})")
        await self.quest_actions.send_command(f"say {keyword}")
        await asyncio.sleep(2.0)

        if node_data.hub_type == "talk":
            # Talk-through node — saying the keyword advances to next node
            logger.info(f"[{self.bot.config.name}] Talk-through node complete")
            self._phase = StoryPhase.CHECKING
            return "STORY_PROGRESSION"

        # Hub node — now do tasks
        self._determine_next_task(node_data)
        return None

    def _determine_next_task(self, node_data: StoryNode) -> None:
        """Figure out which task to work on next based on progress bits."""
        tasks = self._story.tasks

        # Task bit 0x01: kills or first examine-only task
        if not tasks[0]:
            if node_data.kill_mobs and node_data.kill_threshold > 0:
                self._phase = StoryPhase.KILLING
            elif node_data.examine_rooms:
                self._examine_idx = 0
                self._phase = StoryPhase.EXAMINING
            else:
                # Some nodes have only fetch as first task
                self._phase = StoryPhase.FETCHING
            return

        # Task bit 0x02: fetch or second examine
        if not tasks[1]:
            if node_data.fetch_obj:
                self._phase = StoryPhase.FETCHING
            elif len(node_data.examine_rooms) > 1:
                self._examine_idx = 1
                self._phase = StoryPhase.EXAMINING
            else:
                self._phase = StoryPhase.RETURNING
            return

        # Task bit 0x04: examine or talk NPC
        if not tasks[2]:
            if node_data.examine_rooms:
                # Use last examine room
                self._examine_idx = len(node_data.examine_rooms) - 1
                self._phase = StoryPhase.EXAMINING
            elif node_data.talk_npcs:
                self._talk_idx = 0
                self._phase = StoryPhase.TALKING_NPC
            else:
                self._phase = StoryPhase.RETURNING
            return

        # All tasks done — return to hub NPC
        self._phase = StoryPhase.RETURNING

    async def _do_kills(self) -> Optional[str]:
        """Kill story mobs for the current hub."""
        node_data = STORY_NODE_MAP.get(self._story.node)
        if not node_data or not node_data.kill_mobs:
            self._phase = StoryPhase.CHECKING
            return "STORY_PROGRESSION"

        self._kill_attempts += 1
        if self._kill_attempts > self._max_kill_attempts:
            logger.warning(f"[{self.bot.config.name}] Too many kill attempts, "
                           "skipping to next task")
            self._phase = StoryPhase.CHECKING
            return "STORY_PROGRESSION"

        remaining = node_data.kill_threshold - self._story.kills
        logger.info(f"[{self.bot.config.name}] Story kills: "
                    f"{self._story.kills}/{node_data.kill_threshold} "
                    f"({remaining} remaining)")

        if remaining <= 0:
            # Kills done — re-check state
            self._phase = StoryPhase.CHECKING
            return "STORY_PROGRESSION"

        # Navigate to an area with the target mobs
        # Story mobs are in rooms near the hub NPC
        # Try killing each mob type from the list
        for mob_vnum in node_data.kill_mobs:
            # Teleport to the mob's room (mob_vnum == room they're loaded in)
            await self.actions.goto_room(mob_vnum)
            await asyncio.sleep(0.5)

            # Look for something to kill
            room_text = await self.actions.look()
            room_lower = room_text.lower() if room_text else ""

            # Try to kill anything in the room (story mobs have various keywords)
            # Look for non-corpse creatures
            if room_text:
                for line in room_text.split('\n'):
                    line_stripped = line.strip()
                    if not line_stripped:
                        continue
                    # Skip room title, exits, items
                    if line_stripped.startswith('[') or 'exit' in line_stripped.lower():
                        continue
                    if any(w in line_stripped.lower() for w in
                           ['corpse', 'blood', 'remains', 'entrails']):
                        continue
                    # If there's a living thing here, try to kill it
                    # Extract first word as potential keyword
                    words = line_stripped.split()
                    if len(words) >= 2:
                        # Try the last significant word as a keyword
                        for keyword_candidate in reversed(words[-3:]):
                            kw = re.sub(r'[^a-zA-Z]', '', keyword_candidate).lower()
                            if len(kw) >= 3 and kw not in ('the', 'and', 'for', 'are', 'here'):
                                combat_started, instant_kill = await self.actions.kill(kw)
                                if instant_kill:
                                    self._story.kills += 1
                                    logger.info(f"[{self.bot.config.name}] Story instant kill "
                                                f"({self._story.kills}/{node_data.kill_threshold})")
                                    await self.actions.get_all()
                                    # Re-check if kills are done
                                    if self._story.kills >= node_data.kill_threshold:
                                        self._phase = StoryPhase.CHECKING
                                        return "STORY_PROGRESSION"
                                    return None  # Stay in KILLING, try more
                                elif combat_started:
                                    # Multi-round combat — this is what we want for stance/skill training!
                                    logger.info(f"[{self.bot.config.name}] Multi-round combat started")
                                    # Return to quest bot's WAITING_COMBAT state
                                    self.bot._kills_before_train = 1
                                    return "WAITING_COMBAT"
                                break  # keyword didn't work, try next line

        # Couldn't find mobs — re-check story state
        self._phase = StoryPhase.CHECKING
        return "STORY_PROGRESSION"

    async def _do_fetch(self) -> Optional[str]:
        """Navigate to fetch room and pick up the object."""
        node_data = STORY_NODE_MAP.get(self._story.node)
        if not node_data or not node_data.fetch_obj:
            self._phase = StoryPhase.CHECKING
            return "STORY_PROGRESSION"

        logger.info(f"[{self.bot.config.name}] Story: fetching obj {node_data.fetch_obj} "
                    f"from room {node_data.fetch_room}")

        # Navigate to the fetch room
        await self.actions.goto_room(node_data.fetch_room)
        await asyncio.sleep(0.5)

        # Pick up everything
        await self.actions.get_all()
        await asyncio.sleep(0.5)

        # Re-check state — the server sets the bit when we pick up the right object
        self._phase = StoryPhase.CHECKING
        return "STORY_PROGRESSION"

    async def _do_examine(self) -> Optional[str]:
        """Navigate to examine room and examine the keyword."""
        node_data = STORY_NODE_MAP.get(self._story.node)
        if not node_data or not node_data.examine_rooms:
            self._phase = StoryPhase.CHECKING
            return "STORY_PROGRESSION"

        # Get the examine target at current index
        idx = min(self._examine_idx, len(node_data.examine_rooms) - 1)
        room_vnum, keyword = node_data.examine_rooms[idx]

        logger.info(f"[{self.bot.config.name}] Story: examining '{keyword}' "
                    f"in room {room_vnum}")

        # Navigate to the room
        await self.actions.goto_room(room_vnum)
        await asyncio.sleep(0.5)

        # Examine the keyword
        await self.actions.examine(keyword)
        await asyncio.sleep(1.0)

        # If there are more examine targets for the same node, try them all
        # (e.g., node 8 has statue + foundation)
        if idx + 1 < len(node_data.examine_rooms):
            self._examine_idx = idx + 1
            return None  # Stay in EXAMINING, do next one

        # Re-check state
        self._phase = StoryPhase.CHECKING
        return "STORY_PROGRESSION"

    async def _do_talk_npc(self) -> Optional[str]:
        """Talk to task NPCs (non-hub NPCs with keywords)."""
        node_data = STORY_NODE_MAP.get(self._story.node)
        if not node_data or not node_data.talk_npcs:
            self._phase = StoryPhase.CHECKING
            return "STORY_PROGRESSION"

        idx = min(self._talk_idx, len(node_data.talk_npcs) - 1)
        npc_vnum, keyword = node_data.talk_npcs[idx]

        logger.info(f"[{self.bot.config.name}] Story: saying '{keyword}' "
                    f"to NPC {npc_vnum}")

        # Navigate to NPC's room
        await self.actions.goto_room(npc_vnum)
        await asyncio.sleep(0.5)

        await self.quest_actions.send_command(f"say {keyword}")
        await asyncio.sleep(1.0)

        if idx + 1 < len(node_data.talk_npcs):
            self._talk_idx = idx + 1
            return None  # More NPCs to talk to

        # Re-check state
        self._phase = StoryPhase.CHECKING
        return "STORY_PROGRESSION"

    async def _do_return(self) -> Optional[str]:
        """Return to hub NPC and say the return keyword."""
        node_data = STORY_NODE_MAP.get(self._story.node)
        if not node_data or not node_data.return_keyword:
            # No return keyword (talk-through nodes)
            self._phase = StoryPhase.CHECKING
            return "STORY_PROGRESSION"

        # Navigate back to hub NPC
        await self.actions.goto_room(node_data.npc_vnum)
        await asyncio.sleep(0.5)

        logger.info(f"[{self.bot.config.name}] Story: returning to hub NPC "
                    f"{node_data.npc_vnum}, saying '{node_data.return_keyword}'")
        await self.quest_actions.send_command(f"say {node_data.return_keyword}")
        await asyncio.sleep(2.0)

        # Re-check story state to see if we advanced
        self._phase = StoryPhase.CHECKING
        return "STORY_PROGRESSION"
