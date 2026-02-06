"""Bot logic and state machines."""

from .state_machine import BotState, LoginStateMachine
from .base import MudBot
from .avatar_bot import AvatarProgressionBot
from .actions import BotActions

# Progression framework
from .progression import (
    ClassProgressionBot,
    BaseProgressionState,
    AvatarProgressionMixin,
    AvatarState,
    ClassRegistry,
)

# Import class modules to register them
from .progression import classes  # noqa: F401

# Class actions
from .class_actions import ClassActions, DisciplineActions

__all__ = [
    # Base classes
    "BotState",
    "LoginStateMachine",
    "MudBot",
    "BotActions",
    # Legacy avatar bot
    "AvatarProgressionBot",
    # Progression framework
    "ClassProgressionBot",
    "BaseProgressionState",
    "AvatarProgressionMixin",
    "AvatarState",
    "ClassRegistry",
    # Class actions
    "ClassActions",
    "DisciplineActions",
]
