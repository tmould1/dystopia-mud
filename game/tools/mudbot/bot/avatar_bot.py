"""
Avatar Progression Bot - completes early game progression to avatar status.

Progression path:
1. Login/create character
2. Kill 5 newbie school arena monsters
3. Save character
4. Train HP to 2000
5. Train avatar
"""

import asyncio
import logging
from enum import Enum, auto
from typing import Optional, Callable, Any

from ..config import BotConfig, ProgressionConfig
from .base import MudBot
from .state_machine import BotState
from .actions import BotActions

logger = logging.getLogger(__name__)


class ProgressionState(Enum):
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
    COMPLETE = auto()
    FAILED = auto()


class AvatarProgressionBot(MudBot):
    """
    Bot that completes the early game progression to avatar status.

    Flow:
    1. Connect and login (create character if needed)
    2. Go south from spawn to enter the arena
    3. Kill 5 arena monsters (fox, rabbit, snail, etc.)
    4. Save character
    5. Train HP to 2000
    6. Train avatar (become level 3)
    """

    # Arena monster keywords, ordered by level (easiest first)
    ARENA_MONSTERS = ['fox', 'rabbit', 'snail', 'bear', 'beast', 'wolf', 'lizard', 'boar']

    # Only explore these directions (no up/down in arena)
    EXPLORE_DIRECTIONS = ['south', 'north', 'east', 'west']

    def __init__(
        self,
        config: BotConfig,
        progression_config: Optional[ProgressionConfig] = None
    ):
        super().__init__(config)
        self.prog_config = progression_config or ProgressionConfig()
        self.actions = BotActions(self)

        # Progression tracking
        self._prog_state = ProgressionState.START
        self._kills = 0
        self._kill_attempts = 0
        self._exploration_path: list[str] = []
        self._in_arena = False
        self._current_target: Optional[str] = None

        # Pause support
        self._paused = asyncio.Event()
        self._paused.set()  # Not paused by default

        # Status callback (called when state changes)
        self.on_status_change: Optional[Callable[[dict], Any]] = None

    @property
    def progression_state(self) -> ProgressionState:
        """Current progression state."""
        return self._prog_state

    @progression_state.setter
    def progression_state(self, value: ProgressionState):
        if value != self._prog_state:
            logger.info(f"[{self.config.name}] Progression: {self._prog_state.name} -> {value.name}")
            self._prog_state = value
            # Notify status change
            self._notify_status_change()

    @property
    def is_paused(self) -> bool:
        """Check if bot is paused."""
        return not self._paused.is_set()

    def pause(self) -> None:
        """Pause the bot progression."""
        if self._paused.is_set():
            logger.info(f"[{self.config.name}] Pausing...")
            self._paused.clear()
            self._notify_status_change()

    def resume(self) -> None:
        """Resume the bot progression."""
        if not self._paused.is_set():
            logger.info(f"[{self.config.name}] Resuming...")
            self._paused.set()
            self._notify_status_change()

    async def _wait_if_paused(self) -> None:
        """Wait if the bot is paused."""
        await self._paused.wait()

    def _notify_status_change(self) -> None:
        """Call status change callback if set."""
        if self.on_status_change:
            try:
                self.on_status_change(self.get_status())
            except Exception as e:
                logger.warning(f"[{self.config.name}] Status callback error: {e}")

    async def run_progression(self) -> bool:
        """
        Execute the full avatar progression.

        Returns:
            True if progression completed successfully.
        """
        logger.info(f"[{self.config.name}] Starting avatar progression")
        logger.info(f"[{self.config.name}] Goal: Kill {self.prog_config.kills_needed} monsters, train HP to {self.prog_config.hp_target}, become avatar")

        self.progression_state = ProgressionState.CHECKING_STATS

        # Main progression loop
        while self._prog_state not in (ProgressionState.COMPLETE, ProgressionState.FAILED):
            try:
                await self._progression_tick()
                await asyncio.sleep(0.5)  # Pace the bot
            except asyncio.CancelledError:
                logger.info(f"[{self.config.name}] Progression cancelled")
                return False
            except Exception as e:
                logger.error(f"[{self.config.name}] Progression error: {e}", exc_info=True)
                self.progression_state = ProgressionState.FAILED
                return False

        return self._prog_state == ProgressionState.COMPLETE

    async def _progression_tick(self) -> None:
        """Execute one step of the progression based on current state."""
        # Wait if paused
        await self._wait_if_paused()

        if self._prog_state == ProgressionState.CHECKING_STATS:
            await self._check_initial_stats()

        elif self._prog_state == ProgressionState.FINDING_ARENA:
            await self._find_arena()

        elif self._prog_state == ProgressionState.KILLING_MONSTERS:
            await self._kill_monster()

        elif self._prog_state == ProgressionState.WAITING_COMBAT:
            await self._wait_combat()

        elif self._prog_state == ProgressionState.REGENERATING:
            await self._regenerate()

        elif self._prog_state == ProgressionState.SAVING:
            await self._save_and_prepare()

        elif self._prog_state == ProgressionState.TRAINING_HP:
            await self._train_hp()

        elif self._prog_state == ProgressionState.TRAINING_AVATAR:
            await self._train_avatar()

    async def _check_initial_stats(self) -> None:
        """Check starting stats to determine what we need to do."""
        stats = await self.actions.score()
        if not stats:
            logger.error(f"[{self.config.name}] Cannot get initial stats")
            self.progression_state = ProgressionState.FAILED
            return

        logger.info(f"[{self.config.name}] Current stats: HP={stats.max_hp}, Exp={stats.exp}, Level={stats.level}")

        # If already avatar (level 3+), we're done
        if stats.level >= 3:
            logger.info(f"[{self.config.name}] Already an avatar!")
            self.progression_state = ProgressionState.COMPLETE
            return

        # If HP >= 2000, can try avatar training
        if stats.max_hp >= self.prog_config.hp_target:
            logger.info(f"[{self.config.name}] HP sufficient, attempting avatar training")
            self.progression_state = ProgressionState.TRAINING_AVATAR
            return

        # Need to kill monsters and train HP
        logger.info(f"[{self.config.name}] Need to earn exp and train HP")
        self.progression_state = ProgressionState.FINDING_ARENA

    async def _find_arena(self) -> None:
        """Navigate to arena and find monsters to kill."""
        # First, look around
        room_text = await self.actions.look()
        room_lower = room_text.lower()

        # Check if we're in the arena (room name on first line contains "arena")
        # Room format: "Room Name\nExits: ..." - check first line only
        first_line = room_text.split('\n')[0].strip().lower() if room_text else ""
        if 'arena' in first_line:
            self._in_arena = True
            logger.info(f"[{self.config.name}] In the arena: {first_line}")

        # Look for arena monsters in the room (but not corpses/blood pools)
        corpse_words = ['entrails', 'corpse', 'blood', 'pool', 'body', 'remains']
        for monster in self.ARENA_MONSTERS:
            if monster in room_lower:
                # Check if this is a corpse/blood description
                is_corpse = False
                for line in room_text.split('\n'):
                    line_lower = line.lower()
                    if monster in line_lower:
                        if any(cw in line_lower for cw in corpse_words):
                            is_corpse = True
                            break
                        # Found the monster in a non-corpse line
                        logger.info(f"[{self.config.name}] Found target: {monster}")
                        self._current_target = monster
                        self.progression_state = ProgressionState.KILLING_MONSTERS
                        return
                if is_corpse:
                    logger.debug(f"[{self.config.name}] Skipping corpse: {monster}")

        # No monster found, need to navigate
        exits = await self.actions.get_exits()
        if not exits:
            logger.warning(f"[{self.config.name}] No exits found, stuck!")
            self.progression_state = ProgressionState.FAILED
            return

        # If not in arena yet, go south (arena is south of spawn)
        if not self._in_arena and 'south' in exits:
            logger.info(f"[{self.config.name}] Heading south to arena...")
            await self.actions.move('south')
            return

        # In arena but no monster here, explore within arena (NESW only)
        for direction in self.EXPLORE_DIRECTIONS:
            if direction in exits and direction not in self._exploration_path:
                logger.info(f"[{self.config.name}] Exploring arena: {direction}")
                self._exploration_path.append(direction)
                if await self.actions.move(direction):
                    return

        # Tried all directions, reset and try again
        self._exploration_path = []
        # Pick a random available direction from NESW
        for direction in self.EXPLORE_DIRECTIONS:
            if direction in exits:
                logger.info(f"[{self.config.name}] Continuing exploration: {direction}")
                await self.actions.move(direction)
                return

    async def _kill_monster(self) -> None:
        """Attack a monster."""
        if self._kills >= self.prog_config.kills_needed:
            logger.info(f"[{self.config.name}] Killed enough monsters ({self._kills}), saving...")
            self.progression_state = ProgressionState.SAVING
            return

        if self._kill_attempts >= self.prog_config.max_kill_attempts:
            logger.error(f"[{self.config.name}] Too many kill attempts, giving up")
            self.progression_state = ProgressionState.FAILED
            return

        # Check HP before fighting
        stats = await self.actions.score()
        if stats and stats.current_hp < self.prog_config.min_hp_to_fight:
            logger.info(f"[{self.config.name}] HP too low ({stats.current_hp}), regenerating")
            self.progression_state = ProgressionState.REGENERATING
            return

        # Use the target we found, or try each arena monster
        target = self._current_target
        if not target:
            # No specific target, try the easiest monsters first
            for monster in self.ARENA_MONSTERS:
                target = monster
                break

        # Try to kill
        self._kill_attempts += 1
        logger.info(f"[{self.config.name}] Attacking {target}...")
        combat_started, instant_kill = await self.actions.kill(target)
        if not combat_started:
            # Target not found, clear it and search again
            logger.info(f"[{self.config.name}] Target '{target}' not found, searching again")
            self._current_target = None
            self.progression_state = ProgressionState.FINDING_ARENA
        elif instant_kill:
            # Target died instantly, count the kill
            self._kills += 1
            logger.info(f"[{self.config.name}] Kill #{self._kills}/{self.prog_config.kills_needed}")
            self._current_target = None
            self.progression_state = ProgressionState.FINDING_ARENA
        else:
            self.progression_state = ProgressionState.WAITING_COMBAT

    async def _wait_combat(self) -> None:
        """Wait for combat to end."""
        victory = await self.actions.wait_for_combat_end(timeout=30.0)
        if victory:
            self._kills += 1
            logger.info(f"[{self.config.name}] Kill #{self._kills}/{self.prog_config.kills_needed}")
            # Clear target so we find a new one
            self._current_target = None
            # Go back to finding to look for next target
            self.progression_state = ProgressionState.FINDING_ARENA
        else:
            # Died or timeout
            logger.warning(f"[{self.config.name}] Combat ended unfavorably")
            self._current_target = None
            self.progression_state = ProgressionState.REGENERATING

    async def _regenerate(self) -> None:
        """Wait for HP to regenerate."""
        logger.info(f"[{self.config.name}] Waiting to regenerate...")

        # Simple approach: just wait and check stats periodically
        for _ in range(10):  # Wait up to ~20 seconds
            await asyncio.sleep(2.0)
            stats = await self.actions.score()
            if stats and stats.current_hp >= self.prog_config.min_hp_to_fight:
                logger.info(f"[{self.config.name}] HP recovered: {stats.current_hp}")
                self.progression_state = ProgressionState.KILLING_MONSTERS
                return

        # Still low HP, try again
        self.progression_state = ProgressionState.KILLING_MONSTERS

    async def _save_and_prepare(self) -> None:
        """Save character and prepare for training."""
        await self.actions.save()
        await asyncio.sleep(1.0)
        self.progression_state = ProgressionState.TRAINING_HP

    async def _train_hp(self) -> None:
        """Train HP to target."""
        stats = await self.actions.score()
        if not stats:
            logger.error(f"[{self.config.name}] Cannot get stats for HP training")
            self.progression_state = ProgressionState.FAILED
            return

        if stats.max_hp >= self.prog_config.hp_target:
            logger.info(f"[{self.config.name}] HP target reached: {stats.max_hp}")
            self.progression_state = ProgressionState.TRAINING_AVATAR
            return

        # Train HP
        logger.info(f"[{self.config.name}] Training HP: {stats.max_hp}/{self.prog_config.hp_target}")
        result = await self.actions.train("hp")

        if not result['success']:
            # Out of exp, need to kill more
            logger.warning(f"[{self.config.name}] HP training failed (likely out of exp)")
            # Reset kills and go back to farming
            self._kills = 0
            self.progression_state = ProgressionState.FINDING_ARENA

    async def _train_avatar(self) -> None:
        """Attempt to become an avatar."""
        # Double check HP requirement
        stats = await self.actions.score()
        if stats and stats.max_hp < self.prog_config.hp_target:
            logger.warning(f"[{self.config.name}] HP too low for avatar: {stats.max_hp}")
            self.progression_state = ProgressionState.TRAINING_HP
            return

        if await self.actions.train_avatar():
            logger.info(f"[{self.config.name}] AVATAR PROGRESSION COMPLETE!")
            self.progression_state = ProgressionState.COMPLETE
        else:
            logger.error(f"[{self.config.name}] Avatar training failed")
            # Maybe need more HP? Try training more
            self.progression_state = ProgressionState.TRAINING_HP

    async def run(self) -> None:
        """
        Main bot loop - connect, login, and run progression.
        """
        # Connect
        if not await self.connect():
            logger.error(f"[{self.config.name}] Connection failed")
            return

        # Login
        if not await self.login():
            logger.error(f"[{self.config.name}] Login failed")
            await self.disconnect()
            return

        # Wait for login sequence to complete and drain any pending messages
        logger.info(f"[{self.config.name}] Waiting for login to settle...")
        await asyncio.sleep(3.0)
        # Send a blank line to clear any input state
        await self.client.send("")
        await asyncio.sleep(1.0)
        # Drain pending data (login announcements, room description, etc.)
        drained_text = ""
        for _ in range(10):
            chunk = await self.client.read(timeout=1.0)
            if chunk:
                drained_text += chunk
                logger.debug(f"[{self.config.name}] Drained chunk: {chunk[:300]}...")
            else:
                break
        if drained_text:
            logger.info(f"[{self.config.name}] Total drained ({len(drained_text)} chars): {drained_text[:500]}...")

        # Run progression
        success = await self.run_progression()

        if success:
            logger.info(f"[{self.config.name}] Bot completed successfully!")
        else:
            logger.error(f"[{self.config.name}] Bot failed to complete progression")

        # Save and quit gracefully
        await self.actions.save()
        await self.actions.quit()
        await self.disconnect()

    def get_status(self) -> dict:
        """Get current bot status."""
        return {
            'name': self.config.name,
            'connected': self.connected,
            'logged_in': self.logged_in,
            'state': self.state.name,
            'progression_state': self._prog_state.name,
            'kills': self._kills,
            'target_kills': self.prog_config.kills_needed,
            'hp_target': self.prog_config.hp_target,
            'is_paused': self.is_paused,
        }
