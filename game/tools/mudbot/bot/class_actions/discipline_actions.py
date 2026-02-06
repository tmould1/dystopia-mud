"""
Shared discipline actions for discipline-based classes.

Used by: Demon, Vampire, Werewolf

These classes share the research/train discipline progression system:
1. research <discipline> - Start researching
2. Kill mobs to gain discipline points
3. train <discipline> - Advance level when research complete
"""

import logging
from typing import Dict, Optional, TYPE_CHECKING

from .base import ClassActions
from ...utils.class_parsers.discipline_parser import DisciplineParser

if TYPE_CHECKING:
    from ..base import MudBot

logger = logging.getLogger(__name__)


class DisciplineActions(ClassActions):
    """
    Actions for discipline-based progression.

    Provides research, train, and discipline management commands
    shared by Demon, Vampire, and Werewolf classes.
    """

    def __init__(self, bot: "MudBot"):
        super().__init__(bot)
        self.parser = DisciplineParser()

        # Track current state
        self._current_research: Optional[str] = None
        self._research_complete: bool = False
        self._discipline_levels: Dict[str, int] = {}

    @property
    def current_research(self) -> Optional[str]:
        """Currently researching discipline name."""
        return self._current_research

    @property
    def research_complete(self) -> bool:
        """Whether current research is complete and ready to train."""
        return self._research_complete

    @property
    def discipline_levels(self) -> Dict[str, int]:
        """Cached discipline levels from last query."""
        return self._discipline_levels

    async def get_disciplines(self) -> Dict[str, int]:
        """
        Query current discipline levels.

        Returns:
            Dict mapping discipline name to level.
        """
        response = await self.send_and_read("disciplines", timeout=5.0)

        if response:
            self._discipline_levels = self.parser.parse_discipline_list(response)
            logger.debug(f"[{self.bot.config.name}] Disciplines: {self._discipline_levels}")

        return self._discipline_levels

    async def start_research(self, discipline: str) -> bool:
        """
        Start researching a discipline.

        Args:
            discipline: Name of discipline to research (e.g., "attack").

        Returns:
            True if research started successfully.
        """
        logger.info(f"[{self.bot.config.name}] Starting research: {discipline}")
        response = await self.send_and_read(f"research {discipline}", timeout=5.0)

        if response:
            result = self.parser.parse_research_response(response)

            if result['started']:
                self._current_research = discipline
                self._research_complete = False
                logger.info(f"[{self.bot.config.name}] Research started: {discipline}")
                return True

            if result['already_researching']:
                logger.info(f"[{self.bot.config.name}] Already researching")
                # Check if it's already complete
                if result['complete']:
                    self._research_complete = True
                return True

            if result['complete']:
                self._research_complete = True
                logger.info(f"[{self.bot.config.name}] Research already complete!")
                return True

        return False

    async def cancel_research(self) -> bool:
        """
        Cancel current research.

        Returns:
            True if research cancelled.
        """
        logger.info(f"[{self.bot.config.name}] Cancelling research")
        response = await self.send_and_read("research cancel", timeout=5.0)

        if response:
            result = self.parser.parse_research_response(response)
            if result['cancelled']:
                self._current_research = None
                self._research_complete = False
                return True

        return False

    async def check_research_status(self) -> tuple[bool, bool]:
        """
        Check current research status.

        Returns:
            Tuple of (is_researching, is_complete).
        """
        # Query disciplines which shows research status
        response = await self.send_and_read("disciplines", timeout=5.0)

        is_researching = self._current_research is not None
        is_complete = False

        if response:
            # Check for completion message in the response
            if self.parser.check_research_complete(response):
                self._research_complete = True
                is_complete = True

            # Update discipline levels
            self._discipline_levels = self.parser.parse_discipline_list(response)

        return is_researching, self._research_complete or is_complete

    async def train_discipline(self, discipline: str) -> dict:
        """
        Train a researched discipline.

        Args:
            discipline: Name of discipline to train.

        Returns:
            Dict with:
            - success: bool
            - discipline: str (if successful)
            - unlocked: list of unlocked abilities
            - not_researching: bool
            - not_finished: bool
        """
        logger.info(f"[{self.bot.config.name}] Training discipline: {discipline}")
        response = await self.send_and_read(
            f"train {discipline}",
            timeout=5.0
        )

        result = {
            'success': False,
            'discipline': None,
            'unlocked': [],
            'not_researching': False,
            'not_finished': False,
        }

        if response:
            parsed = self.parser.parse_train_response(response)

            if parsed['success']:
                result['success'] = True
                result['discipline'] = parsed.get('discipline', discipline)
                result['unlocked'] = parsed.get('unlocked', [])

                # Reset research state
                self._current_research = None
                self._research_complete = False

                # Log unlocked abilities
                if result['unlocked']:
                    logger.info(f"[{self.bot.config.name}] Unlocked: {result['unlocked']}")

            else:
                result['not_researching'] = parsed.get('not_researching', False)
                result['not_finished'] = parsed.get('not_finished', False)

        return result

    def check_output_for_disc_point(self, text: str) -> bool:
        """
        Check if text contains discipline point gain message.

        Call this from the main bot loop to detect point gains during combat.

        Args:
            text: Text output from server.

        Returns:
            True if discipline point was gained.
        """
        return self.parser.check_disc_point_gain(text)

    def check_output_for_research_complete(self, text: str) -> bool:
        """
        Check if text contains research completion message.

        Call this from the main bot loop to detect research completion.

        Args:
            text: Text output from server.

        Returns:
            True if research is complete.
        """
        if self.parser.check_research_complete(text):
            self._research_complete = True
            return True
        return False

    async def toggle_ability(self, ability: str, enable: bool = True) -> bool:
        """
        Toggle a combat ability on/off.

        Args:
            ability: Ability name (e.g., "claws", "horns").
            enable: True to enable, False to disable.

        Returns:
            True if toggle was successful.
        """
        logger.debug(f"[{self.bot.config.name}] Toggling {ability}: {'on' if enable else 'off'}")
        response = await self.send_and_read(ability, timeout=3.0)

        if response:
            response_lower = response.lower()
            # Check for toggle-on indicators
            if enable:
                if any(word in response_lower for word in ['extend', 'grow', 'unfold', 'sprout']):
                    return True
            # Check for toggle-off indicators
            else:
                if any(word in response_lower for word in ['retract', 'shrink', 'fold', 'withdraw']):
                    return True

        return False
