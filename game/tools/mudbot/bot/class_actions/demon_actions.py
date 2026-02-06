"""
Demon-specific actions for mudbot.

Provides demon-specific abilities beyond the shared discipline system:
- Warp system (obtain)
- Graft system
- Inpart system
- Demonarmour creation
"""

import logging
from typing import List, Optional, TYPE_CHECKING

from .discipline_actions import DisciplineActions
from ...utils.class_parsers.demon_parser import DemonParser

if TYPE_CHECKING:
    from ..base import MudBot

logger = logging.getLogger(__name__)


class DemonActions(DisciplineActions):
    """
    Demon-specific actions.

    Extends DisciplineActions with demon-specific abilities.
    """

    # Default demon armour pieces in priority order
    DEFAULT_ARMOUR_PIECES = [
        'plate', 'helmet', 'leggings', 'boots', 'gauntlets',
        'sleeves', 'cape', 'belt', 'collar', 'ring',
        'bracer', 'visor', 'longsword', 'shortsword'
    ]

    def __init__(self, bot: "MudBot"):
        super().__init__(bot)
        # Override parser with demon-specific parser
        self.parser = DemonParser()

        # Track demon-specific state
        self._warps_obtained = 0
        self._extra_arms = 0
        self._armour_created: List[str] = []

    @property
    def warps_obtained(self) -> int:
        """Number of warps obtained."""
        return self._warps_obtained

    @property
    def extra_arms(self) -> int:
        """Number of extra arms grafted (0-2)."""
        return self._extra_arms

    @property
    def armour_created(self) -> List[str]:
        """List of armour pieces created."""
        return self._armour_created

    async def obtain_warp(self) -> dict:
        """
        Attempt to obtain a new warp.

        Returns:
            Dict with:
            - success: bool
            - not_enough_points: bool
            - max_warps: bool
        """
        logger.info(f"[{self.bot.config.name}] Attempting to obtain warp")
        response = await self.send_and_read("obtain", timeout=5.0)

        result = {'success': False, 'not_enough_points': False, 'max_warps': False}

        if response:
            result = self.parser.parse_obtain_response(response)
            if result['success']:
                self._warps_obtained += 1
                logger.info(f"[{self.bot.config.name}] Obtained warp #{self._warps_obtained}!")

        return result

    async def graft_arm(self, arm_keyword: str = "arm") -> dict:
        """
        Attempt to graft an arm.

        Args:
            arm_keyword: Keyword for arm object in inventory.

        Returns:
            Dict with:
            - success: bool
            - no_arm: bool
            - not_arm: bool
            - full: bool
        """
        logger.info(f"[{self.bot.config.name}] Attempting to graft: {arm_keyword}")
        response = await self.send_and_read(f"graft {arm_keyword}", timeout=5.0)

        result = {'success': False, 'no_arm': False, 'not_arm': False, 'full': False}

        if response:
            result = self.parser.parse_graft_response(response)
            if result['success']:
                self._extra_arms += 1
                logger.info(f"[{self.bot.config.name}] Grafted arm #{self._extra_arms}!")

        return result

    async def inpart_power(self, power: str, target: str = "self") -> dict:
        """
        Inpart a demonic power.

        Args:
            power: Power name (e.g., "fangs", "claws").
            target: Target for power (usually "self").

        Returns:
            Dict with:
            - success: bool
            - not_enough_points: bool
            - already_have: bool
        """
        logger.info(f"[{self.bot.config.name}] Inparting {power} to {target}")
        response = await self.send_and_read(f"inpart {target} {power}", timeout=5.0)

        result = {'success': False, 'not_enough_points': False, 'already_have': False}

        if response:
            result = self.parser.parse_inpart_response(response)
            if result['success']:
                logger.info(f"[{self.bot.config.name}] Inparted {power}!")

        return result

    async def create_demonarmour(self, piece: str) -> bool:
        """
        Create a piece of demon armour.

        Args:
            piece: Armour piece name (plate, helmet, etc.).

        Returns:
            True if created successfully.
        """
        logger.info(f"[{self.bot.config.name}] Creating demonarmour: {piece}")
        response = await self.send_and_read(f"demonarmour {piece}", timeout=5.0)

        if response:
            result = self.parser.parse_armour_response(response)
            if result['success']:
                self._armour_created.append(piece)
                logger.info(f"[{self.bot.config.name}] Created {piece}!")
                return True
            if result['not_enough_primal']:
                logger.info(f"[{self.bot.config.name}] Not enough primal for {piece}")

        return False

    async def find_arms_in_inventory(self) -> List[str]:
        """
        Look for arms in inventory.

        Returns:
            List of arm keywords found.
        """
        response = await self.send_and_read("inventory", timeout=5.0)

        if response:
            arms = self.parser.find_arms_in_text(response)
            if arms:
                logger.debug(f"[{self.bot.config.name}] Found arms: {arms}")
            return arms

        return []

    async def get_demon_points(self) -> Optional[int]:
        """
        Get current demon points from score.

        Returns:
            Current demon points or None if cannot parse.
        """
        response = await self.send_and_read("score", timeout=5.0)

        if response:
            # Look for demon points in score output
            # Pattern: "Demon Points: 12345" or similar
            import re
            match = re.search(r"Demon\s*Points?:\s*(\d+)", response, re.IGNORECASE)
            if match:
                return int(match.group(1))

        return None

    async def toggle_claws(self, enable: bool = True) -> bool:
        """Toggle claws on/off."""
        return await self.toggle_ability("claws", enable)

    async def toggle_horns(self, enable: bool = True) -> bool:
        """Toggle horns on/off."""
        return await self.toggle_ability("horns", enable)

    async def toggle_wings(self, enable: bool = True) -> bool:
        """Toggle wings on/off."""
        return await self.toggle_ability("wings", enable)

    async def toggle_tail(self, enable: bool = True) -> bool:
        """Toggle tail on/off."""
        return await self.toggle_ability("tail", enable)

    async def enable_combat_abilities(self, discipline_levels: dict) -> None:
        """
        Enable all available combat abilities based on discipline levels.

        Args:
            discipline_levels: Dict mapping discipline name to level.
        """
        attack_level = discipline_levels.get("attack", 0)

        if attack_level >= 2:
            await self.toggle_claws(True)
        if attack_level >= 3:
            await self.toggle_tail(True)
        if attack_level >= 4:
            await self.toggle_horns(True)
        if attack_level >= 5:
            await self.toggle_wings(True)

    async def create_all_armour(self, pieces: Optional[List[str]] = None) -> int:
        """
        Create all specified armour pieces.

        Args:
            pieces: List of piece names, or None for default list.

        Returns:
            Number of pieces successfully created.
        """
        if pieces is None:
            pieces = self.DEFAULT_ARMOUR_PIECES

        created = 0
        for piece in pieces:
            if piece in self._armour_created:
                continue  # Already created

            if await self.create_demonarmour(piece):
                created += 1
            else:
                # Probably out of primal, stop trying
                break

        return created
