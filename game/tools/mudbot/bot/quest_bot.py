"""
Quest Testing Bot - systematically tests the quest system end-to-end.

Accepts quests, executes objectives, turns in completions, and verifies
the cascading unlock chain works correctly.

Default progression: tutorial chain → M01 (avatar + selfclass) → M02 (kill 25)
"""

import asyncio
import logging
import re
import time
from enum import Enum, auto
from dataclasses import dataclass, field
from typing import Optional

from ..config import BotConfig, ProgressionConfig, QuestConfig
from .base import MudBot
from .state_machine import BotState
from .actions import BotActions
from .quest_actions import QuestActions
from .objective_handlers import ObjectiveHandlers
from .story_navigator import StoryNavigator
from ..utils.quest_parser import QuestEntry, QuestProgress

logger = logging.getLogger(__name__)


# ---------------------------------------------------------------------------
# State machine
# ---------------------------------------------------------------------------

class QuestBotState(Enum):
    """States for quest bot progression."""
    START = auto()
    INITIAL_SETUP = auto()
    REFRESHING_STATE = auto()
    ACCEPTING_QUEST = auto()
    EXECUTING_OBJECTIVE = auto()
    COMPLETING_QUEST = auto()
    # Sub-states for objective execution
    KILLING_MOBS = auto()
    FINDING_ARENA = auto()
    WAITING_COMBAT = auto()
    REGENERATING = auto()
    TRAINING_STATS = auto()
    TRAINING_AVATAR = auto()
    SELECTING_CLASS = auto()
    STORY_PROGRESSION = auto()
    COMPLETE = auto()
    FAILED = auto()


# ---------------------------------------------------------------------------
# Quest report tracking
# ---------------------------------------------------------------------------

@dataclass
class QuestResult:
    """Result of a single quest attempt."""
    id: str
    name: str
    status: str  # "PASS", "FAIL", "SKIP"
    objectives_met: int = 0
    objectives_total: int = 0
    duration_secs: float = 0.0


# ---------------------------------------------------------------------------
# Command mapping for USE_COMMAND objectives
# ---------------------------------------------------------------------------

COMMAND_MAP = {
    "look": "look",
    "score": "score",
    "inventory": "inventory",
    "north": "north",
    "south": "south",
    "east": "east",
    "west": "west",
    "up": "up",
    "down": "down",
    "exits": "exits",
    "scan": "scan",
    "say": "say hello",
    "newbie": "newbie testing",
    "equipment": "equipment",
    "get": "get all",
    "wield": "wield all",
    "help": "help score",
    "commands": "commands",
    "consider": "consider fox",
    "map": "map",
    "train": "train hp",
    "stance": "stance bull",
    "practice": "practice",
    "research": "research",
}

# Quest category priority order
CATEGORY_PRIORITY = {"T": 0, "M": 1, "CL": 2, "C": 3, "E": 4, "A": 5}

# Arena monsters (easiest first) — Mud School arena
ARENA_MONSTERS = ['fox', 'rabbit', 'snail', 'bear', 'beast', 'wolf', 'lizard', 'boar']

# Black Dragon's Lair monsters (tougher — for boosted characters)
LAIR_MONSTERS = [
    'snake', 'rat', 'hobgoblin', 'shaman', 'orc', 'spider',
    'gargoyle', 'fire fly', 'painting', 'drummer',
]

# Combined list — lair mobs checked first (tougher = better for training)
ALL_MONSTERS = LAIR_MONSTERS + ARENA_MONSTERS


class QuestBot(MudBot):
    """
    Bot that systematically tests the quest system.

    Flow:
    1. Connect and login
    2. Check quest progress and available quests
    3. Accept highest-priority available quest
    4. Execute objectives (commands, kills, training, etc.)
    5. Turn in completed quests
    6. Repeat until stop condition or no more quests
    """

    def __init__(
        self,
        config: BotConfig,
        quest_config: Optional[QuestConfig] = None,
        prog_config: Optional[ProgressionConfig] = None,
    ):
        super().__init__(config)
        self.quest_config = quest_config or QuestConfig()
        self.prog_config = prog_config or ProgressionConfig()
        self.actions = BotActions(self)
        self.quest_actions = QuestActions(self)
        self.objective_handlers = ObjectiveHandlers(self)
        self.story_navigator = StoryNavigator(self)

        # State machine
        self._state_q = QuestBotState.START
        self._cycle_count = 0

        # Current quest being worked on
        self._current_quest: Optional[QuestProgress] = None
        self._current_objective_idx: int = 0

        # Kill tracking (for KILL_MOB objectives)
        self._kill_target: Optional[str] = None
        self._in_arena = False
        self._exploration_path: list[str] = []
        self._kills_since_train: int = 0
        self._kills_before_train: int = 5  # Kill this many before attempting train
        self._story_combat: bool = False  # True when combat was started by story navigator

        # Report
        self._results: list[QuestResult] = []
        self._quest_start_time: float = 0.0
        self._bot_start_time: float = 0.0

        # Track quests we've seen completed (to detect stop condition)
        self._completed_ids: set[str] = set()

        # Stalled quest tracking: quest_id -> (last_progress_snapshot, kills_without_progress)
        self._stall_tracker: dict[str, tuple[int, int]] = {}
        self._stall_threshold: int = 15  # kills with no progress before skipping

    @property
    def quest_state(self) -> QuestBotState:
        return self._state_q

    @quest_state.setter
    def quest_state(self, value: QuestBotState):
        if value != self._state_q:
            logger.info(f"[{self.config.name}] Quest: {self._state_q.name} -> {value.name}")
            self._state_q = value

    # =========================================================================
    # Main loop
    # =========================================================================

    async def run(self) -> None:
        """Main bot loop - connect, login, run quest progression."""
        self._bot_start_time = time.time()

        # Connect
        if not await self.connect():
            logger.error(f"[{self.config.name}] Connection failed")
            return

        # Login
        if not await self.login():
            logger.error(f"[{self.config.name}] Login failed")
            await self.disconnect()
            return

        # Wait for login to settle and drain pending output
        logger.info(f"[{self.config.name}] Waiting for login to settle...")
        await asyncio.sleep(3.0)
        await self.client.send("")
        await asyncio.sleep(1.0)
        for _ in range(10):
            chunk = await self.client.read(timeout=1.0)
            if not chunk:
                break

        # Run quest progression
        success = await self._run_quest_loop()

        # Print report
        self._print_report()

        # Save and quit
        await self.actions.save()
        await self.actions.quit()
        await self.disconnect()

        if not success:
            logger.error(f"[{self.config.name}] Quest bot ended with failures")

    async def _run_quest_loop(self) -> bool:
        """
        Execute the quest progression loop.

        Returns True if all targeted quests completed successfully.
        """
        logger.info(f"[{self.config.name}] Starting quest progression "
                    f"(mode={self.quest_config.mode}, "
                    f"stop_after={self.quest_config.stop_after_quest})")

        self.quest_state = QuestBotState.INITIAL_SETUP

        while self._state_q not in (QuestBotState.COMPLETE, QuestBotState.FAILED):
            try:
                await self._quest_tick()
                await asyncio.sleep(0.5)
            except asyncio.CancelledError:
                logger.info(f"[{self.config.name}] Quest progression cancelled")
                return False
            except Exception as e:
                logger.error(f"[{self.config.name}] Quest error: {e}", exc_info=True)
                self.quest_state = QuestBotState.FAILED
                return False

        return self._state_q == QuestBotState.COMPLETE

    async def _quest_tick(self) -> None:
        """Execute one step of the quest state machine."""
        self._cycle_count += 1
        if self._cycle_count > self.quest_config.max_quest_cycles:
            logger.error(f"[{self.config.name}] Max quest cycles exceeded")
            self.quest_state = QuestBotState.FAILED
            return

        if self._state_q == QuestBotState.INITIAL_SETUP:
            await self._initial_setup()
        elif self._state_q == QuestBotState.REFRESHING_STATE:
            await self._refresh_state()
        elif self._state_q == QuestBotState.ACCEPTING_QUEST:
            await self._accept_next_quest()
        elif self._state_q == QuestBotState.EXECUTING_OBJECTIVE:
            await self._execute_objective()
        elif self._state_q == QuestBotState.COMPLETING_QUEST:
            await self._complete_quests()
        elif self._state_q == QuestBotState.KILLING_MOBS:
            await self._kill_mobs()
        elif self._state_q == QuestBotState.FINDING_ARENA:
            await self._find_arena()
        elif self._state_q == QuestBotState.WAITING_COMBAT:
            await self._wait_combat()
        elif self._state_q == QuestBotState.REGENERATING:
            await self._regenerate()
        elif self._state_q == QuestBotState.TRAINING_STATS:
            await self._train_stats()
        elif self._state_q == QuestBotState.TRAINING_AVATAR:
            await self._train_avatar()
        elif self._state_q == QuestBotState.SELECTING_CLASS:
            await self._select_class()
        elif self._state_q == QuestBotState.STORY_PROGRESSION:
            await self._story_tick()

    # =========================================================================
    # State handlers
    # =========================================================================

    async def _initial_setup(self) -> None:
        """Check initial stats and set up."""
        stats = await self.actions.score()
        if stats:
            logger.info(f"[{self.config.name}] Starting stats: "
                        f"HP={stats.max_hp}, Level={stats.level}")
        self.quest_state = QuestBotState.REFRESHING_STATE

    async def _refresh_state(self) -> None:
        """Check active quests and decide next action."""
        # Detect auto-completed quests by checking quest history
        await self._detect_auto_completions()
        if self._state_q == QuestBotState.COMPLETE:
            return

        # Optional campaign mode: prioritize active story nodes when avatar.
        if await self._should_prioritize_story():
            self.quest_state = QuestBotState.STORY_PROGRESSION
            return

        # Check for completed quests to turn in
        progress = await self.quest_actions.quest_progress()

        # Update stall tracker: detect quests with no progress after kills
        self._update_stall_tracker(progress)

        for qp in progress:
            if qp.all_met:
                logger.info(f"[{self.config.name}] Quest {qp.id} objectives all met")
                self.quest_state = QuestBotState.COMPLETING_QUEST
                return

        # Check for available quests to accept BEFORE working on active ones.
        # This ensures tutorial quests (T02, T03...) get accepted as they unlock,
        # rather than immediately jumping to M01's harder objectives.
        available = await self.quest_actions.quest_list()
        available = self._filter_quests(available)
        available = [q for q in available if q.status == "available"]

        if available:
            available.sort(key=lambda q: CATEGORY_PRIORITY.get(q.category, 99))
            # Prioritize accepting tutorial quests over working on main quests
            best = available[0]
            active_categories = {qp.id[:1] for qp in progress if not qp.all_met}
            if (CATEGORY_PRIORITY.get(best.category, 99) <=
                    min((CATEGORY_PRIORITY.get(cat, 99) for cat in active_categories), default=99)):
                self.quest_state = QuestBotState.ACCEPTING_QUEST
                return

        # Check if we have an active quest with unmet objectives
        # Sort active quests by category priority (T before M before C)
        active_quests = [qp for qp in progress if not qp.all_met]
        active_quests.sort(key=lambda qp: CATEGORY_PRIORITY.get(qp.id.split('_')[0][:1] if '_' in qp.id else qp.id[:1], 99))

        for qp in active_quests:
            # Skip stalled quests (no progress after many kills)
            if qp.id in self._stall_tracker:
                _, stall_kills = self._stall_tracker[qp.id]
                if stall_kills >= self._stall_threshold:
                    logger.debug(f"[{self.config.name}] Skipping stalled quest {qp.id}")
                    continue

            self._current_quest = qp
            self._current_objective_idx = 0
            for i, obj in enumerate(qp.objectives):
                if not obj.met:
                    self._current_objective_idx = i
                    break
            logger.info(f"[{self.config.name}] Working on {qp.id}: "
                        f"objective {self._current_objective_idx + 1}/{len(qp.objectives)}")
            self.quest_state = QuestBotState.EXECUTING_OBJECTIVE
            return

        # If we had available quests but didn't prioritize them above, accept now
        if available:
            self.quest_state = QuestBotState.ACCEPTING_QUEST
            return

        # No available quests — check stop condition or done
        logger.info(f"[{self.config.name}] No more available quests")
        self.quest_state = QuestBotState.COMPLETE

    async def _accept_next_quest(self) -> None:
        """Accept the highest-priority available quest."""
        available = await self.quest_actions.quest_list()
        available = self._filter_quests(available)
        available = [q for q in available if q.status == "available"]

        if not available:
            self.quest_state = QuestBotState.REFRESHING_STATE
            return

        available.sort(key=lambda q: CATEGORY_PRIORITY.get(q.category, 99))
        target = available[0]

        result = await self.quest_actions.quest_accept(target.id)
        if result.success:
            self._quest_start_time = time.time()
            logger.info(f"[{self.config.name}] Accepted quest: {target.id} - {result.quest_name}")
        else:
            logger.warning(f"[{self.config.name}] Failed to accept {target.id}: {result.error}")

        # Either way, refresh state to see what's active
        self.quest_state = QuestBotState.REFRESHING_STATE

    async def _complete_quests(self) -> None:
        """Turn in completed quests."""
        result = await self.quest_actions.quest_complete()

        for name in result.completed:
            elapsed = time.time() - self._quest_start_time if self._quest_start_time else 0
            # Try to find the quest ID from current quest
            quest_id = self._current_quest.id if self._current_quest else "?"
            self._results.append(QuestResult(
                id=quest_id,
                name=name,
                status="PASS",
                duration_secs=elapsed,
            ))
            self._completed_ids.add(quest_id)
            logger.info(f"[{self.config.name}] PASS: {quest_id} - {name} ({elapsed:.1f}s)")

        if not result.completed:
            # Nothing to complete — auto-complete quests handle themselves
            # Just refresh and continue
            pass

        # Check stop condition
        if self._should_stop():
            self.quest_state = QuestBotState.COMPLETE
            return

        self._current_quest = None
        self.quest_state = QuestBotState.REFRESHING_STATE

    async def _execute_objective(self) -> None:
        """Dispatch to the appropriate objective handler."""
        if not self._current_quest or not self._current_quest.objectives:
            self.quest_state = QuestBotState.REFRESHING_STATE
            return

        obj = self._current_quest.objectives[self._current_objective_idx]
        if obj.met:
            # Find next unmet objective
            for i in range(len(self._current_quest.objectives)):
                if not self._current_quest.objectives[i].met:
                    self._current_objective_idx = i
                    return
            # All met — go complete
            self.quest_state = QuestBotState.COMPLETING_QUEST
            return

        desc_lower = obj.description.lower()
        logger.info(f"[{self.config.name}] Objective: {obj.description} ({obj.current}/{obj.threshold})")

        # Dispatch based on objective description patterns
        # The quest system uses human-readable descriptions, so we pattern-match
        # NOTE: stance check must come before kill check because "skill" contains "kill"
        if self._is_use_command_objective(desc_lower):
            await self._handle_use_command(desc_lower)
        elif "stance" in desc_lower and ("raise" in desc_lower or "skill" in desc_lower):
            await self._handle_stance_objective()
        elif re.search(r'\bkill\b', desc_lower) and "mob" in desc_lower:
            # Auto-complete — batch kills based on remaining count
            remaining = obj.threshold - obj.current if obj.threshold > obj.current else 1
            self._kills_before_train = min(remaining, 25)
            self.quest_state = QuestBotState.KILLING_MOBS
        elif re.search(r'\bkill\b', desc_lower):
            self._kills_before_train = 1
            self.quest_state = QuestBotState.KILLING_MOBS
        elif "reach" in desc_lower and "hp" in desc_lower:
            self.quest_state = QuestBotState.TRAINING_STATS
        elif "reach" in desc_lower and "hit points" in desc_lower:
            self.quest_state = QuestBotState.TRAINING_STATS
        elif "reach" in desc_lower and "mana" in desc_lower:
            self.quest_state = QuestBotState.TRAINING_STATS
        elif "train avatar" in desc_lower or "qualify for avatar" in desc_lower:
            self.quest_state = QuestBotState.TRAINING_AVATAR
        elif "choose a class" in desc_lower or "selfclass" in desc_lower:
            self.quest_state = QuestBotState.SELECTING_CLASS
        elif "earn" in desc_lower and "quest points" in desc_lower:
            # Milestone — just keep completing quests, check progress later
            logger.info(f"[{self.config.name}] QP milestone — continuing quest chain")
            self.quest_state = QuestBotState.REFRESHING_STATE
        else:
            # Try extended objective handlers for advanced quest types
            next_state = await self.objective_handlers.dispatch(
                self._current_quest, self._current_objective_idx)
            if next_state:
                self.quest_state = QuestBotState[next_state]
            else:
                logger.warning(f"[{self.config.name}] Unhandled objective: {obj.description}")
                await asyncio.sleep(2.0)
                self.quest_state = QuestBotState.REFRESHING_STATE

    # =========================================================================
    # Objective handlers
    # =========================================================================

    def _is_use_command_objective(self, desc: str) -> bool:
        """Check if this is a USE_COMMAND type objective."""
        command_keywords = [
            "use the", "check your", "walk", "say something",
            "use the newbie", "pick up", "equip", "read a help",
            "view available commands", "consider", "view the area map",
            "scan", "enter a combat stance", "practice",
            "check available exits",
        ]
        return any(kw in desc for kw in command_keywords)

    async def _handle_use_command(self, desc: str) -> None:
        """Execute a USE_COMMAND objective based on its description."""
        # Map descriptions to commands
        cmd = None
        if "'look'" in desc or "look command" in desc:
            cmd = "look"
        elif "score" in desc and "check" in desc:
            cmd = "score"
        elif "inventory" in desc:
            cmd = "inventory"
        elif "walk north" in desc:
            cmd = "north"
        elif "walk south" in desc:
            cmd = "south"
        elif "exits" in desc:
            cmd = "exits"
        elif "scan" in desc:
            cmd = "scan"
        elif "say something" in desc:
            cmd = "say hello"
        elif "newbie" in desc:
            cmd = "newbie testing"
        elif "equipment" in desc and "check" in desc:
            cmd = "equipment"
        elif "pick up" in desc:
            cmd = "get all"
        elif "equip" in desc or "wield" in desc:
            cmd = "wield all"
        elif "help" in desc and "read" in desc:
            cmd = "help score"
        elif "commands" in desc and "view" in desc:
            cmd = "commands"
        elif "consider" in desc:
            cmd = "consider fox"
        elif "map" in desc and "view" in desc:
            cmd = "map"
        elif "train" in desc and "train avatar" not in desc:
            cmd = "train hp"
        elif "stance" in desc and "enter" in desc:
            cmd = "stance bull"
        elif "practice" in desc:
            cmd = "practice"
        elif "research" in desc:
            # Need discipline name — bare "research" just prompts
            from ..utils.story_data import CLASS_COMMANDS
            info = CLASS_COMMANDS.get(self.quest_config.selfclass)
            disc = info.disc_default if info and info.disc_default else "attack"
            cmd = f"research {disc}"
        else:
            # Fallback: try to extract the command from quotes
            import re
            m = re.search(r"'(\w+)'", desc)
            if m:
                target = m.group(1).lower()
                cmd = COMMAND_MAP.get(target, target)

        if cmd:
            logger.info(f"[{self.config.name}] Executing command: {cmd}")
            await self.quest_actions.send_command(cmd)
            await asyncio.sleep(1.0)
        else:
            logger.warning(f"[{self.config.name}] Could not map objective to command: {desc}")
            await asyncio.sleep(1.0)

        # Refresh to check progress
        self.quest_state = QuestBotState.REFRESHING_STATE

    async def _handle_stance_objective(self) -> None:
        """Handle stance training objectives.

        Stance skill improves via improve_stance() in kav_fight.c on each
        one_hit() call.  With autostance set, stance auto-activates even
        on instant kills.  Arena combat is the most reliable path.
        """
        await self.actions.set_autostance("bull")
        logger.info(f"[{self.config.name}] Stance training via arena combat (autostance bull)")
        self.quest_state = QuestBotState.KILLING_MOBS

    # =========================================================================
    # Combat sub-states (reused from AvatarProgressionBot pattern)
    # =========================================================================

    async def _kill_mobs(self) -> None:
        """Start the kill cycle - navigate to arena and fight."""
        stats = await self.actions.score()
        if stats and stats.current_hp < self.prog_config.min_hp_to_fight:
            self.quest_state = QuestBotState.REGENERATING
            return
        self.quest_state = QuestBotState.FINDING_ARENA

    async def _find_arena(self) -> None:
        """Navigate to hunting area and find a monster to kill.

        Handles two hunting zones:
        - Mud School arena (recall=3700): south into 5x5 arena grid
        - Black Dragon's Lair (recall=32000): down into lair tunnels
        """
        import re
        import random

        # Ensure character is awake and standing (may be sleeping from regen)
        await self.send_command("wake")
        await asyncio.sleep(0.3)
        await self.send_command("stand")
        await asyncio.sleep(0.3)

        # Strip ANSI/MUD color codes from text for reliable matching
        def strip_codes(text):
            return re.sub(r'\x1b\[[0-9;]*m|\[0[;0-9]*m|#[RGBYPCWnrx0-9]+', '', text)

        room_text = await self.actions.look()
        room_clean = strip_codes(room_text)
        room_lower = room_clean.lower()
        # First non-empty line that looks like a room name (skip combat spam, prompts)
        # Room names don't contain: prompt brackets [, combat words, "you " actions
        skip_patterns = [
            'you ', 'your ', 'a spray', '<', 'the day ', 'the night ',
            'the sun ', 'the moon ', 'it is ', 'you dodge', 'you parry',
        ]
        first_line = ""
        for line in room_clean.split('\n'):
            line = line.strip()
            if not line:
                continue
            line_lower = line.lower()
            if any(line_lower.startswith(p) for p in skip_patterns):
                continue
            first_line = line_lower
            break
        logger.info(f"[{self.config.name}] Room: '{first_line}' (len={len(room_clean)})")
        if len(room_clean) > 300:
            logger.info(f"[{self.config.name}] Room contents: {room_clean[:500]}")

        # Check if at an entrance (not yet in hunting area — need to go deeper)
        at_lair_entrance = ('dark tunnel' in first_line and 'entrance' in first_line)
        at_school_entrance = ('mud school' in first_line or 'school' in first_line)
        at_temple = ('temple' in first_line)

        # Detect hunting area by room name keywords (exclude entrance rooms)
        if not at_lair_entrance and not at_school_entrance:
            in_hunting_area = (
                'arena' in first_line
                or 'tunnel' in first_line
                or 'cavern' in first_line
                or 'lair' in first_line
                or 'passage' in first_line
                or 'chamber' in first_line
            )
            if in_hunting_area:
                self._in_arena = True

        # Look for monsters (skip corpses and body parts)
        debris_words = [
            'entrails', 'corpse', 'blood', 'pool', 'body', 'remains',
            'sliced-off', 'severed', 'torn', 'ripped', 'leg of', 'arm of',
            'head of', 'heart of', 'brains of', 'guts of', 'decomposes',
        ]
        # Check HP before looking for mobs — regenerate if low
        stats = await self.actions.score()
        if stats and stats.current_hp < stats.max_hp * 0.15:
            logger.info(f"[{self.config.name}] HP low ({stats.current_hp}/{stats.max_hp}), regenerating")
            self.quest_state = QuestBotState.REGENERATING
            return

        for monster in ALL_MONSTERS:
            monster_pattern = re.compile(r'\b' + re.escape(monster) + r'\b', re.IGNORECASE)
            if not monster_pattern.search(room_lower):
                continue
            is_corpse = False
            for line in room_text.split('\n'):
                line_lower = line.lower()
                if monster_pattern.search(line_lower):
                    if any(cw in line_lower for cw in debris_words):
                        is_corpse = True
                        break
                    self._kill_target = monster
                    logger.info(f"[{self.config.name}] Attacking {monster}")
                    combat_started, instant_kill = await self.actions.kill(monster)
                    if instant_kill:
                        self._kills_since_train += 1
                        logger.info(f"[{self.config.name}] Instant kill: {monster} "
                                    f"(kills: {self._kills_since_train})")
                        await self._move_to_next_arena_room()
                        if (self._kills_before_train > 1
                                and self._kills_since_train < self._kills_before_train):
                            self._kill_target = None
                            return
                        self.quest_state = QuestBotState.REFRESHING_STATE
                        return
                    elif combat_started:
                        self.quest_state = QuestBotState.WAITING_COMBAT
                        return
                    else:
                        self._kill_target = None
                        break

        # No monster found, navigate
        logger.info(f"[{self.config.name}] No mob found, in_arena={self._in_arena}, "
                    f"path_depth={len(self._exploration_path)}")
        # Parse exits from the look response we already have (avoid double look)
        exits = self.actions.room_parser.parse_exits(room_text)
        if not exits:
            logger.warning(f"[{self.config.name}] No exits, recalling")
            await self.actions.recall()
            self._in_arena = False
            self._exploration_path = []
            return

        # Limit exploration depth to avoid getting lost
        if len(self._exploration_path) > 20:
            logger.info(f"[{self.config.name}] Exploration depth limit, recalling")
            await self.actions.recall()
            self._in_arena = False
            self._exploration_path = []
            return

        # At an entrance? Always go deeper (even if _in_arena from a previous cycle)
        if at_lair_entrance:
            if 'down' in exits:
                logger.info(f"[{self.config.name}] Entering lair (down)")
                # Ensure standing position (recall doesn't guarantee it)
                await self.send_command("stand")
                await asyncio.sleep(0.3)
                self._exploration_path.append('down')
                moved = await self.actions.move('down')
                if not moved:
                    logger.warning(f"[{self.config.name}] Failed to enter lair! Retrying after stand")
                    await self.send_command("wake")
                    await asyncio.sleep(0.5)
                    await self.send_command("stand")
                    await asyncio.sleep(0.3)
                    await self.actions.move('down')
                return
        if at_school_entrance:
            if 'south' in exits:
                logger.info(f"[{self.config.name}] Entering arena (south)")
                await self.send_command("stand")
                await asyncio.sleep(0.3)
                self._exploration_path.append('south')
                await self.actions.move('south')
                return

        # At temple? Go up to reach school entrance
        if at_temple and 'up' in exits:
            logger.info(f"[{self.config.name}] At temple, going up to school")
            self._exploration_path.append('up')
            await self.actions.move('up')
            return

        # Not in hunting area and not at entrance — recall to temple
        if not self._in_arena:
            logger.info(f"[{self.config.name}] Not at hunting entrance, recalling")
            await self.actions.recall()
            return

        # Explore hunting area — prefer going deeper (north/east in lair)
        # Avoid backtracking by deprioritizing the reverse of last move
        reverse = {'north': 'south', 'south': 'north', 'east': 'west',
                    'west': 'east', 'up': 'down', 'down': 'up'}
        last_dir = self._exploration_path[-1] if self._exploration_path else None
        back_dir = reverse.get(last_dir) if last_dir else None

        # Priority: forward directions first, then lateral, then back
        forward_dirs = ['north', 'east', 'down']
        lateral_dirs = ['west', 'south']
        random.shuffle(forward_dirs)
        random.shuffle(lateral_dirs)
        explore_dirs = forward_dirs + lateral_dirs + ['up']

        for d in explore_dirs:
            if d == back_dir:
                continue  # skip backtracking
            if d in exits:
                self._exploration_path.append(d)
                await self.actions.move(d)
                return

        # No forward path — allow backtracking
        if back_dir and back_dir in exits:
            self._exploration_path.append(back_dir)
            await self.actions.move(back_dir)
            return

        # No exits at all — recall
        available = [d for d in explore_dirs if d in exits]
        if available:
            d = random.choice(available)
            self._exploration_path.append(d)
            await self.actions.move(d)

    async def _wait_combat(self) -> None:
        """Wait for combat to end."""
        victory = await self.actions.wait_for_combat_end(timeout=30.0)
        if victory:
            self._kills_since_train += 1
            logger.info(f"[{self.config.name}] Victory! "
                        f"(kills since train: {self._kills_since_train})")
            # Pick up drops (forge materials, quest items)
            await self.actions.get_all()
            # Move to a new room to find fresh mobs (avoids corpse confusion)
            if not self._story_combat:
                await self._move_to_next_arena_room()
        else:
            logger.warning(f"[{self.config.name}] Combat ended unfavorably")

        self._kill_target = None

        # If combat was initiated by story navigator, return to story
        if self._story_combat:
            self._story_combat = False
            self.quest_state = QuestBotState.STORY_PROGRESSION
            return

        # Check if we're farming for REACH_STAT (batched kills) vs KILL_MOB (refresh each kill)
        if (self._current_quest and self._kills_before_train > 1
                and self._kills_since_train < self._kills_before_train):
            # Still batching kills for training — keep killing
            self.quest_state = QuestBotState.FINDING_ARENA
        else:
            # KILL_MOB quests auto-complete, or batch complete — refresh
            self.quest_state = QuestBotState.REFRESHING_STATE

    async def _regenerate(self) -> None:
        """Wait for HP to regenerate using sleep for faster recovery."""
        logger.info(f"[{self.config.name}] Regenerating...")
        # Recall first (safe room = faster regen) then sleep
        await self.actions.recall()
        self._in_arena = False
        self._exploration_path = []
        await self.send_command("sleep")
        await asyncio.sleep(1.0)

        for _ in range(30):  # Up to 60 seconds of sleep regen
            await asyncio.sleep(2.0)
            stats = await self.actions.score()
            if stats and stats.current_hp >= stats.max_hp * 0.25:
                logger.info(f"[{self.config.name}] HP recovered: {stats.current_hp}/{stats.max_hp}")
                await self.send_command("wake")
                await asyncio.sleep(0.5)
                self.quest_state = QuestBotState.REFRESHING_STATE
                return

        # Timeout — wake up and continue anyway
        await self.send_command("wake")
        await asyncio.sleep(0.5)
        self.quest_state = QuestBotState.REFRESHING_STATE

    async def _train_stats(self) -> None:
        """Train HP/mana/move for REACH_STAT objectives."""
        if not self._current_quest:
            self.quest_state = QuestBotState.REFRESHING_STATE
            return

        obj = self._current_quest.objectives[self._current_objective_idx]
        desc_lower = obj.description.lower()

        # Parse the target value from description like "Reach 2,000 HP"
        import re
        target_match = re.search(r'reach\s+([\d,]+)', desc_lower)
        target = obj.threshold  # fallback to threshold
        if target_match:
            target = int(target_match.group(1).replace(',', ''))

        if 'hp' in desc_lower or 'hit points' in desc_lower:
            stat = 'hp'
        elif 'mana' in desc_lower:
            stat = 'mana'
        elif 'move' in desc_lower:
            stat = 'move'
        else:
            stat = 'hp'  # default

        # Train in a loop until out of exp
        trained_any = False
        for _ in range(50):  # Safety limit
            result = await self.actions.train(stat)
            if result['success']:
                trained_any = True
                await asyncio.sleep(0.1)
            else:
                break

        if trained_any:
            self._kills_since_train = 0
            stats = await self.actions.score()
            if stats:
                current = getattr(stats, f'max_{stat}', 0)
                logger.info(f"[{self.config.name}] Trained {stat}: now {current}/{target}")
                if current >= target:
                    logger.info(f"[{self.config.name}] {stat} target reached!")
                    self.quest_state = QuestBotState.REFRESHING_STATE
                    return

        # Need more exp — kill mobs in batches before training again
        self._kills_since_train = 0
        self._kills_before_train = 25  # Kill more to accumulate exp (HP cost scales)
        logger.info(f"[{self.config.name}] Need more exp, farming {self._kills_before_train} mobs")
        self.quest_state = QuestBotState.KILLING_MOBS

    async def _train_avatar(self) -> None:
        """Train avatar (become level 3).  Requires 2000+ HP."""
        stats = await self.actions.score()

        if stats and stats.level >= 3:
            logger.info(f"[{self.config.name}] Already avatar!")
            self.quest_state = QuestBotState.REFRESHING_STATE
            return

        if stats and stats.max_hp < 2000:
            logger.info(f"[{self.config.name}] Need more HP for avatar "
                        f"({stats.max_hp}/2000), farming")
            # Train HP toward 2000 then kill mobs for more exp
            for _ in range(50):
                result = await self.actions.train('hp')
                if result['success']:
                    await asyncio.sleep(0.1)
                else:
                    break
            stats = await self.actions.score()
            if stats and stats.max_hp < 2000:
                # Still not enough — go farm
                self._kills_since_train = 0
                self._kills_before_train = 25
                self.quest_state = QuestBotState.KILLING_MOBS
                return
            # Fall through to attempt avatar training

        if await self.actions.train_avatar():
            logger.info(f"[{self.config.name}] BECAME AVATAR!")
            # clearstats2 now silently re-equips gear server-side
            await asyncio.sleep(0.5)
        else:
            logger.warning(f"[{self.config.name}] Avatar training failed, "
                           "farming more HP")
            self._kills_since_train = 0
            self._kills_before_train = 25
            self.quest_state = QuestBotState.KILLING_MOBS
            return

        self.quest_state = QuestBotState.REFRESHING_STATE

    async def _select_class(self) -> None:
        """Select a class via selfclass command.

        The 'selfclass' command requires level 3 (avatar).  If the character
        is below level 3 the command interpreter rejects it with "Huh?" rather
        than a descriptive error, so we must check level first.
        """
        # Must be avatar (level 3) before the command is even available
        stats = await self.actions.score()
        if stats and stats.level < 3:
            logger.info(f"[{self.config.name}] Level {stats.level} < 3, "
                        "need avatar before selfclass")
            self.quest_state = QuestBotState.TRAINING_AVATAR
            return

        class_name = self.quest_config.selfclass
        logger.info(f"[{self.config.name}] Selecting class: {class_name}")

        response = await self.send_and_read(
            f"selfclass {class_name}",
            wait_for=["already have a class", "stats have been cleared",
                       "you have chosen", "must be avatar", "Huh?"],
            timeout=5.0,
        )

        logger.debug(f"[{self.config.name}] Selfclass response: "
                     f"{response[:300] if response else 'None'}")

        if response:
            resp_lower = response.lower()

            # Success — class was selected
            if ("already have a class" in resp_lower or
                "you have chosen" in resp_lower or
                "stats have been cleared" in resp_lower):
                logger.info(f"[{self.config.name}] Class selected: {class_name}")

                # clearstats2 now silently re-equips gear server-side
                if "stats have been cleared" in resp_lower:
                    await asyncio.sleep(0.5)

                self.quest_state = QuestBotState.REFRESHING_STATE
                return

            # Not avatar yet (level check in do_classself, or cmd blocked)
            if "must be avatar" in resp_lower or "huh?" in resp_lower:
                logger.warning(f"[{self.config.name}] Need avatar first")
                self.quest_state = QuestBotState.TRAINING_AVATAR
                return

        logger.warning(f"[{self.config.name}] Selfclass response unclear, "
                       "refreshing")
        self.quest_state = QuestBotState.REFRESHING_STATE

    async def _story_tick(self) -> None:
        """Advance the story navigator one step."""
        next_state = await self.story_navigator.tick()
        if next_state:
            if next_state == "WAITING_COMBAT":
                # Story navigator started multi-round combat — track it
                self._story_combat = True
            self.quest_state = QuestBotState[next_state]
        else:
            # Story idle or complete — fall back to normal quest work
            self.quest_state = QuestBotState.REFRESHING_STATE

    # =========================================================================
    # Helpers
    # =========================================================================

    def _update_stall_tracker(self, progress: list) -> None:
        """Track quest objective progress to detect stalled quests.

        Increments stall count per refresh cycle when no progress is detected
        on the currently active quest. Resets stall count when progress resumes
        (even on previously stalled quests), so quests that were temporarily
        blocked can be retried.
        """
        current_id = self._current_quest.id if self._current_quest else None

        for qp in progress:
            if qp.all_met:
                self._stall_tracker.pop(qp.id, None)
                continue

            snapshot = sum(obj.current for obj in qp.objectives)

            if qp.id in self._stall_tracker:
                last_snapshot, stall_count = self._stall_tracker[qp.id]
                if snapshot > last_snapshot:
                    # Progress detected — reset stall count (even if was stalled)
                    if stall_count >= self._stall_threshold:
                        logger.info(
                            f"[{self.config.name}] Quest {qp.id} un-stalled "
                            f"(progress {last_snapshot} -> {snapshot})")
                    self._stall_tracker[qp.id] = (snapshot, 0)
                elif qp.id == current_id:
                    # Only increment stall for the quest we're actively working on
                    new_stall = stall_count + 1
                    self._stall_tracker[qp.id] = (snapshot, new_stall)
                    if new_stall >= self._stall_threshold and stall_count < self._stall_threshold:
                        logger.warning(
                            f"[{self.config.name}] Quest {qp.id} stalled after "
                            f"{new_stall} refresh cycles with no progress — will skip")
            else:
                self._stall_tracker[qp.id] = (snapshot, 0)

    async def _move_to_next_arena_room(self) -> None:
        """Move to an adjacent room in the arena to find fresh mobs."""
        exits = await self.actions.get_exits()
        if not exits:
            # Dead end — recall to reset position
            await self.actions.recall()
            self._in_arena = False
            self._exploration_path = []
            return

        # Limit exploration depth to avoid getting lost in dead ends
        if len(self._exploration_path) > 10:
            await self.actions.recall()
            self._in_arena = False
            self._exploration_path = []
            return

        # Prefer directions we haven't been to recently
        # Exclude 'up' in school arena (up = exit to safe room/temple)
        import random
        explore_dirs = ['north', 'south', 'east', 'west', 'down']
        random.shuffle(explore_dirs)
        for d in explore_dirs:
            if d in exits and d not in self._exploration_path[-3:]:
                self._exploration_path.append(d)
                await self.actions.move(d)
                return
        # Pick any exit
        d = random.choice(exits)
        self._exploration_path.append(d)
        await self.actions.move(d)

    async def _detect_auto_completions(self) -> None:
        """Detect auto-completed quests via quest history and record them as PASS."""
        history, _ = await self.quest_actions.quest_history()
        for entry in history:
            if entry.id not in self._completed_ids:
                elapsed = time.time() - self._quest_start_time if self._quest_start_time else 0
                self._results.append(QuestResult(
                    id=entry.id,
                    name=entry.name,
                    status="PASS",
                    duration_secs=elapsed,
                ))
                self._completed_ids.add(entry.id)
                logger.info(f"[{self.config.name}] PASS (auto): {entry.id} - {entry.name}")

        # Check stop condition after detecting completions
        if self._should_stop():
            self.quest_state = QuestBotState.COMPLETE

    async def _should_prioritize_story(self) -> bool:
        """Return True when campaign mode should advance the story first."""
        if not self.quest_config.enable_story or not self.quest_config.force_story_progress:
            return False

        # Avoid expensive checks on every refresh loop.
        if self._cycle_count % 8 != 0:
            return False

        stats = await self.actions.score()
        level = stats.level if stats else self.config.experience_level
        if level < 3:
            return False

        story = await self.actions.storyadmin(self.config.name)
        if story.not_started or story.completed:
            return False
        if story.node <= 0 or story.node > self.quest_config.max_story_node:
            return False

        return True

    def _filter_quests(self, quests: list[QuestEntry]) -> list[QuestEntry]:
        """Filter quests based on mode config."""
        mode = self.quest_config.mode
        if mode == "tutorial":
            return [q for q in quests if q.category == "T"]
        elif mode == "main":
            return [q for q in quests if q.category in ("T", "M")]
        # "all" mode — no filter
        return quests

    def _should_stop(self) -> bool:
        """Check if we've reached the stop condition."""
        stop = self.quest_config.stop_after_quest
        if stop and stop in self._completed_ids:
            logger.info(f"[{self.config.name}] Stop condition reached: {stop}")
            return True
        return False

    def _print_report(self) -> None:
        """Print the quest test report."""
        elapsed = time.time() - self._bot_start_time
        logger.info("")
        logger.info(f"=== Quest Test Report ===")
        logger.info(f"Character: {self.config.name} (explevel {self.config.experience_level})")
        logger.info(f"Duration: {elapsed:.0f}s")
        logger.info("")

        if not self._results:
            logger.info("  No quests completed.")
        else:
            for r in self._results:
                logger.info(f"  [{r.status}] {r.id} - {r.name} ({r.duration_secs:.1f}s)")

        passed = sum(1 for r in self._results if r.status == "PASS")
        failed = sum(1 for r in self._results if r.status == "FAIL")
        logger.info("")
        logger.info(f"Total: {passed} passed, {failed} failed")
        logger.info("")

    def get_status(self) -> dict:
        """Get current bot status."""
        return {
            'name': self.config.name,
            'connected': self.connected,
            'logged_in': self.logged_in,
            'state': self.state.name,
            'quest_state': self._state_q.name,
            'current_quest': self._current_quest.id if self._current_quest else None,
            'completed': len(self._completed_ids),
            'cycle': self._cycle_count,
        }

    def get_report_data(self) -> dict:
        """Return detailed run data for campaign reporting."""
        return {
            'completed_ids': sorted(self._completed_ids),
            'results': [
                {
                    'id': r.id,
                    'name': r.name,
                    'status': r.status,
                    'objectives_met': r.objectives_met,
                    'objectives_total': r.objectives_total,
                    'duration_secs': r.duration_secs,
                }
                for r in self._results
            ],
            'story_nodes': sorted(self.story_navigator.visited_nodes),
        }
