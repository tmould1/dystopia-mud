"""
Base MUD bot implementation.
"""

import asyncio
import logging
from typing import Optional, Callable, Awaitable

from ..config import BotConfig
from ..connection import TelnetClient
from .state_machine import BotState, LoginStateMachine

logger = logging.getLogger(__name__)


class MudBot:
    """
    Base MUD bot with connection and login handling.

    Subclass this to add specific gameplay behaviors.
    """

    def __init__(self, config: BotConfig):
        self.config = config
        self.client = TelnetClient(
            host=config.host,
            port=config.port,
        )
        self.login_sm = LoginStateMachine(
            name=config.name,
            password=config.password,
            sex=config.sex,
            experience_level=config.experience_level,
            color_mode=config.color_mode,
        )

        self._state = BotState.DISCONNECTED
        self._running = False
        self._output_buffer = ""

        # Callbacks
        self.on_connected: Optional[Callable[[], Awaitable[None]]] = None
        self.on_logged_in: Optional[Callable[[], Awaitable[None]]] = None
        self.on_output: Optional[Callable[[str], Awaitable[None]]] = None

    @property
    def state(self) -> BotState:
        """Current bot state."""
        return self._state

    @state.setter
    def state(self, value: BotState):
        if value != self._state:
            logger.debug(f"[{self.config.name}] State: {self._state.name} -> {value.name}")
            self._state = value

    @property
    def connected(self) -> bool:
        """Check if connected to server."""
        return self.client.connected

    @property
    def logged_in(self) -> bool:
        """Check if login is complete."""
        return self.login_sm.logged_in

    async def connect(self) -> bool:
        """
        Connect to the MUD server.

        Returns:
            True if connection successful.
        """
        self.state = BotState.CONNECTING
        logger.info(f"[{self.config.name}] Connecting to {self.config.host}:{self.config.port}")

        success = await self.client.connect()
        if success:
            self.state = BotState.AWAITING_NAME
            if self.on_connected:
                await self.on_connected()
        else:
            self.state = BotState.ERROR

        return success

    async def disconnect(self) -> None:
        """Disconnect from the server."""
        await self.client.disconnect()
        self.state = BotState.DISCONNECTED
        self._running = False

    async def login(self) -> bool:
        """
        Complete the login sequence.

        Handles both new character creation and existing character login.

        Returns:
            True if login successful.
        """
        logger.info(f"[{self.config.name}] Starting login sequence")

        while not self.login_sm.logged_in and self.connected:
            # Read server output
            text = await self.client.read(timeout=self.config.read_timeout)
            if text is None:
                if not self.connected:
                    logger.error(f"[{self.config.name}] Disconnected during login")
                    self.state = BotState.ERROR
                    return False
                continue

            # Store output for debugging
            self._output_buffer += text
            logger.debug(f"[{self.config.name}] Received: {text[:200]}...")

            # Process through login state machine
            response = self.login_sm.process(text)
            if response is not None:
                await asyncio.sleep(0.1)  # Small delay before responding
                await self.client.send(response)

            # Check for errors in text
            if "Illegal name" in text or "Name already exists" in text:
                logger.error(f"[{self.config.name}] Login error: name rejected")
                self.state = BotState.ERROR
                return False

            if "Wrong password" in text:
                logger.error(f"[{self.config.name}] Login error: wrong password")
                self.state = BotState.ERROR
                return False

        if self.login_sm.logged_in:
            logger.info(f"[{self.config.name}] Login successful!")
            self.state = BotState.PLAYING
            if self.on_logged_in:
                await self.on_logged_in()
            return True

        return False

    async def send_command(self, command: str) -> bool:
        """
        Send a command to the server.

        Args:
            command: The command to send.

        Returns:
            True if sent successfully.
        """
        if not self.connected:
            logger.warning(f"[{self.config.name}] Cannot send: not connected")
            return False

        logger.debug(f"[{self.config.name}] Sending: {command}")
        result = await self.client.send(command)
        await asyncio.sleep(self.config.command_delay)
        return result

    async def send_and_read(
        self,
        command: str,
        wait_for: Optional[list[str]] = None,
        timeout: float = 5.0
    ) -> Optional[str]:
        """
        Send a command and read the response.

        Args:
            command: Command to send.
            wait_for: Optional patterns to wait for.
            timeout: Read timeout.

        Returns:
            Server response text.
        """
        await self.send_command(command)

        if wait_for:
            text, _ = await self.client.read_until(wait_for, timeout=timeout)
            return text
        else:
            # Accumulate all available data (server may send multiple chunks)
            accumulated = ""
            # Wait a bit longer for server response, then collect data
            chunk = await self.client.read(timeout=2.0)
            if chunk:
                accumulated += chunk
                # Got some data, read more if available
                for _ in range(5):
                    await asyncio.sleep(0.1)
                    more = await self.client.read(timeout=0.3)
                    if more:
                        accumulated += more
                    else:
                        break
            return accumulated if accumulated else None

    async def run(self) -> None:
        """
        Main bot loop.

        Override in subclasses for specific behaviors.
        """
        self._running = True

        while self._running and self.connected:
            try:
                # Read server output
                text = await self.client.read(timeout=2.0)
                if text:
                    self._output_buffer += text
                    if self.on_output:
                        await self.on_output(text)

                # Subclasses override this for specific behavior
                await self.tick()

            except asyncio.CancelledError:
                logger.info(f"[{self.config.name}] Bot cancelled")
                break
            except Exception as e:
                logger.error(f"[{self.config.name}] Error in run loop: {e}")
                self.state = BotState.ERROR
                break

        logger.info(f"[{self.config.name}] Bot loop ended")

    async def tick(self) -> None:
        """
        Called each iteration of the run loop.

        Override in subclasses to implement bot behavior.
        """
        # Default: do nothing, just read output
        pass

    def stop(self) -> None:
        """Signal the bot to stop."""
        self._running = False

    async def reconnect(self) -> bool:
        """
        Attempt to reconnect after disconnection.

        Returns:
            True if reconnection successful.
        """
        for attempt in range(self.config.reconnect_attempts):
            logger.info(f"[{self.config.name}] Reconnect attempt {attempt + 1}/{self.config.reconnect_attempts}")

            self.login_sm.reset()
            await asyncio.sleep(self.config.reconnect_delay)

            if await self.connect():
                if await self.login():
                    return True

        logger.error(f"[{self.config.name}] Failed to reconnect after {self.config.reconnect_attempts} attempts")
        return False
