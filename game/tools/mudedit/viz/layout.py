"""
BFS-based coordinate assignment for room layout.

Ports the algorithm from map_generator.py to work with SQLite room/exit data.
"""

import sys
from collections import deque
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from mudlib.models import (
    DIR_NORTH, DIR_EAST, DIR_SOUTH, DIR_WEST, DIR_UP, DIR_DOWN,
    DIR_NAMES, REVERSE_DIR, DIR_OFFSETS, SECTOR_NAMES, ROOM_FLAGS
)

SECTOR_COLORS = {
    0: '#808080',   # inside - gray
    1: '#FFD700',   # city - gold
    2: '#90EE90',   # field - light green
    3: '#228B22',   # forest - dark green
    4: '#8B4513',   # hills - brown
    5: '#A0522D',   # mountain - saddle brown
    6: '#4169E1',   # water_swim - royal blue
    7: '#00008B',   # water_noswim - dark blue
    9: '#87CEEB',   # air - sky blue
    10: '#F4A460',  # desert - sandy brown
}


def decode_flags(value: int, flag_map: dict) -> List[str]:
    """Decode a bitmask into a list of flag names."""
    return [name for bit, name in sorted(flag_map.items()) if value & bit]


def assign_room_coordinates(
    rooms: List[dict],
    exits_by_room: Dict[int, List[dict]]
) -> Dict[int, Tuple[int, int, int]]:
    """
    Assign X/Y/Z coordinates to rooms using multi-pass BFS.

    Args:
        rooms: List of room dicts with 'vnum' key
        exits_by_room: Dict mapping room_vnum -> list of exit dicts
                       (each with 'direction', 'to_vnum')

    Returns:
        Dict mapping vnum -> (x, y, z) coordinates
    """
    if not rooms:
        return {}

    room_vnums = {r['vnum'] for r in rooms}
    coords: Dict[int, Tuple[int, int, int]] = {}
    occupied: Dict[Tuple[int, int, int], int] = {}
    extended: Dict[int, Tuple[int, int, Tuple[int, int, int]]] = {}

    start_vnum = min(room_vnums)
    coords[start_vnum] = (0, 0, 0)
    occupied[(0, 0, 0)] = start_vnum

    visited: Set[int] = set()
    queue = deque([start_vnum])

    # Pass 1: Initial BFS coordinate assignment
    while queue:
        current_vnum = queue.popleft()
        if current_vnum in visited:
            continue
        visited.add(current_vnum)

        if current_vnum not in coords:
            continue

        cx, cy, cz = coords[current_vnum]

        for exit_data in exits_by_room.get(current_vnum, []):
            dest_vnum = exit_data['to_vnum']
            direction = exit_data['direction']

            if dest_vnum not in room_vnums or dest_vnum <= 0:
                continue

            dx, dy, dz = DIR_OFFSETS.get(direction, (0, 0, 0))
            ideal = (cx + dx, cy + dy, cz + dz)

            if dest_vnum not in coords:
                if ideal not in occupied:
                    coords[dest_vnum] = ideal
                    occupied[ideal] = dest_vnum
                else:
                    # Position occupied - extend outward
                    multiplier = 1
                    pos = ideal
                    while pos in occupied and multiplier < 20:
                        multiplier += 1
                        pos = (cx + dx * multiplier, cy + dy * multiplier, cz + dz)

                    coords[dest_vnum] = pos
                    occupied[pos] = dest_vnum
                    extended[dest_vnum] = (current_vnum, direction, ideal)

                queue.append(dest_vnum)

    # Pass 2: Try to fix extended rooms by relocating blockers
    for dest_vnum, (from_vnum, direction, ideal) in extended.items():
        if ideal not in occupied:
            continue
        blocker_vnum = occupied[ideal]

        dest_exits = len([e for e in exits_by_room.get(dest_vnum, [])
                         if e['to_vnum'] in room_vnums])
        blocker_exits = len([e for e in exits_by_room.get(blocker_vnum, [])
                            if e['to_vnum'] in room_vnums])

        if dest_exits < blocker_exits:
            dx, dy, dz = DIR_OFFSETS[direction]
            shift = (1, 0, 0) if direction in (DIR_NORTH, DIR_SOUTH) else (0, 1, 0)

            bx, by, bz = ideal
            new_pos = (bx + shift[0], by + shift[1], bz + shift[2])
            attempts = 0
            while new_pos in occupied and attempts < 10:
                new_pos = (new_pos[0] + shift[0], new_pos[1] + shift[1],
                          new_pos[2] + shift[2])
                attempts += 1

            if new_pos not in occupied:
                old = coords[blocker_vnum]
                del occupied[old]
                coords[blocker_vnum] = new_pos
                occupied[new_pos] = blocker_vnum

                old_dest = coords[dest_vnum]
                del occupied[old_dest]
                coords[dest_vnum] = ideal
                occupied[ideal] = dest_vnum

    # Handle disconnected rooms
    if coords:
        disconnected_x = max(c[0] for c in occupied.keys()) + 3
    else:
        disconnected_x = 0
    for r in rooms:
        vnum = r['vnum']
        if vnum not in coords:
            while (disconnected_x, 0, 0) in occupied:
                disconnected_x += 1
            coords[vnum] = (disconnected_x, 0, 0)
            occupied[(disconnected_x, 0, 0)] = vnum
            disconnected_x += 1

    return coords


def detect_one_way_exits(
    exits_by_room: Dict[int, List[dict]],
    room_vnums: Set[int]
) -> Set[Tuple[int, int]]:
    """
    Detect one-way exits.

    Returns:
        Set of (room_vnum, direction) pairs that are one-way.
    """
    one_way = set()

    for room_vnum, exits in exits_by_room.items():
        for exit_data in exits:
            dest_vnum = exit_data['to_vnum']
            direction = exit_data['direction']

            if dest_vnum not in room_vnums or dest_vnum <= 0:
                continue

            reverse_dir = REVERSE_DIR[direction]
            dest_exits = exits_by_room.get(dest_vnum, [])
            has_reverse = any(
                e['direction'] == reverse_dir and e['to_vnum'] == room_vnum
                for e in dest_exits
            )
            if not has_reverse:
                one_way.add((room_vnum, direction))

    return one_way


def detect_warped_exits(
    coords: Dict[int, Tuple[int, int, int]],
    exits_by_room: Dict[int, List[dict]],
    room_vnums: Set[int]
) -> Set[Tuple[int, int]]:
    """
    Detect non-Euclidean exits where direction doesn't match spatial offset.

    Returns:
        Set of (room_vnum, direction) pairs that are warped.
    """
    warped = set()

    for room_vnum, exits in exits_by_room.items():
        if room_vnum not in coords:
            continue
        cx, cy, cz = coords[room_vnum]

        for exit_data in exits:
            dest_vnum = exit_data['to_vnum']
            direction = exit_data['direction']

            if dest_vnum not in room_vnums or dest_vnum not in coords:
                continue

            dest_x, dest_y, dest_z = coords[dest_vnum]
            dx, dy, dz = DIR_OFFSETS.get(direction, (0, 0, 0))

            actual_dx = dest_x - cx
            actual_dy = dest_y - cy
            actual_dz = dest_z - cz

            is_warped = False
            if direction in (DIR_NORTH, DIR_SOUTH):
                if (dy > 0) != (actual_dy > 0) or (dy < 0) != (actual_dy < 0):
                    is_warped = True
                elif actual_dy == 0:
                    is_warped = True
                elif actual_dx != 0:
                    is_warped = True
            elif direction in (DIR_EAST, DIR_WEST):
                if (dx > 0) != (actual_dx > 0) or (dx < 0) != (actual_dx < 0):
                    is_warped = True
                elif actual_dx == 0:
                    is_warped = True
                elif actual_dy != 0:
                    is_warped = True
            elif direction in (DIR_UP, DIR_DOWN):
                if (dz > 0) != (actual_dz > 0) or (dz < 0) != (actual_dz < 0):
                    is_warped = True
                elif actual_dz == 0:
                    is_warped = True

            if is_warped:
                warped.add((room_vnum, direction))

    return warped
