"""
Avatar progression mixin for class progression bots.

Provides the shared logic for progressing from level 1 to avatar (level 3).
All character classes must complete avatar progression before selecting a class.
"""

import asyncio
import logging
from enum import Enum, auto
from typing import Optional, TYPE_CHECKING

if TYPE_CHECKING:
    from .base import ClassProgressionBot

logger = logging.getLogger(__name__)


class AvatarState(Enum):
    """States for avatar progression."""
    START = auto()
    CHECKING_STATS = auto()
    FINDING_ARENA = auto()
    KILLING_MONSTERS = auto()
    WAITING_COMBAT = auto()
    REGENERATING = auto()
    SAVING = auto()
    TRAINING_HP = auto()
    TRAINING_AVATAR = auto()
    AVATAR_COMPLETE = auto()
    FAILED = auto()


class AvatarProgressionMixin:
    """
    Mixin providing shared avatar progression logic.

    Use this mixin in class progression bots to share the avatar
    progression logic (level 1 -> 3).

    Usage:
        class DemonProgressionBot(ClassProgressionBot, AvatarProgressionMixin):
            async def _run_full_progression(self):
                if not await self.run_avatar_progression():
                    return False
                if not await self.select_class():
                    return False
                return await self.run_class_progression()
    """

    # Arena monster keywords, ordered by level (easiest first)
    ARENA_MONSTERS = ['fox', 'rabbit', 'snail', 'bear', 'beast', 'wolf', 'lizard', 'boar']

    # Only explore these directions (no up/down in arena)
    EXPLORE_DIRECTIONS = ['south', 'north', 'east', 'west']

    # Default progression settings
    DEFAULT_KILLS_NEEDED = 5
    DEFAULT_HP_TARGET = 2000
    DEFAULT_MIN_HP_TO_FIGHT = 50
    DEFAULT_MAX_KILL_ATTEMPTS = 20

    def __init_avatar_state__(self):
        """Initialize avatar progression state. Call from subclass __init__."""
        self._avatar_state = AvatarState.START
        self._kills = 0
        self._kill_attempts = 0
        self._avatar_train_failures = 0  # Track failed avatar training attempts
        self._exploration_path: list[str] = []
        self._in_arena = False
        self._current_target: Optional[str] = None

    @property
    def avatar_state(self) -> AvatarState:
        """Current avatar progression state."""
        return self._avatar_state

    @avatar_state.setter
    def avatar_state(self, value: AvatarState):
        if value != self._avatar_state:
            logger.info(f"[{self.config.name}] Avatar: {self._avatar_state.name} -> {value.name}")
            self._avatar_state = value
            self._notify_status_change()

    def _get_kills_needed(self) -> int:
        """Get kills needed from config or default."""
        if hasattr(self, 'prog_config') and hasattr(self.prog_config, 'kills_needed'):
            return self.prog_config.kills_needed
        return self.DEFAULT_KILLS_NEEDED

    def _get_hp_target(self) -> int:
        """Get HP target from config or default."""
        if hasattr(self, 'prog_config') and hasattr(self.prog_config, 'hp_target'):
            return self.prog_config.hp_target
        return self.DEFAULT_HP_TARGET

    def _get_min_hp_to_fight(self) -> int:
        """Get minimum HP to fight from config or default."""
        if hasattr(self, 'prog_config') and hasattr(self.prog_config, 'min_hp_to_fight'):
            return self.prog_config.min_hp_to_fight
        return self.DEFAULT_MIN_HP_TO_FIGHT

    def _get_max_kill_attempts(self) -> int:
        """Get max kill attempts from config or default."""
        if hasattr(self, 'prog_config') and hasattr(self.prog_config, 'max_kill_attempts'):
            return self.prog_config.max_kill_attempts
        return self.DEFAULT_MAX_KILL_ATTEMPTS

    async def run_avatar_progression(self: "ClassProgressionBot") -> bool:
        """
        Execute avatar progression from level 1 to level 3.

        Returns:
            True if avatar progression completed successfully.
        """
        self.__init_avatar_state__()

        logger.info(f"[{self.config.name}] Starting avatar progression")
        logger.info(f"[{self.config.name}] Goal: Kill {self._get_kills_needed()} monsters, train HP to {self._get_hp_target()}, become avatar")

        self.avatar_state = AvatarState.CHECKING_STATS

        while self._avatar_state not in (AvatarState.AVATAR_COMPLETE, AvatarState.FAILED):
            try:
                await self._wait_if_paused()
                await self._avatar_tick()
                await asyncio.sleep(0.5)
            except asyncio.CancelledError:
                logger.info(f"[{self.config.name}] Avatar progression cancelled")
                return False
            except Exception as e:
                logger.error(f"[{self.config.name}] Avatar progression error: {e}", exc_info=True)
                self.avatar_state = AvatarState.FAILED
                return False

        return self._avatar_state == AvatarState.AVATAR_COMPLETE

    async def _avatar_tick(self: "ClassProgressionBot") -> None:
        """Execute one step of avatar progression based on current state."""
        if self._avatar_state == AvatarState.CHECKING_STATS:
            await self._check_initial_stats()
        elif self._avatar_state == AvatarState.FINDING_ARENA:
            await self._find_arena()
        elif self._avatar_state == AvatarState.KILLING_MONSTERS:
            await self._kill_monster()
        elif self._avatar_state == AvatarState.WAITING_COMBAT:
            await self._wait_combat()
        elif self._avatar_state == AvatarState.REGENERATING:
            await self._regenerate()
        elif self._avatar_state == AvatarState.SAVING:
            await self._save_and_prepare()
        elif self._avatar_state == AvatarState.TRAINING_HP:
            await self._train_hp()
        elif self._avatar_state == AvatarState.TRAINING_AVATAR:
            await self._train_avatar()

    async def _check_initial_stats(self: "ClassProgressionBot") -> None:
        """Check starting stats to determine what we need to do."""
        stats = await self.actions.score()
        if not stats:
            logger.error(f"[{self.config.name}] Cannot get initial stats")
            self.avatar_state = AvatarState.FAILED
            return

        logger.info(f"[{self.config.name}] Current stats: HP={stats.max_hp}, Exp={stats.exp}, Level={stats.level}")

        # If already avatar (level 3+), we're done
        if stats.level >= 3:
            logger.info(f"[{self.config.name}] Already an avatar!")
            self.avatar_state = AvatarState.AVATAR_COMPLETE
            return

        # If HP >= target, can try avatar training
        if stats.max_hp >= self._get_hp_target():
            logger.info(f"[{self.config.name}] HP sufficient, attempting avatar training")
            self.avatar_state = AvatarState.TRAINING_AVATAR
            return

        # Need to kill monsters and train HP
        logger.info(f"[{self.config.name}] Need to earn exp and train HP")
        self.avatar_state = AvatarState.FINDING_ARENA

    async def _find_arena(self: "ClassProgressionBot") -> None:
        """Navigate to arena and find monsters to kill."""
        room_text = await self.actions.look()
        room_lower = room_text.lower()

        # Check if we're in the arena
        first_line = room_text.split('\n')[0].strip().lower() if room_text else ""
        if 'arena' in first_line:
            self._in_arena = True
            logger.info(f"[{self.config.name}] In the arena: {first_line}")

        # Look for arena monsters (excluding corpses)
        corpse_words = ['entrails', 'corpse', 'blood', 'pool', 'body', 'remains']
        for monster in self.ARENA_MONSTERS:
            if monster in room_lower:
                is_corpse = False
                for line in room_text.split('\n'):
                    line_lower = line.lower()
                    if monster in line_lower:
                        if any(cw in line_lower for cw in corpse_words):
                            is_corpse = True
                            break
                        logger.info(f"[{self.config.name}] Found target: {monster}")
                        self._current_target = monster
                        self.avatar_state = AvatarState.KILLING_MONSTERS
                        return
                if is_corpse:
                    logger.debug(f"[{self.config.name}] Skipping corpse: {monster}")

        # No monster found, navigate
        exits = await self.actions.get_exits()
        if not exits:
            logger.warning(f"[{self.config.name}] No exits found, stuck!")
            self.avatar_state = AvatarState.FAILED
            return

        # If not in arena yet, go south (arena is south of spawn)
        if not self._in_arena and 'south' in exits:
            logger.info(f"[{self.config.name}] Heading south to arena...")
            await self.actions.move('south')
            return

        # In arena but no monster here, explore (NESW only)
        for direction in self.EXPLORE_DIRECTIONS:
            if direction in exits and direction not in self._exploration_path:
                logger.info(f"[{self.config.name}] Exploring arena: {direction}")
                self._exploration_path.append(direction)
                if await self.actions.move(direction):
                    return

        # Tried all directions, reset and try again
        self._exploration_path = []
        for direction in self.EXPLORE_DIRECTIONS:
            if direction in exits:
                logger.info(f"[{self.config.name}] Continuing exploration: {direction}")
                await self.actions.move(direction)
                return

    async def _kill_monster(self: "ClassProgressionBot") -> None:
        """Attack a monster."""
        if self._kills >= self._get_kills_needed():
            logger.info(f"[{self.config.name}] Killed enough monsters ({self._kills}), saving...")
            self.avatar_state = AvatarState.SAVING
            return

        if self._kill_attempts >= self._get_max_kill_attempts():
            logger.error(f"[{self.config.name}] Too many kill attempts, giving up")
            self.avatar_state = AvatarState.FAILED
            return

        # Check HP before fighting
        stats = await self.actions.score()
        if stats and stats.current_hp < self._get_min_hp_to_fight():
            logger.info(f"[{self.config.name}] HP too low ({stats.current_hp}), regenerating")
            self.avatar_state = AvatarState.REGENERATING
            return

        target = self._current_target
        if not target:
            for monster in self.ARENA_MONSTERS:
                target = monster
                break

        self._kill_attempts += 1
        logger.info(f"[{self.config.name}] Attacking {target}...")
        combat_started, instant_kill = await self.actions.kill(target)

        if not combat_started:
            logger.info(f"[{self.config.name}] Target '{target}' not found, searching again")
            self._current_target = None
            self.avatar_state = AvatarState.FINDING_ARENA
        elif instant_kill:
            self._kills += 1
            logger.info(f"[{self.config.name}] Kill #{self._kills}/{self._get_kills_needed()}")
            self._current_target = None
            self.avatar_state = AvatarState.FINDING_ARENA
        else:
            self.avatar_state = AvatarState.WAITING_COMBAT

    async def _wait_combat(self: "ClassProgressionBot") -> None:
        """Wait for combat to end."""
        victory = await self.actions.wait_for_combat_end(timeout=30.0)
        if victory:
            self._kills += 1
            logger.info(f"[{self.config.name}] Kill #{self._kills}/{self._get_kills_needed()}")
            self._current_target = None
            self.avatar_state = AvatarState.FINDING_ARENA
        else:
            logger.warning(f"[{self.config.name}] Combat ended unfavorably")
            self._current_target = None
            self.avatar_state = AvatarState.REGENERATING

    async def _regenerate(self: "ClassProgressionBot") -> None:
        """Wait for HP to regenerate."""
        logger.info(f"[{self.config.name}] Waiting to regenerate...")

        for _ in range(10):
            await asyncio.sleep(2.0)
            stats = await self.actions.score()
            if stats and stats.current_hp >= self._get_min_hp_to_fight():
                logger.info(f"[{self.config.name}] HP recovered: {stats.current_hp}")
                self.avatar_state = AvatarState.KILLING_MONSTERS
                return

        self.avatar_state = AvatarState.KILLING_MONSTERS

    async def _save_and_prepare(self: "ClassProgressionBot") -> None:
        """Save character and prepare for training."""
        await self.actions.save()
        await asyncio.sleep(1.0)
        self.avatar_state = AvatarState.TRAINING_HP

    async def _train_hp(self: "ClassProgressionBot") -> None:
        """Train HP to target."""
        stats = await self.actions.score()
        if not stats:
            logger.error(f"[{self.config.name}] Cannot get stats for HP training")
            self.avatar_state = AvatarState.FAILED
            return

        if stats.max_hp >= self._get_hp_target():
            logger.info(f"[{self.config.name}] HP target reached: {stats.max_hp}")
            self.avatar_state = AvatarState.TRAINING_AVATAR
            return

        logger.info(f"[{self.config.name}] Training HP: {stats.max_hp}/{self._get_hp_target()}")
        result = await self.actions.train("hp")

        if not result['success']:
            logger.warning(f"[{self.config.name}] HP training failed (likely out of exp)")
            self._kills = 0
            self.avatar_state = AvatarState.FINDING_ARENA

    async def _train_avatar(self: "ClassProgressionBot") -> None:
        """Attempt to become an avatar."""
        stats = await self.actions.score()
        if not stats:
            logger.error(f"[{self.config.name}] Cannot get stats for avatar training")
            self.avatar_state = AvatarState.FAILED
            return

        # Check if already an avatar (level 3+)
        if stats.level >= 3:
            logger.info(f"[{self.config.name}] Already an avatar (level {stats.level})!")
            self.avatar_state = AvatarState.AVATAR_COMPLETE
            return

        if stats.max_hp < self._get_hp_target():
            logger.warning(f"[{self.config.name}] HP too low for avatar: {stats.max_hp}")
            self._avatar_train_failures = 0  # Reset on HP change
            self.avatar_state = AvatarState.TRAINING_HP
            return

        if await self.actions.train_avatar():
            logger.info(f"[{self.config.name}] AVATAR PROGRESSION COMPLETE!")
            self.avatar_state = AvatarState.AVATAR_COMPLETE
        else:
            self._avatar_train_failures += 1
            logger.warning(f"[{self.config.name}] Avatar training failed (attempt {self._avatar_train_failures})")

            # If we've failed 3+ times with sufficient HP, assume we're already an avatar
            # This handles cases where the MUD's response doesn't match our patterns
            if self._avatar_train_failures >= 3:
                logger.info(f"[{self.config.name}] Multiple avatar training failures with HP={stats.max_hp} - assuming already avatar")
                self.avatar_state = AvatarState.AVATAR_COMPLETE
            else:
                self.avatar_state = AvatarState.TRAINING_HP
