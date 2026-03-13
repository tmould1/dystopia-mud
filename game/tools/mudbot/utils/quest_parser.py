"""
Parsers for quest system output.

Handles parsing of quest list, quest progress, quest accept,
quest complete, and quest history command output.
"""

import re
import logging
from dataclasses import dataclass, field
from typing import Optional

logger = logging.getLogger(__name__)


# ---------------------------------------------------------------------------
# Data classes
# ---------------------------------------------------------------------------

@dataclass
class QuestEntry:
    """A quest from quest list or quest history output."""
    id: str             # e.g. "T01", "M01"
    name: str
    category: str       # M/T/C/E/CL/A
    status: str         # "available", "active", "complete"
    qp_reward: int = 0


@dataclass
class QuestObjective:
    """A single objective within a quest."""
    description: str
    current: int
    threshold: int
    met: bool


@dataclass
class QuestProgress:
    """Detailed progress for an active quest."""
    id: str
    name: str
    objectives: list[QuestObjective] = field(default_factory=list)

    @property
    def all_met(self) -> bool:
        return all(o.met for o in self.objectives) if self.objectives else False


@dataclass
class QuestAcceptResult:
    """Result of quest accept command."""
    success: bool
    quest_name: str = ""
    error: str = ""


@dataclass
class QuestCompleteResult:
    """Result of quest complete command."""
    completed: list[str] = field(default_factory=list)
    qp_earned: int = 0
    exp_earned: int = 0
    primal_earned: int = 0


# ---------------------------------------------------------------------------
# Parser
# ---------------------------------------------------------------------------

class QuestParser:
    """
    Parse quest command output from the MUD.

    The MUD uses custom color codes (#tHEXCOLOR, #xNNN, #C, #n, etc.)
    and UTF-8 symbols (✓✗▸◆○✦•) that must be stripped before parsing.
    """

    # Extended ANSI/color stripping pattern:
    # - Standard ANSI: \x1b[...m
    # - Dystopia custom: #t followed by 6 hex digits, #x followed by digits,
    #   #C, #n, #R, #G, #B, etc.
    COLOR_PATTERN = re.compile(
        r'\x1b\[[0-9;]*m'          # Standard ANSI escapes
        r'|\[0[;0-9]*m'            # Bracket-style ANSI
        r'|#t[0-9A-Fa-f]{6}'       # True color: #tFFD700
        r'|#x\d{1,3}'              # Extended color: #x035, #x160
        r'|#[RGBYPCWnr]'           # Single-char color codes
    )

    # UTF-8 symbols used in quest output
    SYMBOLS = re.compile(
        '[\u2713\u2717\u25b8\u25c6\u25cb\u2726\u2022]'  # ✓✗▸◆○✦•
    )

    # --- quest list patterns ---
    # After stripping, lines look like:
    #   "  T01            T  Finding Your Feet  Active  (+10 QP)"
    #   "  M01            M  Choose Your Path  (+50 QP)"
    LIST_ENTRY = re.compile(
        r'^\s*'
        r'([A-Z][A-Z0-9_]+)'       # Quest ID (e.g. T01, M01, CL_VAMP_01)
        r'\s+'
        r'([A-Z]{1,2})'            # Category (M, T, C, E, CL, CR, A)
        r'\s+'
        r'(.+?)'                    # Quest name (non-greedy)
        r'(?:\s+(Active|Complete))?' # Optional status tag
        r'(?:\s+\(\+(\d+)\s*QP\))?' # Optional QP reward
        r'\s*$',
        re.MULTILINE
    )

    # --- quest progress patterns ---
    # Quest header: "  T01 - Finding Your Feet"
    PROGRESS_HEADER = re.compile(
        r'^\s*([A-Z][A-Z0-9_]+)\s+-\s+(.+?)\s*$',
        re.MULTILINE
    )
    # Objective line: "    Use the 'look' command (1/1)"
    #              or "    Kill any mob (0/1)"
    PROGRESS_OBJ = re.compile(
        r'^\s+(.+?)\s+\((\d+)/(\d+)\)\s*$',
        re.MULTILINE
    )

    # --- quest accept patterns ---
    ACCEPT_SUCCESS = re.compile(r'Quest Accepted:\s*(.+)', re.IGNORECASE)
    ACCEPT_ERRORS = [
        re.compile(r'No such quest', re.IGNORECASE),
        re.compile(r'not available to you', re.IGNORECASE),
        re.compile(r'already accepted', re.IGNORECASE),
        re.compile(r'already completed', re.IGNORECASE),
        re.compile(r'not available to you yet', re.IGNORECASE),
    ]

    # --- quest complete patterns ---
    COMPLETE_SUCCESS = re.compile(r'Quest Complete:\s*(.+)', re.IGNORECASE)
    COMPLETE_NONE = re.compile(r'no quests ready to turn in', re.IGNORECASE)
    REWARD_QP = re.compile(r'\+(\d+)\s*quest points', re.IGNORECASE)
    REWARD_EXP = re.compile(r'\+(\d+)\s*experience', re.IGNORECASE)
    REWARD_PRIMAL = re.compile(r'\+(\d+)\s*primal energy', re.IGNORECASE)

    # --- quest history patterns ---
    HISTORY_ENTRY = re.compile(
        r'^\s*([A-Z][A-Z0-9_]+)\s+([A-Z]{1,2})\s+(.+?)\s*$',
        re.MULTILINE
    )
    HISTORY_TOTAL = re.compile(r'Total completed:\s*(\d+)', re.IGNORECASE)

    def strip_colors(self, text: str) -> str:
        """Remove all color codes and UTF-8 decorative symbols."""
        text = self.COLOR_PATTERN.sub('', text)
        text = self.SYMBOLS.sub('', text)
        return text

    def parse_quest_list(self, text: str) -> list[QuestEntry]:
        """
        Parse output of 'quest list' command.

        Returns list of QuestEntry with id, name, category, status, qp_reward.
        """
        clean = self.strip_colors(text)
        entries = []

        for m in self.LIST_ENTRY.finditer(clean):
            status_tag = m.group(4)
            if status_tag:
                status = status_tag.lower()
            else:
                status = "available"

            entries.append(QuestEntry(
                id=m.group(1),
                name=m.group(3).strip(),
                category=m.group(2),
                status=status,
                qp_reward=int(m.group(5)) if m.group(5) else 0,
            ))

        logger.debug(f"Parsed quest list: {len(entries)} entries")
        return entries

    def parse_quest_progress(self, text: str) -> list[QuestProgress]:
        """
        Parse output of 'quest progress' command.

        Returns list of QuestProgress with objectives and their current/threshold.
        """
        clean = self.strip_colors(text)
        quests = []

        # Split into quest blocks by finding headers
        headers = list(self.PROGRESS_HEADER.finditer(clean))
        if not headers:
            logger.debug("No quest progress headers found")
            return quests

        for idx, header_match in enumerate(headers):
            quest_id = header_match.group(1)
            quest_name = header_match.group(2).strip()

            # Get text between this header and the next (or end)
            start = header_match.end()
            end = headers[idx + 1].start() if idx + 1 < len(headers) else len(clean)
            block = clean[start:end]

            objectives = []
            for obj_match in self.PROGRESS_OBJ.finditer(block):
                current = int(obj_match.group(2))
                threshold = int(obj_match.group(3))
                objectives.append(QuestObjective(
                    description=obj_match.group(1).strip(),
                    current=current,
                    threshold=threshold,
                    met=(current >= threshold),
                ))

            quests.append(QuestProgress(
                id=quest_id,
                name=quest_name,
                objectives=objectives,
            ))

        logger.debug(f"Parsed quest progress: {len(quests)} quests")
        return quests

    def parse_quest_accept(self, text: str) -> QuestAcceptResult:
        """Parse output of 'quest accept <id>' command."""
        clean = self.strip_colors(text)

        m = self.ACCEPT_SUCCESS.search(clean)
        if m:
            return QuestAcceptResult(
                success=True,
                quest_name=m.group(1).strip(),
            )

        for pattern in self.ACCEPT_ERRORS:
            if pattern.search(clean):
                return QuestAcceptResult(
                    success=False,
                    error=pattern.pattern,
                )

        return QuestAcceptResult(success=False, error="Unknown response")

    def parse_quest_complete(self, text: str) -> QuestCompleteResult:
        """Parse output of 'quest complete' command."""
        clean = self.strip_colors(text)
        result = QuestCompleteResult()

        if self.COMPLETE_NONE.search(clean):
            return result

        for m in self.COMPLETE_SUCCESS.finditer(clean):
            result.completed.append(m.group(1).strip())

        # Sum up all rewards in the text
        for m in self.REWARD_QP.finditer(clean):
            result.qp_earned += int(m.group(1))
        for m in self.REWARD_EXP.finditer(clean):
            result.exp_earned += int(m.group(1))
        for m in self.REWARD_PRIMAL.finditer(clean):
            result.primal_earned += int(m.group(1))

        logger.debug(f"Parsed quest complete: {len(result.completed)} quests, "
                      f"+{result.qp_earned} QP")
        return result

    def parse_quest_history(self, text: str) -> tuple[list[QuestEntry], int]:
        """
        Parse output of 'quest history' command.

        Returns (list of completed QuestEntry, total_count).
        """
        clean = self.strip_colors(text)
        entries = []

        for m in self.HISTORY_ENTRY.finditer(clean):
            entries.append(QuestEntry(
                id=m.group(1),
                name=m.group(3).strip(),
                category=m.group(2),
                status="turned_in",
            ))

        total = 0
        total_match = self.HISTORY_TOTAL.search(clean)
        if total_match:
            total = int(total_match.group(1))

        logger.debug(f"Parsed quest history: {len(entries)} entries, total={total}")
        return entries, total

    def detect_auto_complete(self, text: str) -> Optional[str]:
        """
        Detect auto-complete notification in output stream.

        Returns quest name if auto-complete detected, None otherwise.
        """
        clean = self.strip_colors(text)
        m = self.COMPLETE_SUCCESS.search(clean)
        if m:
            return m.group(1).strip()
        return None


# ---------------------------------------------------------------------------
# Story parser
# ---------------------------------------------------------------------------

@dataclass
class StoryState:
    """Parsed state of the story quest."""
    node: int = 0               # 0=not started, 1-16=active, 17+=done
    kills: int = 0
    progress: int = 0           # bitfield (0x01, 0x02, 0x04)
    tasks: list[bool] = field(default_factory=lambda: [False, False, False])
    not_started: bool = False
    completed: bool = False
    clue: str = ""


class StoryParser:
    """
    Parse output from 'story' and 'storyadmin' commands.

    Regular 'story' output:
        <clue text>
        [Tasks: *-* | Kills: 3]

    'storyadmin <name>' output:
        Node: 2 (active)
        Kills: 3
        Progress: 0x07 [*-*]
    """

    COLOR_PATTERN = QuestParser.COLOR_PATTERN
    SYMBOLS = QuestParser.SYMBOLS

    # storyadmin patterns
    NODE_RE = re.compile(r'Node:\s*(\d+)')
    KILLS_RE = re.compile(r'Kills:\s*(\d+)')
    PROGRESS_RE = re.compile(r'Progress:\s*0x([0-9A-Fa-f]+)')

    # Regular story command: [Tasks: *-* | Kills: N]
    TASKS_RE = re.compile(r'Tasks:\s*([*-])([*-])([*-])\s*\|\s*Kills:\s*(\d+)')

    # Detection patterns
    NOT_STARTED_RE = re.compile(r'no recollection', re.IGNORECASE)
    COMPLETED_RE = re.compile(r'reflect on your journey|not forgotten', re.IGNORECASE)

    def strip_colors(self, text: str) -> str:
        """Remove color codes and symbols."""
        text = self.COLOR_PATTERN.sub('', text)
        text = self.SYMBOLS.sub('', text)
        return text

    def parse_storyadmin(self, text: str) -> StoryState:
        """Parse 'storyadmin <name>' output for exact numeric state."""
        clean = self.strip_colors(text)
        state = StoryState()

        m = self.NODE_RE.search(clean)
        if m:
            state.node = int(m.group(1))

        m = self.KILLS_RE.search(clean)
        if m:
            state.kills = int(m.group(1))

        m = self.PROGRESS_RE.search(clean)
        if m:
            state.progress = int(m.group(1), 16)
            state.tasks = [
                bool(state.progress & 0x01),
                bool(state.progress & 0x02),
                bool(state.progress & 0x04),
            ]

        if "not started" in clean.lower():
            state.not_started = True
        elif "completed" in clean.lower():
            state.completed = True

        return state

    def parse_story(self, text: str) -> StoryState:
        """Parse regular 'story' command output."""
        clean = self.strip_colors(text)
        state = StoryState()

        if self.NOT_STARTED_RE.search(clean):
            state.not_started = True
            return state

        if self.COMPLETED_RE.search(clean):
            state.completed = True
            state.node = 99
            return state

        # Parse tasks line: [Tasks: *-* | Kills: N]
        m = self.TASKS_RE.search(clean)
        if m:
            state.tasks = [m.group(1) == '*', m.group(2) == '*', m.group(3) == '*']
            state.kills = int(m.group(4))
            state.progress = (
                (0x01 if state.tasks[0] else 0)
                | (0x02 if state.tasks[1] else 0)
                | (0x04 if state.tasks[2] else 0)
            )

        # Store the clue text (everything before the tasks line)
        tasks_pos = clean.find('[Tasks:')
        if tasks_pos > 0:
            state.clue = clean[:tasks_pos].strip()
        else:
            state.clue = clean.strip()

        return state
