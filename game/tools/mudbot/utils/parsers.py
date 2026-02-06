"""
Parsers for extracting information from MUD output.
"""

import re
import logging
from typing import Optional, Dict, Any, Tuple
from dataclasses import dataclass

logger = logging.getLogger(__name__)


@dataclass
class VitalStats:
    """Character vital statistics."""
    current_hp: int
    max_hp: int
    current_mana: int
    max_mana: int
    current_move: int
    max_move: int
    exp: int = 0
    level: int = 0


@dataclass
class CombatResult:
    """Result of combat parsing."""
    in_combat: bool
    target_dead: bool
    player_dead: bool
    target_name: Optional[str] = None


class ScoreParser:
    """
    Parse the 'score' command output to extract character stats.

    The score command shows vital statistics, experience, level, etc.
    """

    # Pattern to strip ANSI color codes
    ANSI_ESCAPE = re.compile(r'\x1b\[[0-9;]*m|\[0[;0-9]*m')

    # Pattern for HP/Mana/Move from score output
    # Dystopia format: "Hit Points: 100/2000. Mana: 100/500. Movement: 200/500."
    # Also matches: "[You have 100/2000 hit][100/500 mana][200/500 movement]"
    VITALS_PATTERN = re.compile(
        r"Hit Points:\s*(\d+)/(\d+).*?Mana:\s*(\d+)/(\d+).*?Movement:\s*(\d+)/(\d+)",
        re.IGNORECASE | re.DOTALL
    )

    # Alternative pattern with brackets: [You have 100/2000 hit]
    VITALS_BRACKET_PATTERN = re.compile(
        r"have\s*(\d+)/(\d+)\s*hit.*?(\d+)/(\d+)\s*mana.*?(\d+)/(\d+)\s*movement",
        re.IGNORECASE | re.DOTALL
    )

    def strip_ansi(self, text: str) -> str:
        """Remove ANSI escape codes from text."""
        return self.ANSI_ESCAPE.sub('', text)

    # Pattern for experience
    # Example: "Exp: 12345" or "Experience: 12345"
    EXP_PATTERN = re.compile(r"(?:Exp|Experience):\s*(\d+)", re.IGNORECASE)

    # Pattern for level
    # Example: "Level: 2" or "You are level 2"
    LEVEL_PATTERN = re.compile(r"Level:\s*(\d+)|level\s+(\d+)", re.IGNORECASE)

    # Alternative HP pattern from prompt: <100hp 100m 100mv>
    PROMPT_PATTERN = re.compile(r"<(\d+)hp (\d+)m (\d+)mv>")

    def parse(self, text: str) -> Optional[VitalStats]:
        """
        Parse score output to extract vital stats.

        Args:
            text: Output from 'score' command.

        Returns:
            VitalStats if parsing successful, None otherwise.
        """
        # Strip ANSI color codes first
        text = self.strip_ansi(text)

        vitals_match = self.VITALS_PATTERN.search(text)
        if not vitals_match:
            # Try bracket pattern: [You have X/Y hit]
            vitals_match = self.VITALS_BRACKET_PATTERN.search(text)
        if not vitals_match:
            # Try prompt pattern as fallback
            prompt_match = self.PROMPT_PATTERN.search(text)
            if prompt_match:
                return VitalStats(
                    current_hp=int(prompt_match.group(1)),
                    max_hp=int(prompt_match.group(1)),  # Can't know max from prompt
                    current_mana=int(prompt_match.group(2)),
                    max_mana=int(prompt_match.group(2)),
                    current_move=int(prompt_match.group(3)),
                    max_move=int(prompt_match.group(3)),
                )
            return None

        stats = VitalStats(
            current_hp=int(vitals_match.group(1)),
            max_hp=int(vitals_match.group(2)),
            current_mana=int(vitals_match.group(3)),
            max_mana=int(vitals_match.group(4)),
            current_move=int(vitals_match.group(5)),
            max_move=int(vitals_match.group(6)),
        )

        # Try to get exp
        exp_match = self.EXP_PATTERN.search(text)
        if exp_match:
            stats.exp = int(exp_match.group(1))

        # Try to get level
        level_match = self.LEVEL_PATTERN.search(text)
        if level_match:
            level_str = level_match.group(1) or level_match.group(2)
            stats.level = int(level_str)

        return stats


class CombatParser:
    """
    Parse combat-related output.
    """

    # Combat start indicators
    COMBAT_START = [
        r"You attack",
        r"attacks you",
        r"You hit",
        r"hits you",
        r"You miss",
        r"misses you",
    ]

    # Target death - multiple patterns for robustness
    TARGET_DEAD = re.compile(
        r"(\w+) is DEAD|You have slain|is dead!|You receive \d+ exp",
        re.IGNORECASE
    )

    # Experience gain (indicates kill)
    EXP_GAINED = re.compile(r"You receive (\d+) exp", re.IGNORECASE)

    # Player death
    PLAYER_DEAD = re.compile(r"You have been KILLED|You are DEAD", re.IGNORECASE)

    # Combat end (fled, target dead, player dead)
    COMBAT_END = [
        r"is DEAD",
        r"You have been KILLED",
        r"You flee",
        r"flees from combat",
    ]

    def __init__(self):
        self._combat_patterns = [re.compile(p, re.IGNORECASE) for p in self.COMBAT_START]
        self._end_patterns = [re.compile(p, re.IGNORECASE) for p in self.COMBAT_END]

    def is_in_combat(self, text: str) -> bool:
        """Check if combat indicators are present."""
        return any(p.search(text) for p in self._combat_patterns)

    def parse(self, text: str) -> CombatResult:
        """
        Parse combat output.

        Returns:
            CombatResult with combat state information.
        """
        in_combat = self.is_in_combat(text)

        target_dead = bool(self.TARGET_DEAD.search(text))
        player_dead = bool(self.PLAYER_DEAD.search(text))

        target_name = None
        if target_dead:
            match = self.TARGET_DEAD.search(text)
            if match and match.group(1):
                target_name = match.group(1)

        return CombatResult(
            in_combat=in_combat,
            target_dead=target_dead,
            player_dead=player_dead,
            target_name=target_name,
        )

    def is_combat_over(self, text: str) -> Tuple[bool, bool]:
        """
        Check if combat has ended.

        Returns:
            Tuple of (is_over, was_victory).
        """
        if self.PLAYER_DEAD.search(text):
            return True, False
        if self.TARGET_DEAD.search(text):
            return True, True
        return False, False


class RoomParser:
    """
    Parse room/look output to find targets and exits.
    """

    # Exit patterns - handle both bracketed [Exits: n s] and screenreader "Exits: north, south"
    EXITS_BRACKET_PATTERN = re.compile(r"\[Exits?:([^\]]+)\]", re.IGNORECASE)
    # Match "Exits: ..." - simpler pattern without anchors
    EXITS_PLAIN_PATTERN = re.compile(r"Exits?:\s*([a-z, ]+)", re.IGNORECASE)
    EXIT_DIRS = {'n': 'north', 's': 'south', 'e': 'east', 'w': 'west', 'u': 'up', 'd': 'down'}

    # Monster/NPC detection (lines starting with uppercase, often colored)
    # This is heuristic and may need tuning
    NPC_PATTERN = re.compile(r"^[A-Z][a-z]+.*(?:is here|stands here|sits here)", re.MULTILINE)

    def parse_exits(self, text: str) -> list[str]:
        """
        Parse available exits from room description.

        Returns:
            List of exit directions (e.g., ['north', 'south', 'east']).
        """
        # Try bracketed format first: [Exits: n s e]
        match = self.EXITS_BRACKET_PATTERN.search(text)
        if not match:
            # Try plain format: Exits: north, south, east
            match = self.EXITS_PLAIN_PATTERN.search(text)

        if not match:
            logger.debug(f"No exits pattern match in text: {text[:200]}...")
            return []

        exit_str = match.group(1).strip().lower()
        logger.debug(f"Found exits string: '{exit_str}'")
        exits = []

        # Handle various formats: "north south", "n s", "north, south, east"
        # Split on whitespace or commas
        for word in re.split(r'[\s,]+', exit_str):
            word = word.strip()
            if not word:
                continue
            if word in self.EXIT_DIRS:
                exits.append(self.EXIT_DIRS[word])
            elif word in self.EXIT_DIRS.values():
                exits.append(word)

        logger.debug(f"Parsed exits: {exits}")
        return exits

    def find_targets(self, text: str) -> list[str]:
        """
        Find potential combat targets in room.

        Returns:
            List of target names/keywords.
        """
        targets = []
        for match in self.NPC_PATTERN.finditer(text):
            line = match.group(0)
            # Extract first word as potential target keyword
            words = line.split()
            if words:
                targets.append(words[0].lower())
        return targets


class TrainParser:
    """
    Parse training-related output.
    """

    # Success patterns
    TRAIN_SUCCESS = re.compile(
        r"Your (\w+) increases|You now have (\d+) hit points|You become an avatar",
        re.IGNORECASE
    )

    # Failure patterns
    TRAIN_FAIL = re.compile(
        r"You do not have enough|You need at least|You must have|cannot train",
        re.IGNORECASE
    )

    # Avatar success
    AVATAR_SUCCESS = re.compile(r"You become an avatar", re.IGNORECASE)

    # HP increase
    HP_INCREASE = re.compile(r"You now have (\d+) hit points", re.IGNORECASE)

    def parse(self, text: str) -> Dict[str, Any]:
        """
        Parse training output.

        Returns:
            Dict with 'success', 'new_hp', 'became_avatar' keys.
        """
        result = {
            'success': False,
            'new_hp': None,
            'became_avatar': False,
            'error': None,
        }

        if self.AVATAR_SUCCESS.search(text):
            result['success'] = True
            result['became_avatar'] = True
            return result

        hp_match = self.HP_INCREASE.search(text)
        if hp_match:
            result['success'] = True
            result['new_hp'] = int(hp_match.group(1))
            return result

        if self.TRAIN_SUCCESS.search(text):
            result['success'] = True
            return result

        if self.TRAIN_FAIL.search(text):
            result['success'] = False
            result['error'] = text.strip()

        return result
