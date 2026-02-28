"""
Data models for Dystopia MUD area data.

Constants are parsed from C headers at import time via merc_constants.py.
Dataclasses define the in-memory structure for area entities.
"""

from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

from .merc_constants import (
    # Direction constants
    DIR_NORTH, DIR_EAST, DIR_SOUTH, DIR_WEST, DIR_UP, DIR_DOWN,
    DIR_NAMES, REVERSE_DIR, DIR_OFFSETS,
    # Bitfield flag dicts (simple {int: str} format)
    ROOM_FLAGS, EXIT_FLAGS, SECTOR_NAMES, ITEM_TYPES,
    ITEM_EXTRA_FLAGS, ITEM_WEAR_FLAGS,
    ACT_FLAGS, AFF_FLAGS,
    # Class table
    CLASS_TABLE,
    # Sex types
    SEX_TYPES,
    # Raw parsed data for building enriched formats
    PLR_FLAGS_RAW, EXTRA_FLAGS_RAW, NEWBITS_FLAGS_RAW, IMM_FLAGS_RAW,
    APPLY_TYPES_RAW, WEAR_LOCATIONS_RAW, DISC_DEFINES, STANCE_DEFINES,
    STAT_SOURCE_ENUM,
)


# =============================================================================
# Enriched Constants (built from parsed raw data + display overrides)
# =============================================================================

def _build_enriched_flags(raw: Dict[int, str], prefix: str,
                          display: Dict[str, str]) -> Dict[int, Tuple[str, str]]:
    """
    Build {value: ('PREFIX_NAME', 'Display Name')} from parsed raw data.

    Unknown names get a default title-cased display name.
    """
    result = {}
    for value, raw_name in raw.items():
        enum_name = f'{prefix}{raw_name.upper()}'
        display_name = display.get(raw_name, raw_name.replace('_', ' ').title())
        result[value] = (enum_name, display_name)
    return result


# --- Player flags (stored in 'act' field for players) ---
PLR_FLAGS = _build_enriched_flags(PLR_FLAGS_RAW, 'PLR_', {
    'is_npc': 'Is NPC (never set)',
    'brief3': 'Brief 3', 'lefthand': 'Left Handed', 'autoexit': 'Auto Exit',
    'autoloot': 'Auto Loot', 'autosac': 'Auto Sacrifice', 'blank': 'Blank Line',
    'brief': 'Brief Mode', 'screenreader': 'Screen Reader', 'combine': 'Combine Items',
    'prompt': 'Show Prompt', 'telnet_ga': 'Telnet GA', 'holylight': 'Holy Light',
    'wizinvis': 'Wizard Invis', 'ansi': 'ANSI Color', 'silence': 'Silenced',
    'vt102': 'VT102 Mode', 'righthand': 'Right Handed', 'no_tell': 'No Tell',
    'log': 'Logged', 'deny': 'Denied', 'freeze': 'Frozen',
    'prefer_mcmp': 'Prefer MCMP', 'automap': 'Auto Map', 'brief2': 'Brief 2',
    'watcher': 'Watcher', 'acid': 'Acid', 'brief4': 'Brief 4',
    'ambi': 'Ambidextrous', 'xterm': 'XTerm-256',
    'prefer_gmcp': 'Prefer GMCP', 'prefer_mxp': 'Prefer MXP',
})

# --- Extra flags (stored in 'extra' field) ---
EXTRA_FLAGS_PLAYER = _build_enriched_flags(EXTRA_FLAGS_RAW, 'EXTRA_', {
    'trusted': 'Trusted', 'newpass': 'New Password', 'oswitch': 'Object Switch',
    'switch': 'Switched', 'fake_con': 'Fake Con', 'stance': 'Stance', 'done': 'Done',
    'exp': 'Exp', 'pregnant': 'Pregnant', 'labour': 'In Labour', 'born': 'Born',
    'prompt': 'Prompt', 'married': 'Married', 'afk': 'AFK', 'dragon': 'Dragon',
    'call_all': 'Call All', 'bsd': 'BSD', 'earthmeld': 'Earth Meld',
    'plasma': 'Plasma', 'anti_godless': 'Anti Godless',
    'truecolor': 'True Color', 'awe': 'Awe', 'rot': 'Rot',
    'zombie': 'Zombie', 'baal': 'Baal', 'flash': 'Flash',
})

# --- Newbits flags (stored in 'newbits' field) ---
NEWBITS_FLAGS = _build_enriched_flags(NEWBITS_FLAGS_RAW, 'NEW_', {
    'slam': 'Slam', 'quills': 'Quills', 'jawlock': 'Jawlock',
    'perception': 'Perception', 'skin': 'Skin', 'tide': 'Tide', 'coil': 'Coil',
    'rend': 'Rend', 'monkflame': 'Monk Flame', 'sclaws': 'Shadow Claws',
    'ironmind': 'Iron Mind', 'monkcloak': 'Monk Cloak',
    'monkadam': 'Monk Adamantine', 'monkskin': 'Monk Skin',
    'monkfavor': 'Monk Favor', 'cloak': 'Cloak', 'drowhate': 'Drow Hate',
    'darkness': 'Darkness', 'mentalblock': 'Mental Block', 'vision': 'Vision',
    'natural': 'Natural', 'power': 'Power', 'dform': 'Demon Form',
    'mastery': 'Mastery', 'darktendrils': 'Dark Tendrils',
    'multiarms': 'Multi Arms', 'bladespin': 'Blade Spin',
    'fightdance': 'Fight Dance', 'cubeform': 'Cube Form',
})

# --- Immunity flags (stored in 'immune' field) ---
IMM_FLAGS = _build_enriched_flags(IMM_FLAGS_RAW, 'IMM_', {
    'slash': 'Slash', 'stab': 'Stab', 'smash': 'Smash', 'animal': 'Animal',
    'misc': 'Misc', 'charm': 'Charm', 'heat': 'Heat', 'cold': 'Cold',
    'lightning': 'Lightning', 'acid': 'Acid', 'summon': 'Summon', 'voodoo': 'Voodoo',
    'vampire': 'Vampire', 'stake': 'Stake', 'sunlight': 'Sunlight',
    'shielded': 'Shielded', 'hurl': 'Hurl', 'backstab': 'Backstab', 'kick': 'Kick',
    'disarm': 'Disarm', 'steal': 'Steal', 'sleep': 'Sleep', 'drain': 'Drain',
    'shield2': 'Chaotic Shield', 'transport': 'Transport', 'travel': 'Travel',
})

# --- Discipline names (enriched from DISC_DEFINES) ---
_DISC_DISPLAY = {
    'vamp_cele': 'Celerity', 'vamp_fort': 'Fortitude', 'vamp_obte': 'Obtenebration',
    'vamp_pres': 'Presence', 'vamp_quie': 'Quietus', 'vamp_thau': 'Thaumaturgy',
    'vamp_ausp': 'Auspex', 'vamp_domi': 'Dominate', 'vamp_obfu': 'Obfuscate',
    'vamp_pote': 'Potence', 'vamp_prot': 'Protean', 'vamp_serp': 'Serpentis',
    'vamp_vici': 'Vicissitude', 'vamp_daim': 'Daimoinon', 'vamp_anim': 'Animalism',
    'were_bear': 'Bear', 'were_lynx': 'Lynx', 'were_boar': 'Boar',
    'were_owl': 'Owl', 'were_spid': 'Spider', 'were_wolf': 'Wolf',
    'were_hawk': 'Hawk', 'were_mant': 'Mantis', 'were_rapt': 'Raptor',
    'were_luna': 'Luna', 'were_pain': 'Pain', 'were_cong': 'Congregation',
    'daem_hell': 'Hellfire', 'daem_atta': 'Attacks', 'daem_temp': 'Tempering',
    'daem_morp': 'Morphology', 'daem_corr': 'Corruption', 'daem_gelu': 'Gelupathy',
    'daem_disc': 'Discord', 'daem_neth': 'Nether', 'daem_immu': 'Immunity',
    'vamp_chim': 'Chimerstry', 'vamp_than': 'Thanatosis', 'vamp_obea': 'Obeah',
    'vamp_necr': 'Necromancy', 'vamp_melp': 'Melpominee',
}
DISCIPLINE_NAMES = {
    v: (f'DISC_{n.upper()}', _DISC_DISPLAY.get(n, n.replace('_', ' ').title()))
    for v, n in DISC_DEFINES.items()
}

# --- Stat sources (from STAT_SOURCE enum in db_class.h) ---
_STAT_DISPLAY = {
    'stat_none': 'None',
    'stat_beast': 'Beast (ch->beast)', 'stat_rage': 'Rage (ch->rage)',
    'stat_chi_current': 'Chi Current', 'stat_chi_maximum': 'Chi Maximum',
    'stat_gnosis_current': 'Gnosis Current', 'stat_gnosis_maximum': 'Gnosis Maximum',
    'stat_monkblock': 'Monk Block', 'stat_siltol': 'Silver Tolerance',
    'stat_souls': 'Souls',
    'stat_demon_power': 'Demon Power (Current)', 'stat_demon_total': 'Demon Power (Total)',
    'stat_droid_power': 'Droid Power',
    'stat_drow_power': 'Drow Power', 'stat_drow_magic': 'Drow Magic',
    'stat_tpoints': 'Tanarri Points',
    'stat_angel_justice': 'Angel Justice', 'stat_angel_love': 'Angel Love',
    'stat_angel_harmony': 'Angel Harmony', 'stat_angel_peace': 'Angel Peace',
    'stat_shape_counter': 'Shape Counter', 'stat_phase_counter': 'Phase Counter',
    'stat_hara_kiri': 'Hara Kiri',
    'stat_dragon_attunement': 'Dragon Attunement',
    'stat_dragon_essence_peak': 'Dragon Essence Peak',
}
STAT_SOURCES = {
    v: (n.upper(), _STAT_DISPLAY.get(n, n.replace('_', ' ').title()))
    for v, n in STAT_SOURCE_ENUM.items()
}


def get_stat_source_name(stat_source: int) -> str:
    """Get human-readable stat source name."""
    if stat_source in STAT_SOURCES:
        return STAT_SOURCES[stat_source][1]
    return f'Unknown ({stat_source})'


# =============================================================================
# Simple Renamed Exports (parsed data, display-friendly names)
# =============================================================================

# Apply types for object affects
APPLY_TYPES = APPLY_TYPES_RAW

# Wear locations (filter out WEAR_NONE=-1)
WEAR_LOCATIONS = {k: v for k, v in WEAR_LOCATIONS_RAW.items() if k >= 0}

# Object extra/wear flags use the ITEM_ parsed variants
EXTRA_FLAGS = ITEM_EXTRA_FLAGS
WEAR_FLAGS = ITEM_WEAR_FLAGS


# =============================================================================
# Manually Curated Constants (not parseable from C headers)
# =============================================================================

# Weapon damage types (array indices from const.c attack_table, not #defines)
WEAPON_TYPES = {
    0: 'hit', 1: 'slice', 2: 'stab', 3: 'slash', 4: 'whip',
    5: 'claw', 6: 'blast', 7: 'pound', 8: 'crush', 9: 'grep',
    10: 'bite', 11: 'pierce', 12: 'suck',
}

# Weapon spell effects (value[0] % 1000 for spell weapons)
# These are skill_table[] array indices from const.c (NOT SLOT numbers!)
WEAPON_SPELLS = {
    0: None,
    1: 'acid_blast', 2: 'armor', 3: 'bless', 4: 'blindness',
    5: 'burning_hands', 6: 'call_lightning', 7: 'cause_critical',
    8: 'cause_light', 9: 'cause_serious', 10: 'change_sex',
    11: 'charm_person', 12: 'chill_touch', 13: 'colour_spray',
    14: 'continual_light', 15: 'control_weather', 16: 'create_food',
    17: 'create_spring', 18: 'create_water', 19: 'cure_blindness',
    20: 'cure_critical', 21: 'cure_light', 22: 'cure_poison',
    23: 'cure_serious', 24: 'curse', 25: 'detect_evil',
    26: 'detect_hidden', 27: 'detect_invis', 28: 'detect_magic',
    29: 'detect_poison', 30: 'dispel_evil', 31: 'dispel_magic',
    32: 'earthquake', 33: 'enchant_weapon', 34: 'energy_drain',
    35: 'faerie_fire', 36: 'faerie_fog', 37: 'fireball',
    38: 'flamestrike', 39: 'fly', 40: 'gate',
    41: 'giant_strength', 42: 'harm', 43: 'heal',
    44: 'identify', 45: 'infravision', 46: 'invis',
    47: 'know_alignment', 48: 'lightning_bolt', 49: 'locate_object',
    50: 'magic_missile', 51: 'mass_invis', 52: 'pass_door',
    53: 'poison', 54: 'protection', 55: 'readaura',
    56: 'refresh', 57: 'remove_curse', 58: 'sanctuary',
    59: 'shield', 60: 'shocking_grasp', 61: 'sleep',
    62: 'stone_skin', 63: 'summon', 64: 'teleport',
    65: 'ventriloquate', 66: 'weaken', 67: 'word_of_recall',
    68: 'acid_breath', 69: 'fire_breath', 70: 'frost_breath',
    71: 'gas_breath', 72: 'lightning_breath',
}

# Weapon wielder affects (value[0] / 1000 for spell weapons)
WEAPON_AFFECTS = {
    0: None,
    1: 'darkness_aura', 2: 'see_invis', 3: 'flight', 4: 'infravision',
    5: 'invisibility', 6: 'pass_door', 7: 'protect_evil', 8: 'sanctuary',
    9: 'sneak', 10: 'shock_shield', 11: 'fire_shield', 12: 'ice_shield',
    13: 'acid_shield', 14: 'god_power', 15: 'chaos_shield', 16: 'regeneration',
    17: 'haste', 18: 'armor_pierce', 19: 'player_protect',
    20: 'darkness_shield', 21: 'superior_protect', 22: 'truesight',
    23: 'fleet_foot', 24: 'concealment', 25: 'beast_power',
    27: 'detect_invis', 39: 'fly', 45: 'dark_vision', 46: 'invis',
    52: 'phase', 54: 'holy_protect', 57: 'combat_protect',
}

# Armor special types (value[3] for armor)
ARMOR_SPECIALS = {
    0: None,
    1: 'darkness_aura', 2: 'see_invis', 3: 'flight', 4: 'infravision',
    5: 'invisibility', 6: 'pass_door', 7: 'protect_evil', 8: 'sanctuary',
    9: 'sneak', 10: 'shock_shield', 11: 'fire_shield', 12: 'ice_shield',
    13: 'acid_shield', 14: 'god_power', 15: 'chaos_shield', 16: 'regeneration',
    17: 'haste', 18: 'armor_pierce', 19: 'player_protect',
    20: 'darkness_shield', 21: 'superior_protect', 22: 'truesight',
    23: 'fleet_foot', 24: 'concealment', 25: 'beast_power',
    27: 'detect_invis', 28: 'ancient_magic', 29: 'third_eye', 30: 'talons',
    39: 'fly', 45: 'dark_vision', 46: 'invis',
    52: 'phase', 54: 'holy_protect', 57: 'combat_protect',
    88: 'unknown_88', 139: 'protect_good',
}

# Container flags (merc.h CONT_* defines, value[1] for containers)
CONTAINER_FLAGS = {1: 'Closeable', 2: 'Pickproof', 4: 'Closed', 8: 'Locked'}

# Weapon proficiency names (index -> name)
WEAPON_NAMES = {
    0: 'Unarmed', 1: 'Slice', 2: 'Stab', 3: 'Slash', 4: 'Whip',
    5: 'Claw', 6: 'Blast', 7: 'Pound', 8: 'Crush', 9: 'Grep',
    10: 'Bite', 11: 'Pierce', 12: 'Suck',
}

# Spell proficiency names (index -> name)
SPELL_NAMES = {
    0: 'Purple (General)', 1: 'Red', 2: 'Blue', 3: 'Green', 4: 'Yellow',
}

# Stance proficiency names (index -> name)
STANCE_NAMES = {
    1: 'Viper', 2: 'Crane', 3: 'Crab', 4: 'Mongoose', 5: 'Bull',
    6: 'Mantis', 7: 'Dragon', 8: 'Tiger', 9: 'Monkey', 10: 'Swallow',
}

# Super stance names
SUPER_STANCE_NAMES = {
    13: 'SS1', 14: 'SS2', 15: 'SS3', 16: 'SS4', 17: 'SS5',
}

MASTERY_THRESHOLD = 200

# Werewolf gifts array indices (gifts[21])
GIFT_NAMES = {i: f'Gift {i}' for i in range(21)}


# =============================================================================
# Utility Functions
# =============================================================================

def decode_flags(value: int, flag_dict: dict) -> List[str]:
    """Decode a bitfield into a list of flag names."""
    flags = []
    for bit, name in flag_dict.items():
        if value & bit:
            flags.append(name)
    return flags


# =============================================================================
# Data Classes
# =============================================================================

@dataclass
class ExtraDescription:
    """An extra description on an object or room."""
    keyword: str
    description: str


@dataclass
class RoomText:
    """A room text trigger."""
    input: str
    output: str
    choutput: str
    name: str
    type: int = 0
    power: int = 0
    mob: int = 0


@dataclass
class Exit:
    """Represents an exit from a room."""
    direction: int
    destination_vnum: int
    is_door: bool = False
    door_flags: int = 0
    key_vnum: int = -1
    description: str = ""
    keyword: str = ""
    one_way: bool = False  # Set during analysis
    warped: bool = False   # Set during coordinate assignment

    def to_dict(self) -> dict:
        return {
            'to': self.destination_vnum,
            'door': self.is_door,
            'one_way': self.one_way,
            'warped': self.warped,
        }


@dataclass
class Room:
    """Represents a room in an area."""
    vnum: int
    name: str
    description: str = ""
    sector_type: int = 0
    room_flags: int = 0
    exits: Dict[int, Exit] = field(default_factory=dict)
    extra_descs: List['ExtraDescription'] = field(default_factory=list)
    room_texts: List['RoomText'] = field(default_factory=list)
    coords: Optional[Tuple[int, int, int]] = None
    dead_end: bool = False

    def to_dict(self) -> dict:
        result = {
            'vnum': self.vnum,
            'name': self.name,
            'sector': self.sector_type,
            'sector_name': SECTOR_NAMES.get(self.sector_type, 'unknown'),
            'exits': {str(d): e.to_dict() for d, e in self.exits.items()},
            'dead_end': self.dead_end,
        }
        if self.coords:
            result['coords'] = {'x': self.coords[0], 'y': self.coords[1], 'z': self.coords[2]}
        return result


@dataclass
class ObjectAffect:
    """An affect modifier on an object."""
    apply_type: int
    modifier: int

    def to_dict(self) -> dict:
        return {
            'type': APPLY_TYPES.get(self.apply_type, f'unknown_{self.apply_type}'),
            'type_id': self.apply_type,
            'modifier': self.modifier,
        }


@dataclass
class Object:
    """Represents an object/item in an area."""
    vnum: int
    name: str  # keywords
    short_descr: str
    long_descr: str
    item_type: int = 0
    extra_flags: int = 0
    wear_flags: int = 0
    value: List[int] = field(default_factory=lambda: [0, 0, 0, 0])
    weight: int = 0
    cost: int = 0
    level: int = 0
    affects: List[ObjectAffect] = field(default_factory=list)
    extra_descs: List['ExtraDescription'] = field(default_factory=list)
    chpoweron: str = ""
    chpoweroff: str = ""
    chpoweruse: str = ""
    victpoweron: str = ""
    victpoweroff: str = ""
    victpoweruse: str = ""
    spectype: int = 0
    specpower: int = 0

    def to_dict(self) -> dict:
        return {
            'vnum': self.vnum,
            'name': self.name,
            'short_descr': self.short_descr,
            'item_type': self.item_type,
            'item_type_name': ITEM_TYPES.get(self.item_type, 'unknown'),
            'extra_flags': self.extra_flags,
            'wear_flags': self.wear_flags,
            'value': self.value,
            'weight': self.weight,
            'cost': self.cost,
            'level': self.level,
            'affects': [a.to_dict() for a in self.affects],
        }


@dataclass
class Mobile:
    """Represents a mobile/NPC in an area."""
    vnum: int
    name: str  # keywords
    short_descr: str
    long_descr: str
    description: str = ""
    act_flags: int = 0
    affected_by: int = 0
    alignment: int = 0
    level: int = 1
    hitroll: int = 0
    ac: int = 0
    hit_dice: Tuple[int, int, int] = (1, 1, 0)  # NdS+P
    dam_dice: Tuple[int, int, int] = (1, 1, 0)  # NdS+P
    gold: int = 0
    sex: int = 0

    def to_dict(self) -> dict:
        return {
            'vnum': self.vnum,
            'name': self.name,
            'short_descr': self.short_descr,
            'level': self.level,
            'alignment': self.alignment,
            'hitroll': self.hitroll,
            'ac': self.ac,
            'hit_dice': list(self.hit_dice),
            'dam_dice': list(self.dam_dice),
            'gold': self.gold,
            'sex': self.sex,
            'act_flags': self.act_flags,
        }


@dataclass
class Reset:
    """Represents a reset command in an area."""
    command: str  # M, O, G, E, P, D, R
    arg1: int = 0
    arg2: int = 0
    arg3: int = 0
    arg4: int = 0  # For E resets (wear location)

    def to_dict(self) -> dict:
        result = {
            'command': self.command,
            'arg1': self.arg1,
            'arg2': self.arg2,
            'arg3': self.arg3,
        }
        if self.command == 'E':
            result['wear_loc'] = self.arg4
            result['wear_name'] = WEAR_LOCATIONS.get(self.arg4, f'unknown_{self.arg4}')
        return result


@dataclass
class Shop:
    """Represents a shop keeper configuration."""
    keeper_vnum: int
    buy_types: List[int] = field(default_factory=list)
    profit_buy: int = 100
    profit_sell: int = 100
    open_hour: int = 0
    close_hour: int = 23

    def to_dict(self) -> dict:
        return {
            'keeper_vnum': self.keeper_vnum,
            'buy_types': [ITEM_TYPES.get(t, f'type_{t}') for t in self.buy_types],
            'profit_buy': self.profit_buy,
            'profit_sell': self.profit_sell,
            'open_hour': self.open_hour,
            'close_hour': self.close_hour,
        }


@dataclass
class Area:
    """Represents a MUD area."""
    name: str
    filename: str
    lvnum: int = 0  # Low vnum
    uvnum: int = 0  # High vnum
    builders: str = ""
    security: int = 3
    recall: int = 0
    area_flags: int = 0
    rooms: Dict[int, Room] = field(default_factory=dict)
    mobiles: Dict[int, Mobile] = field(default_factory=dict)
    objects: Dict[int, Object] = field(default_factory=dict)
    resets: List[Reset] = field(default_factory=list)
    shops: List[Shop] = field(default_factory=list)
    specials: Dict[int, str] = field(default_factory=dict)  # mob_vnum -> spec_fun_name

    def to_dict(self) -> dict:
        return {
            'name': self.name,
            'filename': self.filename,
            'vnum_range': [self.lvnum, self.uvnum],
            'room_count': len(self.rooms),
            'mob_count': len(self.mobiles),
            'obj_count': len(self.objects),
            'reset_count': len(self.resets),
            'rooms': {str(v): r.to_dict() for v, r in self.rooms.items()},
            'mobiles': {str(v): m.to_dict() for v, m in self.mobiles.items()},
            'objects': {str(v): o.to_dict() for v, o in self.objects.items()},
            'resets': [r.to_dict() for r in self.resets],
            'shops': [s.to_dict() for s in self.shops],
        }
