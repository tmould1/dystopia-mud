"""
Demon class progression bot.

Handles demon-specific progression after avatar:
1. Select demon class via selfclass
2. Research and train Attack discipline (for combat abilities + graft)
3. Enable combat abilities as they unlock
4. Obtain warps when demon points available
5. Graft arms when Attack level 5+ and arms available
6. Create demonarmour with primal
7. Continue training other disciplines
"""

import asyncio
import logging
from enum import Enum, auto
from typing import Optional, Dict, Any, List

from ..base import ClassProgressionBot, BaseProgressionState
from ..avatar import AvatarProgressionMixin, AvatarState
from ..registry import ClassRegistry
from ...class_actions.demon_actions import DemonActions
from ....config import BotConfig, ProgressionConfig

logger = logging.getLogger(__name__)


class DemonState(Enum):
    """Demon-specific progression states."""
    # Class selection
    SELECTING_CLASS = auto()

    # Post-selfclass setup (new)
    HANDLING_STAT_CLEAR = auto()
    REWEARING_EQUIPMENT = auto()
    SETTING_AUTOSTANCE = auto()

    # Extended stat training (new) - train HP/mana/move to 6k
    CHECKING_EXTENDED_STATS = auto()
    FARMING_EXTENDED_STATS = auto()
    TRAINING_EXTENDED_HP = auto()
    TRAINING_MANA = auto()
    TRAINING_MOVE = auto()

    # Navigation to advanced farming location (new)
    NAVIGATING_TO_FARM = auto()

    # Discipline progression (at Black Dragon's Lair)
    CHECKING_DISCIPLINES = auto()
    STARTING_RESEARCH = auto()
    FARMING_DISCIPLINE_POINTS = auto()
    TRAINING_DISCIPLINE = auto()

    # Combat abilities
    ENABLING_COMBAT_ABILITIES = auto()

    # Warp acquisition
    CHECKING_WARPS = auto()
    OBTAINING_WARPS = auto()

    # Grafting
    CHECKING_GRAFT = auto()
    FINDING_ARMS = auto()
    GRAFTING_ARMS = auto()

    # Armour creation
    CHECKING_ARMOUR = auto()
    CREATING_ARMOUR = auto()

    # Terminal states
    COMPLETE = auto()
    FAILED = auto()


@ClassRegistry.register("demon")
class DemonProgressionBot(ClassProgressionBot, AvatarProgressionMixin):
    """
    Bot that completes demon class progression.

    Flow:
    1. Avatar progression (level 1 -> 3)
    2. Select demon class
    3. Research/train disciplines (Attack first)
    4. Enable combat abilities as they unlock
    5. Obtain warps with demon points
    6. Graft arms at Attack 5+
    7. Create demonarmour with primal
    """

    # Discipline training priority
    DEFAULT_DISCIPLINE_PRIORITY = [
        "attack",      # First: unlocks combat abilities + graft
        "hellfire",    # Fire damage
        "temptation",  # Social abilities
        "morphosis",   # Transformation
        "corruption",  # Debuffs
        "geluge",      # Ice abilities
        "discord",     # Chaos abilities
        "nether",      # Death/darkness
        "immunae",     # Resistances
    ]

    DEFAULT_ATTACK_TARGET = 7   # Level 7 for blink
    DEFAULT_OTHER_TARGET = 5    # Level 5 for other disciplines
    DEFAULT_MAX_WARPS = 18

    def __init__(
        self,
        config: BotConfig,
        prog_config: Optional[ProgressionConfig] = None,
        discipline_priority: Optional[List[str]] = None,
        attack_target: int = DEFAULT_ATTACK_TARGET,
        other_target: int = DEFAULT_OTHER_TARGET,
        max_warps: int = DEFAULT_MAX_WARPS,
        enable_grafting: bool = True,
        create_armour: bool = True,
    ):
        super().__init__(config)
        self.prog_config = prog_config or ProgressionConfig()

        # Demon-specific configuration
        self.discipline_priority = discipline_priority or self.DEFAULT_DISCIPLINE_PRIORITY
        self.attack_target = attack_target
        self.other_target = other_target
        self.max_warps = max_warps
        self.enable_grafting = enable_grafting
        self.create_armour = create_armour

        # Demon actions
        self.demon_actions = DemonActions(self)

        # State tracking
        self._demon_state = DemonState.SELECTING_CLASS
        self._current_discipline: Optional[str] = None
        self._discipline_levels: Dict[str, int] = {}
        self._combat_abilities_enabled = False

    def get_class_name(self) -> str:
        return "demon"

    def get_progression_states(self):
        return DemonState

    @property
    def demon_state(self) -> DemonState:
        """Current demon progression state."""
        return self._demon_state

    @demon_state.setter
    def demon_state(self, value: DemonState):
        if value != self._demon_state:
            logger.info(f"[{self.config.name}] Demon: {self._demon_state.name} -> {value.name}")
            self._demon_state = value
            self._notify_status_change()

    def _check_selfclass_success(self, response: str) -> bool:
        """Check for demon-specific selfclass success patterns."""
        response_lower = response.lower()
        # Success patterns for becoming a demon
        if ("eyes glow red" in response_lower or
            "become a demon" in response_lower or
            "demonic" in response_lower):
            return True
        # Already a demon or already have a class - treat as success
        if ("already a demon" in response_lower or
            "already have a class" in response_lower or
            "you are a demon" in response_lower or
            "already" in response_lower and "demon" in response_lower):
            return True
        return False

    async def _run_full_progression(self) -> bool:
        """Run complete demon progression from start to finish."""
        # Phase 1: Avatar progression (level 1 -> 3, train HP to 2000)
        if not await self.run_avatar_progression():
            logger.error(f"[{self.config.name}] Avatar progression failed")
            return False

        # Phase 2: Select demon class
        logger.info(f"[{self.config.name}] Selecting demon class...")
        response = await self.send_and_read(f"selfclass demon", timeout=5.0)

        if not response:
            logger.warning(f"[{self.config.name}] No response from selfclass command")
            # Try continuing anyway - might already be a demon
        elif not self._check_selfclass_success(response):
            logger.warning(f"[{self.config.name}] Selfclass response not recognized: {response[:200]}")
            # Check if there's an error message
            response_lower = response.lower()
            if "must be avatar" in response_lower:
                logger.error(f"[{self.config.name}] Must be avatar to selfclass - avatar progression incomplete")
                return False
            if "huh?" in response_lower or "unknown command" in response_lower:
                logger.error(f"[{self.config.name}] Selfclass command not recognized")
                return False
            # Unknown response - try continuing (might already be a demon)
            logger.info(f"[{self.config.name}] Assuming already a demon, continuing...")
        else:
            logger.info(f"[{self.config.name}] Successfully became a demon!")

        self._is_class_selected = True

        # Check for stat clear message and handle it
        if response and self.is_stat_clear_message(response):
            logger.info(f"[{self.config.name}] Stats cleared, handling post-selfclass setup...")
            await self._handle_post_selfclass_setup()
        else:
            # Even if no stat clear message, still do post-selfclass setup
            # (autostance is useful regardless)
            logger.info(f"[{self.config.name}] Running post-selfclass setup...")
            await self._handle_post_selfclass_setup()

        # Phase 3: Demon-specific progression
        return await self.run_class_progression()

    async def _handle_post_selfclass_setup(self) -> None:
        """Handle stat clear, re-equip, autostance after selfclass."""
        # Re-wear all equipment
        self.demon_state = DemonState.REWEARING_EQUIPMENT
        equipped = await self.actions.rewear_all()
        logger.info(f"[{self.config.name}] Re-equipped {equipped} items")

        # Set autostance
        self.demon_state = DemonState.SETTING_AUTOSTANCE
        stance = self.prog_config.autostance if hasattr(self.prog_config, 'autostance') else 'bull'
        await self.actions.set_autostance(stance)
        logger.info(f"[{self.config.name}] Autostance set to {stance}")

    async def run_class_progression(self) -> bool:
        """Execute demon-specific progression."""
        logger.info(f"[{self.config.name}] Starting demon progression")

        # Start with extended stats training (hp/mana/move to 6k)
        self.demon_state = DemonState.CHECKING_EXTENDED_STATS

        while self._demon_state not in (DemonState.COMPLETE, DemonState.FAILED):
            try:
                await self._wait_if_paused()
                await self._demon_tick()
                await asyncio.sleep(0.5)
            except asyncio.CancelledError:
                logger.info(f"[{self.config.name}] Demon progression cancelled")
                return False
            except Exception as e:
                logger.error(f"[{self.config.name}] Demon progression error: {e}", exc_info=True)
                self.demon_state = DemonState.FAILED
                return False

        return self._demon_state == DemonState.COMPLETE

    async def _demon_tick(self) -> None:
        """Execute one step of demon progression."""
        # Post-selfclass setup states
        if self._demon_state == DemonState.SELECTING_CLASS:
            await self._select_class()
        elif self._demon_state == DemonState.HANDLING_STAT_CLEAR:
            await self._handle_stat_clear_state()
        elif self._demon_state == DemonState.REWEARING_EQUIPMENT:
            await self._rewear_equipment_state()
        elif self._demon_state == DemonState.SETTING_AUTOSTANCE:
            await self._set_autostance_state()

        # Extended stat training states
        elif self._demon_state == DemonState.CHECKING_EXTENDED_STATS:
            await self._check_extended_stats()
        elif self._demon_state == DemonState.FARMING_EXTENDED_STATS:
            await self._farm_extended_stats()
        elif self._demon_state == DemonState.TRAINING_EXTENDED_HP:
            await self._train_extended_hp()
        elif self._demon_state == DemonState.TRAINING_MANA:
            await self._train_mana()
        elif self._demon_state == DemonState.TRAINING_MOVE:
            await self._train_move()

        # Navigation to advanced farming location
        elif self._demon_state == DemonState.NAVIGATING_TO_FARM:
            await self._navigate_to_farm()

        # Discipline progression states
        elif self._demon_state == DemonState.CHECKING_DISCIPLINES:
            await self._check_disciplines()
        elif self._demon_state == DemonState.STARTING_RESEARCH:
            await self._start_research()
        elif self._demon_state == DemonState.FARMING_DISCIPLINE_POINTS:
            await self._farm_discipline_points()
        elif self._demon_state == DemonState.TRAINING_DISCIPLINE:
            await self._train_discipline()
        elif self._demon_state == DemonState.ENABLING_COMBAT_ABILITIES:
            await self._enable_combat_abilities()
        elif self._demon_state == DemonState.CHECKING_WARPS:
            await self._check_warps()
        elif self._demon_state == DemonState.OBTAINING_WARPS:
            await self._obtain_warps()
        elif self._demon_state == DemonState.CHECKING_GRAFT:
            await self._check_graft()
        elif self._demon_state == DemonState.GRAFTING_ARMS:
            await self._graft_arms()
        elif self._demon_state == DemonState.CHECKING_ARMOUR:
            await self._check_armour()
        elif self._demon_state == DemonState.CREATING_ARMOUR:
            await self._create_armour()

    async def _select_class(self) -> None:
        """Select demon class."""
        if await self.select_class():
            self.demon_state = DemonState.CHECKING_EXTENDED_STATS
        else:
            self.demon_state = DemonState.FAILED

    # =========================================================================
    # Post-Selfclass Setup States
    # =========================================================================

    async def _handle_stat_clear_state(self) -> None:
        """Handle stat clear after selfclass."""
        await self.handle_stat_clear()
        self.demon_state = DemonState.REWEARING_EQUIPMENT

    async def _rewear_equipment_state(self) -> None:
        """Re-wear all equipment after stat clear."""
        equipped = await self.actions.rewear_all()
        logger.info(f"[{self.config.name}] Re-equipped {equipped} items")
        self.demon_state = DemonState.SETTING_AUTOSTANCE

    async def _set_autostance_state(self) -> None:
        """Set autostance for combat."""
        stance = self.prog_config.autostance if hasattr(self.prog_config, 'autostance') else 'bull'
        await self.actions.set_autostance(stance)
        logger.info(f"[{self.config.name}] Autostance set to {stance}")
        self.demon_state = DemonState.CHECKING_EXTENDED_STATS

    # =========================================================================
    # Extended Stat Training States (HP/Mana/Move to 6k)
    # =========================================================================

    def _get_extended_hp_target(self) -> int:
        """Get extended HP target from config or default."""
        if hasattr(self.prog_config, 'extended_hp_target'):
            return self.prog_config.extended_hp_target
        return 6000

    def _get_mana_target(self) -> int:
        """Get mana target from config or default."""
        if hasattr(self.prog_config, 'mana_target'):
            return self.prog_config.mana_target
        return 6000

    def _get_move_target(self) -> int:
        """Get move target from config or default."""
        if hasattr(self.prog_config, 'move_target'):
            return self.prog_config.move_target
        return 6000

    async def _check_extended_stats(self) -> None:
        """Check if extended stat training is needed."""
        stats = await self.actions.score()
        if not stats:
            logger.error(f"[{self.config.name}] Cannot get stats")
            self.demon_state = DemonState.FAILED
            return

        hp_target = self._get_extended_hp_target()
        mana_target = self._get_mana_target()
        move_target = self._get_move_target()

        logger.info(f"[{self.config.name}] Stats: HP={stats.max_hp}/{hp_target}, "
                    f"Mana={stats.max_mana}/{mana_target}, Move={stats.max_move}/{move_target}")

        # Check if all stats are at target
        if (stats.max_hp >= hp_target and
            stats.max_mana >= mana_target and
            stats.max_move >= move_target):
            logger.info(f"[{self.config.name}] All extended stats at target!")
            self.demon_state = DemonState.NAVIGATING_TO_FARM
            return

        # Need to train stats - check if we need more exp first
        if stats.max_hp < hp_target:
            # Try training HP first
            result = await self.actions.train("hp")
            if result['success']:
                return  # Stay in this state to check again
            else:
                # Out of exp, need to farm
                self.demon_state = DemonState.FARMING_EXTENDED_STATS
                return

        elif stats.max_mana < mana_target:
            self.demon_state = DemonState.TRAINING_MANA
        elif stats.max_move < move_target:
            self.demon_state = DemonState.TRAINING_MOVE

    async def _farm_extended_stats(self) -> None:
        """Farm arena for exp to train extended stats."""
        # Initialize avatar state for arena farming if needed
        if not hasattr(self, '_avatar_state') or self._avatar_state == AvatarState.START:
            self.__init_avatar_state__()
            self.avatar_state = AvatarState.FINDING_ARENA

        # If avatar farming completed a cycle, reset for next iteration
        if self._avatar_state in (AvatarState.AVATAR_COMPLETE, AvatarState.FAILED,
                                   AvatarState.SAVING, AvatarState.TRAINING_HP,
                                   AvatarState.TRAINING_AVATAR):
            # These states indicate we've finished a farming cycle or hit avatar training
            # Reset and continue farming
            self._kills = 0
            self._kill_attempts = 0
            self.avatar_state = AvatarState.FINDING_ARENA

        # Run one iteration of avatar farming
        await self._avatar_tick()

        # After a kill, check if we can train stats
        if self._kills > 0:
            # Try to train some stats
            stats = await self.actions.score()
            if stats:
                hp_target = self._get_extended_hp_target()
                mana_target = self._get_mana_target()
                move_target = self._get_move_target()

                # Train whichever stat is low
                if stats.max_hp < hp_target:
                    result = await self.actions.train("hp")
                    if result['success']:
                        self._kills = 0  # Reset for next round
                elif stats.max_mana < mana_target:
                    result = await self.actions.train("mana")
                    if result['success']:
                        self._kills = 0
                elif stats.max_move < move_target:
                    result = await self.actions.train("move")
                    if result['success']:
                        self._kills = 0

                # Check if we're done
                stats = await self.actions.score()
                if stats:
                    if (stats.max_hp >= hp_target and
                        stats.max_mana >= mana_target and
                        stats.max_move >= move_target):
                        logger.info(f"[{self.config.name}] Extended stat training complete!")
                        self.demon_state = DemonState.NAVIGATING_TO_FARM

    async def _train_extended_hp(self) -> None:
        """Train HP to extended target."""
        hp_target = self._get_extended_hp_target()
        result = await self.actions.train("hp")

        if not result['success']:
            # Out of exp, need to farm
            self.demon_state = DemonState.FARMING_EXTENDED_STATS
            return

        # Check if target reached
        stats = await self.actions.score()
        if stats and stats.max_hp >= hp_target:
            logger.info(f"[{self.config.name}] HP target {hp_target} reached!")
            self.demon_state = DemonState.CHECKING_EXTENDED_STATS

    async def _train_mana(self) -> None:
        """Train mana to target."""
        mana_target = self._get_mana_target()
        result = await self.actions.train("mana")

        if not result['success']:
            # Out of exp, need to farm
            self.demon_state = DemonState.FARMING_EXTENDED_STATS
            return

        # Check if target reached
        stats = await self.actions.score()
        if stats and stats.max_mana >= mana_target:
            logger.info(f"[{self.config.name}] Mana target {mana_target} reached!")
            self.demon_state = DemonState.CHECKING_EXTENDED_STATS

    async def _train_move(self) -> None:
        """Train movement to target."""
        move_target = self._get_move_target()
        result = await self.actions.train("move")

        if not result['success']:
            # Out of exp, need to farm
            self.demon_state = DemonState.FARMING_EXTENDED_STATS
            return

        # Check if target reached
        stats = await self.actions.score()
        if stats and stats.max_move >= move_target:
            logger.info(f"[{self.config.name}] Move target {move_target} reached!")
            self.demon_state = DemonState.CHECKING_EXTENDED_STATS

    # =========================================================================
    # Navigation to Advanced Farming Location
    # =========================================================================

    async def _navigate_to_farm(self) -> None:
        """Navigate to Black Dragon's Lair for discipline farming."""
        logger.info(f"[{self.config.name}] Navigating to Black Dragon's Lair...")

        # First recall to starting point
        if not await self.actions.recall():
            logger.warning(f"[{self.config.name}] Recall failed, trying to continue anyway")

        await asyncio.sleep(1.0)

        # Get farm path from config or use default
        farm_path = self.prog_config.farm_path if hasattr(self.prog_config, 'farm_path') else [
            'down', 'south', 'south',
            'east', 'east', 'east', 'east', 'east', 'east',
            'south', 'south', 'down', 'down', 'north'
        ]

        # Follow the path
        if await self.actions.follow_path(farm_path):
            logger.info(f"[{self.config.name}] Arrived at Black Dragon's Lair!")
            # Update arena monsters for this location
            self._update_farm_monsters()
            self.demon_state = DemonState.CHECKING_DISCIPLINES
        else:
            logger.warning(f"[{self.config.name}] Navigation failed, trying disciplines anyway")
            self.demon_state = DemonState.CHECKING_DISCIPLINES

    def _update_farm_monsters(self) -> None:
        """Update monster list for Black Dragon's Lair farming."""
        # Update the ARENA_MONSTERS class attribute with farm monsters
        farm_monsters = self.prog_config.farm_monsters if hasattr(self.prog_config, 'farm_monsters') else [
            'hobgoblin', 'shaman'
        ]
        # Store original and use new list
        if not hasattr(self, '_original_arena_monsters'):
            self._original_arena_monsters = self.ARENA_MONSTERS.copy()
        self.ARENA_MONSTERS = farm_monsters

    # =========================================================================
    # Discipline Progression States
    # =========================================================================

    async def _check_disciplines(self) -> None:
        """Check discipline levels and decide next action."""
        self._discipline_levels = await self.demon_actions.get_disciplines()

        # Find next discipline to train
        next_disc = self._get_next_discipline()

        if next_disc is None:
            # All disciplines at target, move to warps/grafting
            logger.info(f"[{self.config.name}] All target disciplines trained")
            self.demon_state = DemonState.ENABLING_COMBAT_ABILITIES
        else:
            self._current_discipline = next_disc
            self.demon_state = DemonState.STARTING_RESEARCH

    def _get_next_discipline(self) -> Optional[str]:
        """Get next discipline to train based on priority."""
        for disc in self.discipline_priority:
            current_level = self._discipline_levels.get(disc, 0)
            target = self.attack_target if disc == "attack" else self.other_target
            if current_level < target:
                return disc
        return None

    async def _start_research(self) -> None:
        """Start researching current discipline."""
        if not self._current_discipline:
            self.demon_state = DemonState.CHECKING_DISCIPLINES
            return

        success = await self.demon_actions.start_research(self._current_discipline)
        if success:
            # Check if already complete
            if self.demon_actions.research_complete:
                self.demon_state = DemonState.TRAINING_DISCIPLINE
            else:
                self.demon_state = DemonState.FARMING_DISCIPLINE_POINTS
        else:
            logger.warning(f"[{self.config.name}] Failed to start research")
            self.demon_state = DemonState.FARMING_DISCIPLINE_POINTS

    async def _farm_discipline_points(self) -> None:
        """Kill monsters to gain discipline points."""
        # Check if research is complete
        _, is_complete = await self.demon_actions.check_research_status()

        if is_complete:
            self.demon_state = DemonState.TRAINING_DISCIPLINE
            return

        # Initialize avatar state for farming if not already in progress
        # Only reset if we're in START state or not initialized
        if not hasattr(self, '_avatar_state') or self._avatar_state == AvatarState.START:
            self.__init_avatar_state__()
            self.avatar_state = AvatarState.FINDING_ARENA

        # If avatar farming completed a cycle (back to FINDING_ARENA after kills),
        # that's fine - let it continue
        # But if we hit AVATAR_COMPLETE or FAILED, reset for next farming cycle
        if self._avatar_state in (AvatarState.AVATAR_COMPLETE, AvatarState.FAILED):
            self._kills = 0
            self._kill_attempts = 0
            self.avatar_state = AvatarState.FINDING_ARENA

        # Run one iteration of avatar farming
        await self._avatar_tick()

        # Check for research complete after combat
        if self.demon_actions.research_complete:
            self.demon_state = DemonState.TRAINING_DISCIPLINE

    async def _train_discipline(self) -> None:
        """Train the researched discipline."""
        if not self._current_discipline:
            self.demon_state = DemonState.CHECKING_DISCIPLINES
            return

        result = await self.demon_actions.train_discipline(self._current_discipline)

        if result['success']:
            logger.info(f"[{self.config.name}] Trained {self._current_discipline}!")

            # Update discipline levels
            disc = self._current_discipline
            self._discipline_levels[disc] = self._discipline_levels.get(disc, 0) + 1

            # Enable unlocked abilities
            if result.get('unlocked'):
                await self._enable_unlocked_abilities(result['unlocked'])

            self._current_discipline = None
            self.demon_state = DemonState.CHECKING_DISCIPLINES
        else:
            if result.get('not_finished'):
                self.demon_state = DemonState.FARMING_DISCIPLINE_POINTS
            else:
                logger.warning(f"[{self.config.name}] Training failed")
                self.demon_state = DemonState.CHECKING_DISCIPLINES

    async def _enable_unlocked_abilities(self, abilities: List[str]) -> None:
        """Enable newly unlocked abilities."""
        for ability in abilities:
            if ability == 'claws':
                await self.demon_actions.toggle_claws(True)
            elif ability == 'tail':
                await self.demon_actions.toggle_tail(True)
            elif ability == 'horns':
                await self.demon_actions.toggle_horns(True)
            elif ability == 'wings':
                await self.demon_actions.toggle_wings(True)

    async def _enable_combat_abilities(self) -> None:
        """Enable all available combat abilities."""
        await self.demon_actions.enable_combat_abilities(self._discipline_levels)
        self._combat_abilities_enabled = True
        self.demon_state = DemonState.CHECKING_WARPS

    async def _check_warps(self) -> None:
        """Check if we should try to obtain warps."""
        if self.demon_actions.warps_obtained >= self.max_warps:
            self.demon_state = DemonState.CHECKING_GRAFT
            return

        # Try to obtain
        self.demon_state = DemonState.OBTAINING_WARPS

    async def _obtain_warps(self) -> None:
        """Attempt to obtain warps."""
        result = await self.demon_actions.obtain_warp()

        if result['success']:
            # Try for more if not at max
            if self.demon_actions.warps_obtained >= self.max_warps:
                self.demon_state = DemonState.CHECKING_GRAFT
            # Stay in obtaining state to try again
        elif result['not_enough_points']:
            logger.info(f"[{self.config.name}] Need more demon points for warps")
            self.demon_state = DemonState.CHECKING_GRAFT
        elif result['max_warps']:
            logger.info(f"[{self.config.name}] Max warps reached")
            self.demon_state = DemonState.CHECKING_GRAFT

    async def _check_graft(self) -> None:
        """Check if we can graft arms."""
        if not self.enable_grafting:
            self.demon_state = DemonState.CHECKING_ARMOUR
            return

        attack_level = self._discipline_levels.get("attack", 0)
        if attack_level < 5:
            logger.info(f"[{self.config.name}] Attack level {attack_level} < 5, cannot graft")
            self.demon_state = DemonState.CHECKING_ARMOUR
            return

        if self.demon_actions.extra_arms >= 2:
            self.demon_state = DemonState.CHECKING_ARMOUR
            return

        self.demon_state = DemonState.GRAFTING_ARMS

    async def _graft_arms(self) -> None:
        """Attempt to graft available arms."""
        arms = await self.demon_actions.find_arms_in_inventory()

        if not arms:
            logger.info(f"[{self.config.name}] No arms in inventory to graft")
            self.demon_state = DemonState.CHECKING_ARMOUR
            return

        for arm in arms:
            if self.demon_actions.extra_arms >= 2:
                break

            result = await self.demon_actions.graft_arm(arm)
            if result['full']:
                break

        self.demon_state = DemonState.CHECKING_ARMOUR

    async def _check_armour(self) -> None:
        """Check if we should create armour."""
        if not self.create_armour:
            self.demon_state = DemonState.COMPLETE
            return

        self.demon_state = DemonState.CREATING_ARMOUR

    async def _create_armour(self) -> None:
        """Create demonarmour pieces."""
        created = await self.demon_actions.create_all_armour()
        if created > 0:
            logger.info(f"[{self.config.name}] Created {created} armour pieces")
            await self.actions.save()

        self.demon_state = DemonState.COMPLETE

    def get_status(self) -> Dict[str, Any]:
        """Get current bot status."""
        status = {
            'name': self.config.name,
            'connected': self.connected,
            'logged_in': self.logged_in,
            'state': self.state.name,
            'is_paused': self.is_paused,
            'class': 'demon',
        }

        # Add avatar state if in avatar progression
        if hasattr(self, '_avatar_state'):
            status['avatar_state'] = self._avatar_state.name
            status['kills'] = getattr(self, '_kills', 0)

        # Add demon state
        status['demon_state'] = self._demon_state.name
        status['current_discipline'] = self._current_discipline
        status['discipline_levels'] = self._discipline_levels
        status['warps_obtained'] = self.demon_actions.warps_obtained
        status['extra_arms'] = self.demon_actions.extra_arms
        status['armour_created'] = self.demon_actions.armour_created

        return status
