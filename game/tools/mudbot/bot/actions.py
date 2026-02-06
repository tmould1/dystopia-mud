"""
Reusable bot actions for common MUD commands.
"""

import asyncio
import logging
from typing import Optional, TYPE_CHECKING

from ..utils.parsers import ScoreParser, CombatParser, RoomParser, TrainParser, VitalStats

if TYPE_CHECKING:
    from .base import MudBot

logger = logging.getLogger(__name__)


class BotActions:
    """
    Collection of reusable bot actions.

    Provides high-level commands like move, look, kill, train
    that handle the details of sending commands and parsing responses.
    """

    def __init__(self, bot: "MudBot"):
        self.bot = bot
        self.score_parser = ScoreParser()
        self.combat_parser = CombatParser()
        self.room_parser = RoomParser()
        self.train_parser = TrainParser()

        # Cached state
        self._last_stats: Optional[VitalStats] = None
        self._in_combat = False

    async def look(self) -> str:
        """
        Look at the current room.

        Returns:
            Room description text.
        """
        # Wait for exits line which appears in room descriptions
        # Handle both bracketed [Exits:] and plain "Exits:" formats
        response = await self.bot.send_and_read(
            "look",
            wait_for=["[Exits:", "[Exit:", "Exits:", "Obvious exits:"],
            timeout=5.0
        )
        logger.debug(f"[{self.bot.config.name}] Look response ({len(response) if response else 0} chars): {response[:300] if response else 'None'}...")
        return response or ""

    async def score(self) -> Optional[VitalStats]:
        """
        Get character stats via score command.

        Returns:
            VitalStats if parsing successful.
        """
        # Wait for patterns that indicate score output
        response = await self.bot.send_and_read(
            "score",
            wait_for=["Hit Points:", "primal energy"],
            timeout=10.0
        )
        logger.debug(f"[{self.bot.config.name}] Score response: {response[:300] if response else 'None'}...")
        if response:
            stats = self.score_parser.parse(response)
            if stats:
                self._last_stats = stats
                logger.debug(f"[{self.bot.config.name}] Stats: HP={stats.current_hp}/{stats.max_hp} Exp={stats.exp}")
            else:
                logger.warning(f"[{self.bot.config.name}] Could not parse stats from response")
            return stats
        logger.warning(f"[{self.bot.config.name}] No response from score command")
        return None

    async def move(self, direction: str) -> bool:
        """
        Move in a direction.

        Args:
            direction: north, south, east, west, up, down (or n, s, e, w, u, d)

        Returns:
            True if move was successful (didn't hit a wall).
        """
        # Normalize direction
        dir_map = {'n': 'north', 's': 'south', 'e': 'east', 'w': 'west', 'u': 'up', 'd': 'down'}
        direction = dir_map.get(direction.lower(), direction.lower())

        response = await self.bot.send_and_read(direction, timeout=3.0)
        if response:
            # Check for failure messages
            if "Alas, you cannot go that way" in response or "no exit" in response.lower():
                logger.debug(f"[{self.bot.config.name}] Cannot move {direction}")
                return False
        return True

    async def kill(self, target: str) -> tuple[bool, bool]:
        """
        Start combat with a target.

        Args:
            target: Target name/keyword.

        Returns:
            Tuple of (combat_started, instant_kill).
            instant_kill is True if target died immediately.
        """
        logger.info(f"[{self.bot.config.name}] Attacking: {target}")
        response = await self.bot.send_and_read(f"kill {target}", timeout=3.0)
        if response:
            response_lower = response.lower()
            if "isn't here" in response_lower or "don't see" in response_lower:
                logger.debug(f"[{self.bot.config.name}] Target not found: {target}")
                return False, False
            if "already fighting" in response_lower:
                logger.debug(f"[{self.bot.config.name}] Already in combat")
                self._in_combat = True
                return True, False
            # Check if target died instantly in the response
            is_over, victory = self.combat_parser.is_combat_over(response)
            if is_over and victory:
                logger.info(f"[{self.bot.config.name}] Instant kill!")
                self._in_combat = False
                return True, True
            # Also check for experience gain which means kill
            if "you receive" in response_lower and "exp" in response_lower:
                logger.info(f"[{self.bot.config.name}] Instant kill (exp gained)!")
                self._in_combat = False
                return True, True
            self._in_combat = True
        return True, False

    async def wait_for_combat_end(self, timeout: float = 60.0) -> bool:
        """
        Wait for current combat to end.

        Args:
            timeout: Maximum time to wait.

        Returns:
            True if combat ended in victory.
        """
        logger.debug(f"[{self.bot.config.name}] Waiting for combat to end...")
        start_time = asyncio.get_event_loop().time()
        last_check_time = start_time

        while True:
            elapsed = asyncio.get_event_loop().time() - start_time
            if elapsed > timeout:
                logger.warning(f"[{self.bot.config.name}] Combat timeout after {timeout}s")
                self._in_combat = False
                return False

            text = await self.bot.client.read(timeout=2.0)
            if text:
                is_over, victory = self.combat_parser.is_combat_over(text)
                if is_over:
                    self._in_combat = False
                    if victory:
                        logger.info(f"[{self.bot.config.name}] Victory!")
                    else:
                        logger.warning(f"[{self.bot.config.name}] Defeated!")
                    return victory

            # Every 10 seconds, do a secondary check with "look" to verify target status
            current_time = asyncio.get_event_loop().time()
            if current_time - last_check_time > 10.0:
                last_check_time = current_time
                logger.debug(f"[{self.bot.config.name}] Combat check: verifying target status...")
                # Send a look command to check if we're still in combat
                await self.bot.client.send("look")
                await asyncio.sleep(0.5)
                look_text = await self.bot.client.read(timeout=2.0)
                if look_text:
                    # Check for death indicators in the look response
                    is_over, victory = self.combat_parser.is_combat_over(look_text)
                    if is_over:
                        self._in_combat = False
                        return victory
                    # If no combat indicators in the room, assume victory
                    if not self.combat_parser.is_in_combat(look_text):
                        logger.info(f"[{self.bot.config.name}] No combat activity detected, assuming victory")
                        self._in_combat = False
                        return True

            await asyncio.sleep(0.1)

    async def train(self, stat: str, amount: str = "all") -> dict:
        """
        Train a stat (hp, str, int, etc.).

        Args:
            stat: Stat to train (hp, str, int, wis, dex, con, avatar, etc.)
            amount: Amount to train ("all" or a number)

        Returns:
            Dict with 'success', 'new_hp', 'became_avatar' keys.
        """
        command = f"train {stat} {amount}" if stat != "avatar" else f"train {stat}"
        logger.info(f"[{self.bot.config.name}] Training: {command}")
        # Wait for training response patterns
        response = await self.bot.send_and_read(
            command,
            wait_for=["hit points", "increases", "avatar", "not have enough", "cannot train", "need at least"],
            timeout=5.0
        )
        logger.debug(f"[{self.bot.config.name}] Train response: {response[:200] if response else 'None'}...")
        if response:
            return self.train_parser.parse(response)
        return {'success': False, 'error': 'No response'}

    async def train_hp_to_target(self, target_hp: int) -> bool:
        """
        Train HP until reaching target.

        Args:
            target_hp: Target max HP.

        Returns:
            True if target reached.
        """
        logger.info(f"[{self.bot.config.name}] Training HP to {target_hp}")

        while True:
            # Get current stats
            stats = await self.score()
            if not stats:
                logger.error(f"[{self.bot.config.name}] Cannot get stats")
                return False

            if stats.max_hp >= target_hp:
                logger.info(f"[{self.bot.config.name}] HP target reached: {stats.max_hp}")
                return True

            # Train HP
            result = await self.train("hp")
            if not result['success']:
                logger.warning(f"[{self.bot.config.name}] HP training failed: {result.get('error')}")
                # Probably out of exp, need to kill more
                return False

            await asyncio.sleep(0.2)  # Small delay between trains

    async def train_avatar(self) -> bool:
        """
        Attempt to train avatar (become level 3).

        Returns:
            True if successfully became avatar.
        """
        logger.info(f"[{self.bot.config.name}] Attempting to train avatar")
        result = await self.train("avatar")
        if result.get('became_avatar'):
            logger.info(f"[{self.bot.config.name}] BECAME AVATAR!")
            return True
        else:
            logger.warning(f"[{self.bot.config.name}] Avatar training failed: {result.get('error')}")
            return False

    async def save(self) -> bool:
        """
        Save the character.

        Returns:
            True if save command sent.
        """
        logger.info(f"[{self.bot.config.name}] Saving character")
        await self.bot.send_command("save")
        return True

    async def quit(self) -> bool:
        """
        Quit the game gracefully.

        Returns:
            True if quit command sent.
        """
        logger.info(f"[{self.bot.config.name}] Quitting game")
        await self.bot.send_command("quit")
        await asyncio.sleep(0.5)  # Give server time to process
        return True

    async def get_exits(self) -> list[str]:
        """
        Get available exits from current room.

        Returns:
            List of exit directions.
        """
        text = await self.look()
        return self.room_parser.parse_exits(text)

    async def find_targets(self) -> list[str]:
        """
        Find potential targets in current room.

        Returns:
            List of target keywords.
        """
        text = await self.look()
        return self.room_parser.find_targets(text)

    async def flee(self) -> bool:
        """
        Attempt to flee from combat.

        Returns:
            True if flee command sent.
        """
        logger.info(f"[{self.bot.config.name}] Fleeing!")
        await self.bot.send_command("flee")
        self._in_combat = False
        return True

    @property
    def in_combat(self) -> bool:
        """Check if currently in combat."""
        return self._in_combat

    @property
    def last_stats(self) -> Optional[VitalStats]:
        """Get last known stats."""
        return self._last_stats
