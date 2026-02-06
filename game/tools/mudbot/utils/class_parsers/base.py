"""
Base class parser for class-specific output parsing.
"""

import re
import logging

logger = logging.getLogger(__name__)


class BaseClassParser:
    """
    Base class for class-specific parsers.

    Provides common utilities for parsing MUD output.
    """

    # Pattern to strip ANSI color codes
    ANSI_ESCAPE = re.compile(r'\x1b\[[0-9;]*m|\[0[;0-9]*m|#[RGBYPCWnrx0-9]+')

    def strip_ansi(self, text: str) -> str:
        """
        Remove ANSI escape codes and color markers from text.

        Args:
            text: Text potentially containing ANSI codes.

        Returns:
            Clean text without color codes.
        """
        return self.ANSI_ESCAPE.sub('', text)

    def extract_first_word(self, text: str) -> str:
        """
        Extract the first word from text.

        Args:
            text: Input text.

        Returns:
            First word or empty string.
        """
        words = text.strip().split()
        return words[0] if words else ""
