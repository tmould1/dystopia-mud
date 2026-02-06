"""
Class-specific action modules for mudbot.

Provides reusable action classes for different class abilities.
"""

from .base import ClassActions
from .discipline_actions import DisciplineActions

__all__ = [
    'ClassActions',
    'DisciplineActions',
]
