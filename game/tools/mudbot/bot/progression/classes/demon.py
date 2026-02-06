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

    # Discipline progression
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
        return (
            "eyes glow red" in response_lower or
            "become a demon" in response_lower or
            "demonic" in response_lower
        )

    async def _run_full_progression(self) -> bool:
        """Run complete demon progression from start to finish."""
        # Phase 1: Avatar progression
        if not await self.run_avatar_progression():
            logger.error(f"[{self.config.name}] Avatar progression failed")
            return False

        # Phase 2: Select demon class
        if not await self.select_class():
            logger.error(f"[{self.config.name}] Failed to select demon class")
            return False

        # Phase 3: Demon-specific progression
        return await self.run_class_progression()

    async def run_class_progression(self) -> bool:
        """Execute demon-specific progression."""
        logger.info(f"[{self.config.name}] Starting demon progression")

        self.demon_state = DemonState.CHECKING_DISCIPLINES

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
        if self._demon_state == DemonState.SELECTING_CLASS:
            await self._select_class()
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
            self.demon_state = DemonState.CHECKING_DISCIPLINES
        else:
            self.demon_state = DemonState.FAILED

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

        # Use avatar's farming infrastructure
        # Reset kills counter and farm
        self._kills = 0
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
