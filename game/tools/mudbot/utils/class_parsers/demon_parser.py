"""
Demon-specific parser for mudbot.

Extends DisciplineParser with demon-specific patterns for:
- Warp system (obtain)
- Graft system
- Inpart system
- Demonarmour creation
"""

import re
import logging
from typing import List

from .discipline_parser import DisciplineParser

logger = logging.getLogger(__name__)


class DemonParser(DisciplineParser):
    """
    Parser for demon-specific command output.

    Extends DisciplineParser with demon-specific patterns.
    """

    # Warp/Obtain patterns
    OBTAIN_SUCCESS = re.compile(
        r"You have obtained a new warp!",
        re.IGNORECASE
    )
    OBTAIN_NOT_ENOUGH = re.compile(
        r"You need 15000 demon points|"
        r"not have enough demon points",
        re.IGNORECASE
    )
    OBTAIN_MAX_WARPS = re.compile(
        r"already obtained as many warps as possible|"
        r"maximum number of warps",
        re.IGNORECASE
    )

    # Graft patterns
    GRAFT_SUCCESS = re.compile(
        r"You graft an arm onto your body\.",
        re.IGNORECASE
    )
    GRAFT_FULL = re.compile(
        r"already have four arms!|"
        r"cannot graft more arms",
        re.IGNORECASE
    )
    GRAFT_NO_ARM = re.compile(
        r"do not have that limb|"
        r"don't have that|"
        r"You have no arm",
        re.IGNORECASE
    )
    GRAFT_NOT_ARM = re.compile(
        r"not even an arm|"
        r"That's not an arm",
        re.IGNORECASE
    )

    # Inpart patterns
    INPART_SUCCESS = re.compile(
        r"You inpart|"
        r"power flows into|"
        r"you feel.*power",
        re.IGNORECASE
    )
    INPART_NOT_ENOUGH = re.compile(
        r"not have enough demon points|"
        r"need more demon points",
        re.IGNORECASE
    )
    INPART_ALREADY_HAVE = re.compile(
        r"already have that power|"
        r"already possess",
        re.IGNORECASE
    )

    # Demonarmour patterns
    ARMOUR_SUCCESS = re.compile(
        r"appears in your hands|"
        r"blast of flames|"
        r"You create",
        re.IGNORECASE
    )
    ARMOUR_NOT_ENOUGH = re.compile(
        r"costs? 60 points of primal|"
        r"not have enough primal|"
        r"need more primal",
        re.IGNORECASE
    )

    # Arm detection in inventory
    ARM_ITEM = re.compile(
        r"severed.*arm|"
        r"arm.*limb|"
        r"dismembered.*arm|"
        r"^a.*arm$",
        re.IGNORECASE | re.MULTILINE
    )

    def parse_obtain_response(self, text: str) -> dict:
        """
        Parse response from obtain command.

        Args:
            text: Server response.

        Returns:
            Dict with:
            - success: bool
            - not_enough_points: bool
            - max_warps: bool
        """
        text = self.strip_ansi(text)

        result = {
            'success': False,
            'not_enough_points': False,
            'max_warps': False,
        }

        if self.OBTAIN_SUCCESS.search(text):
            result['success'] = True
        elif self.OBTAIN_NOT_ENOUGH.search(text):
            result['not_enough_points'] = True
        elif self.OBTAIN_MAX_WARPS.search(text):
            result['max_warps'] = True

        return result

    def parse_graft_response(self, text: str) -> dict:
        """
        Parse response from graft command.

        Args:
            text: Server response.

        Returns:
            Dict with:
            - success: bool
            - no_arm: bool
            - not_arm: bool
            - full: bool
        """
        text = self.strip_ansi(text)

        result = {
            'success': False,
            'no_arm': False,
            'not_arm': False,
            'full': False,
        }

        if self.GRAFT_SUCCESS.search(text):
            result['success'] = True
        elif self.GRAFT_NO_ARM.search(text):
            result['no_arm'] = True
        elif self.GRAFT_NOT_ARM.search(text):
            result['not_arm'] = True
        elif self.GRAFT_FULL.search(text):
            result['full'] = True

        return result

    def parse_inpart_response(self, text: str) -> dict:
        """
        Parse response from inpart command.

        Args:
            text: Server response.

        Returns:
            Dict with:
            - success: bool
            - not_enough_points: bool
            - already_have: bool
        """
        text = self.strip_ansi(text)

        result = {
            'success': False,
            'not_enough_points': False,
            'already_have': False,
        }

        if self.INPART_SUCCESS.search(text):
            result['success'] = True
        elif self.INPART_NOT_ENOUGH.search(text):
            result['not_enough_points'] = True
        elif self.INPART_ALREADY_HAVE.search(text):
            result['already_have'] = True

        return result

    def parse_armour_response(self, text: str) -> dict:
        """
        Parse response from demonarmour command.

        Args:
            text: Server response.

        Returns:
            Dict with:
            - success: bool
            - not_enough_primal: bool
        """
        text = self.strip_ansi(text)

        result = {
            'success': False,
            'not_enough_primal': False,
        }

        if self.ARMOUR_SUCCESS.search(text):
            result['success'] = True
        elif self.ARMOUR_NOT_ENOUGH.search(text):
            result['not_enough_primal'] = True

        return result

    def find_arms_in_text(self, text: str) -> List[str]:
        """
        Find arm items in inventory text.

        Args:
            text: Inventory output text.

        Returns:
            List of potential arm keywords.
        """
        text = self.strip_ansi(text)
        arms = []

        for line in text.split('\n'):
            if self.ARM_ITEM.search(line):
                # Try to extract a useful keyword
                words = line.strip().split()
                for word in words:
                    word_clean = word.lower().strip('.,')
                    if 'arm' in word_clean or word_clean in ['limb', 'appendage']:
                        # Use the word before 'arm' as keyword, or 'arm' itself
                        idx = words.index(word)
                        if idx > 0 and words[idx-1].lower() not in ['a', 'an', 'the']:
                            arms.append(words[idx-1].lower())
                        else:
                            arms.append('arm')
                        break

        return list(set(arms))  # Remove duplicates
