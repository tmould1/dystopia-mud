"""
Telnet protocol handling for MUD connections.

Handles IAC (Interpret As Command) sequences used in telnet protocol.
The MUD server sends various telnet options (MCCP, GMCP, MXP, NAWS) on connect.
"""

from dataclasses import dataclass
from enum import IntEnum
from typing import Tuple, List, Optional


class TelnetCommand(IntEnum):
    """Telnet command bytes (RFC 854)."""
    IAC = 255   # Interpret As Command
    DONT = 254
    DO = 253
    WONT = 252
    WILL = 251
    SB = 250    # Subnegotiation Begin
    GA = 249    # Go Ahead
    EL = 248    # Erase Line
    EC = 247    # Erase Character
    AYT = 246   # Are You There
    AO = 245    # Abort Output
    IP = 244    # Interrupt Process
    BRK = 243   # Break
    DM = 242    # Data Mark
    NOP = 241   # No Operation
    SE = 240    # Subnegotiation End


class TelnetOption(IntEnum):
    """Telnet option codes."""
    ECHO = 1
    SGA = 3          # Suppress Go Ahead
    STATUS = 5
    TIMING_MARK = 6
    TERMINAL_TYPE = 24
    NAWS = 31        # Negotiate About Window Size
    TERMINAL_SPEED = 32
    LINEMODE = 34
    ENVIRON = 36
    NEW_ENVIRON = 39
    CHARSET = 42
    MCCP1 = 85       # MUD Client Compression Protocol v1
    MCCP2 = 86       # MUD Client Compression Protocol v2
    MSP = 90         # MUD Sound Protocol
    MXP = 91         # MUD eXtension Protocol
    ZMP = 93         # Zenith MUD Protocol
    ATCP = 200       # Achaea Telnet Client Protocol
    GMCP = 201       # Generic Mud Communication Protocol


@dataclass
class TelnetNegotiation:
    """Represents a telnet negotiation sequence."""
    command: TelnetCommand
    option: TelnetOption
    data: Optional[bytes] = None  # For subnegotiation

    def __repr__(self) -> str:
        cmd_name = self.command.name if isinstance(self.command, TelnetCommand) else str(self.command)
        opt_name = self.option.name if isinstance(self.option, TelnetOption) else str(self.option)
        if self.data:
            return f"<Telnet {cmd_name} {opt_name} data={self.data.hex()}>"
        return f"<Telnet {cmd_name} {opt_name}>"


class TelnetProtocol:
    """
    Handle telnet protocol parsing and response generation.

    The MUD server sends telnet negotiation on connect. For MVP, we decline
    all options to keep things simple.
    """

    def parse(self, data: bytes) -> Tuple[bytes, List[TelnetNegotiation]]:
        """
        Separate telnet commands from regular text data.

        Returns:
            Tuple of (text_data, list_of_negotiations)
        """
        text = bytearray()
        negotiations = []
        i = 0

        while i < len(data):
            if data[i] == TelnetCommand.IAC:
                if i + 1 >= len(data):
                    # Incomplete IAC at end of buffer, keep it for next read
                    text.append(data[i])
                    break

                next_byte = data[i + 1]

                # Double IAC = literal 255 byte
                if next_byte == TelnetCommand.IAC:
                    text.append(255)
                    i += 2
                    continue

                # WILL, WONT, DO, DONT commands
                if next_byte in (TelnetCommand.WILL, TelnetCommand.WONT,
                                 TelnetCommand.DO, TelnetCommand.DONT):
                    if i + 2 >= len(data):
                        # Incomplete, save for next read
                        break
                    option = data[i + 2]
                    try:
                        cmd = TelnetCommand(next_byte)
                        opt = TelnetOption(option) if option in TelnetOption._value2member_map_ else option
                    except ValueError:
                        cmd = next_byte
                        opt = option
                    negotiations.append(TelnetNegotiation(cmd, opt))
                    i += 3
                    continue

                # Subnegotiation
                if next_byte == TelnetCommand.SB:
                    # Find the end (IAC SE)
                    end = self._find_subneg_end(data, i + 2)
                    if end == -1:
                        # Incomplete subnegotiation, save for next read
                        break
                    option = data[i + 2] if i + 2 < len(data) else 0
                    subneg_data = bytes(data[i + 3:end])
                    try:
                        opt = TelnetOption(option) if option in TelnetOption._value2member_map_ else option
                    except ValueError:
                        opt = option
                    negotiations.append(TelnetNegotiation(TelnetCommand.SB, opt, subneg_data))
                    i = end + 2  # Skip past IAC SE
                    continue

                # Other commands (GA, NOP, etc.) - skip them
                if next_byte in (TelnetCommand.GA, TelnetCommand.NOP, TelnetCommand.AYT,
                                TelnetCommand.EC, TelnetCommand.EL, TelnetCommand.AO,
                                TelnetCommand.IP, TelnetCommand.BRK, TelnetCommand.DM):
                    i += 2
                    continue

                # Unknown command, skip it
                i += 2
                continue

            # Regular data byte
            text.append(data[i])
            i += 1

        return bytes(text), negotiations

    def _find_subneg_end(self, data: bytes, start: int) -> int:
        """Find the position of IAC SE marking end of subnegotiation."""
        i = start
        while i < len(data) - 1:
            if data[i] == TelnetCommand.IAC:
                if data[i + 1] == TelnetCommand.SE:
                    return i
                # IAC IAC inside subnegotiation = escaped 255
                if data[i + 1] == TelnetCommand.IAC:
                    i += 2
                    continue
            i += 1
        return -1

    def decline_option(self, negotiation: TelnetNegotiation) -> bytes:
        """
        Generate a response declining a telnet option.

        WILL -> DONT
        DO -> WONT
        """
        if negotiation.command == TelnetCommand.WILL:
            return bytes([TelnetCommand.IAC, TelnetCommand.DONT, negotiation.option])
        elif negotiation.command == TelnetCommand.DO:
            return bytes([TelnetCommand.IAC, TelnetCommand.WONT, negotiation.option])
        return b''

    def decline_all(self, negotiations: List[TelnetNegotiation]) -> bytes:
        """Generate responses declining all telnet options."""
        response = bytearray()
        for neg in negotiations:
            if neg.command in (TelnetCommand.WILL, TelnetCommand.DO):
                response.extend(self.decline_option(neg))
        return bytes(response)

    def accept_option(self, negotiation: TelnetNegotiation) -> bytes:
        """
        Generate a response accepting a telnet option.

        WILL -> DO
        DO -> WILL
        """
        if negotiation.command == TelnetCommand.WILL:
            return bytes([TelnetCommand.IAC, TelnetCommand.DO, negotiation.option])
        elif negotiation.command == TelnetCommand.DO:
            return bytes([TelnetCommand.IAC, TelnetCommand.WILL, negotiation.option])
        return b''
