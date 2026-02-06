"""
Shared discipline parser for discipline-based classes.

Used by: Demon, Vampire, Werewolf

Parses output related to the research/train discipline system.
"""

import re
import logging
from typing import Dict, List

from .base import BaseClassParser

logger = logging.getLogger(__name__)


class DisciplineParser(BaseClassParser):
    """
    Parser for discipline-related command output.

    Handles parsing of:
    - research command responses
    - train command responses
    - disciplines command output
    - discipline point gain messages
    - ability unlock messages
    """

    # Research patterns
    RESEARCH_START = re.compile(
        r"You begin your research into (\w+)\.",
        re.IGNORECASE
    )
    RESEARCH_CANCEL = re.compile(
        r"You stop your research\.",
        re.IGNORECASE
    )
    RESEARCH_ALREADY = re.compile(
        r"You are already researching a discipline\.",
        re.IGNORECASE
    )
    RESEARCH_COMPLETE = re.compile(
        r"You have finished researching your discipline|"
        r"You may now use the 'train' command",
        re.IGNORECASE
    )

    # Discipline point gain
    DISC_POINT_GAIN = re.compile(
        r"You gained a discipline point\.",
        re.IGNORECASE
    )

    # Training patterns
    TRAIN_SUCCESS = re.compile(
        r"Your mastery of (\w+) increases\.",
        re.IGNORECASE
    )
    TRAIN_NOT_RESEARCHING = re.compile(
        r"You haven't started researching a discipline yet\.|"
        r"You are not researching",
        re.IGNORECASE
    )
    TRAIN_NOT_FINISHED = re.compile(
        r"You haven't finished researching|"
        r"You need to finish your research",
        re.IGNORECASE
    )

    # Ability unlock patterns (Attack discipline)
    UNLOCK_CLAWS = re.compile(
        r"You grow a pair of wicked claws\.",
        re.IGNORECASE
    )
    UNLOCK_TAIL = re.compile(
        r"Your spine extends into a long tail\.",
        re.IGNORECASE
    )
    UNLOCK_HORNS = re.compile(
        r"horns extend from|grow.*horns",
        re.IGNORECASE
    )
    UNLOCK_WINGS = re.compile(
        r"A pair of leathery wings grow out of your back\.|"
        r"wings sprout",
        re.IGNORECASE
    )

    # Discipline list parsing
    # Matches lines like "Attack: 5" or "  Hellfire  :  3  "
    DISCIPLINE_LINE = re.compile(
        r"^\s*(\w+)\s*:\s*(\d+)",
        re.MULTILINE
    )

    def parse_research_response(self, text: str) -> dict:
        """
        Parse response from research command.

        Args:
            text: Server response to research command.

        Returns:
            Dict with keys:
            - started: bool
            - cancelled: bool
            - already_researching: bool
            - complete: bool
            - discipline: str or None
        """
        text = self.strip_ansi(text)

        result = {
            'started': False,
            'cancelled': False,
            'already_researching': False,
            'complete': False,
            'discipline': None,
        }

        match = self.RESEARCH_START.search(text)
        if match:
            result['started'] = True
            result['discipline'] = match.group(1).lower()
            return result

        if self.RESEARCH_CANCEL.search(text):
            result['cancelled'] = True
            return result

        if self.RESEARCH_ALREADY.search(text):
            result['already_researching'] = True
            # Check if also complete
            if self.RESEARCH_COMPLETE.search(text):
                result['complete'] = True
            return result

        if self.RESEARCH_COMPLETE.search(text):
            result['complete'] = True

        return result

    def parse_train_response(self, text: str) -> dict:
        """
        Parse response from training a discipline.

        Args:
            text: Server response to train command.

        Returns:
            Dict with keys:
            - success: bool
            - discipline: str or None
            - not_researching: bool
            - not_finished: bool
            - unlocked: list of unlocked abilities
        """
        text = self.strip_ansi(text)

        result = {
            'success': False,
            'discipline': None,
            'not_researching': False,
            'not_finished': False,
            'unlocked': [],
        }

        match = self.TRAIN_SUCCESS.search(text)
        if match:
            result['success'] = True
            result['discipline'] = match.group(1).lower()

            # Check for ability unlocks
            result['unlocked'] = self._parse_unlocks(text)

            return result

        if self.TRAIN_NOT_RESEARCHING.search(text):
            result['not_researching'] = True
            return result

        if self.TRAIN_NOT_FINISHED.search(text):
            result['not_finished'] = True
            return result

        return result

    def _parse_unlocks(self, text: str) -> List[str]:
        """
        Parse ability unlock messages from training response.

        Args:
            text: Training response text.

        Returns:
            List of unlocked ability names.
        """
        unlocked = []

        if self.UNLOCK_CLAWS.search(text):
            unlocked.append('claws')
        if self.UNLOCK_TAIL.search(text):
            unlocked.append('tail')
        if self.UNLOCK_HORNS.search(text):
            unlocked.append('horns')
        if self.UNLOCK_WINGS.search(text):
            unlocked.append('wings')

        return unlocked

    def parse_discipline_list(self, text: str) -> Dict[str, int]:
        """
        Parse disciplines command output.

        Args:
            text: Server response to disciplines command.

        Returns:
            Dict mapping discipline name to level.
        """
        text = self.strip_ansi(text)
        disciplines = {}

        for match in self.DISCIPLINE_LINE.finditer(text):
            name = match.group(1).lower()
            level = int(match.group(2))
            disciplines[name] = level

        return disciplines

    def check_research_complete(self, text: str) -> bool:
        """
        Check if text contains research completion message.

        Args:
            text: Text to check.

        Returns:
            True if research is complete.
        """
        return bool(self.RESEARCH_COMPLETE.search(self.strip_ansi(text)))

    def check_disc_point_gain(self, text: str) -> bool:
        """
        Check if text contains discipline point gain message.

        Args:
            text: Text to check.

        Returns:
            True if discipline point was gained.
        """
        return bool(self.DISC_POINT_GAIN.search(self.strip_ansi(text)))
