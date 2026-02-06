"""
Class-specific parsers for mudbot.

Provides parsers for parsing class-specific MUD output.
"""

from .base import BaseClassParser
from .discipline_parser import DisciplineParser

__all__ = [
    'BaseClassParser',
    'DisciplineParser',
]
