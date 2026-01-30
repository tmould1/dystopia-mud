"""
Area file parser for Dystopia MUD.

Parses .are files and extracts rooms, mobiles, objects, resets, and shops.
"""

import re
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from .models import (
    Area, Room, Exit, Mobile, Object, ObjectAffect, Reset, Shop,
    ExtraDescription, RoomText
)


def strip_color_codes(text: str) -> str:
    """Remove MUD color codes from text."""
    # Handle [#7-#0] style bracketed codes first
    text = re.sub(r'\[[#\d\-]+\]', '', text)
    # Common patterns: #R, #G, #B, #W, #0, #n (reset), etc.
    text = re.sub(r'#[0-9a-zA-Z]', '', text)
    # Handle ^M style codes
    text = re.sub(r'\^[A-Za-z]', '', text)
    return text.strip()


def read_tilde_string(lines: List[str], idx: int) -> Tuple[str, int]:
    """Read a tilde-terminated string from lines, starting at idx."""
    result = []
    while idx < len(lines):
        line = lines[idx]
        if line.endswith('~'):
            result.append(line[:-1])
            return '\n'.join(result), idx + 1
        result.append(line)
        idx += 1
    return '\n'.join(result), idx


def parse_dice(dice_str: str) -> Tuple[int, int, int]:
    """Parse a dice string like '2d6+10' into (num, size, plus)."""
    # Handle formats: NdS+P, NdS-P, NdS
    match = re.match(r'(\d+)d(\d+)([+-]\d+)?', dice_str.replace(' ', ''))
    if match:
        num = int(match.group(1))
        size = int(match.group(2))
        plus = int(match.group(3)) if match.group(3) else 0
        return (num, size, plus)
    return (1, 1, 0)


def parse_mobiles(lines: List[str], start_idx: int, area: Area) -> int:
    """Parse #MOBILES section and add to area."""
    idx = start_idx

    while idx < len(lines):
        line = lines[idx].strip()

        # End of section
        if line == '#0' or line.startswith('#') and not line[1:].lstrip('-').isdigit():
            break

        # Mobile definition starts with #VNUM
        if line.startswith('#') and line[1:].lstrip('-').isdigit():
            vnum = int(line[1:])
            idx += 1

            # Read name (keywords), tilde-terminated
            name, idx = read_tilde_string(lines, idx)
            name = strip_color_codes(name)

            # Read short description
            short_descr, idx = read_tilde_string(lines, idx)
            short_descr = strip_color_codes(short_descr)

            # Read long description
            long_descr, idx = read_tilde_string(lines, idx)
            long_descr = strip_color_codes(long_descr)

            # Read detailed description
            description, idx = read_tilde_string(lines, idx)
            description = strip_color_codes(description)

            # Read stats line 1: act affected_by alignment letter
            if idx < len(lines):
                stats1 = lines[idx].strip().split()
                act_flags = int(stats1[0]) if len(stats1) > 0 else 0
                affected_by = int(stats1[1]) if len(stats1) > 1 else 0
                alignment = int(stats1[2]) if len(stats1) > 2 else 0
                # letter 'S' is at position 3
                idx += 1

            # Read stats line 2: level hitroll ac hitdice damdice
            # Format: level hitroll ac NdS+P NdS+P
            if idx < len(lines):
                stats2 = lines[idx].strip()
                parts = stats2.split()
                level = int(parts[0]) if len(parts) > 0 else 1
                hitroll = int(parts[1]) if len(parts) > 1 else 0
                ac = int(parts[2]) if len(parts) > 2 else 0

                # Parse hit dice (e.g., "1d1+30000")
                hit_dice = (1, 1, 0)
                dam_dice = (1, 1, 0)
                if len(parts) > 3:
                    hit_dice = parse_dice(parts[3])
                if len(parts) > 4:
                    dam_dice = parse_dice(parts[4])
                idx += 1

            # Read stats line 3: gold xp position
            gold = 0
            if idx < len(lines):
                stats3 = lines[idx].strip().split()
                gold = int(stats3[0]) if len(stats3) > 0 else 0
                idx += 1

            # Read stats line 4: sex (and possibly more)
            sex = 0
            if idx < len(lines):
                stats4 = lines[idx].strip().split()
                sex = int(stats4[0]) if len(stats4) > 0 else 0
                idx += 1

            mobile = Mobile(
                vnum=vnum,
                name=name,
                short_descr=short_descr,
                long_descr=long_descr,
                description=description,
                act_flags=act_flags,
                affected_by=affected_by,
                alignment=alignment,
                level=level,
                hitroll=hitroll,
                ac=ac,
                hit_dice=hit_dice,
                dam_dice=dam_dice,
                gold=gold,
                sex=sex,
            )
            area.mobiles[vnum] = mobile
        else:
            idx += 1

    return idx


def parse_objects(lines: List[str], start_idx: int, area: Area) -> int:
    """Parse #OBJECTS section and add to area."""
    idx = start_idx

    while idx < len(lines):
        line = lines[idx].strip()

        # End of section
        if line == '#0' or (line.startswith('#') and
                           line[1:] not in [''] and
                           not line[1:].lstrip('-').isdigit()):
            break

        # Object definition starts with #VNUM
        if line.startswith('#') and line[1:].lstrip('-').isdigit():
            vnum = int(line[1:])
            idx += 1

            # Read name (keywords)
            name, idx = read_tilde_string(lines, idx)
            name = strip_color_codes(name)

            # Read short description
            short_descr, idx = read_tilde_string(lines, idx)
            short_descr = strip_color_codes(short_descr)

            # Read long description
            long_descr, idx = read_tilde_string(lines, idx)
            long_descr = strip_color_codes(long_descr)

            # Read action description (often empty/tilde)
            _, idx = read_tilde_string(lines, idx)

            # Read type line: item_type extra_flags wear_flags
            item_type = 0
            extra_flags = 0
            wear_flags = 0
            if idx < len(lines):
                type_line = lines[idx].strip().split()
                item_type = int(type_line[0]) if len(type_line) > 0 else 0
                extra_flags = int(type_line[1]) if len(type_line) > 1 else 0
                wear_flags = int(type_line[2]) if len(type_line) > 2 else 0
                idx += 1

            # Read values line: v0 v1 v2 v3
            values = [0, 0, 0, 0]
            if idx < len(lines):
                val_line = lines[idx].strip().split()
                for i in range(min(4, len(val_line))):
                    try:
                        values[i] = int(val_line[i])
                    except ValueError:
                        values[i] = 0
                idx += 1

            # Read weight/cost line: weight cost level
            weight = 0
            cost = 0
            level = 0
            if idx < len(lines):
                wc_line = lines[idx].strip().split()
                weight = int(wc_line[0]) if len(wc_line) > 0 else 0
                cost = int(wc_line[1]) if len(wc_line) > 1 else 0
                level = int(wc_line[2]) if len(wc_line) > 2 else 0
                idx += 1

            obj = Object(
                vnum=vnum,
                name=name,
                short_descr=short_descr,
                long_descr=long_descr,
                item_type=item_type,
                extra_flags=extra_flags,
                wear_flags=wear_flags,
                value=values,
                weight=weight,
                cost=cost,
                level=level,
            )

            # Parse optional extras (A for affects, E for extra desc, Q for powers)
            while idx < len(lines):
                extra_line = lines[idx].strip()

                if extra_line == 'A':
                    # Affect: apply_type modifier
                    idx += 1
                    if idx < len(lines):
                        affect_parts = lines[idx].strip().split()
                        if len(affect_parts) >= 2:
                            apply_type = int(affect_parts[0])
                            modifier = int(affect_parts[1])
                            obj.affects.append(ObjectAffect(apply_type, modifier))
                        idx += 1
                elif extra_line == 'E':
                    # Extra description: keyword~ then description~
                    idx += 1
                    ed_keyword, idx = read_tilde_string(lines, idx)
                    ed_desc, idx = read_tilde_string(lines, idx)
                    obj.extra_descs.append(ExtraDescription(ed_keyword, ed_desc))
                elif extra_line == 'Q':
                    # Power strings: 6 tilde-terminated strings + spectype specpower
                    idx += 1
                    obj.chpoweron, idx = read_tilde_string(lines, idx)
                    obj.chpoweroff, idx = read_tilde_string(lines, idx)
                    obj.chpoweruse, idx = read_tilde_string(lines, idx)
                    obj.victpoweron, idx = read_tilde_string(lines, idx)
                    obj.victpoweroff, idx = read_tilde_string(lines, idx)
                    obj.victpoweruse, idx = read_tilde_string(lines, idx)
                    if idx < len(lines):
                        qparts = lines[idx].strip().split()
                        obj.spectype = int(qparts[0]) if len(qparts) > 0 else 0
                        obj.specpower = int(qparts[1]) if len(qparts) > 1 else 0
                        idx += 1
                elif extra_line.startswith('#') or not extra_line:
                    break
                else:
                    idx += 1

            area.objects[vnum] = obj
        else:
            idx += 1

    return idx


def parse_resets(lines: List[str], start_idx: int, area: Area) -> int:
    """Parse #RESETS section and add to area."""
    idx = start_idx

    while idx < len(lines):
        line = lines[idx].strip()

        # End of section
        if line == 'S' or line.startswith('#'):
            break

        if not line or line.startswith('*'):
            idx += 1
            continue

        parts = line.split()
        if not parts:
            idx += 1
            continue

        cmd = parts[0]

        if cmd in ('M', 'O', 'G', 'E', 'P', 'D', 'R'):
            # Parse reset command
            # M: mob_vnum limit room_vnum
            # O: obj_vnum limit room_vnum
            # G: obj_vnum limit (give to last mob)
            # E: obj_vnum limit wear_loc (equip on last mob)
            # P: obj_vnum limit container_vnum
            # D: room_vnum door state
            # R: room_vnum num_doors (randomize exits)

            arg1 = int(parts[1]) if len(parts) > 1 else 0
            arg2 = int(parts[2]) if len(parts) > 2 else 0
            arg3 = int(parts[3]) if len(parts) > 3 else 0
            arg4 = int(parts[4]) if len(parts) > 4 else 0

            # For E resets, arg4 is wear location; for others shift args
            if cmd == 'E':
                reset = Reset(cmd, arg2, arg3, arg4, arg4)  # vnum, limit, wear_loc
            elif cmd in ('M', 'O', 'P'):
                reset = Reset(cmd, arg2, arg3, arg4)  # vnum, limit, room/container
            elif cmd == 'G':
                reset = Reset(cmd, arg2, arg3, 0)  # vnum, limit, 0
            else:
                reset = Reset(cmd, arg2, arg3, arg4)

            area.resets.append(reset)

        idx += 1

    return idx


def parse_rooms(lines: List[str], start_idx: int, area: Area) -> int:
    """Parse #ROOMS or #ROOMDATA section and add to area."""
    idx = start_idx

    while idx < len(lines):
        rline = lines[idx].strip()

        # End of rooms section
        if rline == '#0' or rline.startswith('#RESETS') or rline.startswith('#SPECIALS'):
            break

        # Room definition starts with #VNUM
        if rline.startswith('#') and rline[1:].isdigit():
            vnum = int(rline[1:])
            idx += 1

            # Read room name (tilde-terminated)
            room_name, idx = read_tilde_string(lines, idx)
            room_name = strip_color_codes(room_name)

            # Read room description (tilde-terminated)
            room_desc, idx = read_tilde_string(lines, idx)

            # Read room flags line: area_num room_flags sector_type
            sector_type = 0
            room_flags = 0
            if idx < len(lines):
                flags_line = lines[idx].strip()
                parts = flags_line.split()
                if len(parts) >= 3:
                    try:
                        room_flags = int(parts[1])
                        sector_type = int(parts[2])
                    except ValueError:
                        pass
                idx += 1

            room = Room(vnum, room_name, room_desc, sector_type, room_flags)

            # Parse room extras (E, D, T, S markers)
            while idx < len(lines):
                marker = lines[idx].strip()

                if marker == 'S':
                    idx += 1
                    break

                # Extra description
                elif marker.startswith('E') and (len(marker) == 1 or not marker[1:].isdigit()):
                    idx += 1
                    ed_keyword, idx = read_tilde_string(lines, idx)
                    ed_desc, idx = read_tilde_string(lines, idx)
                    room.extra_descs.append(ExtraDescription(ed_keyword, ed_desc))

                # Door/Exit
                elif marker.startswith('D'):
                    try:
                        direction = int(marker[1:])
                    except ValueError:
                        idx += 1
                        continue

                    idx += 1
                    exit_desc, idx = read_tilde_string(lines, idx)
                    exit_keyword, idx = read_tilde_string(lines, idx)

                    # Lock flags, key vnum, destination vnum
                    if idx < len(lines):
                        exit_line = lines[idx].strip()
                        parts = exit_line.split()
                        if len(parts) >= 3:
                            try:
                                lock_flags = int(parts[0])
                                key_vnum = int(parts[1])
                                dest_vnum = int(parts[2])

                                exit_obj = Exit(
                                    direction=direction,
                                    destination_vnum=dest_vnum,
                                    is_door=(lock_flags > 0),
                                    door_flags=lock_flags,
                                    key_vnum=key_vnum,
                                    description=exit_desc,
                                    keyword=exit_keyword,
                                )
                                room.exits[direction] = exit_obj
                            except ValueError:
                                pass
                        idx += 1

                # Roomtext trigger: 4 strings + 3 numbers
                elif marker.startswith('T'):
                    idx += 1
                    rt_input, idx = read_tilde_string(lines, idx)
                    rt_output, idx = read_tilde_string(lines, idx)
                    rt_choutput, idx = read_tilde_string(lines, idx)
                    rt_name, idx = read_tilde_string(lines, idx)
                    rt_type = 0
                    rt_power = 0
                    rt_mob = 0
                    if idx < len(lines):
                        rtparts = lines[idx].strip().split()
                        rt_type = int(rtparts[0]) if len(rtparts) > 0 else 0
                        rt_power = int(rtparts[1]) if len(rtparts) > 1 else 0
                        rt_mob = int(rtparts[2]) if len(rtparts) > 2 else 0
                        idx += 1
                    room.room_texts.append(RoomText(
                        rt_input, rt_output, rt_choutput, rt_name,
                        rt_type, rt_power, rt_mob
                    ))

                else:
                    idx += 1

            # Check for dead-end
            room.dead_end = len(room.exits) == 0
            area.rooms[vnum] = room
        else:
            idx += 1

    return idx


def parse_shops(lines: List[str], start_idx: int, area: Area) -> int:
    """Parse #SHOPS section and add to area."""
    idx = start_idx

    while idx < len(lines):
        line = lines[idx].strip()

        if line == '0' or line.startswith('#'):
            break

        parts = line.split()
        if len(parts) >= 8:
            try:
                keeper_vnum = int(parts[0])
                buy_types = []
                for i in range(1, 6):
                    bt = int(parts[i])
                    if bt > 0:
                        buy_types.append(bt)
                profit_buy = int(parts[6])
                profit_sell = int(parts[7])
                open_hour = int(parts[8]) if len(parts) > 8 else 0
                close_hour = int(parts[9]) if len(parts) > 9 else 23

                shop = Shop(
                    keeper_vnum=keeper_vnum,
                    buy_types=buy_types,
                    profit_buy=profit_buy,
                    profit_sell=profit_sell,
                    open_hour=open_hour,
                    close_hour=close_hour,
                )
                area.shops.append(shop)
            except ValueError:
                pass

        idx += 1

    return idx


def parse_specials(lines: List[str], start_idx: int, area: Area) -> int:
    """Parse #SPECIALS section and add to area."""
    idx = start_idx

    while idx < len(lines):
        line = lines[idx].strip()

        if line == 'S' or line.startswith('#'):
            break

        if not line or line.startswith('*'):
            idx += 1
            continue

        parts = line.split()
        if len(parts) >= 3 and parts[0] == 'M':
            try:
                mob_vnum = int(parts[1])
                spec_fun = parts[2]
                area.specials[mob_vnum] = spec_fun
            except ValueError:
                pass

        idx += 1

    return idx


def parse_area_file(filepath: Path) -> Optional[Area]:
    """Parse a single .are file and return an Area object."""
    try:
        with open(filepath, 'r', encoding='latin-1') as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        return None

    lines = content.split('\n')
    area = None
    idx = 0

    while idx < len(lines):
        line = lines[idx].strip()

        # Parse #AREADATA section
        if line == '#AREADATA':
            idx += 1
            name = ""
            lvnum = 0
            uvnum = 0
            builders = ""
            security = 3
            recall = 0
            area_flags = 0
            while idx < len(lines):
                aline = lines[idx].strip()
                if aline == 'End':
                    idx += 1
                    break
                if aline.startswith('Name'):
                    name_part = aline[4:].strip()
                    if name_part.endswith('~'):
                        name = name_part[:-1].strip()
                    else:
                        name = name_part
                elif aline.startswith('VNUMs'):
                    parts = aline.split()
                    if len(parts) >= 3:
                        lvnum = int(parts[1])
                        uvnum = int(parts[2])
                elif aline.startswith('Builders'):
                    b_part = aline[8:].strip()
                    if b_part.endswith('~'):
                        builders = b_part[:-1].strip()
                    else:
                        builders = b_part
                elif aline.startswith('Security'):
                    parts = aline.split()
                    if len(parts) >= 2:
                        security = int(parts[1])
                elif aline.startswith('Recall'):
                    parts = aline.split()
                    if len(parts) >= 2:
                        recall = int(parts[1])
                elif aline.startswith('Flags'):
                    parts = aline.split()
                    if len(parts) >= 2:
                        area_flags = int(parts[1])
                idx += 1

            area = Area(
                name=strip_color_codes(name),
                filename=filepath.name,
                lvnum=lvnum,
                uvnum=uvnum,
                builders=builders,
                security=security,
                recall=recall,
                area_flags=area_flags,
            )
            continue

        # Parse #MOBILES section
        if line == '#MOBILES':
            if area is None:
                area = Area(name=filepath.stem, filename=filepath.name)
            idx = parse_mobiles(lines, idx + 1, area)
            continue

        # Parse #OBJECTS section
        if line == '#OBJECTS':
            if area is None:
                area = Area(name=filepath.stem, filename=filepath.name)
            idx = parse_objects(lines, idx + 1, area)
            continue

        # Parse #ROOMS or #ROOMDATA section
        if line in ('#ROOMDATA', '#ROOMS'):
            if area is None:
                area = Area(name=filepath.stem, filename=filepath.name)
            idx = parse_rooms(lines, idx + 1, area)
            continue

        # Parse #RESETS section
        if line == '#RESETS':
            if area is None:
                area = Area(name=filepath.stem, filename=filepath.name)
            idx = parse_resets(lines, idx + 1, area)
            continue

        # Parse #SHOPS section
        if line == '#SHOPS':
            if area is None:
                area = Area(name=filepath.stem, filename=filepath.name)
            idx = parse_shops(lines, idx + 1, area)
            continue

        # Parse #SPECIALS section
        if line == '#SPECIALS':
            if area is None:
                area = Area(name=filepath.stem, filename=filepath.name)
            idx = parse_specials(lines, idx + 1, area)
            continue

        idx += 1

    return area


def parse_all_areas(area_dir: Path) -> Dict[str, Area]:
    """Parse all areas listed in area.lst."""
    areas = {}

    area_list_path = area_dir / 'area.lst'
    if not area_list_path.exists():
        print(f"Error: {area_list_path} not found")
        return areas

    with open(area_list_path, 'r') as f:
        area_files = [line.strip() for line in f if line.strip() and not line.startswith('$')]

    for area_file in area_files:
        filepath = area_dir / area_file
        if not filepath.exists():
            print(f"  Warning: {area_file} not found, skipping")
            continue

        area = parse_area_file(filepath)
        if area:
            area_id = filepath.stem
            areas[area_id] = area

    return areas
