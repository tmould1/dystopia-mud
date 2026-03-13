"""
objective_handlers.py - Dispatch table for quest objective handlers.

Maps objective description patterns to handler methods for objective types
that the quest bot needs to actively work on (beyond the basic tutorial
handlers in quest_bot.py).
"""
from __future__ import annotations

import asyncio
import logging
import re
from typing import TYPE_CHECKING, Optional

from ..utils.story_data import CLASS_COMMANDS

if TYPE_CHECKING:
    from .quest_bot import QuestBot, QuestBotState
    from ..utils.quest_parser import QuestProgress

logger = logging.getLogger(__name__)


class ObjectiveHandlers:
    """
    Extended objective handlers for advanced quest types.

    Called from QuestBot._execute_objective when the description doesn't
    match any of the basic tutorial patterns.
    """

    def __init__(self, bot: "QuestBot"):
        self.bot = bot
        self.actions = bot.actions
        self.quest_actions = bot.quest_actions

    async def dispatch(self, quest: "QuestProgress", obj_idx: int) -> Optional[str]:
        """
        Try to handle an objective.  Returns the next QuestBotState name
        if handled, or None if unrecognized.
        """
        obj = quest.objectives[obj_idx]
        desc = obj.description.lower()

        # REACH_GEN — "reach generation N" or similar
        if "generation" in desc and "reach" in desc:
            return await self._handle_reach_gen(desc)

        # REACH_UPGRADE — "reach upgrade level N"
        if "upgrade" in desc and ("reach" in desc or "level" in desc):
            return await self._handle_reach_upgrade()

        # LEARN_DISCIPLINE — "learn a discipline" / "raise discipline to N"
        if "discipline" in desc and ("learn" in desc or "raise" in desc or "research" in desc):
            return await self._handle_learn_discipline()

        # CLASS_TRAIN — "use class training"
        if "class training" in desc or "class train" in desc:
            return await self._handle_class_train()

        # CLASS_POWER — "reach power level N"
        if "power level" in desc or "class power" in desc:
            # Milestone — server evaluates ch_power levels.
            # Bot just needs to keep training disciplines.
            return await self._handle_learn_discipline()

        # FORGE_ITEM — "forge an item"
        if "forge" in desc and ("item" in desc or "metal" in desc or "gem" in desc):
            return await self._handle_forge_item()

        # QUEST_CREATE — "create a quest item"
        if "quest" in desc and "create" in desc and "item" in desc:
            return await self._handle_quest_create()

        # QUEST_MODIFY — "modify a quest item"
        if "quest" in desc and "modify" in desc:
            return await self._handle_quest_modify()

        # ARENA_WIN — "win an arena match"
        if "arena" in desc and "win" in desc:
            return await self._handle_arena_win()

        # COMPLETE_QUEST — "complete N quests"
        if "complete" in desc and "quest" in desc:
            # Passive — tracked by server. Just keep working.
            logger.info(f"[{self.bot.config.name}] COMPLETE_QUEST milestone — continuing")
            return "REFRESHING_STATE"

        # VISIT_AREA — "visit N areas"
        if "visit" in desc and "area" in desc:
            return await self._handle_visit_area()

        # WEAPON_SKILL / SPELL_SKILL — milestones, train via combat
        if ("weapon" in desc and "skill" in desc) or ("spell" in desc and "skill" in desc):
            logger.info(f"[{self.bot.config.name}] Combat skill milestone — fighting")
            # Prefer story areas for multi-round combat (skills improve per round)
            if self.bot.quest_config.enable_story:
                return "STORY_PROGRESSION"
            return "KILLING_MOBS"

        # MASTERY
        if "mastery" in desc:
            return await self._handle_mastery()

        # LEARN_SUPERSTANCE
        if "superstance" in desc:
            return await self._handle_superstance()

        # REACH_STAT for mana/move (extend existing handler)
        if "reach" in desc and ("mana" in desc or "move" in desc):
            return "TRAINING_STATS"

        return None  # Unhandled

    # ── Individual handlers ────────────────────────────────────────────

    async def _handle_reach_gen(self, desc: str) -> str:
        """Train generation — requires exp.

        Generation costs: gen 3→4: 400M, gen 4→5: 200M, gen 5→6: 50M, gen 6+: 10M.
        If not enough exp, farm in story areas for better xp yields.
        """
        result = await self.quest_actions.send_command("train generation")
        if result:
            result_lower = result.lower()
            if "not enough" in result_lower or "experience" in result_lower or "need" in result_lower:
                logger.info(f"[{self.bot.config.name}] Need more exp for generation")
                self.bot._kills_before_train = 50
                if self.bot.quest_config.enable_story:
                    return "STORY_PROGRESSION"
                return "KILLING_MOBS"
        return "REFRESHING_STATE"

    async def _handle_reach_upgrade(self) -> str:
        """Navigate to Temple Altar (recall room) and upgrade."""
        await self.actions.recall()
        await asyncio.sleep(1.0)
        result = await self.quest_actions.send_command("upgrade")
        if result:
            logger.info(f"[{self.bot.config.name}] Upgrade result: "
                        f"{result[:200]}")
        return "REFRESHING_STATE"

    async def _handle_learn_discipline(self) -> str:
        """Research and train a discipline.

        Flow: research <name> → kill mobs for disc_points (500+ xp kills
        in fight.c:3788) → detect "finished researching" → train <name>.
        Story areas provide good xp mobs for this.
        """
        # Check current research state
        resp = await self.quest_actions.send_command("research")
        if not resp:
            return "REFRESHING_STATE"

        resp_lower = resp.lower()

        if "finished researching" in resp_lower or "research complete" in resp_lower:
            # Train the discipline
            class_name = self.bot.quest_config.selfclass
            info = CLASS_COMMANDS.get(class_name)
            disc = info.disc_default if info and info.disc_default else "attack"
            result = await self.quest_actions.send_command(f"train {disc}")
            logger.info(f"[{self.bot.config.name}] Training discipline {disc}: "
                        f"{result[:200] if result else 'no response'}")
            return "REFRESHING_STATE"

        if "not currently" in resp_lower or "not researching" in resp_lower:
            # Start researching — pick discipline based on class
            class_name = self.bot.quest_config.selfclass
            info = CLASS_COMMANDS.get(class_name)
            disc = info.disc_default if info and info.disc_default else "attack"
            await self.quest_actions.send_command(f"research {disc}")
            logger.info(f"[{self.bot.config.name}] Started researching {disc}")

        # Kill mobs to accumulate disc_points (need 500+ xp kills)
        # Prefer story areas for tougher mobs with more xp
        self.bot._kills_before_train = 20
        if self.bot.quest_config.enable_story:
            return "STORY_PROGRESSION"
        return "KILLING_MOBS"

    async def _handle_class_train(self) -> str:
        """Execute class-specific training command."""
        class_name = self.bot.quest_config.selfclass
        info = CLASS_COMMANDS.get(class_name)

        if info and info.train_cmd:
            result = await self.quest_actions.send_command(info.train_cmd)
            if result:
                logger.info(f"[{self.bot.config.name}] {info.train_cmd}: "
                            f"{result[:200]}")
        else:
            # Classes without dedicated train use discipline system
            return await self._handle_learn_discipline()

        return "REFRESHING_STATE"

    async def _handle_forge_item(self) -> str:
        """Forge items. Check inventory for materials first."""
        inv = await self.actions.inventory()
        if not inv:
            return "KILLING_MOBS"

        inv_lower = inv.lower()

        # Check for metal slabs
        for metal in ["copper", "iron", "steel", "adamantite"]:
            if metal in inv_lower and "slab" in inv_lower:
                # Need a target item to forge onto — check for weapon/armor
                result = await self.quest_actions.send_command(f"forge {metal} all")
                if result and "forge" in result.lower():
                    logger.info(f"[{self.bot.config.name}] Forged {metal}")
                    return "REFRESHING_STATE"

        # Check for gems
        for gem in ["diamond", "emerald", "sapphire", "ruby"]:
            if gem in inv_lower:
                result = await self.quest_actions.send_command(f"forge {gem} all")
                if result and "forge" in result.lower():
                    logger.info(f"[{self.bot.config.name}] Forged {gem}")
                    return "REFRESHING_STATE"

        # No materials — continue killing to farm drops
        # Story areas have tougher mobs with better loot tables
        logger.info(f"[{self.bot.config.name}] No forge materials, farming")
        self.bot._kills_before_train = 20
        if self.bot.quest_config.enable_story:
            return "STORY_PROGRESSION"
        return "KILLING_MOBS"

    async def _handle_quest_create(self) -> str:
        """Create a quest item (costs QP)."""
        result = await self.quest_actions.send_command("questcreate create armor")
        if result:
            logger.info(f"[{self.bot.config.name}] Quest create: {result[:200]}")
        return "REFRESHING_STATE"

    async def _handle_quest_modify(self) -> str:
        """Modify quest item stats."""
        # First check if we have a quest item to modify
        result = await self.quest_actions.send_command("questcreate")
        if result and "not carrying" in result.lower():
            # Need to create one first
            return await self._handle_quest_create()

        result = await self.quest_actions.send_command("questcreate all str 1")
        if result:
            logger.info(f"[{self.bot.config.name}] Quest modify: {result[:200]}")
        return "REFRESHING_STATE"

    async def _handle_arena_win(self) -> str:
        """Queue for arena PvP.

        Arena requires another player.  Only attempt if PvP is enabled.
        Otherwise this will stall-skip via the stall tracker.
        """
        if not self.bot.quest_config.enable_pvp:
            logger.info(f"[{self.bot.config.name}] ARENA_WIN — PvP disabled, will stall-skip")
            return "REFRESHING_STATE"

        result = await self.quest_actions.send_command("arena")
        if result:
            logger.info(f"[{self.bot.config.name}] Arena: {result[:200]}")
        return "REFRESHING_STATE"

    async def _handle_visit_area(self) -> str:
        """Visit areas — delegate to story navigator.

        Story progression naturally visits 16 different areas (one per node).
        The server fires quest_check_progress(VISIT_AREA) when the character
        enters a new area, so just progressing the story covers this.
        """
        if self.bot.quest_config.enable_story:
            logger.info(f"[{self.bot.config.name}] VISIT_AREA — progressing story")
            return "STORY_PROGRESSION"
        # Without story, just walk around
        logger.info(f"[{self.bot.config.name}] VISIT_AREA — no story, refreshing")
        return "REFRESHING_STATE"

    async def _handle_mastery(self) -> str:
        """Check what combat skills are below 200 and train the lowest."""
        # Mastery requires all weapons, spells, and base stances at 200
        # The bot should fight in areas where all skills improve
        await self.actions.set_autostance("bull")
        self.bot._kills_before_train = 30
        if self.bot.quest_config.enable_story:
            return "STORY_PROGRESSION"
        return "KILLING_MOBS"

    async def _handle_superstance(self) -> str:
        """Train superstances via base stance combat.

        Base stances need to reach 200 first, then superstances unlock.
        Uses autostance to cycle through stances during multi-round combat.
        Note: superstances affect pkpower — only train when quest requires it.
        """
        # Cycle base stances: bull first, then others
        stances = ["bull", "viper", "crane", "crab", "mongoose"]
        # Pick next stance to train (simple rotation)
        current = getattr(self.bot, '_stance_idx', 0)
        stance = stances[current % len(stances)]
        self.bot._stance_idx = current + 1
        await self.actions.set_autostance(stance)
        self.bot._kills_before_train = 30
        if self.bot.quest_config.enable_story:
            return "STORY_PROGRESSION"
        return "KILLING_MOBS"
