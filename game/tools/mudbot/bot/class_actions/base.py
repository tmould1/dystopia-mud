"""
Base class for class-specific actions.
"""

import logging
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from ..base import MudBot

logger = logging.getLogger(__name__)


class ClassActions:
    """
    Base class for class-specific action collections.

    Provides common utilities and the bot reference.
    """

    def __init__(self, bot: "MudBot"):
        self.bot = bot

    async def send_and_read(self, command: str, timeout: float = 5.0):
        """Send command and read response."""
        return await self.bot.send_and_read(command, timeout=timeout)

    async def send_command(self, command: str):
        """Send a command without waiting for specific response."""
        return await self.bot.send_command(command)
