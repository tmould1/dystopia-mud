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
from .class_aura import ClassAuraPanel
from .class_equipment import ClassEquipmentPanel
from .class_starting import ClassStartingPanel
from .class_score import ClassScorePanel
from .class_registry import ClassRegistryPanel
from .area_map import AreaMapPanel
from .tables_panels import (
    SocialsPanel, SlaysPanel, LiquidsPanel, WearLocationsPanel, CalendarPanel
)

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
    'ClassAuraPanel',
    'ClassEquipmentPanel',
    'ClassStartingPanel',
    'ClassScorePanel',
    'ClassRegistryPanel',
    'AreaMapPanel',
    'SocialsPanel',
    'SlaysPanel',
    'LiquidsPanel',
    'WearLocationsPanel',
    'CalendarPanel',
]
