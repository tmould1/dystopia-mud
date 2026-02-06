"""
Async TCP client for MUD connections with telnet handling.
"""

import asyncio
import logging
from typing import Optional, Callable, Awaitable

from .protocol import TelnetProtocol, TelnetNegotiation

logger = logging.getLogger(__name__)


class TelnetClient:
    """
    Async TCP client with telnet protocol handling.

    Manages connection to MUD server, handles telnet negotiation,
    and provides clean text I/O.
    """

    def __init__(
        self,
        host: str = "localhost",
        port: int = 8888,
        encoding: str = "utf-8",
    ):
        self.host = host
        self.port = port
        self.encoding = encoding
        self.protocol = TelnetProtocol()

        self._reader: Optional[asyncio.StreamReader] = None
        self._writer: Optional[asyncio.StreamWriter] = None
        self._connected = False
        self._buffer = bytearray()

        # Callback for telnet negotiations (optional)
        self.on_negotiation: Optional[Callable[[TelnetNegotiation], Awaitable[None]]] = None

    @property
    def connected(self) -> bool:
        """Check if currently connected."""
        return self._connected and self._writer is not None

    async def connect(self) -> bool:
        """
        Establish connection to the MUD server.

        Returns:
            True if connection successful, False otherwise.
        """
        try:
            logger.info(f"Connecting to {self.host}:{self.port}...")
            self._reader, self._writer = await asyncio.open_connection(
                self.host, self.port
            )
            self._connected = True
            logger.info(f"Connected to {self.host}:{self.port}")
            return True
        except (ConnectionRefusedError, OSError) as e:
            logger.error(f"Connection failed: {e}")
            self._connected = False
            return False

    async def disconnect(self) -> None:
        """Close the connection."""
        if self._writer:
            try:
                self._writer.close()
                await self._writer.wait_closed()
            except Exception as e:
                logger.debug(f"Error closing connection: {e}")
            finally:
                self._writer = None
                self._reader = None
                self._connected = False
                logger.info("Disconnected")

    async def read(self, timeout: Optional[float] = 5.0) -> Optional[str]:
        """
        Read data from the server.

        Handles telnet negotiation automatically, declining all options.
        Returns cleaned text data.

        Args:
            timeout: Read timeout in seconds. None for no timeout.

        Returns:
            Text data received, or None on timeout/disconnect.
        """
        if not self._reader:
            return None

        try:
            if timeout:
                raw = await asyncio.wait_for(
                    self._reader.read(4096),
                    timeout=timeout
                )
            else:
                raw = await self._reader.read(4096)

            if not raw:
                logger.warning("Server closed connection")
                self._connected = False
                return None

            # Parse telnet commands from data
            text_bytes, negotiations = self.protocol.parse(raw)

            # Handle telnet negotiations by declining all (MVP approach)
            if negotiations:
                for neg in negotiations:
                    logger.debug(f"Telnet: {neg}")
                    if self.on_negotiation:
                        await self.on_negotiation(neg)

                # Send decline responses with small delay to avoid mixing with text
                response = self.protocol.decline_all(negotiations)
                if response:
                    await self._send_raw(response)
                    # Give MUD time to process telnet responses before any text
                    await asyncio.sleep(0.2)

            # Decode text
            try:
                text = text_bytes.decode(self.encoding, errors='replace')
            except UnicodeDecodeError:
                text = text_bytes.decode('latin-1', errors='replace')

            return text

        except asyncio.TimeoutError:
            logger.debug("Read timed out")
            return None
        except (ConnectionResetError, BrokenPipeError, OSError) as e:
            logger.error(f"Read error: {e}")
            self._connected = False
            return None

    async def read_until(
        self,
        patterns: list[str],
        timeout: float = 30.0
    ) -> tuple[Optional[str], Optional[str]]:
        """
        Read until one of the patterns is found.

        Args:
            patterns: List of strings to match.
            timeout: Total timeout for operation.

        Returns:
            Tuple of (accumulated_text, matched_pattern) or (None, None) on timeout.
        """
        accumulated = ""
        start_time = asyncio.get_event_loop().time()

        while True:
            remaining = timeout - (asyncio.get_event_loop().time() - start_time)
            if remaining <= 0:
                return accumulated if accumulated else None, None

            chunk = await self.read(timeout=min(remaining, 2.0))
            if chunk is None:
                if not self._connected:
                    return accumulated if accumulated else None, None
                continue

            accumulated += chunk

            # Check for patterns
            for pattern in patterns:
                if pattern.lower() in accumulated.lower():
                    return accumulated, pattern

        return accumulated, None

    async def send(self, text: str) -> bool:
        """
        Send a command/text to the server.

        Automatically appends newline if not present.

        Args:
            text: Text to send.

        Returns:
            True if sent successfully.
        """
        if not text.endswith('\n') and not text.endswith('\r\n'):
            text = text + '\r\n'

        return await self._send_raw(text.encode(self.encoding))

    async def _send_raw(self, data: bytes) -> bool:
        """Send raw bytes to the server."""
        if not self._writer:
            logger.error("Cannot send: not connected")
            return False

        try:
            self._writer.write(data)
            await self._writer.drain()
            return True
        except (ConnectionResetError, BrokenPipeError, OSError) as e:
            logger.error(f"Send error: {e}")
            self._connected = False
            return False

    async def send_command(self, command: str, delay: float = 0.1) -> bool:
        """
        Send a game command with optional delay.

        Args:
            command: The command to send.
            delay: Delay after sending (to avoid flooding).

        Returns:
            True if sent successfully.
        """
        result = await self.send(command)
        if delay > 0:
            await asyncio.sleep(delay)
        return result
