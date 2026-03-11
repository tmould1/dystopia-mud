"""
Quest-specific bot actions.

Provides high-level methods for quest commands that send commands
to the MUD and parse the responses.
"""

import logging
from typing import Optional, TYPE_CHECKING

from ..utils.quest_parser import (
    QuestParser, QuestEntry, QuestProgress,
    QuestAcceptResult, QuestCompleteResult,
)

if TYPE_CHECKING:
    from .base import MudBot

logger = logging.getLogger(__name__)


class QuestActions:
    """
    Quest command wrappers that send commands and return parsed results.

    Works alongside BotActions to provide quest-specific functionality.
    """

    def __init__(self, bot: "MudBot"):
        self.bot = bot
        self.parser = QuestParser()

    async def quest_list(self, category: Optional[str] = None) -> list[QuestEntry]:
        """
        Get list of available/active quests.

        Args:
            category: Optional category filter (M, T, C, E, CL, etc.)

        Returns:
            List of QuestEntry with status info.
        """
        cmd = f"quest list {category}" if category else "quest list"
        response = await self.bot.send_and_read(
            cmd,
            wait_for=["Available Quests", "No quests available"],
            timeout=5.0,
        )
        if not response:
            logger.warning(f"[{self.bot.config.name}] No response from quest list")
            return []
        return self.parser.parse_quest_list(response)

    async def quest_progress(self) -> list[QuestProgress]:
        """
        Get detailed progress for active quests.

        Returns:
            List of QuestProgress with objective details.
        """
        response = await self.bot.send_and_read(
            "quest progress",
            wait_for=["Quest Progress", "No active quests"],
            timeout=5.0,
        )
        if not response:
            logger.warning(f"[{self.bot.config.name}] No response from quest progress")
            return []
        return self.parser.parse_quest_progress(response)

    async def quest_accept(self, quest_id: str) -> QuestAcceptResult:
        """
        Accept a quest by ID.

        Args:
            quest_id: Quest ID to accept (e.g. "T01", "M01").

        Returns:
            QuestAcceptResult with success/error info.
        """
        response = await self.bot.send_and_read(
            f"quest accept {quest_id}",
            wait_for=["Quest Accepted", "No such quest", "not available",
                       "already accepted", "already completed"],
            timeout=5.0,
        )
        if not response:
            return QuestAcceptResult(success=False, error="No response")
        result = self.parser.parse_quest_accept(response)
        if result.success:
            logger.info(f"[{self.bot.config.name}] Quest accepted: {result.quest_name}")
        else:
            logger.warning(f"[{self.bot.config.name}] Quest accept failed: {result.error}")
        return result

    async def quest_complete(self) -> QuestCompleteResult:
        """
        Turn in all completed quests.

        Returns:
            QuestCompleteResult with completed quest names and rewards.
        """
        response = await self.bot.send_and_read(
            "quest complete",
            wait_for=["Quest Complete", "no quests ready"],
            timeout=5.0,
        )
        if not response:
            return QuestCompleteResult()
        result = self.parser.parse_quest_complete(response)
        if result.completed:
            logger.info(f"[{self.bot.config.name}] Quests turned in: "
                        f"{', '.join(result.completed)} (+{result.qp_earned} QP)")
        return result

    async def quest_history(self) -> tuple[list[QuestEntry], int]:
        """
        Get completed quest history.

        Returns:
            Tuple of (list of completed QuestEntry, total count).
        """
        response = await self.bot.send_and_read(
            "quest history",
            wait_for=["Completed Quests", "No completed quests", "Total completed"],
            timeout=5.0,
        )
        if not response:
            return [], 0
        return self.parser.parse_quest_history(response)

    async def send_command(self, command: str) -> str:
        """
        Send an arbitrary command and return the response.

        Used for USE_COMMAND quest objectives.

        Args:
            command: The command to send.

        Returns:
            Server response text.
        """
        response = await self.bot.send_and_read(command, timeout=5.0)
        return response or ""

    async def say(self, message: str) -> str:
        """
        Say something in the room.

        Args:
            message: Message to say.

        Returns:
            Server response text.
        """
        response = await self.bot.send_and_read(f"say {message}", timeout=3.0)
        return response or ""
