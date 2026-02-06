"""
Class progression framework for mudbot.

This package provides the base classes and registry for implementing
character class progression bots.
"""

from .base import ClassProgressionBot, BaseProgressionState
from .avatar import AvatarProgressionMixin, AvatarState
from .registry import ClassRegistry

__all__ = [
    'ClassProgressionBot',
    'BaseProgressionState',
    'AvatarProgressionMixin',
    'AvatarState',
    'ClassRegistry',
]
