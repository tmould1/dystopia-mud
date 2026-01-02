#!/usr/bin/env python3
"""
Dystopia MUD Data Generator

Parses area files and generates JSON data for web visualization and analysis.

Usage:
    python generate_data.py              # Generate all data
    python generate_data.py --area NAME  # Generate for single area
    python generate_data.py --maps-only  # Only generate map data
    python generate_data.py --analysis   # Only generate analysis data
"""

import argparse
from pathlib import Path

from mudlib import (
    parse_all_areas, parse_area_file,
    assign_coordinates, analyze_one_way_exits, find_cross_area_connections,
    analyze_area_difficulty
)
from mudlib.web_builder import write_json_files


def main():
    parser = argparse.ArgumentParser(description='Generate MUD data for web viewer')
    parser.add_argument('--area', type=str, help='Generate for a single area')
    parser.add_argument('--output', type=str, default='tools/webmap/data',
                       help='Output directory')
    parser.add_argument('--maps-only', action='store_true',
                       help='Only generate map data (areas.json, world_graph.json)')
    parser.add_argument('--analysis', action='store_true',
                       help='Only generate analysis data')
    args = parser.parse_args()

    # Determine paths
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    area_dir = project_root / 'area'
    output_dir = project_root / args.output

    # Parse areas
    if args.area:
        target = args.area if args.area.endswith('.are') else f"{args.area}.are"
        filepath = area_dir / target
        if not filepath.exists():
            print(f"Error: Area file '{target}' not found")
            return 1
        area = parse_area_file(filepath)
        if area is None:
            print(f"Error: Failed to parse '{target}'")
            return 1
        areas = {filepath.stem: area}
        print(f"Parsed: {target}")
        print(f"  Rooms: {len(area.rooms)}")
        print(f"  Mobs: {len(area.mobiles)}")
        print(f"  Objects: {len(area.objects)}")
        print(f"  Resets: {len(area.resets)}")
    else:
        print(f"Parsing area files from {area_dir}...")
        areas = parse_all_areas(area_dir)
        print(f"Parsed {len(areas)} areas")

    if not areas:
        print("No areas to process")
        return 1

    # Analyze one-way exits
    print("Analyzing one-way exits...")
    analyze_one_way_exits(areas)

    # Assign coordinates
    print("Assigning room coordinates...")
    all_conflicts = []
    for area_id, area in areas.items():
        conflicts = assign_coordinates(area)
        if conflicts:
            all_conflicts.extend(conflicts)

    # Find cross-area connections
    print("Finding cross-area connections...")
    connections = find_cross_area_connections(areas)

    # Print statistics
    total_rooms = sum(len(a.rooms) for a in areas.values())
    total_mobs = sum(len(a.mobiles) for a in areas.values())
    total_objects = sum(len(a.objects) for a in areas.values())
    total_resets = sum(len(a.resets) for a in areas.values())
    one_way_count = sum(
        sum(1 for r in a.rooms.values() for e in r.exits.values() if e.one_way)
        for a in areas.values()
    )

    print(f"\nStatistics:")
    print(f"  Total areas: {len(areas)}")
    print(f"  Total rooms: {total_rooms}")
    print(f"  Total mobs: {total_mobs}")
    print(f"  Total objects: {total_objects}")
    print(f"  Total resets: {total_resets}")
    print(f"  One-way exits: {one_way_count}")
    print(f"  Cross-area connections: {len(connections)}")
    print(f"  Non-Euclidean conflicts: {len(all_conflicts)}")

    # Write output files
    print(f"\nWriting output to {output_dir}...")
    write_json_files(output_dir, areas, connections)

    # Print area analysis summary
    if not args.maps_only:
        print("\nArea Difficulty Summary:")
        print("-" * 60)
        for area_id, area in sorted(areas.items(), key=lambda x: x[0]):
            if not area.mobiles:
                continue
            analysis = analyze_area_difficulty(area)
            print(f"  {area.name[:30]:<30} Lvl:{analysis.avg_mob_level:>5.1f}  "
                  f"Diff:{analysis.avg_difficulty:>6.1f}  "
                  f"Mobs:{analysis.mob_count:>3}")

    print("\nDone!")
    return 0


if __name__ == '__main__':
    exit(main())
