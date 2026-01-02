#!/usr/bin/env python3
"""
Dystopia MUD Map Generator

Parses .are area files and generates JSON data for web-based map visualization.
Handles one-way exits, non-Euclidean geometry, and cross-area connections.

Usage:
    python map_generator.py              # Generate all areas
    python map_generator.py --area NAME  # Generate single area
    python map_generator.py --watch      # Watch mode (requires watchdog)
"""

import os
import re
import json
import argparse
from collections import defaultdict, deque
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Set, Any

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

# Sector type names
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


class Exit:
    """Represents an exit from a room."""
    def __init__(self, direction: int, destination_vnum: int,
                 is_door: bool = False, door_flags: int = 0, key_vnum: int = -1):
        self.direction = direction
        self.destination_vnum = destination_vnum
        self.is_door = is_door
        self.door_flags = door_flags
        self.key_vnum = key_vnum
        self.one_way = False  # Set during analysis
        self.warped = False   # Set during coordinate assignment

    def to_dict(self) -> dict:
        return {
            'to': self.destination_vnum,
            'door': self.is_door,
            'one_way': self.one_way,
            'warped': self.warped,
        }


class Room:
    """Represents a room in an area."""
    def __init__(self, vnum: int, name: str, sector_type: int = 0, room_flags: int = 0):
        self.vnum = vnum
        self.name = name
        self.sector_type = sector_type
        self.room_flags = room_flags
        self.exits: Dict[int, Exit] = {}
        self.coords: Optional[Tuple[int, int, int]] = None
        self.dead_end = False

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


class Area:
    """Represents a MUD area."""
    def __init__(self, name: str, filename: str, lvnum: int = 0, uvnum: int = 0):
        self.name = name
        self.filename = filename
        self.lvnum = lvnum
        self.uvnum = uvnum
        self.rooms: Dict[int, Room] = {}

    def to_dict(self) -> dict:
        return {
            'name': self.name,
            'filename': self.filename,
            'vnum_range': [self.lvnum, self.uvnum],
            'room_count': len(self.rooms),
            'rooms': {str(v): r.to_dict() for v, r in self.rooms.items()},
        }


def strip_color_codes(text: str) -> str:
    """Remove MUD color codes from text."""
    # Handle [#7-#0] style bracketed codes first - remove entire bracket section
    text = re.sub(r'\[[#\d\-]+\]', '', text)
    # Common patterns: #R, #G, #B, #W, #0, #n (reset), ^M, etc.
    # Handle #n (reset code) and other single-letter codes
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
            while idx < len(lines):
                aline = lines[idx].strip()
                if aline == 'End':
                    idx += 1
                    break
                if aline.startswith('Name'):
                    # Name may be on same line or next
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
                idx += 1

            area = Area(
                name=strip_color_codes(name),
                filename=filepath.name,
                lvnum=lvnum,
                uvnum=uvnum
            )
            continue

        # Parse #ROOMDATA or #ROOMS section
        if line in ('#ROOMDATA', '#ROOMS'):
            if area is None:
                area = Area(name=filepath.stem, filename=filepath.name)
            idx += 1
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
                    _, idx = read_tilde_string(lines, idx)

                    # Read room flags line: area_num room_flags sector_type
                    if idx < len(lines):
                        flags_line = lines[idx].strip()
                        parts = flags_line.split()
                        sector_type = 0
                        room_flags = 0
                        if len(parts) >= 3:
                            try:
                                room_flags = int(parts[1])
                                sector_type = int(parts[2])
                            except ValueError:
                                pass
                        idx += 1

                    room = Room(vnum, room_name, sector_type, room_flags)

                    # Parse room extras (E, D, T, S markers)
                    while idx < len(lines):
                        marker = lines[idx].strip()

                        if marker == 'S':
                            idx += 1
                            break

                        # Extra description
                        elif marker.startswith('E'):
                            idx += 1
                            _, idx = read_tilde_string(lines, idx)  # keyword
                            _, idx = read_tilde_string(lines, idx)  # description

                        # Door/Exit
                        elif marker.startswith('D'):
                            try:
                                direction = int(marker[1:])
                            except ValueError:
                                idx += 1
                                continue

                            idx += 1
                            _, idx = read_tilde_string(lines, idx)  # exit description
                            _, idx = read_tilde_string(lines, idx)  # keywords

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
                                            key_vnum=key_vnum
                                        )
                                        room.exits[direction] = exit_obj
                                    except ValueError:
                                        pass
                                idx += 1

                        # Roomtext trigger
                        elif marker.startswith('T'):
                            idx += 1
                            # Skip roomtext data until we hit a line that's a new marker
                            while idx < len(lines):
                                tline = lines[idx].strip()
                                if tline in ('S', 'E') or tline.startswith('D') or tline.startswith('T'):
                                    break
                                if tline.startswith('#') and len(tline) > 1:
                                    break
                                idx += 1

                        else:
                            idx += 1

                    # Check for dead-end
                    room.dead_end = len(room.exits) == 0
                    area.rooms[vnum] = room
                else:
                    idx += 1
            continue

        idx += 1

    return area


def analyze_one_way_exits(areas: Dict[str, Area]) -> None:
    """Detect one-way exits by checking for missing reverse exits."""
    # Build global room lookup
    all_rooms: Dict[int, Room] = {}
    for area in areas.values():
        all_rooms.update(area.rooms)

    for area in areas.values():
        for room in area.rooms.values():
            for direction, exit_obj in room.exits.items():
                dest_vnum = exit_obj.destination_vnum
                if dest_vnum not in all_rooms:
                    # Cross-area exit to unknown room
                    continue

                dest_room = all_rooms[dest_vnum]
                reverse_dir = REVERSE_DIR[direction]

                # Check if reverse exit exists and points back
                reverse_exit = dest_room.exits.get(reverse_dir)
                if reverse_exit is None or reverse_exit.destination_vnum != room.vnum:
                    exit_obj.one_way = True


def assign_coordinates(area: Area) -> List[dict]:
    """
    Assign X/Y/Z coordinates to rooms using multi-pass BFS.
    Pass 1: Assign initial coordinates, tracking extended connections.
    Pass 2: For rooms that were extended, check if shifting the blocker would help.
    Pass 3: Detect spine conflicts and shift entire branches if needed.
    Returns list of conflicts (non-Euclidean geometry).
    """
    if not area.rooms:
        return []

    conflicts = []
    occupied: Dict[Tuple[int, int, int], int] = {}  # coords -> vnum
    extended: Dict[int, Tuple[int, int, Tuple[int, int, int]]] = {}  # dest_vnum -> (from_vnum, direction, ideal_coords)
    parent: Dict[int, int] = {}  # child -> parent in BFS tree

    # Start from first room in area
    start_vnum = min(area.rooms.keys())
    start_room = area.rooms[start_vnum]
    start_room.coords = (0, 0, 0)
    occupied[(0, 0, 0)] = start_vnum

    visited: Set[int] = set()
    queue = deque([start_vnum])

    # Pass 1: Initial coordinate assignment
    while queue:
        current_vnum = queue.popleft()
        if current_vnum in visited:
            continue
        visited.add(current_vnum)

        current_room = area.rooms.get(current_vnum)
        if not current_room or not current_room.coords:
            continue

        cx, cy, cz = current_room.coords

        for direction, exit_obj in current_room.exits.items():
            dest_vnum = exit_obj.destination_vnum

            # Only process rooms within this area
            if dest_vnum not in area.rooms:
                continue

            dest_room = area.rooms[dest_vnum]
            dx, dy, dz = DIR_OFFSETS[direction]
            ideal_coords = (cx + dx, cy + dy, cz + dz)

            if dest_room.coords is None:
                if ideal_coords not in occupied:
                    dest_room.coords = ideal_coords
                    occupied[ideal_coords] = dest_vnum
                else:
                    # Position occupied - extend and record for pass 2
                    multiplier = 1
                    expected_coords = ideal_coords
                    while expected_coords in occupied and multiplier < 20:
                        multiplier += 1
                        expected_coords = (cx + dx * multiplier, cy + dy * multiplier, cz + dz)

                    dest_room.coords = expected_coords
                    occupied[expected_coords] = dest_vnum
                    extended[dest_vnum] = (current_vnum, direction, ideal_coords)

                parent[dest_vnum] = current_vnum
                queue.append(dest_vnum)
            elif dest_room.coords != ideal_coords:
                exit_obj.warped = True
                conflicts.append({
                    'from_vnum': current_vnum,
                    'to_vnum': dest_vnum,
                    'direction': DIR_NAMES[direction],
                    'expected': ideal_coords,
                    'actual': dest_room.coords,
                })

    # Pass 2: Try to fix extended rooms by relocating blockers
    for dest_vnum, (_, direction, ideal_coords) in extended.items():
        dest_room = area.rooms[dest_vnum]
        if ideal_coords in occupied:
            blocker_vnum = occupied[ideal_coords]
            blocker_room = area.rooms[blocker_vnum]

            # Check if blocker has fewer connections than the room we're trying to place
            dest_exits = sum(1 for e in dest_room.exits.values() if e.destination_vnum in area.rooms)
            blocker_exits = sum(1 for e in blocker_room.exits.values() if e.destination_vnum in area.rooms)

            # If dest has fewer exits (e.g., dead-end like Bakery), it deserves the ideal spot
            if dest_exits < blocker_exits:
                # Find new position for blocker - shift perpendicular to the direction
                dx, dy, dz = DIR_OFFSETS[direction]
                if direction in (DIR_NORTH, DIR_SOUTH):
                    shift = (1, 0, 0)  # Shift east
                else:
                    shift = (0, 1, 0)  # Shift north

                # Find empty spot for blocker
                bx, by, bz = ideal_coords
                new_blocker_coords = (bx + shift[0], by + shift[1], bz + shift[2])
                attempts = 0
                while new_blocker_coords in occupied and attempts < 10:
                    new_blocker_coords = (new_blocker_coords[0] + shift[0],
                                         new_blocker_coords[1] + shift[1],
                                         new_blocker_coords[2] + shift[2])
                    attempts += 1

                if new_blocker_coords not in occupied:
                    # Move blocker
                    old_coords = blocker_room.coords
                    del occupied[old_coords]
                    blocker_room.coords = new_blocker_coords
                    occupied[new_blocker_coords] = blocker_vnum

                    # Move dest to ideal position
                    old_dest_coords = dest_room.coords
                    del occupied[old_dest_coords]
                    dest_room.coords = ideal_coords
                    occupied[ideal_coords] = dest_vnum

    # Pass 3: Detect spine conflicts and shift extended rooms away
    # If an extended room lands on the main spine (x=0), move it to a non-conflicting position
    # The key insight is: if a room was extended (because its ideal position was occupied),
    # and it ended up at x=0, we should extend it further in the same direction instead
    for dest_vnum, (from_vnum, direction, ideal_coords) in extended.items():
        dest_room = area.rooms[dest_vnum]
        if dest_room.coords is None:
            continue

        dx_coord, dy_coord, dz_coord = dest_room.coords
        from_room = area.rooms.get(from_vnum)
        if not from_room or not from_room.coords:
            continue

        fx, _, _ = from_room.coords

        # Extended room is on spine (x=0), coming from off-spine via E/W direction
        # This creates a visual conflict - shift it further in the same direction
        if dx_coord == 0 and fx != 0 and direction in (DIR_EAST, DIR_WEST):
            ddx, ddy, ddz = DIR_OFFSETS[direction]
            # Find an empty position further in the same direction (away from x=0)
            multiplier = 1
            new_coords = (dx_coord + ddx * multiplier, dy_coord + ddy * multiplier, dz_coord + ddz * multiplier)
            while new_coords in occupied and multiplier < 10:
                multiplier += 1
                new_coords = (dx_coord + ddx * multiplier, dy_coord + ddy * multiplier, dz_coord + ddz * multiplier)

            if new_coords not in occupied:
                old_coords = dest_room.coords
                del occupied[old_coords]
                dest_room.coords = new_coords
                occupied[new_coords] = dest_vnum

    # Pass 4: Recalculate warped flags now that positions may have changed
    # "Warped" means the direction doesn't match - e.g., going North lands you
    # somewhere that's not north of your current position. Extended distances
    # (going North lands you 3 tiles north instead of 1) are NOT warped.
    for room in area.rooms.values():
        for exit_obj in room.exits.values():
            exit_obj.warped = False

    conflicts = []  # Reset conflicts list
    for room in area.rooms.values():
        if room.coords is None:
            continue
        cx, cy, cz = room.coords
        for direction, exit_obj in room.exits.items():
            dest_vnum = exit_obj.destination_vnum
            if dest_vnum not in area.rooms:
                continue
            dest_room = area.rooms[dest_vnum]
            if dest_room.coords is None:
                continue

            dest_x, dest_y, dest_z = dest_room.coords
            dx, dy, dz = DIR_OFFSETS[direction]

            # Check if the direction is correct (ignoring distance)
            # For horizontal/vertical directions, check the sign matches
            actual_dx = dest_x - cx
            actual_dy = dest_y - cy
            actual_dz = dest_z - cz

            # Direction is warped if the sign doesn't match OR if we moved
            # in a perpendicular direction when we shouldn't have
            is_warped = False

            if direction in (DIR_NORTH, DIR_SOUTH):
                # N/S movement: dy should have correct sign, dx should be 0
                if dy * dz != 0:  # Can't have both dy and dz
                    pass
                if (dy > 0) != (actual_dy > 0) or (dy < 0) != (actual_dy < 0):
                    is_warped = True  # Wrong vertical direction
                elif actual_dy == 0 and dy != 0:
                    is_warped = True  # Didn't move vertically when we should
                elif actual_dx != 0:
                    is_warped = True  # Moved horizontally when going N/S
            elif direction in (DIR_EAST, DIR_WEST):
                # E/W movement: dx should have correct sign, dy should be 0
                if (dx > 0) != (actual_dx > 0) or (dx < 0) != (actual_dx < 0):
                    is_warped = True  # Wrong horizontal direction
                elif actual_dx == 0 and dx != 0:
                    is_warped = True  # Didn't move horizontally when we should
                elif actual_dy != 0:
                    is_warped = True  # Moved vertically when going E/W
            elif direction in (DIR_UP, DIR_DOWN):
                # U/D movement: dz should have correct sign
                if (dz > 0) != (actual_dz > 0) or (dz < 0) != (actual_dz < 0):
                    is_warped = True  # Wrong z direction
                elif actual_dz == 0 and dz != 0:
                    is_warped = True  # Didn't move in z when we should

            if is_warped:
                exit_obj.warped = True
                conflicts.append({
                    'from_vnum': room.vnum,
                    'to_vnum': dest_vnum,
                    'direction': DIR_NAMES[direction],
                    'expected_dir': (dx, dy, dz),
                    'actual_offset': (actual_dx, actual_dy, actual_dz),
                })

    # Handle disconnected rooms - place them in a separate region
    disconnected_x = max((c[0] for c in occupied.keys()), default=0) + 3
    for room in area.rooms.values():
        if room.coords is None:
            while (disconnected_x, 0, 0) in occupied:
                disconnected_x += 1
            room.coords = (disconnected_x, 0, 0)
            occupied[room.coords] = room.vnum
            disconnected_x += 1

    return conflicts


def find_cross_area_connections(areas: Dict[str, Area]) -> List[dict]:
    """Find connections between different areas."""
    connections = []

    # Build area lookup by vnum range
    def find_area_for_vnum(vnum: int) -> Optional[str]:
        for area_id, area in areas.items():
            if area.lvnum <= vnum <= area.uvnum:
                return area_id
        return None

    for area_id, area in areas.items():
        for room in area.rooms.values():
            for direction, exit_obj in room.exits.items():
                dest_vnum = exit_obj.destination_vnum

                # Check if destination is outside this area's vnum range
                if not (area.lvnum <= dest_vnum <= area.uvnum):
                    dest_area_id = find_area_for_vnum(dest_vnum)
                    if dest_area_id and dest_area_id != area_id:
                        connections.append({
                            'from_area': area_id,
                            'from_room': room.vnum,
                            'to_area': dest_area_id,
                            'to_room': dest_vnum,
                            'direction': DIR_NAMES[direction],
                            'one_way': exit_obj.one_way,
                        })

    return connections


def assign_area_coordinates(areas: Dict[str, Area], connections: List[dict]) -> Dict[str, Tuple[int, int]]:
    """
    Assign X/Y coordinates to areas based on directional connections.
    Uses BFS from midgaard (central hub) outward.
    """
    # Direction name to offset mapping
    dir_offsets = {
        'north': (0, 1),
        'south': (0, -1),
        'east': (1, 0),
        'west': (-1, 0),
        'up': (0, 0),    # Ignore vertical for 2D
        'down': (0, 0),
    }

    # Build bidirectional adjacency with directions
    # area_connections[area] = [(neighbor_area, direction), ...]
    # Store both directions - if A->B is east, B->A is west
    reverse_dirs = {
        'north': 'south',
        'south': 'north',
        'east': 'west',
        'west': 'east',
        'up': 'down',
        'down': 'up',
    }
    area_connections: Dict[str, List[Tuple[str, str]]] = defaultdict(list)
    for conn in connections:
        from_area = conn['from_area']
        to_area = conn['to_area']
        direction = conn['direction']
        # Add forward direction
        area_connections[from_area].append((to_area, direction))
        # Add reverse direction for bidirectional traversal
        area_connections[to_area].append((from_area, reverse_dirs.get(direction, 'unknown')))

    coords: Dict[str, Tuple[int, int]] = {}

    # Start from midgaard if it exists, otherwise first area
    start_area = 'midgaard' if 'midgaard' in areas else next(iter(areas.keys()), None)
    if not start_area:
        return coords

    coords[start_area] = (0, 0)
    visited: Set[str] = set()
    queue = deque([start_area])

    while queue:
        current = queue.popleft()
        if current in visited:
            continue
        visited.add(current)

        cx, cy = coords.get(current, (0, 0))

        for dest_area, direction in area_connections.get(current, []):
            if dest_area in coords:
                continue  # Already positioned

            dx, dy = dir_offsets.get(direction, (0, 0))
            if dx == 0 and dy == 0:
                continue  # Skip up/down

            new_x, new_y = cx + dx, cy + dy

            # Check for collision and spiral outward if needed
            occupied = set(coords.values())
            if (new_x, new_y) in occupied:
                # Spiral search for empty spot
                for radius in range(1, 20):
                    for sx in range(-radius, radius + 1):
                        for sy in range(-radius, radius + 1):
                            if abs(sx) == radius or abs(sy) == radius:
                                test_x, test_y = new_x + sx, new_y + sy
                                if (test_x, test_y) not in occupied:
                                    new_x, new_y = test_x, test_y
                                    break
                        else:
                            continue
                        break
                    else:
                        continue
                    break

            coords[dest_area] = (new_x, new_y)
            queue.append(dest_area)

    # Assign coordinates to unvisited areas (disconnected)
    next_x = max((c[0] for c in coords.values()), default=0) + 2
    for area_id in areas:
        if area_id not in coords:
            coords[area_id] = (next_x, 0)
            next_x += 2

    return coords


def generate_world_graph(areas: Dict[str, Area], connections: List[dict]) -> dict:
    """Generate simplified area-to-area graph with coordinates."""
    # Calculate area positions based on connections
    area_coords = assign_area_coordinates(areas, connections)

    nodes = []
    for area_id, area in areas.items():
        x, y = area_coords.get(area_id, (0, 0))
        nodes.append({
            'id': area_id,
            'name': area.name,
            'filename': area.filename,
            'room_count': len(area.rooms),
            'vnum_range': [area.lvnum, area.uvnum],
            'x': x,
            'y': y,
        })

    # Aggregate connections between areas, tracking primary direction
    edge_data: Dict[Tuple[str, str], dict] = {}
    for conn in connections:
        key = (conn['from_area'], conn['to_area'])
        if key not in edge_data:
            edge_data[key] = {
                'count': 0,
                'one_way_count': 0,
                'directions': defaultdict(int)
            }
        edge_data[key]['count'] += 1
        edge_data[key]['directions'][conn['direction']] += 1
        if conn['one_way']:
            edge_data[key]['one_way_count'] += 1

    edges = []
    for (from_a, to_a), data in edge_data.items():
        # Find most common direction
        primary_dir = max(data['directions'], key=data['directions'].get) if data['directions'] else 'unknown'
        edges.append({
            'from': from_a,
            'to': to_a,
            'connection_count': data['count'],
            'one_way_count': data['one_way_count'],
            'direction': primary_dir,
        })

    return {'nodes': nodes, 'edges': edges}


def main():
    parser = argparse.ArgumentParser(description='Generate MUD map data from area files')
    parser.add_argument('--area', type=str, help='Generate for a single area (by filename without .are)')
    parser.add_argument('--all', action='store_true', default=True, help='Generate for all areas (default)')
    parser.add_argument('--output', type=str, default='tools/webmap/data', help='Output directory')
    parser.add_argument('--watch', action='store_true', help='Watch for file changes')
    args = parser.parse_args()

    # Determine paths
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    area_dir = project_root / 'area'
    output_dir = project_root / args.output

    # Create output directory
    output_dir.mkdir(parents=True, exist_ok=True)

    # Read area list
    area_list_path = area_dir / 'area.lst'
    if not area_list_path.exists():
        print(f"Error: {area_list_path} not found")
        return 1

    with open(area_list_path, 'r') as f:
        area_files = [line.strip() for line in f if line.strip() and not line.startswith('$')]

    # Filter if single area requested
    if args.area:
        target = args.area if args.area.endswith('.are') else f"{args.area}.are"
        area_files = [f for f in area_files if f == target]
        if not area_files:
            print(f"Error: Area '{args.area}' not found in area.lst")
            return 1

    print(f"Parsing {len(area_files)} area files...")

    # Parse all areas
    areas: Dict[str, Area] = {}
    all_conflicts = []

    for area_file in area_files:
        filepath = area_dir / area_file
        if not filepath.exists():
            print(f"  Warning: {area_file} not found, skipping")
            continue

        area = parse_area_file(filepath)
        if area:
            area_id = filepath.stem  # Use filename without extension as ID
            areas[area_id] = area
            print(f"  {area_file}: {len(area.rooms)} rooms - {area.name}")

    print(f"\nAnalyzing {len(areas)} areas...")

    # Analyze one-way exits
    analyze_one_way_exits(areas)

    # Assign coordinates and detect non-Euclidean geometry
    for area_id, area in areas.items():
        conflicts = assign_coordinates(area)
        if conflicts:
            all_conflicts.extend(conflicts)
            print(f"  {area_id}: {len(conflicts)} non-Euclidean connections")

    # Find cross-area connections
    cross_area = find_cross_area_connections(areas)
    print(f"\nFound {len(cross_area)} cross-area connections")

    # Generate world graph
    world_graph = generate_world_graph(areas, cross_area)

    # Count statistics
    total_rooms = sum(len(a.rooms) for a in areas.values())
    total_exits = sum(sum(len(r.exits) for r in a.rooms.values()) for a in areas.values())
    one_way_count = sum(
        sum(1 for r in a.rooms.values() for e in r.exits.values() if e.one_way)
        for a in areas.values()
    )
    dead_end_count = sum(
        sum(1 for r in a.rooms.values() if r.dead_end)
        for a in areas.values()
    )

    print(f"\nStatistics:")
    print(f"  Total areas: {len(areas)}")
    print(f"  Total rooms: {total_rooms}")
    print(f"  Total exits: {total_exits}")
    print(f"  One-way exits: {one_way_count}")
    print(f"  Dead-end rooms: {dead_end_count}")
    print(f"  Non-Euclidean conflicts: {len(all_conflicts)}")

    # Write output files
    areas_json = {area_id: area.to_dict() for area_id, area in areas.items()}

    areas_path = output_dir / 'areas.json'
    with open(areas_path, 'w') as f:
        json.dump(areas_json, f, indent=2)
    print(f"\nWritten: {areas_path}")

    world_path = output_dir / 'world_graph.json'
    with open(world_path, 'w') as f:
        json.dump(world_graph, f, indent=2)
    print(f"Written: {world_path}")

    # Write conflicts for debugging
    if all_conflicts:
        conflicts_path = output_dir / 'conflicts.json'
        with open(conflicts_path, 'w') as f:
            json.dump(all_conflicts, f, indent=2)
        print(f"Written: {conflicts_path}")

    print("\nDone!")
    return 0


if __name__ == '__main__':
    exit(main())
