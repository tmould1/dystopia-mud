"""
Area analyzer for Dystopia MUD.

Calculates mob difficulty ratings, object power ratings, and analyzes resets.
"""

from dataclasses import dataclass
from typing import Dict, List, Any, Optional

from .models import Area, Mobile, Object, Reset, ITEM_TYPES, APPLY_TYPES, WEAR_LOCATIONS, WEAPON_TYPES, ARMOR_SPECIALS, WEAPON_SPELLS, WEAPON_AFFECTS, ACT_FLAGS, AFF_FLAGS, EXTRA_FLAGS, WEAR_FLAGS, decode_flags


@dataclass
class MobAnalysis:
    """Analysis results for a mobile."""
    vnum: int
    name: str
    level: int
    difficulty_score: float
    difficulty_tier: str  # trivial, easy, normal, hard, deadly
    avg_hp: int
    avg_damage: int
    hitroll: int
    ac: int
    gold: int
    alignment: int
    spawn_count: int  # How many spawn from resets
    spawn_rooms: List[int]  # Rooms where it spawns
    # Decoded flags
    act_flags: List[str] = None
    aff_flags: List[str] = None
    # Combat effectiveness metrics
    hp_per_level: float = 0.0
    damage_per_level: float = 0.0
    is_aggressive: bool = False
    has_sanctuary: bool = False
    is_mount: bool = False
    gives_no_exp: bool = False

    def to_dict(self) -> dict:
        return {
            'vnum': self.vnum,
            'name': self.name,
            'level': self.level,
            'difficulty_score': round(self.difficulty_score, 1),
            'difficulty_tier': self.difficulty_tier,
            'avg_hp': self.avg_hp,
            'avg_damage': self.avg_damage,
            'hitroll': self.hitroll,
            'ac': self.ac,
            'gold': self.gold,
            'alignment': self.alignment,
            'spawn_count': self.spawn_count,
            'spawn_rooms': self.spawn_rooms,
            'act_flags': self.act_flags or [],
            'aff_flags': self.aff_flags or [],
            'hp_per_level': round(self.hp_per_level, 1),
            'damage_per_level': round(self.damage_per_level, 1),
            'is_aggressive': self.is_aggressive,
            'has_sanctuary': self.has_sanctuary,
            'is_mount': self.is_mount,
            'gives_no_exp': self.gives_no_exp,
        }


@dataclass
class ObjectAnalysis:
    """Analysis results for an object."""
    vnum: int
    name: str
    item_type: str
    power_score: float
    power_tier: str  # junk, common, uncommon, rare, epic, legendary
    stats_summary: Dict[str, int]
    wear_location: Optional[str]
    spawn_count: int
    spawn_locations: List[Dict[str, Any]]  # room/mob/container info
    # Weapon-specific fields
    weapon_damage_min: int = 0
    weapon_damage_max: int = 0
    weapon_damage_avg: float = 0.0
    weapon_type: Optional[str] = None
    is_spell_weapon: bool = False
    weapon_spell: Optional[str] = None      # On-hit spell effect
    weapon_affect: Optional[str] = None     # Passive wielder affect
    # Armor-specific fields
    armor_ac: int = 0
    armor_special: Optional[str] = None
    # Common fields
    weight: int = 0
    cost: int = 0
    level: int = 0
    # Key combat stats (extracted for easy display)
    hitroll: int = 0
    damroll: int = 0
    ac_mod: int = 0  # AC modifier from affects (negative = better)
    # Object flags for query filtering
    extra_flags: List[str] = None  # Decoded extra flags (nodrop, noremove, etc.)
    can_take: bool = True          # Can be picked up (ITEM_TAKE flag)
    can_drop: bool = True          # Can be dropped (not NODROP)

    def to_dict(self) -> dict:
        result = {
            'vnum': self.vnum,
            'name': self.name,
            'item_type': self.item_type,
            'power_score': round(self.power_score, 1),
            'power_tier': self.power_tier,
            'stats_summary': self.stats_summary,
            'wear_location': self.wear_location,
            'spawn_count': self.spawn_count,
            'spawn_locations': self.spawn_locations,
            'weight': self.weight,
            'cost': self.cost,
            'level': self.level,
            'hitroll': self.hitroll,
            'damroll': self.damroll,
            'ac_mod': self.ac_mod,
            'extra_flags': self.extra_flags or [],
            'can_take': self.can_take,
            'can_drop': self.can_drop,
        }
        # Add weapon fields if relevant
        if self.item_type == 'weapon':
            result['weapon'] = {
                'damage_min': self.weapon_damage_min,
                'damage_max': self.weapon_damage_max,
                'damage_avg': round(self.weapon_damage_avg, 1),
                'weapon_type': self.weapon_type,
                'is_spell_weapon': self.is_spell_weapon,
                'spell': self.weapon_spell,
                'affect': self.weapon_affect,
            }
        # Add armor fields if relevant
        if self.item_type == 'armor':
            result['armor'] = {
                'ac': self.armor_ac,
                'special': self.armor_special,
            }
        return result


@dataclass
class AreaAnalysis:
    """Complete analysis of an area."""
    name: str
    filename: str
    mob_count: int
    object_count: int
    room_count: int
    avg_mob_level: float
    avg_difficulty: float
    difficulty_distribution: Dict[str, int]
    avg_object_power: float
    power_distribution: Dict[str, int]
    total_gold_per_reset: int
    shops: List[Dict[str, Any]]

    def to_dict(self) -> dict:
        return {
            'name': self.name,
            'filename': self.filename,
            'mob_count': self.mob_count,
            'object_count': self.object_count,
            'room_count': self.room_count,
            'avg_mob_level': round(self.avg_mob_level, 1),
            'avg_difficulty': round(self.avg_difficulty, 1),
            'difficulty_distribution': self.difficulty_distribution,
            'avg_object_power': round(self.avg_object_power, 1),
            'power_distribution': self.power_distribution,
            'total_gold_per_reset': self.total_gold_per_reset,
            'shops': self.shops,
        }


def calculate_mob_difficulty(mob: Mobile) -> MobAnalysis:
    """
    Calculate difficulty rating for a mobile.

    Based on actual game mechanics from fight.c:
    - NPC damage = level/2 to level*1.5 per hit (not dam_dice!)
    - NPC attacks = 1 + level bonuses + hit_dice_num (the N in NdS+P)
    - Disarm/trip chance = level * 0.5%
    - Hitroll directly affects to-hit
    - AC reduces incoming damage

    Difficulty is based on:
    - Hit points (survivability)
    - Level-based damage output
    - Number of attacks (from level + hit dice)
    - AC (defense)
    - Hitroll (accuracy)
    - Special abilities (sanctuary, etc.)
    """
    # Calculate average HP: NdS+P = N*(S+1)/2 + P
    hit_num, hit_size, hit_plus = mob.hit_dice
    avg_hp = int(hit_num * (hit_size + 1) / 2 + hit_plus)

    # NPC damage is level-based, not from dam_dice!
    # dam = number_range(level/2, level*3/2) per hit
    # Average = (level/2 + level*1.5) / 2 = level
    level_damage = mob.level  # Average damage per hit

    # Calculate number of attacks per round
    # Base 1, plus level bonuses, plus hit_dice_num (capped at 20)
    num_attacks = 1
    if mob.level >= 50: num_attacks += 1
    if mob.level >= 100: num_attacks += 1
    if mob.level >= 500: num_attacks += 1
    if mob.level >= 1000: num_attacks += 1
    if mob.level >= 1500: num_attacks += 1
    if mob.level >= 2000: num_attacks += 2
    num_attacks += min(hit_num, 20)  # hit_dice_num contributes to attacks

    # Total damage per round = attacks * damage per hit
    avg_damage = level_damage * num_attacks

    # Decode flags
    act_flag_list = decode_flags(mob.act_flags, ACT_FLAGS)
    aff_flag_list = decode_flags(mob.affected_by, AFF_FLAGS)

    # Check special flags
    is_aggressive = 'aggressive' in act_flag_list
    has_sanctuary = 'sanctuary' in aff_flag_list
    is_mount = 'mount' in act_flag_list
    gives_no_exp = 'noexp' in act_flag_list or 'noexp2' in act_flag_list

    # Calculate per-level metrics (avoid division by zero)
    level = max(mob.level, 1)
    hp_per_level = avg_hp / level
    damage_per_level = avg_damage / level

    # DIFFICULTY FORMULA based on actual combat mechanics
    # HP contribution (survivability)
    hp_score = min(avg_hp / 20, 500)  # 10000 HP = 500 points max

    # Damage contribution = attacks * level-based damage
    # Typical: level 100 with 5 attacks = 500 damage/round = 100 points
    damage_score = avg_damage / 5

    # AC contribution (lower AC = harder, typical range 0 to -1000)
    if mob.ac < 0:
        ac_score = min(-mob.ac / 10, 100)  # -1000 AC = 100 points max
    else:
        ac_score = -mob.ac / 20  # Positive AC = penalty (easier to hit)

    # Hitroll contribution - higher hitroll = more accurate
    hitroll_score = min(mob.hitroll, 100)  # Cap at 100 points

    # Disarm/trip threat (level * 0.5% chance per attack)
    # Higher level mobs are more disruptive
    disarm_score = min(mob.level * 0.1, 50)

    # Calculate base difficulty
    difficulty_score = hp_score + damage_score + ac_score + hitroll_score + disarm_score

    # Apply modifiers for special abilities
    if has_sanctuary:
        difficulty_score *= 1.5  # 50% harder with sanctuary (halves damage taken)
    if is_aggressive:
        difficulty_score *= 1.1  # 10% harder if aggressive

    # Reduce difficulty for non-combat mobs
    if is_mount:
        difficulty_score *= 0.3  # Mounts are not meant to be fought
    if gives_no_exp:
        difficulty_score *= 0.5  # No-exp mobs are usually not real challenges

    # Determine tier based on scale
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

    return MobAnalysis(
        vnum=mob.vnum,
        name=mob.short_descr,
        level=mob.level,
        difficulty_score=difficulty_score,
        difficulty_tier=tier,
        avg_hp=avg_hp,
        avg_damage=avg_damage,
        hitroll=mob.hitroll,
        ac=mob.ac,
        gold=mob.gold,
        alignment=mob.alignment,
        spawn_count=0,
        spawn_rooms=[],
        act_flags=act_flag_list,
        aff_flags=aff_flag_list,
        hp_per_level=hp_per_level,
        damage_per_level=damage_per_level,
        is_aggressive=is_aggressive,
        has_sanctuary=has_sanctuary,
        is_mount=is_mount,
        gives_no_exp=gives_no_exp,
    )


def calculate_object_power(obj: Object) -> ObjectAnalysis:
    """
    Calculate power rating for an object.

    Power is based on actual combat effectiveness:
    - Hitroll/damroll affects (primary combat stats)
    - AC affects (negative = better, reduces incoming damage)
    - Base armor AC value
    - Special armor effects (truesight, etc.)
    - Weapon damage ranges
    - Spell weapon effects

    The formula normalizes values so similar items get similar scores.
    """
    power_score = 0.0
    stats_summary: Dict[str, int] = {}

    item_type_name = ITEM_TYPES.get(obj.item_type, 'unknown')

    # Initialize weapon/armor specific fields
    weapon_damage_min = 0
    weapon_damage_max = 0
    weapon_damage_avg = 0.0
    weapon_type = None
    is_spell_weapon = False
    weapon_spell = None
    weapon_affect = None
    armor_ac = 0
    armor_special = None

    # Process affects - collect stats first
    for affect in obj.affects:
        apply_name = APPLY_TYPES.get(affect.apply_type, f'unknown_{affect.apply_type}')
        mod = affect.modifier

        if apply_name in stats_summary:
            stats_summary[apply_name] += mod
        else:
            stats_summary[apply_name] = mod

    # Calculate power from collected stats
    # Combat stats: hitroll and damroll are primary
    hitroll = stats_summary.get('hitroll', 0)
    damroll = stats_summary.get('damroll', 0)
    ac_mod = stats_summary.get('ac', 0)

    # Hitroll/damroll: direct combat effectiveness
    # Use actual value, not abs - negative would be a penalty
    power_score += max(hitroll, 0) * 3
    power_score += max(damroll, 0) * 3

    # AC modifier: NEGATIVE is better (harder to hit)
    # -100 AC is very good, +500 AC is terrible
    if ac_mod < 0:
        power_score += abs(ac_mod) * 1  # Reward negative AC
    else:
        power_score -= ac_mod * 0.5  # Penalize positive AC (but less harshly)

    # Other stats
    for stat_name, mod in stats_summary.items():
        if stat_name in ('hitroll', 'damroll', 'ac'):
            continue  # Already handled
        elif stat_name in ('str', 'dex', 'con', 'int', 'wis'):
            power_score += abs(mod) * 2
        elif stat_name == 'hit':
            power_score += abs(mod) * 0.05  # HP bonus
        elif stat_name == 'mana':
            power_score += abs(mod) * 0.03
        elif 'save' in stat_name:
            # Negative saves are better (resist spells)
            power_score += abs(mod) * 0.5
        else:
            power_score += abs(mod) * 0.3

    # Add type-specific value
    if obj.item_type == 5:  # WEAPON
        # value[0] encoding:
        #   value[0] % 1000 = on-hit spell effect (acid, darkness, etc.)
        #   value[0] / 1000 = passive wielder affect (see invis, fly, etc.)
        # value[1]: min damage
        # value[2]: max damage
        # value[3]: weapon type (slash, pierce, etc.)
        spell_flags = obj.value[0] if len(obj.value) > 0 else 0
        weapon_damage_min = obj.value[1] if len(obj.value) > 1 else 0
        weapon_damage_max = obj.value[2] if len(obj.value) > 2 else 0
        weapon_type_id = obj.value[3] if len(obj.value) > 3 else 0

        weapon_damage_avg = (weapon_damage_min + weapon_damage_max) / 2 \
            if weapon_damage_max > 0 else float(weapon_damage_min)
        weapon_type = WEAPON_TYPES.get(weapon_type_id, f'type_{weapon_type_id}')

        # Decode spell weapon components
        spell_id = spell_flags % 1000  # On-hit spell
        affect_id = spell_flags // 1000  # Passive wielder affect

        is_spell_weapon = spell_id > 0 or affect_id > 0

        if spell_id > 0:
            weapon_spell = WEAPON_SPELLS.get(spell_id, f'spell_{spell_id}')
        if affect_id > 0:
            weapon_affect = WEAPON_AFFECTS.get(affect_id, f'affect_{affect_id}')

        # Power contribution from weapon damage
        power_score += weapon_damage_avg * 1.5

        # Spell weapons are more powerful
        if spell_id > 0:
            power_score += 25
        if affect_id > 0:
            power_score += 35

    elif obj.item_type == 9:  # ARMOR
        # value[0]: AC bonus (higher = better base armor)
        # value[3]: special armor affects (gemstone types)
        armor_ac = obj.value[0] if len(obj.value) > 0 else 0
        armor_special_id = obj.value[3] if len(obj.value) > 3 else 0

        if armor_special_id > 0:
            armor_special = ARMOR_SPECIALS.get(armor_special_id, f'special_{armor_special_id}')
            # Special armor bonuses add significant power
            power_score += 50

        # Base armor AC contributes to power
        power_score += armor_ac * 2

    # Determine wear location from wear_flags
    wear_loc = None
    if obj.wear_flags & 2:
        wear_loc = 'finger'
    elif obj.wear_flags & 4:
        wear_loc = 'neck'
    elif obj.wear_flags & 8:
        wear_loc = 'body'
    elif obj.wear_flags & 16:
        wear_loc = 'head'
    elif obj.wear_flags & 32:
        wear_loc = 'legs'
    elif obj.wear_flags & 64:
        wear_loc = 'feet'
    elif obj.wear_flags & 128:
        wear_loc = 'hands'
    elif obj.wear_flags & 256:
        wear_loc = 'arms'
    elif obj.wear_flags & 512:
        wear_loc = 'shield'
    elif obj.wear_flags & 1024:
        wear_loc = 'about'
    elif obj.wear_flags & 2048:
        wear_loc = 'waist'
    elif obj.wear_flags & 4096:
        wear_loc = 'wrist'
    elif obj.wear_flags & 8192:
        wear_loc = 'wield'
    elif obj.wear_flags & 16384:
        wear_loc = 'hold'
    elif obj.wear_flags & 32768:
        wear_loc = 'face'

    # Determine tier based on new formula
    # Top items have ~1800+ (300 hr + 300 dr + special + AC)
    # Good items have ~500-1000
    # Average items have ~100-500
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

    # Decode extra flags for filtering
    extra_flag_list = decode_flags(obj.extra_flags, EXTRA_FLAGS)
    # Can pick up: ITEM_TAKE is bit 0 (value 1) in wear_flags
    can_take = bool(obj.wear_flags & 1)
    # Can drop: NOT nodrop flag (bit 128 in extra_flags)
    can_drop = not bool(obj.extra_flags & 128)

    return ObjectAnalysis(
        vnum=obj.vnum,
        name=obj.short_descr,
        item_type=item_type_name,
        power_score=power_score,
        power_tier=tier,
        stats_summary=stats_summary,
        wear_location=wear_loc,
        spawn_count=0,
        spawn_locations=[],
        # Weapon fields
        weapon_damage_min=weapon_damage_min,
        weapon_damage_max=weapon_damage_max,
        weapon_damage_avg=weapon_damage_avg,
        weapon_type=weapon_type,
        is_spell_weapon=is_spell_weapon,
        weapon_spell=weapon_spell,
        weapon_affect=weapon_affect,
        # Armor fields
        armor_ac=armor_ac,
        armor_special=armor_special,
        # Common fields
        weight=obj.weight,
        cost=obj.cost,
        level=obj.level,
        # Key combat stats
        hitroll=hitroll,
        damroll=damroll,
        ac_mod=ac_mod,
        # Object flags for query filtering
        extra_flags=extra_flag_list,
        can_take=can_take,
        can_drop=can_drop,
    )


def analyze_resets(area: Area) -> Dict[str, Any]:
    """
    Analyze resets to determine spawn counts and locations.

    Returns a dict with:
    - mob_spawns: {vnum: [(room_vnum, max_count), ...]}
    - obj_spawns: {vnum: [{'type': 'room/mob/container', 'location': vnum}, ...]}
    - total_gold: estimated gold spawned per reset
    """
    mob_spawns: Dict[int, List[tuple]] = {}
    obj_spawns: Dict[int, List[Dict[str, Any]]] = {}
    total_gold = 0

    current_mob_vnum = None
    current_room_vnum = None

    for reset in area.resets:
        if reset.command == 'M':
            # M: mob vnum, max count, room vnum
            mob_vnum = reset.arg1
            max_count = reset.arg2
            room_vnum = reset.arg3

            if mob_vnum not in mob_spawns:
                mob_spawns[mob_vnum] = []
            mob_spawns[mob_vnum].append((room_vnum, max_count))

            current_mob_vnum = mob_vnum
            current_room_vnum = room_vnum

            # Add gold from mob
            if mob_vnum in area.mobiles:
                total_gold += area.mobiles[mob_vnum].gold * max_count

        elif reset.command == 'O':
            # O: obj vnum, 0, room vnum
            obj_vnum = reset.arg1
            room_vnum = reset.arg3

            if obj_vnum not in obj_spawns:
                obj_spawns[obj_vnum] = []
            obj_spawns[obj_vnum].append({
                'type': 'room',
                'room_vnum': room_vnum,
            })

            current_room_vnum = room_vnum

        elif reset.command == 'G':
            # G: obj vnum, 0 (give to last mob)
            obj_vnum = reset.arg1

            if obj_vnum not in obj_spawns:
                obj_spawns[obj_vnum] = []
            obj_spawns[obj_vnum].append({
                'type': 'inventory',
                'mob_vnum': current_mob_vnum,
            })

        elif reset.command == 'E':
            # E: obj vnum, 0, wear_loc (equip on last mob)
            obj_vnum = reset.arg1
            wear_loc = reset.arg4

            if obj_vnum not in obj_spawns:
                obj_spawns[obj_vnum] = []
            obj_spawns[obj_vnum].append({
                'type': 'equipped',
                'mob_vnum': current_mob_vnum,
                'wear_loc': WEAR_LOCATIONS.get(wear_loc, f'loc_{wear_loc}'),
            })

        elif reset.command == 'P':
            # P: obj vnum, 0, container vnum
            obj_vnum = reset.arg1
            container_vnum = reset.arg3

            if obj_vnum not in obj_spawns:
                obj_spawns[obj_vnum] = []
            obj_spawns[obj_vnum].append({
                'type': 'container',
                'container_vnum': container_vnum,
            })

    return {
        'mob_spawns': mob_spawns,
        'obj_spawns': obj_spawns,
        'total_gold': total_gold,
    }


def analyze_area_difficulty(area: Area) -> AreaAnalysis:
    """Perform complete analysis of an area."""
    # Analyze mobs
    mob_analyses = {}
    for mob in area.mobiles.values():
        mob_analyses[mob.vnum] = calculate_mob_difficulty(mob)

    # Analyze objects
    obj_analyses = {}
    for obj in area.objects.values():
        obj_analyses[obj.vnum] = calculate_object_power(obj)

    # Analyze resets
    reset_info = analyze_resets(area)

    # Update spawn counts
    for vnum, spawns in reset_info['mob_spawns'].items():
        if vnum in mob_analyses:
            mob_analyses[vnum].spawn_count = sum(s[1] for s in spawns)
            mob_analyses[vnum].spawn_rooms = [s[0] for s in spawns]

    for vnum, spawns in reset_info['obj_spawns'].items():
        if vnum in obj_analyses:
            obj_analyses[vnum].spawn_count = len(spawns)
            obj_analyses[vnum].spawn_locations = spawns

    # Calculate averages and distributions
    difficulty_dist = {'trivial': 0, 'easy': 0, 'normal': 0, 'hard': 0, 'deadly': 0}
    power_dist = {'junk': 0, 'common': 0, 'uncommon': 0, 'rare': 0, 'epic': 0, 'legendary': 0}

    total_level = 0
    total_difficulty = 0
    total_power = 0

    for ma in mob_analyses.values():
        difficulty_dist[ma.difficulty_tier] += 1
        total_level += ma.level
        total_difficulty += ma.difficulty_score

    for oa in obj_analyses.values():
        power_dist[oa.power_tier] += 1
        total_power += oa.power_score

    avg_level = total_level / len(mob_analyses) if mob_analyses else 0
    avg_difficulty = total_difficulty / len(mob_analyses) if mob_analyses else 0
    avg_power = total_power / len(obj_analyses) if obj_analyses else 0

    # Build shop info
    shops_info = []
    for shop in area.shops:
        keeper_name = "Unknown"
        if shop.keeper_vnum in area.mobiles:
            keeper_name = area.mobiles[shop.keeper_vnum].short_descr
        shops_info.append({
            'keeper_vnum': shop.keeper_vnum,
            'keeper_name': keeper_name,
            'buy_types': shop.to_dict()['buy_types'],
            'profit_buy': shop.profit_buy,
            'profit_sell': shop.profit_sell,
        })

    return AreaAnalysis(
        name=area.name,
        filename=area.filename,
        mob_count=len(area.mobiles),
        object_count=len(area.objects),
        room_count=len(area.rooms),
        avg_mob_level=avg_level,
        avg_difficulty=avg_difficulty,
        difficulty_distribution=difficulty_dist,
        avg_object_power=avg_power,
        power_distribution=power_dist,
        total_gold_per_reset=reset_info['total_gold'],
        shops=shops_info,
    )


def get_mob_analysis(area: Area) -> Dict[int, MobAnalysis]:
    """Get mob analyses for all mobs in an area."""
    mob_analyses = {}
    for mob in area.mobiles.values():
        mob_analyses[mob.vnum] = calculate_mob_difficulty(mob)

    reset_info = analyze_resets(area)
    for vnum, spawns in reset_info['mob_spawns'].items():
        if vnum in mob_analyses:
            mob_analyses[vnum].spawn_count = sum(s[1] for s in spawns)
            mob_analyses[vnum].spawn_rooms = [s[0] for s in spawns]

    return mob_analyses


def get_object_analysis(area: Area) -> Dict[int, ObjectAnalysis]:
    """Get object analyses for all objects in an area."""
    obj_analyses = {}
    for obj in area.objects.values():
        obj_analyses[obj.vnum] = calculate_object_power(obj)

    reset_info = analyze_resets(area)
    for vnum, spawns in reset_info['obj_spawns'].items():
        if vnum in obj_analyses:
            obj_analyses[vnum].spawn_count = len(spawns)
            obj_analyses[vnum].spawn_locations = spawns

    return obj_analyses
