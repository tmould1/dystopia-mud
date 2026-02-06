"""
Base class for class progression bots.

Provides the abstract interface that all class-specific bots must implement.
"""

import asyncio
import logging
from abc import ABC, abstractmethod
from enum import Enum, auto
from typing import Optional, Type, Callable, Any, Dict

from ..base import MudBot
from ..actions import BotActions
from ...config import BotConfig

logger = logging.getLogger(__name__)


class BaseProgressionState(Enum):
    """Base states shared by all progression bots."""
    START = auto()
    CHECKING_STATS = auto()
    FARMING = auto()
    COMBAT = auto()
    REGENERATING = auto()
    SAVING = auto()
    COMPLETE = auto()
    FAILED = auto()


class ClassProgressionBot(MudBot, ABC):
    """
    Abstract base class for all class progression bots.

    Subclasses must implement:
    - get_class_name(): Return the class name for selfclass command
    - get_progression_states(): Return class-specific progression states enum
    - run_class_progression(): Execute class-specific progression logic

    Provides shared functionality:
    - Class selection via selfclass
    - Armor creation via <class>armour
    - State tracking and callbacks
    - Pause/resume support
    """

    def __init__(self, config: BotConfig):
        super().__init__(config)
        self.actions = BotActions(self)

        # State tracking
        self._is_class_selected = False

        # Pause support
        self._paused = asyncio.Event()
        self._paused.set()  # Not paused by default

        # Status callback
        self.on_status_change: Optional[Callable[[Dict[str, Any]], Any]] = None

    @abstractmethod
    def get_class_name(self) -> str:
        """
        Return the class name for the selfclass command.

        Returns:
            Class name string (e.g., "demon", "vampire").
        """
        pass

    @abstractmethod
    def get_progression_states(self) -> Type[Enum]:
        """
        Return the enum type for class-specific progression states.

        Returns:
            Enum type with class-specific states.
        """
        pass

    @abstractmethod
    async def run_class_progression(self) -> bool:
        """
        Execute class-specific progression logic.

        Called after avatar progression and class selection.

        Returns:
            True if progression completed successfully.
        """
        pass

    @property
    def is_paused(self) -> bool:
        """Check if bot is paused."""
        return not self._paused.is_set()

    def pause(self) -> None:
        """Pause the bot progression."""
        if self._paused.is_set():
            logger.info(f"[{self.config.name}] Pausing...")
            self._paused.clear()
            self._notify_status_change()

    def resume(self) -> None:
        """Resume the bot progression."""
        if not self._paused.is_set():
            logger.info(f"[{self.config.name}] Resuming...")
            self._paused.set()
            self._notify_status_change()

    async def _wait_if_paused(self) -> None:
        """Wait if the bot is paused."""
        await self._paused.wait()

    def _notify_status_change(self) -> None:
        """Call status change callback if set."""
        if self.on_status_change:
            try:
                self.on_status_change(self.get_status())
            except Exception as e:
                logger.warning(f"[{self.config.name}] Status callback error: {e}")

    async def select_class(self) -> bool:
        """
        Select the character class via selfclass command.

        Returns:
            True if class selection successful.
        """
        class_name = self.get_class_name()
        logger.info(f"[{self.config.name}] Selecting class: {class_name}")

        response = await self.send_and_read(
            f"selfclass {class_name}",
            timeout=5.0
        )

        if response:
            # Check for success - class-specific success patterns can be overridden
            if self._check_selfclass_success(response):
                self._is_class_selected = True
                logger.info(f"[{self.config.name}] Successfully became a {class_name}!")
                return True

            # Check for common failure messages
            if "must be avatar" in response.lower():
                logger.warning(f"[{self.config.name}] Must be avatar to selfclass")
                return False
            if "already have a class" in response.lower():
                # Already has a class - check if it's the right one
                logger.info(f"[{self.config.name}] Already has a class")
                self._is_class_selected = True
                return True

        return False

    def _check_selfclass_success(self, response: str) -> bool:
        """
        Check if selfclass was successful.

        Override in subclasses for class-specific success patterns.

        Args:
            response: Server response to selfclass command.

        Returns:
            True if selection was successful.
        """
        # Generic success patterns
        class_name = self.get_class_name()
        response_lower = response.lower()

        return (
            f"become a {class_name}" in response_lower or
            f"you are now a {class_name}" in response_lower or
            "your eyes glow" in response_lower or
            "transformation" in response_lower
        )

    async def create_armor(self, piece: str) -> bool:
        """
        Create a class armor piece.

        Args:
            piece: Armor piece name (e.g., "plate", "helmet").

        Returns:
            True if armor created successfully.
        """
        class_name = self.get_class_name()
        command = f"{class_name}armour {piece}"

        logger.info(f"[{self.config.name}] Creating armor: {piece}")
        response = await self.send_and_read(command, timeout=5.0)

        if response:
            response_lower = response.lower()
            if "appears in your hands" in response_lower or "blast of flames" in response_lower:
                logger.info(f"[{self.config.name}] Created {piece}!")
                return True
            if "not have enough" in response_lower or "primal" in response_lower:
                logger.info(f"[{self.config.name}] Not enough primal for {piece}")
                return False

        return False

    async def equip_item(self, item_keyword: str) -> bool:
        """
        Equip an item by keyword.

        Args:
            item_keyword: Item keyword to wear.

        Returns:
            True if equipped successfully.
        """
        logger.debug(f"[{self.config.name}] Equipping: {item_keyword}")
        response = await self.send_and_read(f"wear {item_keyword}", timeout=3.0)

        if response:
            response_lower = response.lower()
            if "you wear" in response_lower or "you wield" in response_lower:
                return True
            if "can't wear" in response_lower or "don't have" in response_lower:
                return False

        return False

    @abstractmethod
    def get_status(self) -> Dict[str, Any]:
        """
        Get current bot status for monitoring.

        Returns:
            Dict with status information.
        """
        pass

    async def run(self) -> None:
        """
        Main bot execution loop.

        Connects, logs in, runs avatar progression, selects class,
        then runs class-specific progression.
        """
        # Connect
        if not await self.connect():
            logger.error(f"[{self.config.name}] Connection failed")
            return

        # Login
        if not await self.login():
            logger.error(f"[{self.config.name}] Login failed")
            await self.disconnect()
            return

        # Wait for login to settle
        logger.info(f"[{self.config.name}] Waiting for login to settle...")
        await asyncio.sleep(3.0)
        await self.client.send("")
        await asyncio.sleep(1.0)

        # Drain pending data
        for _ in range(10):
            chunk = await self.client.read(timeout=1.0)
            if not chunk:
                break

        try:
            # Run the full progression
            success = await self._run_full_progression()

            if success:
                logger.info(f"[{self.config.name}] Bot completed successfully!")
            else:
                logger.error(f"[{self.config.name}] Bot failed to complete progression")

        finally:
            # Save and quit gracefully
            await self.actions.save()
            await self.actions.quit()
            await self.disconnect()

    async def _run_full_progression(self) -> bool:
        """
        Run the complete progression from start to finish.

        Returns:
            True if all progression completed successfully.
        """
        # Subclasses typically implement this to call:
        # 1. run_avatar_progression() (from mixin)
        # 2. select_class()
        # 3. run_class_progression()
        raise NotImplementedError("Subclass must implement _run_full_progression")
