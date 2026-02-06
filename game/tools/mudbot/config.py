"""
Configuration dataclasses for MUD bot.
"""

from dataclasses import dataclass, field
from typing import Optional, List


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
        name = f"{self.bot_prefix}{bot_id:03d}"
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
