"""
Inline stat computation for editor panels.

Ports key formulas from mudlib/area_analyzer.py to operate on raw DB dicts
(the same format the editor panels already use from repositories).
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from mudlib.models import (
    ITEM_TYPES, WEAPON_TYPES, WEAPON_SPELLS, WEAPON_AFFECTS,
    ARMOR_SPECIALS, APPLY_TYPES, ACT_FLAGS, AFF_FLAGS, decode_flags,
)


def compute_mob_stats(mob: dict) -> dict:
    """
    Compute display stats from a raw mobile dict.

    Args:
        mob: Dict with keys from MobileRepository (level, hitnodice,
             hitsizedice, hitplus, hitroll, ac, act, affected_by, etc.)

    Returns:
        Dict with computed stats for display.
    """
    level = max(mob.get('level', 1), 1)

    # Average HP: NdS+P = N*(S+1)/2 + P
    hit_n = mob.get('hitnodice', 1)
    hit_s = mob.get('hitsizedice', 1)
    hit_p = mob.get('hitplus', 0)
    avg_hp = int(hit_n * (hit_s + 1) / 2 + hit_p)

    # NPC damage is level-based (not dam_dice): avg = level per hit
    level_damage = level

    # Number of attacks per round
    num_attacks = 1
    if level >= 50: num_attacks += 1
    if level >= 100: num_attacks += 1
    if level >= 500: num_attacks += 1
    if level >= 1000: num_attacks += 1
    if level >= 1500: num_attacks += 1
    if level >= 2000: num_attacks += 2
    num_attacks += min(hit_n, 20)

    avg_damage = level_damage * num_attacks

    # Decode flags
    act = mob.get('act', 0)
    aff = mob.get('affected_by', 0)
    act_flag_list = decode_flags(act, ACT_FLAGS)
    aff_flag_list = decode_flags(aff, AFF_FLAGS)

    is_aggressive = 'aggressive' in act_flag_list
    has_sanctuary = 'sanctuary' in aff_flag_list
    is_mount = 'mount' in act_flag_list
    gives_no_exp = 'noexp' in act_flag_list or 'noexp2' in act_flag_list

    # Difficulty formula (mirrors area_analyzer.py)
    hp_score = min(avg_hp / 20, 500)
    damage_score = avg_damage / 5
    ac_val = mob.get('ac', 0)
    ac_score = min(-ac_val / 10, 100) if ac_val < 0 else -ac_val / 20
    hitroll = mob.get('hitroll', 0)
    hitroll_score = min(hitroll, 100)
    disarm_score = min(level * 0.1, 50)

    difficulty_score = hp_score + damage_score + ac_score + hitroll_score + disarm_score

    if has_sanctuary:
        difficulty_score *= 1.5
    if is_aggressive:
        difficulty_score *= 1.1
    if is_mount:
        difficulty_score *= 0.3
    if gives_no_exp:
        difficulty_score *= 0.5

    if difficulty_score < 30:
        tier = 'trivial'
    elif difficulty_score < 100:
        tier = 'easy'
    elif difficulty_score < 300:
        tier = 'normal'
    elif difficulty_score < 600:
        tier = 'hard'
    else:
        tier = 'deadly'

    return {
        'avg_hp': avg_hp,
        'avg_damage': avg_damage,
        'num_attacks': num_attacks,
        'difficulty_score': round(difficulty_score, 1),
        'difficulty_tier': tier,
        'hp_per_level': round(avg_hp / level, 1),
        'is_aggressive': is_aggressive,
        'has_sanctuary': has_sanctuary,
    }


def compute_object_stats(obj: dict, affects: list) -> dict:
    """
    Compute display stats from a raw object dict + affects list.

    Args:
        obj: Dict with keys from ObjectRepository (item_type, value0-3,
             extra_flags, wear_flags, weight, cost, etc.)
        affects: List of dicts with 'location' and 'modifier' keys.

    Returns:
        Dict with computed stats for display.
    """
    item_type = obj.get('item_type', 0)
    power_score = 0.0
    stats_summary = {}

    # Collect stats from affects
    for aff in affects:
        apply_id = aff.get('location', 0)
        apply_name = APPLY_TYPES.get(apply_id, f'unknown_{apply_id}')
        mod = aff.get('modifier', 0)
        stats_summary[apply_name] = stats_summary.get(apply_name, 0) + mod

    # Power from stats
    hitroll = stats_summary.get('hitroll', 0)
    damroll = stats_summary.get('damroll', 0)
    ac_mod = stats_summary.get('ac', 0)

    power_score += max(hitroll, 0) * 3
    power_score += max(damroll, 0) * 3

    if ac_mod < 0:
        power_score += abs(ac_mod) * 1
    else:
        power_score -= ac_mod * 0.5

    for stat_name, mod in stats_summary.items():
        if stat_name in ('hitroll', 'damroll', 'ac'):
            continue
        elif stat_name in ('str', 'dex', 'con', 'int', 'wis'):
            power_score += abs(mod) * 2
        elif stat_name == 'hit':
            power_score += abs(mod) * 0.05
        elif stat_name == 'mana':
            power_score += abs(mod) * 0.03
        elif 'save' in stat_name:
            power_score += abs(mod) * 0.5
        else:
            power_score += abs(mod) * 0.3

    result = {
        'hitroll': hitroll,
        'damroll': damroll,
        'ac_mod': ac_mod,
    }

    # Weapon-specific
    if item_type == 5:
        v0 = obj.get('value0', 0)
        v1 = obj.get('value1', 0)
        v2 = obj.get('value2', 0)
        v3 = obj.get('value3', 0)

        spell_id = v0 % 1000
        affect_id = v0 // 1000

        damage_avg = (v1 + v2) / 2 if v2 > 0 else float(v1)
        power_score += damage_avg * 1.5
        if spell_id > 0:
            power_score += 25
        if affect_id > 0:
            power_score += 35

        result['weapon_min'] = v1
        result['weapon_max'] = v2
        result['weapon_avg'] = round(damage_avg, 1)
        result['weapon_type'] = WEAPON_TYPES.get(v3, f'type_{v3}')
        result['weapon_spell'] = WEAPON_SPELLS.get(spell_id) if spell_id > 0 else None
        result['weapon_affect'] = WEAPON_AFFECTS.get(affect_id) if affect_id > 0 else None

    # Armor-specific
    elif item_type == 9:
        v0 = obj.get('value0', 0)
        v3 = obj.get('value3', 0)
        power_score += v0 * 2
        if v3 > 0:
            power_score += 50

        result['armor_ac'] = v0
        result['armor_special'] = ARMOR_SPECIALS.get(v3) if v3 > 0 else None

    # Power tier
    if power_score < 10:
        tier = 'junk'
    elif power_score < 50:
        tier = 'common'
    elif power_score < 200:
        tier = 'uncommon'
    elif power_score < 500:
        tier = 'rare'
    elif power_score < 1000:
        tier = 'epic'
    else:
        tier = 'legendary'

    result['power_score'] = round(power_score, 1)
    result['power_tier'] = tier

    return result
