"""
Data models for Dystopia MUD area data.

Contains data classes for all game entities and constants matching merc.h.
"""

from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple, Any

# Direction constants matching merc.h
DIR_NORTH = 0
DIR_EAST = 1
DIR_SOUTH = 2
DIR_WEST = 3
DIR_UP = 4
DIR_DOWN = 5

DIR_NAMES = ['north', 'east', 'south', 'west', 'up', 'down']
REVERSE_DIR = [2, 3, 0, 1, 5, 4]  # N<->S, E<->W, U<->D

# Direction offsets for coordinate assignment
DIR_OFFSETS = {
    DIR_NORTH: (0, 1, 0),
    DIR_EAST: (1, 0, 0),
    DIR_SOUTH: (0, -1, 0),
    DIR_WEST: (-1, 0, 0),
    DIR_UP: (0, 0, 1),
    DIR_DOWN: (0, 0, -1),
}

# Character classes from class.h (bitfield values)
CLASS_TABLE = {
    0: 'None',
    1: 'Demon',
    2: 'Mage',
    4: 'Werewolf',
    8: 'Vampire',
    16: 'Samurai',
    32: 'Drow',
    64: 'Monk',
    128: 'Ninja',
    256: 'Lich',
    512: 'Shapeshifter',
    1024: 'Tanarri',
    2048: 'Angel',
    4096: 'Undead Knight',
    8192: 'Spider Droid',
    16384: 'Dirgesinger',
    32768: 'Siren',
    65536: 'Psion',
    131072: 'Mindflayer',
}

# Sector types from merc.h
SECTOR_NAMES = {
    0: 'inside',
    1: 'city',
    2: 'field',
    3: 'forest',
    4: 'hills',
    5: 'mountain',
    6: 'water_swim',
    7: 'water_noswim',
    9: 'air',
    10: 'desert',
}

# Item types from merc.h
ITEM_TYPES = {
    1: 'light',
    2: 'scroll',
    3: 'wand',
    4: 'staff',
    5: 'weapon',
    8: 'treasure',
    9: 'armor',
    10: 'potion',
    12: 'furniture',
    13: 'trash',
    15: 'container',
    17: 'drink',
    18: 'key',
    19: 'food',
    20: 'money',
    22: 'boat',
    23: 'corpse_npc',
    24: 'corpse_pc',
    25: 'fountain',
    26: 'pill',
    27: 'portal',
    28: 'egg',
    29: 'voodoo',
    30: 'stake',
    31: 'missile',
    32: 'ammo',
    33: 'quest',
    34: 'questcard',
    35: 'questmachine',
    36: 'symbol',
    37: 'book',
    38: 'page',
    39: 'tool',
    40: 'wall',
    41: 'copper',
    42: 'iron',
    43: 'steel',
    44: 'adamantite',
    45: 'gemstone',
    46: 'hilt',
    47: 'dtoken',
    48: 'head',
    50: 'cookingpot',
    52: 'wgate',
    53: 'instrument',
}

# Weapon types from merc.h (value[3] for weapons)
WEAPON_TYPES = {
    0: 'hit',
    1: 'slice',
    2: 'stab',
    3: 'slash',
    4: 'whip',
    5: 'claw',
    6: 'blast',
    7: 'pound',
    8: 'crush',
    9: 'grep',
    10: 'bite',
    11: 'pierce',
    12: 'suck',
}

# Weapon spell effects (value[0] % 1000 for spell weapons)
# These are skill_table[] array indices from const.c (NOT SLOT numbers!)
# The spell triggers on hit against the victim
WEAPON_SPELLS = {
    0: None,                # reserved
    1: 'acid_blast',
    2: 'armor',
    3: 'bless',
    4: 'blindness',
    5: 'burning_hands',
    6: 'call_lightning',
    7: 'cause_critical',
    8: 'cause_light',
    9: 'cause_serious',
    10: 'change_sex',
    11: 'charm_person',
    12: 'chill_touch',
    13: 'colour_spray',
    14: 'continual_light',
    15: 'control_weather',
    16: 'create_food',
    17: 'create_spring',
    18: 'create_water',
    19: 'cure_blindness',
    20: 'cure_critical',
    21: 'cure_light',
    22: 'cure_poison',
    23: 'cure_serious',
    24: 'curse',
    25: 'detect_evil',
    26: 'detect_hidden',
    27: 'detect_invis',
    28: 'detect_magic',
    29: 'detect_poison',
    30: 'dispel_evil',
    31: 'dispel_magic',
    32: 'earthquake',
    33: 'enchant_weapon',
    34: 'energy_drain',
    35: 'faerie_fire',
    36: 'faerie_fog',
    37: 'fireball',
    38: 'flamestrike',
    39: 'fly',
    40: 'gate',
    41: 'giant_strength',
    42: 'harm',
    43: 'heal',
    44: 'identify',
    45: 'infravision',
    46: 'invis',
    47: 'know_alignment',
    48: 'lightning_bolt',
    49: 'locate_object',
    50: 'magic_missile',
    51: 'mass_invis',
    52: 'pass_door',
    53: 'poison',
    54: 'protection',
    55: 'readaura',
    56: 'refresh',
    57: 'remove_curse',
    58: 'sanctuary',
    59: 'shield',
    60: 'shocking_grasp',
    61: 'sleep',
    62: 'stone_skin',
    63: 'summon',
    64: 'teleport',
    65: 'ventriloquate',
    66: 'weaken',
    67: 'word_of_recall',
    68: 'acid_breath',
    69: 'fire_breath',
    70: 'frost_breath',
    71: 'gas_breath',
    72: 'lightning_breath',
}

# Weapon wielder affects (value[0] / 1000 for spell weapons)
# These provide passive effects when wielded
WEAPON_AFFECTS = {
    0: None,
    1: 'darkness_aura',     # Radiates an aura of darkness (also: 4)
    2: 'see_invis',         # See invisible things (also: 27)
    3: 'flight',            # Power of flight (also: 39)
    4: 'infravision',       # See in the dark (also: 45)
    5: 'invisibility',      # Render wielder invisible (also: 46)
    6: 'pass_door',         # Walk through solid doors (also: 52)
    7: 'protect_evil',      # Protection from evil (also: 54)
    8: 'sanctuary',         # Protection in combat (also: 57)
    9: 'sneak',             # Walk in complete silence
    10: 'shock_shield',     # Shield of lightning
    11: 'fire_shield',      # Shield of fire
    12: 'ice_shield',       # Shield of ice
    13: 'acid_shield',      # Shield of acid
    14: 'god_power',        # Channel power of god
    15: 'chaos_shield',     # Shield of chaos
    16: 'regeneration',     # Regenerate wounds
    17: 'haste',            # Supernatural speed
    18: 'armor_pierce',     # Slice through armour
    19: 'player_protect',   # Protection from player attacks
    20: 'darkness_shield',  # Shield of darkness
    21: 'superior_protect', # Superior protection
    22: 'truesight',        # Supernatural vision
    23: 'fleet_foot',       # Fleet-footed
    24: 'concealment',      # Conceal from sight
    25: 'beast_power',      # Invoke power of the beast (rage weapon)
    27: 'detect_invis',     # See invisible (alternate)
    39: 'fly',              # Flight (alternate)
    45: 'dark_vision',      # See in the dark (alternate)
    46: 'invis',            # Invisibility (alternate)
    52: 'phase',            # Pass door (alternate)
    54: 'holy_protect',     # Protection from evil (alternate)
    57: 'combat_protect',   # Combat protection (alternate)
}

# Armor special types (value[3] for armor)
# These provide magical effects when worn
ARMOR_SPECIALS = {
    0: None,
    # Gemstone enchantments (via forge command)
    1: 'darkness_aura',        # Radiates an aura of darkness (also: infravision at 45)
    2: 'see_invis',            # Allows wearer to see invisible (also: lazuli, 27)
    3: 'flight',               # Grants power of flight (also: amethyst, 39)
    4: 'infravision',          # Allows wearer to see in the dark (also: 45)
    5: 'invisibility',         # Renders wearer invisible (also: pearl, 46)
    6: 'pass_door',            # Walk through solid doors (also: opal, 52)
    7: 'protect_evil',         # Protects wearer from evil (also: 54)
    8: 'sanctuary',            # Protects wearer in combat (diamond, also: 57)
    9: 'sneak',                # Walk in complete silence (onyx)
    10: 'shock_shield',        # Shield of lightning (topaz)
    11: 'fire_shield',         # Shield of fire (ruby)
    12: 'ice_shield',          # Shield of ice (sapphire)
    13: 'acid_shield',         # Shield of acid (emerald)
    14: 'god_power',           # Protection from DarkBlade clan guardians
    15: 'chaos_shield',        # Shield of chaos
    16: 'regeneration',        # Regenerates wounds
    17: 'haste',               # Supernatural speed
    18: 'armor_pierce',        # Slice through armour
    19: 'player_protect',      # Protection from player attacks
    20: 'darkness_shield',     # Shield of darkness
    21: 'superior_protect',    # Superior protection
    22: 'truesight',           # Supernatural vision
    23: 'fleet_foot',          # Fleet-footed
    24: 'concealment',         # Conceals from sight
    25: 'beast_power',         # Invokes power of beast
    27: 'detect_invis',        # See invisible (alternate)
    28: 'ancient_magic',       # Power of ancient magic
    29: 'third_eye',           # Power of the third eye
    30: 'talons',              # Power of talons
    39: 'fly',                 # Flight (alternate)
    45: 'dark_vision',         # See in the dark (alternate)
    46: 'invis',               # Invisibility (alternate)
    52: 'phase',               # Pass door (alternate)
    54: 'holy_protect',        # Protection from evil (alternate)
    57: 'combat_protect',      # Combat protection (alternate)
    88: 'unknown_88',          # Unknown effect
    139: 'protect_good',       # Protection from good (unholy relic)
}

# Wear locations
WEAR_LOCATIONS = {
    0: 'light',
    1: 'finger_l',
    2: 'finger_r',
    3: 'neck_1',
    4: 'neck_2',
    5: 'body',
    6: 'head',
    7: 'legs',
    8: 'feet',
    9: 'hands',
    10: 'arms',
    11: 'shield',
    12: 'about',
    13: 'waist',
    14: 'wrist_l',
    15: 'wrist_r',
    16: 'wield',
    17: 'hold',
    18: 'face',
}

# Act flags for mobiles (bitfield)
ACT_FLAGS = {
    1: 'npc',           # A - Auto set for mobs
    2: 'sentinel',      # B - Stays in one room
    4: 'scavenger',     # C - Picks up objects
    8: 'aggressive',    # D - Attacks PCs
    16: 'stay_area',    # E - Won't leave area
    32: 'wimpy',        # F - Flees when hurt
    64: 'pet',          # G - Auto set for pets
    128: 'train',       # H - Can train PCs
    256: 'practice',    # I - Can practice PCs
    512: 'mount',       # J - Can be mounted
    1024: 'noparts',    # K - Dead = no body parts
    2048: 'noexp',      # L - No exp for killing
    4096: 'prototype',  # M
    8192: 'noautokill', # N
    16384: 'noexp2',    # O
}

# Affected_by flags for mobiles (bitfield)
AFF_FLAGS = {
    1: 'blind',
    2: 'invisible',
    4: 'detect_evil',
    8: 'detect_invis',
    16: 'detect_magic',
    32: 'detect_hidden',
    64: 'shadowplane',
    128: 'sanctuary',
    256: 'faerie_fire',
    512: 'infrared',
    1024: 'curse',
    2048: 'flaming',
    4096: 'poison',
    8192: 'protect',
    16384: 'ethereal',
    32768: 'sneak',
    65536: 'hide',
    131072: 'sleep',
    262144: 'charm',
    524288: 'flying',
    1048576: 'pass_door',
    2097152: 'polymorph',
    4194304: 'shadowsight',
    8388608: 'webbed',
    16777216: 'protect_good',
    33554432: 'drowfire',
    67108864: 'zuloform',
    134217728: 'shift',
    268435456: 'peace',
    536870912: 'infirmity',
}

# =============================================================================
# Player Character Constants (from merc.h)
# =============================================================================

# Discipline/Power array indices (power[MAX_DISCIPLINES] = power[44])
# Used by vampires, werewolves, demons, and other classes
DISCIPLINE_NAMES = {
    # Vampire disciplines (2-16)
    2: ('DISC_VAMP_CELE', 'Celerity'),
    3: ('DISC_VAMP_FORT', 'Fortitude'),
    4: ('DISC_VAMP_OBTE', 'Obtenebration'),
    5: ('DISC_VAMP_PRES', 'Presence'),
    6: ('DISC_VAMP_QUIE', 'Quietus'),
    7: ('DISC_VAMP_THAU', 'Thaumaturgy'),
    8: ('DISC_VAMP_AUSP', 'Auspex'),
    9: ('DISC_VAMP_DOMI', 'Dominate'),
    10: ('DISC_VAMP_OBFU', 'Obfuscate'),
    11: ('DISC_VAMP_POTE', 'Potence'),
    12: ('DISC_VAMP_PROT', 'Protean'),
    13: ('DISC_VAMP_SERP', 'Serpentis'),
    14: ('DISC_VAMP_VICI', 'Vicissitude'),
    15: ('DISC_VAMP_DAIM', 'Daimoinon'),
    16: ('DISC_VAMP_ANIM', 'Animalism'),
    # Werewolf totem spirits (18-29)
    18: ('DISC_WERE_BEAR', 'Bear'),
    19: ('DISC_WERE_LYNX', 'Lynx'),
    20: ('DISC_WERE_BOAR', 'Boar'),
    21: ('DISC_WERE_OWL', 'Owl'),
    22: ('DISC_WERE_SPID', 'Spider'),
    23: ('DISC_WERE_WOLF', 'Wolf'),
    24: ('DISC_WERE_HAWK', 'Hawk'),
    25: ('DISC_WERE_MANT', 'Mantis'),
    26: ('DISC_WERE_RAPT', 'Raptor'),
    27: ('DISC_WERE_LUNA', 'Luna'),
    28: ('DISC_WERE_PAIN', 'Pain'),
    29: ('DISC_WERE_CONG', 'Congregation'),
    # Demon disciplines (30-38)
    30: ('DISC_DAEM_HELL', 'Hellfire'),
    31: ('DISC_DAEM_ATTA', 'Attacks'),
    32: ('DISC_DAEM_TEMP', 'Tempering'),
    33: ('DISC_DAEM_MORP', 'Morphology'),
    34: ('DISC_DAEM_CORR', 'Corruption'),
    35: ('DISC_DAEM_GELU', 'Gelupathy'),
    36: ('DISC_DAEM_DISC', 'Discord'),
    37: ('DISC_DAEM_NETH', 'Nether'),
    38: ('DISC_DAEM_IMMU', 'Immunity'),
    # Additional vampire disciplines (39-43)
    39: ('DISC_VAMP_CHIM', 'Chimerstry'),
    40: ('DISC_VAMP_THAN', 'Thanatosis'),
    41: ('DISC_VAMP_OBEA', 'Obeah'),
    42: ('DISC_VAMP_NECR', 'Necromancy'),
    43: ('DISC_VAMP_MELP', 'Melpominee'),
}

# Player flags (stored in 'act' field for players, different from mob ACT_*)
PLR_FLAGS = {
    1: ('PLR_IS_NPC', 'Is NPC (never set)'),
    2: ('PLR_BRIEF3', 'Brief 3'),
    4: ('PLR_LEFTHAND', 'Left Handed'),
    8: ('PLR_AUTOEXIT', 'Auto Exit'),
    16: ('PLR_AUTOLOOT', 'Auto Loot'),
    32: ('PLR_AUTOSAC', 'Auto Sacrifice'),
    64: ('PLR_BLANK', 'Blank Line'),
    128: ('PLR_BRIEF', 'Brief Mode'),
    256: ('PLR_SCREENREADER', 'Screen Reader'),
    512: ('PLR_COMBINE', 'Combine Items'),
    1024: ('PLR_PROMPT', 'Show Prompt'),
    2048: ('PLR_TELNET_GA', 'Telnet GA'),
    4096: ('PLR_HOLYLIGHT', 'Holy Light'),
    8192: ('PLR_WIZINVIS', 'Wizard Invis'),
    16384: ('PLR_ANSI', 'ANSI Color'),
    32768: ('PLR_SILENCE', 'Silenced'),
    65536: ('PLR_VT102', 'VT102 Mode'),
    131072: ('PLR_RIGHTHAND', 'Right Handed'),
    262144: ('PLR_NO_TELL', 'No Tell'),
    524288: ('PLR_LOG', 'Logged'),
    1048576: ('PLR_DENY', 'Denied'),
    2097152: ('PLR_FREEZE', 'Frozen'),
    4194304: ('PLR_PREFER_MCMP', 'Prefer MCMP'),
    8388608: ('PLR_AUTOMAP', 'Auto Map'),
    16777216: ('PLR_BRIEF2', 'Brief 2'),
    33554432: ('PLR_WATCHER', 'Watcher'),
    67108864: ('PLR_ACID', 'Acid'),
    134217728: ('PLR_BRIEF4', 'Brief 4'),
    268435456: ('PLR_AMBI', 'Ambidextrous'),
    536870912: ('PLR_XTERM', 'XTerm-256'),
    1073741824: ('PLR_PREFER_GMCP', 'Prefer GMCP'),
    2147483648: ('PLR_PREFER_MXP', 'Prefer MXP'),
}

# Extra flags (stored in 'extra' field)
EXTRA_FLAGS_PLAYER = {
    4: ('EXTRA_TRUSTED', 'Trusted'),
    8: ('EXTRA_NEWPASS', 'New Password'),
    16: ('EXTRA_OSWITCH', 'Object Switch'),
    32: ('EXTRA_SWITCH', 'Switched'),
    64: ('EXTRA_FAKE_CON', 'Fake Con'),
    1024: ('EXTRA_STANCE', 'Stance'),
    2048: ('EXTRA_DONE', 'Done'),
    4096: ('EXTRA_EXP', 'Exp'),
    8192: ('EXTRA_PREGNANT', 'Pregnant'),
    16384: ('EXTRA_LABOUR', 'In Labour'),
    32768: ('EXTRA_BORN', 'Born'),
    65536: ('EXTRA_PROMPT', 'Prompt'),
    131072: ('EXTRA_MARRIED', 'Married'),
    262144: ('EXTRA_AFK', 'AFK'),
    524288: ('EXTRA_DRAGON', 'Dragon'),
    1048576: ('EXTRA_CALL_ALL', 'Call All'),
    4194304: ('EXTRA_BSD', 'BSD'),
    8388608: ('EXTRA_EARTHMELD', 'Earth Meld'),
    16777216: ('EXTRA_PLASMA', 'Plasma'),
    67108864: ('EXTRA_AWE', 'Awe'),
    134217728: ('EXTRA_ROT', 'Rot'),
    268435456: ('EXTRA_ZOMBIE', 'Zombie'),
    536870912: ('EXTRA_BAAL', 'Baal'),
    1073741824: ('EXTRA_FLASH', 'Flash'),
}

# Newbits flags (stored in 'newbits' field)
NEWBITS_FLAGS = {
    1: ('NEW_SLAM', 'Slam'),
    2: ('NEW_QUILLS', 'Quills'),
    4: ('NEW_JAWLOCK', 'Jawlock'),
    8: ('NEW_PERCEPTION', 'Perception'),
    16: ('NEW_SKIN', 'Skin'),
    32: ('NEW_TIDE', 'Tide'),
    64: ('NEW_COIL', 'Coil'),
    128: ('NEW_REND', 'Rend'),
    256: ('NEW_MONKFLAME', 'Monk Flame'),
    512: ('NEW_SCLAWS', 'Shadow Claws'),
    1024: ('NEW_IRONMIND', 'Iron Mind'),
    2048: ('NEW_MONKCLOAK', 'Monk Cloak'),
    4096: ('NEW_MONKADAM', 'Monk Adamantine'),
    8192: ('NEW_MONKSKIN', 'Monk Skin'),
    16384: ('NEW_MONKFAVOR', 'Monk Favor'),
    32768: ('NEW_CLOAK', 'Cloak'),
    65536: ('NEW_DROWHATE', 'Drow Hate'),
    131072: ('NEW_DARKNESS', 'Darkness'),
    1048576: ('NEW_MENTALBLOCK', 'Mental Block'),
    2097152: ('NEW_VISION', 'Vision'),
    4194304: ('NEW_NATURAL', 'Natural'),
    8388608: ('NEW_POWER', 'Power'),
    16777216: ('NEW_DFORM', 'Demon Form'),
    33554432: ('NEW_MASTERY', 'Mastery'),
    67108864: ('NEW_DARKTENDRILS', 'Dark Tendrils'),
    134217728: ('NEW_MULTIARMS', 'Multi Arms'),
    268435456: ('NEW_BLADESPIN', 'Blade Spin'),
    536870912: ('NEW_FIGHTDANCE', 'Fight Dance'),
    1073741824: ('NEW_CUBEFORM', 'Cube Form'),
}

# Immunity flags (stored in 'immune' field)
IMM_FLAGS = {
    1: ('IMM_SLASH', 'Slash'),
    2: ('IMM_STAB', 'Stab'),
    4: ('IMM_SMASH', 'Smash'),
    8: ('IMM_ANIMAL', 'Animal'),
    16: ('IMM_MISC', 'Misc'),
    32: ('IMM_CHARM', 'Charm'),
    64: ('IMM_HEAT', 'Heat'),
    128: ('IMM_COLD', 'Cold'),
    256: ('IMM_LIGHTNING', 'Lightning'),
    512: ('IMM_ACID', 'Acid'),
    1024: ('IMM_SUMMON', 'Summon'),
    2048: ('IMM_VOODOO', 'Voodoo'),
    4096: ('IMM_VAMPIRE', 'Vampire'),
    8192: ('IMM_STAKE', 'Stake'),
    16384: ('IMM_SUNLIGHT', 'Sunlight'),
    32768: ('IMM_SHIELDED', 'Shielded'),
    65536: ('IMM_HURL', 'Hurl'),
    131072: ('IMM_BACKSTAB', 'Backstab'),
    262144: ('IMM_KICK', 'Kick'),
    524288: ('IMM_DISARM', 'Disarm'),
    1048576: ('IMM_STEAL', 'Steal'),
    2097152: ('IMM_SLEEP', 'Sleep'),
    4194304: ('IMM_DRAIN', 'Drain'),
    8388608: ('IMM_SHIELD2', 'Chaotic Shield'),
    16777216: ('IMM_TRANSPORT', 'Transport'),
    33554432: ('IMM_TRAVEL', 'Travel'),
}

# Werewolf gifts array indices (gifts[21])
GIFT_NAMES = {
    0: 'Gift 0',
    1: 'Gift 1',
    2: 'Gift 2',
    3: 'Gift 3',
    4: 'Gift 4',
    5: 'Gift 5',
    6: 'Gift 6',
    7: 'Gift 7',
    8: 'Gift 8',
    9: 'Gift 9',
    10: 'Gift 10',
    11: 'Gift 11',
    12: 'Gift 12',
    13: 'Gift 13',
    14: 'Gift 14',
    15: 'Gift 15',
    16: 'Gift 16',
    17: 'Gift 17',
    18: 'Gift 18',
    19: 'Gift 19',
    20: 'Gift 20',
}


def decode_flags(value: int, flag_dict: dict) -> List[str]:
    """Decode a bitfield into a list of flag names."""
    flags = []
    for bit, name in flag_dict.items():
        if value & bit:
            flags.append(name)
    return flags


# Extra flags for objects (bitfield) - from merc.h
EXTRA_FLAGS = {
    1: 'glow',           # Item glows
    2: 'hum',            # Item hums
    4: 'thrown',         # Throwable weapon
    8: 'keep',           # Cannot be dropped
    16: 'vanish',        # Vanishes on drop
    32: 'invis',         # Invisible
    64: 'magic',         # Magical item
    128: 'nodrop',       # Cannot be dropped
    256: 'bless',        # Blessed
    512: 'anti_good',    # Cannot be used by good
    1024: 'anti_evil',   # Cannot be used by evil
    2048: 'anti_neutral', # Cannot be used by neutral
    4096: 'noremove',    # Cannot be removed once worn
    8192: 'inventory',   # Stays in inventory
    16384: 'loyal',      # Loyal to owner
    32768: 'shadowplane', # Exists in shadow plane
    65536: 'menchant',   # Merchant item
}

# Wear flags for objects (bitfield) - from merc.h
# Bit 0 (value 1) = ITEM_TAKE = can be picked up
WEAR_FLAGS = {
    1: 'take',           # Can be picked up
    2: 'wear_finger',    # Worn on finger
    4: 'wear_neck',      # Worn on neck
    8: 'wear_body',      # Worn on body
    16: 'wear_head',     # Worn on head
    32: 'wear_legs',     # Worn on legs
    64: 'wear_feet',     # Worn on feet
    128: 'wear_hands',   # Worn on hands
    256: 'wear_arms',    # Worn on arms
    512: 'wear_shield',  # Used as shield
    1024: 'wear_about',  # Worn about body
    2048: 'wear_waist',  # Worn on waist
    4096: 'wear_wrist',  # Worn on wrist
    8192: 'wield',       # Can be wielded
    16384: 'hold',       # Can be held
    32768: 'wear_face',  # Worn on face
    65536: 'wear_special', # Special wear slot
    131072: 'wear_bodyart', # Body art
    262144: 'wear_medal', # Medal slot
    524288: 'wear_float', # Floating item
}

# Apply types for object affects - from merc.h
APPLY_TYPES = {
    0: 'none',
    1: 'str',
    2: 'dex',
    3: 'int',
    4: 'wis',
    5: 'con',
    6: 'sex',
    7: 'class',
    8: 'level',
    9: 'age',
    10: 'height',
    11: 'weight',
    12: 'mana',
    13: 'hit',
    14: 'move',
    15: 'gold',
    16: 'exp',
    17: 'ac',
    18: 'hitroll',
    19: 'damroll',
    20: 'save_para',
    21: 'save_rod',
    22: 'save_petri',
    23: 'save_breath',
    24: 'save_spell',
    25: 'poly',
}


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
