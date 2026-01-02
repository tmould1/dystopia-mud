"""
Dystopia MUD Library

A Python package for parsing, analyzing, and visualizing MUD area data.

Modules:
    models - Data classes for areas, rooms, mobs, objects, resets
    area_parser - Parse .are area files
    map_generator - Coordinate assignment for room maps
    area_analyzer - Calculate mob difficulty and object power ratings
    web_builder - Generate JSON for web visualization
"""

from .models import (
    Exit, Room, Mobile, Object, Reset, Shop, Area,
    DIR_NORTH, DIR_EAST, DIR_SOUTH, DIR_WEST, DIR_UP, DIR_DOWN,
    DIR_NAMES, REVERSE_DIR, DIR_OFFSETS,
    SECTOR_NAMES, ITEM_TYPES, WEAR_LOCATIONS, WEAPON_TYPES, ARMOR_SPECIALS,
    ACT_FLAGS, AFF_FLAGS, decode_flags
)
from .area_parser import parse_area_file, parse_all_areas
from .map_generator import (
    assign_coordinates, find_cross_area_connections,
    analyze_one_way_exits, assign_area_coordinates
)
from .area_analyzer import (
    calculate_mob_difficulty, calculate_object_power,
    analyze_area_difficulty, analyze_resets,
    get_mob_analysis, get_object_analysis,
    MobAnalysis, ObjectAnalysis, AreaAnalysis
)
from .web_builder import (
    build_areas_json, build_world_graph, build_analysis_json,
    build_mob_rankings, build_object_rankings, build_shop_listings,
    build_spawn_data, write_json_files
)

__version__ = '1.0.0'
