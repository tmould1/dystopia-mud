"""
Map coordinate generator for Dystopia MUD.

Assigns X/Y/Z coordinates to rooms using BFS and detects non-Euclidean geometry.
"""

from collections import deque, defaultdict
from typing import Dict, List, Set, Tuple, Optional

from .models import (
    Area, Room,
    DIR_NORTH, DIR_EAST, DIR_SOUTH, DIR_WEST, DIR_UP, DIR_DOWN,
    DIR_NAMES, DIR_OFFSETS, REVERSE_DIR
)


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
    Pass 3: Detect spine conflicts and shift extended rooms away.
    Pass 4: Recalculate warped flags.
    Returns list of conflicts (non-Euclidean geometry).
    """
    if not area.rooms:
        return []

    conflicts = []
    occupied: Dict[Tuple[int, int, int], int] = {}  # coords -> vnum
    extended: Dict[int, Tuple[int, int, Tuple[int, int, int]]] = {}  # dest_vnum -> (from_vnum, direction, ideal_coords)

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
        if dx_coord == 0 and fx != 0 and direction in (DIR_EAST, DIR_WEST):
            ddx, ddy, ddz = DIR_OFFSETS[direction]
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

    # Pass 4: Recalculate warped flags
    for room in area.rooms.values():
        for exit_obj in room.exits.values():
            exit_obj.warped = False

    conflicts = []
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

            actual_dx = dest_x - cx
            actual_dy = dest_y - cy
            actual_dz = dest_z - cz

            is_warped = False

            if direction in (DIR_NORTH, DIR_SOUTH):
                if (dy > 0) != (actual_dy > 0) or (dy < 0) != (actual_dy < 0):
                    is_warped = True
                elif actual_dy == 0 and dy != 0:
                    is_warped = True
                elif actual_dx != 0:
                    is_warped = True
            elif direction in (DIR_EAST, DIR_WEST):
                if (dx > 0) != (actual_dx > 0) or (dx < 0) != (actual_dx < 0):
                    is_warped = True
                elif actual_dx == 0 and dx != 0:
                    is_warped = True
                elif actual_dy != 0:
                    is_warped = True
            elif direction in (DIR_UP, DIR_DOWN):
                if (dz > 0) != (actual_dz > 0) or (dz < 0) != (actual_dz < 0):
                    is_warped = True
                elif actual_dz == 0 and dz != 0:
                    is_warped = True

            if is_warped:
                exit_obj.warped = True
                conflicts.append({
                    'from_vnum': room.vnum,
                    'to_vnum': dest_vnum,
                    'direction': DIR_NAMES[direction],
                    'expected_dir': (dx, dy, dz),
                    'actual_offset': (actual_dx, actual_dy, actual_dz),
                })

    # Handle disconnected rooms
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

    def find_area_for_vnum(vnum: int) -> Optional[str]:
        for area_id, area in areas.items():
            if area.lvnum <= vnum <= area.uvnum:
                return area_id
        return None

    for area_id, area in areas.items():
        for room in area.rooms.values():
            for direction, exit_obj in room.exits.items():
                dest_vnum = exit_obj.destination_vnum

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
    """Assign X/Y coordinates to areas based on directional connections."""
    dir_offsets = {
        'north': (0, 1),
        'south': (0, -1),
        'east': (1, 0),
        'west': (-1, 0),
        'up': (0, 0),
        'down': (0, 0),
    }

    reverse_dirs = {
        'north': 'south', 'south': 'north',
        'east': 'west', 'west': 'east',
        'up': 'down', 'down': 'up',
    }

    area_connections: Dict[str, List[Tuple[str, str]]] = defaultdict(list)
    for conn in connections:
        from_area = conn['from_area']
        to_area = conn['to_area']
        direction = conn['direction']
        area_connections[from_area].append((to_area, direction))
        area_connections[to_area].append((from_area, reverse_dirs.get(direction, 'unknown')))

    coords: Dict[str, Tuple[int, int]] = {}

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
                continue

            dx, dy = dir_offsets.get(direction, (0, 0))
            if dx == 0 and dy == 0:
                continue

            new_x, new_y = cx + dx, cy + dy

            occupied = set(coords.values())
            if (new_x, new_y) in occupied:
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

    # Assign coordinates to unvisited areas
    next_x = max((c[0] for c in coords.values()), default=0) + 2
    for area_id in areas:
        if area_id not in coords:
            coords[area_id] = (next_x, 0)
            next_x += 2

    return coords
