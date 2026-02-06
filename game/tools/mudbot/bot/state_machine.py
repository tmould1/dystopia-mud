"""
State machine for bot login and gameplay states.
"""

import re
import logging
from enum import Enum, auto
from typing import Optional, Tuple
from dataclasses import dataclass

logger = logging.getLogger(__name__)


class BotState(Enum):
    """Bot states for login and gameplay."""

    # Connection states
    DISCONNECTED = auto()
    CONNECTING = auto()

    # Login states (character creation)
    AWAITING_NAME = auto()
    CONFIRMING_NAME = auto()
    ENTERING_PASSWORD = auto()
    CONFIRMING_PASSWORD = auto()
    SELECTING_SEX = auto()
    SELECTING_EXPLEVEL = auto()
    SELECTING_COLOR = auto()
    READING_MOTD = auto()

    # Login states (existing character)
    ENTERING_OLD_PASSWORD = auto()

    # Playing states
    PLAYING = auto()
    IDLE = auto()

    # Progression states
    NAVIGATING = auto()
    FIGHTING = auto()
    TRAINING = auto()
    SAVING = auto()

    # Terminal states
    COMPLETE = auto()
    ERROR = auto()
    DEAD = auto()


@dataclass
class PromptMatch:
    """Result of a prompt detection."""
    state: BotState
    pattern: str
    match: re.Match


class PromptDetector:
    """
    Detect login prompts and game states from server output.

    Each prompt pattern maps to a bot state, allowing the state machine
    to determine what response to send.
    """

    # Login prompt patterns (order matters for some)
    LOGIN_PATTERNS = [
        # New character flow
        (BotState.AWAITING_NAME, r"What be thy name\?|Enter thy name"),
        (BotState.CONFIRMING_NAME, r"engraved on your tombstone.*\(Y/N\)"),
        (BotState.ENTERING_PASSWORD, r"Give me a password"),
        (BotState.CONFIRMING_PASSWORD, r"[Rr]etype password"),
        (BotState.SELECTING_SEX, r"What is your sex.*\(M/F\)"),
        (BotState.SELECTING_EXPLEVEL, r"Your choice \(1/2/3\)|experience level"),
        (BotState.SELECTING_COLOR, r"\(N/A/X/S\)\?|display mode"),
        (BotState.READING_MOTD, r"\[ENTER\]|hit return|Press Enter"),

        # Existing character flow
        (BotState.ENTERING_OLD_PASSWORD, r"Please enter password"),

        # Error states
        (BotState.ERROR, r"Illegal name|Name already exists|Too short|Too long"),
    ]

    # Game state patterns
    GAME_PATTERNS = [
        # Standard MUD prompt: <100hp 100m 100mv>
        (BotState.PLAYING, r"<\d+hp \d+m \d+mv>"),
        # Welcome message after login (this MUD has no default prompt)
        (BotState.PLAYING, r"Welcome to Dystopia"),
        # Room description indicators (Exits line)
        (BotState.PLAYING, r"\[Exits?:"),
        # Combat indicator
        (BotState.FIGHTING, r"You (?:hit|miss|attack)|attacks you|hits you|misses you"),
        # Death
        (BotState.DEAD, r"You have been KILLED|You are DEAD"),
    ]

    def __init__(self):
        # Compile all patterns for efficiency
        self._login_compiled = [
            (state, re.compile(pattern, re.IGNORECASE))
            for state, pattern in self.LOGIN_PATTERNS
        ]
        self._game_compiled = [
            (state, re.compile(pattern, re.IGNORECASE))
            for state, pattern in self.GAME_PATTERNS
        ]

    def detect_login_state(self, text: str) -> Optional[PromptMatch]:
        """
        Detect login state from server output.

        Args:
            text: Server output text.

        Returns:
            PromptMatch if a login prompt was detected, None otherwise.
        """
        for state, pattern in self._login_compiled:
            match = pattern.search(text)
            if match:
                return PromptMatch(state, pattern.pattern, match)
        return None

    def detect_game_state(self, text: str) -> Optional[PromptMatch]:
        """
        Detect game state from server output.

        Args:
            text: Server output text.

        Returns:
            PromptMatch if a game state was detected, None otherwise.
        """
        for state, pattern in self._game_compiled:
            match = pattern.search(text)
            if match:
                return PromptMatch(state, pattern.pattern, match)
        return None

    def detect(self, text: str) -> Optional[PromptMatch]:
        """
        Detect any state from server output.

        Checks login states first, then game states.
        """
        result = self.detect_login_state(text)
        if result:
            return result
        return self.detect_game_state(text)


class LoginStateMachine:
    """
    State machine for handling the login sequence.

    Tracks current login state and provides appropriate responses
    for each prompt.
    """

    def __init__(
        self,
        name: str,
        password: str,
        sex: str = "m",
        experience_level: int = 3,
        color_mode: str = "a"
    ):
        self.name = name
        self.password = password
        self.sex = sex
        self.experience_level = str(experience_level)
        self.color_mode = color_mode

        self.state = BotState.DISCONNECTED
        self.detector = PromptDetector()
        self.is_new_character = True
        self._login_complete = False

    @property
    def logged_in(self) -> bool:
        """Check if login is complete."""
        return self._login_complete

    def process(self, text: str) -> Optional[str]:
        """
        Process server output and return command to send.

        Args:
            text: Server output text.

        Returns:
            Command to send, or None if no action needed.
        """
        match = self.detector.detect_login_state(text)
        if not match:
            # Check if we see game prompt (login complete)
            game_match = self.detector.detect_game_state(text)
            if game_match and game_match.state == BotState.PLAYING:
                self._login_complete = True
                self.state = BotState.PLAYING
                logger.info(f"Login complete, now in PLAYING state")
            return None

        old_state = self.state
        self.state = match.state
        logger.debug(f"State transition: {old_state.name} -> {self.state.name}")

        return self._get_response()

    def _get_response(self) -> Optional[str]:
        """Get the appropriate response for current state."""
        responses = {
            BotState.AWAITING_NAME: self.name,
            BotState.CONFIRMING_NAME: "y",
            BotState.ENTERING_PASSWORD: self.password,
            BotState.CONFIRMING_PASSWORD: self.password,
            BotState.ENTERING_OLD_PASSWORD: self.password,
            BotState.SELECTING_SEX: self.sex,
            BotState.SELECTING_EXPLEVEL: self.experience_level,
            BotState.SELECTING_COLOR: self.color_mode,
            BotState.READING_MOTD: "",  # Just press enter
        }

        response = responses.get(self.state)
        if response is not None:
            logger.info(f"Responding to {self.state.name}: {response if response else '<enter>'}")
        return response

    def reset(self):
        """Reset state machine for reconnection."""
        self.state = BotState.DISCONNECTED
        self._login_complete = False
        self.is_new_character = True
