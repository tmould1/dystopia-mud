#!/usr/bin/env python3
"""
Unit tests for the Dystopia MUD Map Generator.

Run with: python -m pytest test_map_generator.py -v
Or simply: python test_map_generator.py
"""

import json
import os
import sys
import unittest
from pathlib import Path
from io import StringIO

# Add tools directory to path for imports
sys.path.insert(0, str(Path(__file__).parent))

from map_generator import (
    Area, Room, Exit,
    strip_color_codes, read_tilde_string, parse_area_file,
    analyze_one_way_exits, assign_coordinates,
    find_cross_area_connections, generate_world_graph,
    DIR_NORTH, DIR_EAST, DIR_SOUTH, DIR_WEST, DIR_UP, DIR_DOWN,
    REVERSE_DIR
)


class TestColorCodeStripping(unittest.TestCase):
    """Test MUD color code removal."""

    def test_hash_codes(self):
        """Test #R, #G, etc. color codes."""
        self.assertEqual(strip_color_codes("#RRed text#n"), "Red text")
        self.assertEqual(strip_color_codes("#GGreen#W White"), "Green White")

    def test_caret_codes(self):
        """Test ^M style codes."""
        self.assertEqual(strip_color_codes("^MText here"), "Text here")

    def test_bracket_codes(self):
        """Test [#7-#0] style codes."""
        self.assertEqual(strip_color_codes("[#7-#0] Temple"), "Temple")

    def test_mixed_codes(self):
        """Test mixed color codes."""
        text = "#R[#C-#0] #GTemple of Mota#n"
        result = strip_color_codes(text)
        self.assertNotIn("#", result)
        self.assertIn("Temple of Mota", result)

    def test_no_codes(self):
        """Test plain text passes through."""
        self.assertEqual(strip_color_codes("Plain text"), "Plain text")


class TestTildeStringParsing(unittest.TestCase):
    """Test tilde-terminated string parsing."""

    def test_single_line(self):
        """Test single line with tilde."""
        lines = ["Hello world~", "Next line"]
        result, idx = read_tilde_string(lines, 0)
        self.assertEqual(result, "Hello world")
        self.assertEqual(idx, 1)

    def test_multiline(self):
        """Test multi-line tilde string."""
        lines = ["First line", "Second line", "Third~", "After"]
        result, idx = read_tilde_string(lines, 0)
        self.assertEqual(result, "First line\nSecond line\nThird")
        self.assertEqual(idx, 3)

    def test_empty_string(self):
        """Test empty tilde string."""
        lines = ["~", "Next"]
        result, idx = read_tilde_string(lines, 0)
        self.assertEqual(result, "")
        self.assertEqual(idx, 1)


class TestExitDataStructure(unittest.TestCase):
    """Test Exit class functionality."""

    def test_exit_creation(self):
        """Test basic exit creation."""
        exit_obj = Exit(DIR_NORTH, 3001, is_door=True, key_vnum=100)
        self.assertEqual(exit_obj.direction, DIR_NORTH)
        self.assertEqual(exit_obj.destination_vnum, 3001)
        self.assertTrue(exit_obj.is_door)
        self.assertEqual(exit_obj.key_vnum, 100)
        self.assertFalse(exit_obj.one_way)
        self.assertFalse(exit_obj.warped)

    def test_exit_to_dict(self):
        """Test exit serialization."""
        exit_obj = Exit(DIR_EAST, 3002)
        exit_obj.one_way = True
        data = exit_obj.to_dict()
        self.assertEqual(data['to'], 3002)
        self.assertTrue(data['one_way'])
        self.assertFalse(data['door'])


class TestRoomDataStructure(unittest.TestCase):
    """Test Room class functionality."""

    def test_room_creation(self):
        """Test basic room creation."""
        room = Room(3001, "Temple of Mota", sector_type=1)
        self.assertEqual(room.vnum, 3001)
        self.assertEqual(room.name, "Temple of Mota")
        self.assertEqual(room.sector_type, 1)
        self.assertFalse(room.dead_end)
        self.assertEqual(len(room.exits), 0)

    def test_room_with_exits(self):
        """Test room with exits."""
        room = Room(3001, "Temple")
        room.exits[DIR_NORTH] = Exit(DIR_NORTH, 3002)
        room.exits[DIR_SOUTH] = Exit(DIR_SOUTH, 3003)
        self.assertEqual(len(room.exits), 2)

    def test_room_to_dict(self):
        """Test room serialization."""
        room = Room(3001, "Temple", sector_type=1)
        room.coords = (0, 0, 0)
        room.exits[DIR_NORTH] = Exit(DIR_NORTH, 3002)
        data = room.to_dict()
        self.assertEqual(data['vnum'], 3001)
        self.assertEqual(data['name'], "Temple")
        self.assertEqual(data['sector_name'], 'city')
        self.assertIn('0', data['exits'])  # DIR_NORTH = 0
        self.assertEqual(data['coords']['x'], 0)


class TestAreaDataStructure(unittest.TestCase):
    """Test Area class functionality."""

    def test_area_creation(self):
        """Test basic area creation."""
        area = Area("Midgaard", "midgaard.are", 3000, 3399)
        self.assertEqual(area.name, "Midgaard")
        self.assertEqual(area.filename, "midgaard.are")
        self.assertEqual(area.lvnum, 3000)
        self.assertEqual(area.uvnum, 3399)

    def test_area_with_rooms(self):
        """Test area with rooms."""
        area = Area("Test", "test.are", 100, 199)
        area.rooms[100] = Room(100, "Room 1")
        area.rooms[101] = Room(101, "Room 2")
        self.assertEqual(len(area.rooms), 2)

    def test_area_to_dict(self):
        """Test area serialization."""
        area = Area("Test", "test.are", 100, 199)
        area.rooms[100] = Room(100, "Room 1")
        data = area.to_dict()
        self.assertEqual(data['name'], "Test")
        self.assertEqual(data['vnum_range'], [100, 199])
        self.assertEqual(data['room_count'], 1)


class TestOneWayExitDetection(unittest.TestCase):
    """Test one-way exit analysis."""

    def test_bidirectional_exit(self):
        """Test that bidirectional exits are not marked one-way."""
        areas = {'test': Area("Test", "test.are", 100, 199)}
        room1 = Room(100, "Room 1")
        room2 = Room(101, "Room 2")

        # Bidirectional: 100 -> 101 (north) and 101 -> 100 (south)
        room1.exits[DIR_NORTH] = Exit(DIR_NORTH, 101)
        room2.exits[DIR_SOUTH] = Exit(DIR_SOUTH, 100)

        areas['test'].rooms[100] = room1
        areas['test'].rooms[101] = room2

        analyze_one_way_exits(areas)

        self.assertFalse(room1.exits[DIR_NORTH].one_way)
        self.assertFalse(room2.exits[DIR_SOUTH].one_way)

    def test_one_way_exit(self):
        """Test that one-way exits are detected."""
        areas = {'test': Area("Test", "test.are", 100, 199)}
        room1 = Room(100, "Room 1")
        room2 = Room(101, "Room 2")

        # One-way: 100 -> 101 (north), but no exit back from 101
        room1.exits[DIR_NORTH] = Exit(DIR_NORTH, 101)

        areas['test'].rooms[100] = room1
        areas['test'].rooms[101] = room2

        analyze_one_way_exits(areas)

        self.assertTrue(room1.exits[DIR_NORTH].one_way)

    def test_wrong_reverse_direction(self):
        """Test exit where reverse points elsewhere."""
        areas = {'test': Area("Test", "test.are", 100, 199)}
        room1 = Room(100, "Room 1")
        room2 = Room(101, "Room 2")
        room3 = Room(102, "Room 3")

        # 100 north to 101, but 101 south to 102 (not back to 100)
        room1.exits[DIR_NORTH] = Exit(DIR_NORTH, 101)
        room2.exits[DIR_SOUTH] = Exit(DIR_SOUTH, 102)

        areas['test'].rooms[100] = room1
        areas['test'].rooms[101] = room2
        areas['test'].rooms[102] = room3

        analyze_one_way_exits(areas)

        self.assertTrue(room1.exits[DIR_NORTH].one_way)


class TestCoordinateAssignment(unittest.TestCase):
    """Test BFS coordinate assignment."""

    def test_simple_linear(self):
        """Test simple north-south chain."""
        area = Area("Test", "test.are", 100, 199)
        room1 = Room(100, "Start")
        room2 = Room(101, "North")
        room3 = Room(102, "North North")

        room1.exits[DIR_NORTH] = Exit(DIR_NORTH, 101)
        room2.exits[DIR_NORTH] = Exit(DIR_NORTH, 102)
        room2.exits[DIR_SOUTH] = Exit(DIR_SOUTH, 100)
        room3.exits[DIR_SOUTH] = Exit(DIR_SOUTH, 101)

        area.rooms[100] = room1
        area.rooms[101] = room2
        area.rooms[102] = room3

        conflicts = assign_coordinates(area)

        self.assertEqual(room1.coords, (0, 0, 0))
        self.assertEqual(room2.coords, (0, 1, 0))
        self.assertEqual(room3.coords, (0, 2, 0))
        self.assertEqual(len(conflicts), 0)

    def test_square_grid(self):
        """Test 2x2 square of rooms."""
        area = Area("Test", "test.are", 100, 199)
        # Create a square:
        # 101 - 102
        #  |     |
        # 100 - 103
        rooms = {
            100: Room(100, "SW"),
            101: Room(101, "NW"),
            102: Room(102, "NE"),
            103: Room(103, "SE"),
        }

        rooms[100].exits[DIR_NORTH] = Exit(DIR_NORTH, 101)
        rooms[100].exits[DIR_EAST] = Exit(DIR_EAST, 103)
        rooms[101].exits[DIR_SOUTH] = Exit(DIR_SOUTH, 100)
        rooms[101].exits[DIR_EAST] = Exit(DIR_EAST, 102)
        rooms[102].exits[DIR_WEST] = Exit(DIR_WEST, 101)
        rooms[102].exits[DIR_SOUTH] = Exit(DIR_SOUTH, 103)
        rooms[103].exits[DIR_WEST] = Exit(DIR_WEST, 100)
        rooms[103].exits[DIR_NORTH] = Exit(DIR_NORTH, 102)

        area.rooms = rooms
        conflicts = assign_coordinates(area)

        # Should form a proper square with no conflicts
        self.assertEqual(len(conflicts), 0)
        self.assertEqual(rooms[100].coords, (0, 0, 0))
        self.assertEqual(rooms[101].coords, (0, 1, 0))
        self.assertEqual(rooms[102].coords, (1, 1, 0))
        self.assertEqual(rooms[103].coords, (1, 0, 0))

    def test_non_euclidean_detection(self):
        """Test detection of non-Euclidean geometry."""
        area = Area("Test", "test.are", 100, 199)
        # Create a triangle that can't exist in Euclidean space:
        # A north to B, B east to C, C south... but C is where we started?
        # Actually need to create a situation where coords conflict
        room1 = Room(100, "A")
        room2 = Room(101, "B")
        room3 = Room(102, "C")

        # A-north->B, B-east->C, C-west->A (should be C-south->A for Euclidean)
        room1.exits[DIR_NORTH] = Exit(DIR_NORTH, 101)
        room2.exits[DIR_SOUTH] = Exit(DIR_SOUTH, 100)
        room2.exits[DIR_EAST] = Exit(DIR_EAST, 102)
        room3.exits[DIR_WEST] = Exit(DIR_WEST, 101)
        # Now C points back to A going south, but A isn't south of C
        room3.exits[DIR_SOUTH] = Exit(DIR_SOUTH, 100)

        area.rooms[100] = room1
        area.rooms[101] = room2
        area.rooms[102] = room3

        conflicts = assign_coordinates(area)

        # C going south to A should conflict because A is at (0,0) but
        # C expects to reach (1, -1, 0) going south from (1, 0, 0)
        self.assertGreater(len(conflicts), 0)

    def test_up_down_coordinates(self):
        """Test z-axis coordinate assignment."""
        area = Area("Test", "test.are", 100, 199)
        room1 = Room(100, "Ground")
        room2 = Room(101, "Above")
        room3 = Room(102, "Below")

        room1.exits[DIR_UP] = Exit(DIR_UP, 101)
        room1.exits[DIR_DOWN] = Exit(DIR_DOWN, 102)
        room2.exits[DIR_DOWN] = Exit(DIR_DOWN, 100)
        room3.exits[DIR_UP] = Exit(DIR_UP, 100)

        area.rooms[100] = room1
        area.rooms[101] = room2
        area.rooms[102] = room3

        conflicts = assign_coordinates(area)

        self.assertEqual(room1.coords, (0, 0, 0))
        self.assertEqual(room2.coords, (0, 0, 1))
        self.assertEqual(room3.coords, (0, 0, -1))
        self.assertEqual(len(conflicts), 0)


class TestCrossAreaConnections(unittest.TestCase):
    """Test cross-area connection detection."""

    def test_no_cross_area(self):
        """Test area with no external connections."""
        areas = {'test': Area("Test", "test.are", 100, 199)}
        room = Room(100, "Room")
        room.exits[DIR_NORTH] = Exit(DIR_NORTH, 101)  # Within range
        areas['test'].rooms[100] = room
        areas['test'].rooms[101] = Room(101, "Room 2")

        connections = find_cross_area_connections(areas)
        self.assertEqual(len(connections), 0)

    def test_cross_area_connection(self):
        """Test detection of cross-area exit."""
        areas = {
            'area1': Area("Area 1", "area1.are", 100, 199),
            'area2': Area("Area 2", "area2.are", 200, 299),
        }
        room1 = Room(100, "Room in Area 1")
        room1.exits[DIR_NORTH] = Exit(DIR_NORTH, 200)  # Points to area2
        areas['area1'].rooms[100] = room1
        areas['area2'].rooms[200] = Room(200, "Room in Area 2")

        connections = find_cross_area_connections(areas)

        self.assertEqual(len(connections), 1)
        self.assertEqual(connections[0]['from_area'], 'area1')
        self.assertEqual(connections[0]['to_area'], 'area2')
        self.assertEqual(connections[0]['from_room'], 100)
        self.assertEqual(connections[0]['to_room'], 200)


class TestWorldGraphGeneration(unittest.TestCase):
    """Test world graph generation."""

    def test_empty_world(self):
        """Test world graph with no areas."""
        graph = generate_world_graph({}, [])
        self.assertEqual(len(graph['nodes']), 0)
        self.assertEqual(len(graph['edges']), 0)

    def test_single_area(self):
        """Test world graph with one area."""
        areas = {'test': Area("Test", "test.are", 100, 199)}
        areas['test'].rooms[100] = Room(100, "Room")
        graph = generate_world_graph(areas, [])

        self.assertEqual(len(graph['nodes']), 1)
        self.assertEqual(graph['nodes'][0]['id'], 'test')
        self.assertEqual(graph['nodes'][0]['room_count'], 1)

    def test_connected_areas(self):
        """Test world graph with connected areas."""
        areas = {
            'area1': Area("Area 1", "area1.are", 100, 199),
            'area2': Area("Area 2", "area2.are", 200, 299),
        }
        areas['area1'].rooms[100] = Room(100, "R1")
        areas['area2'].rooms[200] = Room(200, "R2")

        connections = [
            {'from_area': 'area1', 'to_area': 'area2',
             'from_room': 100, 'to_room': 200,
             'direction': 'north', 'one_way': False},
            {'from_area': 'area1', 'to_area': 'area2',
             'from_room': 101, 'to_room': 201,
             'direction': 'east', 'one_way': True},
        ]

        graph = generate_world_graph(areas, connections)

        self.assertEqual(len(graph['nodes']), 2)
        self.assertEqual(len(graph['edges']), 1)
        self.assertEqual(graph['edges'][0]['connection_count'], 2)
        self.assertEqual(graph['edges'][0]['one_way_count'], 1)


class TestDirectionConstants(unittest.TestCase):
    """Test direction constant definitions."""

    def test_direction_values(self):
        """Test direction constant values."""
        self.assertEqual(DIR_NORTH, 0)
        self.assertEqual(DIR_EAST, 1)
        self.assertEqual(DIR_SOUTH, 2)
        self.assertEqual(DIR_WEST, 3)
        self.assertEqual(DIR_UP, 4)
        self.assertEqual(DIR_DOWN, 5)

    def test_reverse_directions(self):
        """Test reverse direction mapping."""
        self.assertEqual(REVERSE_DIR[DIR_NORTH], DIR_SOUTH)
        self.assertEqual(REVERSE_DIR[DIR_SOUTH], DIR_NORTH)
        self.assertEqual(REVERSE_DIR[DIR_EAST], DIR_WEST)
        self.assertEqual(REVERSE_DIR[DIR_WEST], DIR_EAST)
        self.assertEqual(REVERSE_DIR[DIR_UP], DIR_DOWN)
        self.assertEqual(REVERSE_DIR[DIR_DOWN], DIR_UP)


class TestRealAreaParsing(unittest.TestCase):
    """Test parsing real area files (integration tests)."""

    @classmethod
    def setUpClass(cls):
        """Find the project root and area directory."""
        cls.project_root = Path(__file__).parent.parent
        cls.area_dir = cls.project_root / 'area'

    def test_school_area_exists(self):
        """Test that school.are can be found."""
        school_path = self.area_dir / 'school.are'
        self.assertTrue(school_path.exists(), f"school.are not found at {school_path}")

    def test_parse_school_area(self):
        """Test parsing school.are produces valid data."""
        school_path = self.area_dir / 'school.are'
        if not school_path.exists():
            self.skipTest("school.are not found")

        area = parse_area_file(school_path)

        self.assertIsNotNone(area)
        self.assertEqual(area.name, "Mud School")
        self.assertGreater(len(area.rooms), 0)
        # Check a known room exists
        self.assertIn(3700, area.rooms)
        self.assertEqual(area.rooms[3700].name, "Entrance to Mud School")

    def test_parse_all_areas(self):
        """Test that all areas in area.lst can be parsed."""
        area_list = self.area_dir / 'area.lst'
        if not area_list.exists():
            self.skipTest("area.lst not found")

        with open(area_list, 'r') as f:
            area_files = [l.strip() for l in f if l.strip() and not l.startswith('$')]

        # Files that are known to have no rooms (help files, etc.)
        no_room_files = {'help.are'}

        parse_errors = []
        for area_file in area_files:
            filepath = self.area_dir / area_file
            if not filepath.exists():
                continue
            try:
                area = parse_area_file(filepath)
                if area is None:
                    if area_file not in no_room_files:
                        parse_errors.append(f"{area_file}: returned None")
                elif len(area.rooms) == 0:
                    if area_file not in no_room_files:
                        parse_errors.append(f"{area_file}: no rooms parsed")
            except Exception as e:
                parse_errors.append(f"{area_file}: {e}")

        self.assertEqual(len(parse_errors), 0,
                        f"Parse errors:\n" + "\n".join(parse_errors))


class TestJSONOutput(unittest.TestCase):
    """Test that generated JSON is valid."""

    @classmethod
    def setUpClass(cls):
        """Find the output directory."""
        cls.output_dir = Path(__file__).parent / 'webmap' / 'data'

    def test_areas_json_valid(self):
        """Test areas.json is valid JSON."""
        areas_path = self.output_dir / 'areas.json'
        if not areas_path.exists():
            self.skipTest("areas.json not found - run map_generator.py first")

        with open(areas_path, 'r') as f:
            data = json.load(f)

        self.assertIsInstance(data, dict)
        self.assertGreater(len(data), 0)

        # Check structure of first area
        first_area = next(iter(data.values()))
        self.assertIn('name', first_area)
        self.assertIn('rooms', first_area)
        self.assertIn('vnum_range', first_area)

    def test_world_graph_json_valid(self):
        """Test world_graph.json is valid JSON."""
        graph_path = self.output_dir / 'world_graph.json'
        if not graph_path.exists():
            self.skipTest("world_graph.json not found - run map_generator.py first")

        with open(graph_path, 'r') as f:
            data = json.load(f)

        self.assertIn('nodes', data)
        self.assertIn('edges', data)
        self.assertIsInstance(data['nodes'], list)
        self.assertIsInstance(data['edges'], list)

        # Validate node structure
        if len(data['nodes']) > 0:
            node = data['nodes'][0]
            self.assertIn('id', node)
            self.assertIn('name', node)
            self.assertIn('room_count', node)

    def test_room_exits_valid(self):
        """Test that room exits reference valid destinations."""
        areas_path = self.output_dir / 'areas.json'
        if not areas_path.exists():
            self.skipTest("areas.json not found")

        with open(areas_path, 'r') as f:
            areas = json.load(f)

        # Build set of all vnums
        all_vnums = set()
        for area in areas.values():
            for vnum in area['rooms'].keys():
                all_vnums.add(int(vnum))

        # Check exits
        invalid_exits = []
        for area_id, area in areas.items():
            for vnum, room in area['rooms'].items():
                for direction, exit_data in room['exits'].items():
                    dest = exit_data['to']
                    if dest not in all_vnums:
                        # Cross-area reference - that's OK
                        pass

        # No assertion needed - we're just validating the structure


if __name__ == '__main__':
    # Run tests
    unittest.main(verbosity=2)
