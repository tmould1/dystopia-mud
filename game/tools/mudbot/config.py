"""
Configuration dataclasses for MUD bot.
"""

from dataclasses import dataclass, field
from typing import Optional, List


def _alpha_suffix(bot_id: int, min_len: int = 3) -> str:
    """Return a stable alphabetic suffix for a 1-based bot index.

    Examples: 1 -> 'aaa', 2 -> 'aab', 26 -> 'aaz', 27 -> 'aba'.
    """
    if bot_id < 1:
        raise ValueError("bot_id must be >= 1")

    n = bot_id - 1
    chars: list[str] = []
    while True:
        n, rem = divmod(n, 26)
        chars.append(chr(ord('a') + rem))
        if n == 0:
            break

    suffix = ''.join(reversed(chars))
    if len(suffix) < min_len:
        suffix = ('a' * (min_len - len(suffix))) + suffix
    return suffix


@dataclass
class BotConfig:
    """Configuration for a single bot instance."""

    # Character identity
    name: str
    password: str

    # Server connection
    host: str = "localhost"
    port: int = 8888

    # Character creation settings
    sex: str = "m"                # m or f
    experience_level: int = 3     # 1=newbie, 2=mud exp, 3=veteran
    color_mode: str = "s"         # n=none, a=ansi, x=xterm, s=screen reader (best for bots)

    # Bot behavior
    command_delay: float = 0.5    # Seconds between commands
    read_timeout: float = 30.0    # Timeout for reading server responses
    reconnect_attempts: int = 3   # Number of reconnection attempts
    reconnect_delay: float = 5.0  # Delay between reconnection attempts

    # Features (for future use)
    use_gmcp: bool = False        # Enable GMCP if available

    def __post_init__(self):
        """Validate configuration."""
        if not self.name:
            raise ValueError("Bot name cannot be empty")
        if len(self.name) < 3:
            raise ValueError("Bot name must be at least 3 characters")
        if len(self.name) > 12:
            raise ValueError("Bot name cannot exceed 12 characters")
        if not self.name.isalnum():
            raise ValueError("Bot name must be alphanumeric")

        if not self.password:
            raise ValueError("Password cannot be empty")
        if len(self.password) < 5:
            raise ValueError("Password must be at least 5 characters")

        self.sex = self.sex.lower()
        if self.sex not in ('m', 'f'):
            raise ValueError("Sex must be 'm' or 'f'")

        if self.experience_level not in (1, 2, 3):
            raise ValueError("Experience level must be 1, 2, or 3")

        self.color_mode = self.color_mode.lower()
        if self.color_mode not in ('n', 'a', 'x', 's'):
            raise ValueError("Color mode must be 'n', 'a', 'x', or 's'")


@dataclass
class CommanderConfig:
    """Configuration for the bot commander (multi-bot manager)."""

    # Server connection
    host: str = "localhost"
    port: int = 8888

    # Bot spawning
    bot_prefix: str = "TestBot"   # Bot names: TestBot001, TestBot002, etc.
    bot_password: str = "testpass123"
    num_bots: int = 1

    # Timing
    stagger_delay: float = 2.0    # Delay between bot connections
    command_delay: float = 0.5    # Delay between commands per bot

    # Character settings for spawned bots
    sex: str = "m"
    experience_level: int = 3
    color_mode: str = "s"  # screenreader mode for clean output

    def generate_bot_config(self, bot_id: int) -> BotConfig:
        """Generate a BotConfig for a specific bot instance."""
        suffix = _alpha_suffix(bot_id)
        max_prefix_len = max(1, 12 - len(suffix))
        prefix = self.bot_prefix[:max_prefix_len]
        name = f"{prefix}{suffix}"
        return BotConfig(
            name=name,
            password=self.bot_password,
            host=self.host,
            port=self.port,
            sex=self.sex,
            experience_level=self.experience_level,
            color_mode=self.color_mode,
            command_delay=self.command_delay,
        )

    def __post_init__(self):
        """Validate configuration."""
        if not self.bot_prefix:
            raise ValueError("Bot prefix cannot be empty")
        if len(self.bot_prefix) > 9:
            raise ValueError("Bot prefix too long (max 9 chars to allow for 3-digit suffix)")
        if not self.bot_prefix.isalnum():
            raise ValueError("Bot prefix must be alphanumeric")

        if not self.bot_password:
            raise ValueError("Bot password cannot be empty")
        if len(self.bot_password) < 5:
            raise ValueError("Bot password must be at least 5 characters")

        if self.num_bots < 1:
            raise ValueError("Must spawn at least 1 bot")
        if self.num_bots > 100:
            raise ValueError("Cannot spawn more than 100 bots")


@dataclass
class ProgressionConfig:
    """Configuration for avatar progression bot."""

    # Progression targets (avatar phase)
    kills_needed: int = 5         # Monsters to kill before training
    hp_target: int = 2000         # HP required for avatar training (initial)

    # Extended training targets (post-selfclass)
    extended_hp_target: int = 6000   # HP target after class selection
    mana_target: int = 6000          # Mana target after class selection
    move_target: int = 6000          # Movement target after class selection

    # Autostance setting
    autostance: str = "bull"         # Default combat stance

    # Combat settings
    target_monster: str = "monster"  # Monster keyword to kill
    max_kill_attempts: int = 20   # Give up after this many failed kills

    # Navigation (room vnums for school area)
    school_entrance: int = 3700
    arena_room: Optional[int] = None  # Will be discovered

    # Black Dragon's Lair farming location
    # Path from recall: down, 2x south, 6x east, 2x south, 2x down, north
    farm_path: List[str] = field(default_factory=lambda: [
        'down', 'south', 'south',
        'east', 'east', 'east', 'east', 'east', 'east',
        'south', 'south', 'down', 'down', 'north'
    ])
    farm_monsters: List[str] = field(default_factory=lambda: [
        'hobgoblin', 'shaman'
    ])

    # Safety
    min_hp_to_fight: int = 50     # Don't fight if HP below this
    flee_threshold: int = 20      # Flee if HP drops below this


@dataclass
class DemonProgressionConfig:
    """Configuration for demon class progression bot."""

    # Discipline training priority and targets
    discipline_priority: List[str] = field(default_factory=lambda: [
        "attack",      # First: unlocks combat abilities + graft
        "hellfire",    # Fire damage
        "temptation",  # Social abilities
        "morphosis",   # Transformation
        "corruption",  # Debuffs
        "geluge",      # Ice abilities
        "discord",     # Chaos abilities
        "nether",      # Death/darkness
        "immunae",     # Resistances
    ])
    attack_target: int = 7        # Level 7 for blink
    other_discipline_target: int = 5  # Default target for other disciplines

    # Warp settings
    max_warps_to_obtain: int = 18  # Max warps (costs 15000 demon points each)
    obtain_warps: bool = True      # Whether to attempt obtaining warps

    # Graft settings
    enable_grafting: bool = True
    max_extra_arms: int = 2       # 3rd and 4th hand

    # Demonarmour settings
    create_demonarmour: bool = True
    demonarmour_pieces: List[str] = field(default_factory=lambda: [
        "plate", "helmet", "leggings", "boots", "gauntlets",
        "sleeves", "cape", "belt", "collar", "ring",
        "bracer", "visor", "longsword", "shortsword"
    ])

    # Combat ability toggles
    enable_claws: bool = True
    enable_horns: bool = True
    enable_wings: bool = True
    enable_tail: bool = True


@dataclass
class QuestConfig:
    """Configuration for quest testing bot."""

    # Quest selection
    mode: str = "all"                   # "tutorial", "main", "all"
    stop_after_quest: str = "M05"       # Stop after this quest ID (empty = run all)
    max_quest_cycles: int = 500          # Safety limit on state machine iterations

    # Class selection for M01 selfclass objective
    selfclass: str = "demon"            # Default class to pick

    # Story system
    enable_story: bool = False           # Story navigation not yet functional
    max_story_node: int = 16             # How far to progress story
    force_story_progress: bool = False   # Periodically prioritize active story nodes

    # PvP
    enable_pvp: bool = False             # Enable PvP handlers (arena, kill_player)
