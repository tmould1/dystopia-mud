"""Entity editor panels for MUD editor."""

from .help import HelpEditorPanel
from .mobile import MobileEditorPanel
from .object import ObjectEditorPanel
from .room import RoomEditorPanel
from .reset import ResetEditorPanel
from .area import AreaInfoPanel
from .shop import ShopEditorPanel
from .gameconfig import GameConfigPanel, BalanceConfigPanel
from .ability_config import AbilityConfigPanel
from .audio_config import AudioConfigPanel
from .game_panels import (
    KingdomsPanel, BansPanel, DisabledCommandsPanel,
    LeaderboardPanel, NotesPanel, BugsPanel,
    SuperAdminsPanel, ImmortalPretitlesPanel
)
from .player import PlayerEditorPanel
from .class_display import ClassDisplayPanel

__all__ = [
    'HelpEditorPanel',
    'MobileEditorPanel',
    'ObjectEditorPanel',
    'RoomEditorPanel',
    'ResetEditorPanel',
    'AreaInfoPanel',
    'ShopEditorPanel',
    'GameConfigPanel',
    'BalanceConfigPanel',
    'AbilityConfigPanel',
    'AudioConfigPanel',
    'KingdomsPanel',
    'BansPanel',
    'DisabledCommandsPanel',
    'LeaderboardPanel',
    'NotesPanel',
    'BugsPanel',
    'SuperAdminsPanel',
    'ImmortalPretitlesPanel',
    'PlayerEditorPanel',
    'ClassDisplayPanel',
]
