"""
Web JSON builder for Dystopia MUD.

Generates JSON files for the web-based map viewer and analysis pages.
"""

import json
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Any

from .models import Area, DIR_NAMES, WEAR_LOCATIONS
from .map_generator import assign_area_coordinates
from .area_analyzer import (
    analyze_area_difficulty, get_mob_analysis, get_object_analysis,
    MobAnalysis, ObjectAnalysis
)


def build_spawn_data(area: Area, all_areas: Dict[str, Area]) -> dict:
    """
    Build spawn data for an area, including cross-area references.

    Returns a dict with:
    - mob_spawns: list of mob spawn entries
    - obj_spawns: list of object spawn entries
    - cross_area_mobs: mobs from other areas spawned here
    - cross_area_objs: objects from other areas spawned here
    """
    # Build a global vnum->area lookup
    mob_to_area = {}
    obj_to_area = {}
    for aid, a in all_areas.items():
        for vnum in a.mobiles:
            mob_to_area[vnum] = aid
        for vnum in a.objects:
            obj_to_area[vnum] = aid

    mob_spawns = []
    obj_spawns = []
    cross_area_mobs = []
    cross_area_objs = []

    current_mob_vnum = None
    current_room_vnum = None

    for reset in area.resets:
        if reset.command == 'M':
            # Mob spawn: arg1=mob_vnum, arg2=max_count, arg3=room_vnum
            mob_vnum = reset.arg1
            max_count = reset.arg2
            room_vnum = reset.arg3

            current_mob_vnum = mob_vnum
            current_room_vnum = room_vnum

            # Get mob info
            mob_name = f"Unknown mob #{mob_vnum}"
            mob_level = 0
            source_area = mob_to_area.get(mob_vnum)

            if source_area and source_area in all_areas:
                src = all_areas[source_area]
                if mob_vnum in src.mobiles:
                    mob_name = src.mobiles[mob_vnum].short_descr
                    mob_level = src.mobiles[mob_vnum].level

            # Get room info
            room_name = f"Room #{room_vnum}"
            if room_vnum in area.rooms:
                room_name = area.rooms[room_vnum].name

            entry = {
                'type': 'mob',
                'vnum': mob_vnum,
                'name': mob_name,
                'level': mob_level,
                'max_count': max_count,
                'room_vnum': room_vnum,
                'room_name': room_name,
                'source_area': source_area,
                'is_cross_area': source_area != area.filename.replace('.are', ''),
            }

            mob_spawns.append(entry)
            if entry['is_cross_area']:
                cross_area_mobs.append(entry)

        elif reset.command == 'O':
            # Object in room: arg1=obj_vnum, arg3=room_vnum
            obj_vnum = reset.arg1
            room_vnum = reset.arg3

            current_room_vnum = room_vnum

            # Get object info
            obj_name = f"Unknown object #{obj_vnum}"
            source_area = obj_to_area.get(obj_vnum)

            if source_area and source_area in all_areas:
                src = all_areas[source_area]
                if obj_vnum in src.objects:
                    obj_name = src.objects[obj_vnum].short_descr

            # Get room info
            room_name = f"Room #{room_vnum}"
            if room_vnum in area.rooms:
                room_name = area.rooms[room_vnum].name

            entry = {
                'type': 'object_room',
                'vnum': obj_vnum,
                'name': obj_name,
                'room_vnum': room_vnum,
                'room_name': room_name,
                'source_area': source_area,
                'is_cross_area': source_area != area.filename.replace('.are', ''),
            }

            obj_spawns.append(entry)
            if entry['is_cross_area']:
                cross_area_objs.append(entry)

        elif reset.command == 'G':
            # Give object to mob: arg1=obj_vnum
            obj_vnum = reset.arg1

            obj_name = f"Unknown object #{obj_vnum}"
            source_area = obj_to_area.get(obj_vnum)

            if source_area and source_area in all_areas:
                src = all_areas[source_area]
                if obj_vnum in src.objects:
                    obj_name = src.objects[obj_vnum].short_descr

            # Get mob info for context
            mob_name = f"mob #{current_mob_vnum}"
            mob_source = mob_to_area.get(current_mob_vnum)
            if mob_source and mob_source in all_areas:
                src = all_areas[mob_source]
                if current_mob_vnum in src.mobiles:
                    mob_name = src.mobiles[current_mob_vnum].short_descr

            entry = {
                'type': 'object_inventory',
                'vnum': obj_vnum,
                'name': obj_name,
                'mob_vnum': current_mob_vnum,
                'mob_name': mob_name,
                'source_area': source_area,
                'is_cross_area': source_area != area.filename.replace('.are', ''),
            }

            obj_spawns.append(entry)
            if entry['is_cross_area']:
                cross_area_objs.append(entry)

        elif reset.command == 'E':
            # Equip object on mob: arg1=obj_vnum, arg4=wear_loc
            obj_vnum = reset.arg1
            wear_loc = reset.arg4

            obj_name = f"Unknown object #{obj_vnum}"
            source_area = obj_to_area.get(obj_vnum)

            if source_area and source_area in all_areas:
                src = all_areas[source_area]
                if obj_vnum in src.objects:
                    obj_name = src.objects[obj_vnum].short_descr

            # Get mob info for context
            mob_name = f"mob #{current_mob_vnum}"
            mob_source = mob_to_area.get(current_mob_vnum)
            if mob_source and mob_source in all_areas:
                src = all_areas[mob_source]
                if current_mob_vnum in src.mobiles:
                    mob_name = src.mobiles[current_mob_vnum].short_descr

            entry = {
                'type': 'object_equipped',
                'vnum': obj_vnum,
                'name': obj_name,
                'mob_vnum': current_mob_vnum,
                'mob_name': mob_name,
                'wear_loc': WEAR_LOCATIONS.get(wear_loc, f'loc_{wear_loc}'),
                'source_area': source_area,
                'is_cross_area': source_area != area.filename.replace('.are', ''),
            }

            obj_spawns.append(entry)
            if entry['is_cross_area']:
                cross_area_objs.append(entry)

        elif reset.command == 'P':
            # Put object in container: arg1=obj_vnum, arg3=container_vnum
            obj_vnum = reset.arg1
            container_vnum = reset.arg3

            obj_name = f"Unknown object #{obj_vnum}"
            source_area = obj_to_area.get(obj_vnum)

            if source_area and source_area in all_areas:
                src = all_areas[source_area]
                if obj_vnum in src.objects:
                    obj_name = src.objects[obj_vnum].short_descr

            # Get container info
            container_name = f"container #{container_vnum}"
            container_source = obj_to_area.get(container_vnum)
            if container_source and container_source in all_areas:
                src = all_areas[container_source]
                if container_vnum in src.objects:
                    container_name = src.objects[container_vnum].short_descr

            entry = {
                'type': 'object_container',
                'vnum': obj_vnum,
                'name': obj_name,
                'container_vnum': container_vnum,
                'container_name': container_name,
                'source_area': source_area,
                'is_cross_area': source_area != area.filename.replace('.are', ''),
            }

            obj_spawns.append(entry)
            if entry['is_cross_area']:
                cross_area_objs.append(entry)

    return {
        'mob_spawns': mob_spawns,
        'obj_spawns': obj_spawns,
        'cross_area_mobs': cross_area_mobs,
        'cross_area_objs': cross_area_objs,
        'total_mob_resets': len(mob_spawns),
        'total_obj_resets': len(obj_spawns),
        'cross_area_mob_count': len(cross_area_mobs),
        'cross_area_obj_count': len(cross_area_objs),
    }


def build_areas_json(areas: Dict[str, Area]) -> dict:
    """Build the areas.json structure for the web viewer."""
    return {area_id: area.to_dict() for area_id, area in areas.items()}


def build_world_graph(areas: Dict[str, Area], connections: List[dict]) -> dict:
    """Build the world_graph.json structure for the area overview map."""
    area_coords = assign_area_coordinates(areas, connections)

    nodes = []
    for area_id, area in areas.items():
        x, y = area_coords.get(area_id, (0, 0))
        nodes.append({
            'id': area_id,
            'name': area.name,
            'filename': area.filename,
            'room_count': len(area.rooms),
            'mob_count': len(area.mobiles),
            'obj_count': len(area.objects),
            'vnum_range': [area.lvnum, area.uvnum],
            'x': x,
            'y': y,
        })

    # Aggregate connections between areas
    edge_data: Dict[tuple, dict] = {}
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
        primary_dir = max(data['directions'], key=data['directions'].get) if data['directions'] else 'unknown'
        edges.append({
            'from': from_a,
            'to': to_a,
            'connection_count': data['count'],
            'one_way_count': data['one_way_count'],
            'direction': primary_dir,
        })

    return {'nodes': nodes, 'edges': edges}


def build_analysis_json(areas: Dict[str, Area]) -> dict:
    """Build comprehensive analysis data for all areas."""
    all_analysis = {}

    for area_id, area in areas.items():
        area_analysis = analyze_area_difficulty(area)
        mob_analyses = get_mob_analysis(area)
        obj_analyses = get_object_analysis(area)
        spawn_data = build_spawn_data(area, areas)

        all_analysis[area_id] = {
            'summary': area_analysis.to_dict(),
            'mobs': {str(v): ma.to_dict() for v, ma in mob_analyses.items()},
            'objects': {str(v): oa.to_dict() for v, oa in obj_analyses.items()},
            'spawns': spawn_data,
        }

    return all_analysis


def build_mob_rankings(areas: Dict[str, Area]) -> List[dict]:
    """Build a global ranking of all mobs by difficulty."""
    all_mobs = []

    for area_id, area in areas.items():
        mob_analyses = get_mob_analysis(area)
        for vnum, ma in mob_analyses.items():
            all_mobs.append({
                'area_id': area_id,
                'area_name': area.name,
                **ma.to_dict()
            })

    # Sort by difficulty score descending
    all_mobs.sort(key=lambda x: x['difficulty_score'], reverse=True)
    return all_mobs


def build_object_rankings(areas: Dict[str, Area]) -> List[dict]:
    """Build a global ranking of all objects by power."""
    all_objects = []

    for area_id, area in areas.items():
        obj_analyses = get_object_analysis(area)
        for vnum, oa in obj_analyses.items():
            all_objects.append({
                'area_id': area_id,
                'area_name': area.name,
                **oa.to_dict()
            })

    # Sort by power score descending
    all_objects.sort(key=lambda x: x['power_score'], reverse=True)
    return all_objects


def build_shop_listings(areas: Dict[str, Area]) -> List[dict]:
    """Build a listing of all shops across areas."""
    all_shops = []

    for area_id, area in areas.items():
        for shop in area.shops:
            keeper_name = "Unknown"
            if shop.keeper_vnum in area.mobiles:
                keeper_name = area.mobiles[shop.keeper_vnum].short_descr

            # Find which room the shop keeper spawns in
            shop_room = None
            for reset in area.resets:
                if reset.command == 'M' and reset.arg1 == shop.keeper_vnum:
                    shop_room = reset.arg3
                    break

            room_name = None
            if shop_room and shop_room in area.rooms:
                room_name = area.rooms[shop_room].name

            all_shops.append({
                'area_id': area_id,
                'area_name': area.name,
                'keeper_vnum': shop.keeper_vnum,
                'keeper_name': keeper_name,
                'room_vnum': shop_room,
                'room_name': room_name,
                'buy_types': [str(t) for t in shop.buy_types],
                'profit_buy': shop.profit_buy,
                'profit_sell': shop.profit_sell,
                'hours': f"{shop.open_hour}:00 - {shop.close_hour}:00",
            })

    return all_shops


def write_json_files(output_dir: Path, areas: Dict[str, Area], connections: List[dict]) -> None:
    """Write all JSON files to the output directory."""
    output_dir.mkdir(parents=True, exist_ok=True)

    # Write areas.json
    areas_path = output_dir / 'areas.json'
    with open(areas_path, 'w') as f:
        json.dump(build_areas_json(areas), f, indent=2)
    print(f"Written: {areas_path}")

    # Write world_graph.json
    world_path = output_dir / 'world_graph.json'
    with open(world_path, 'w') as f:
        json.dump(build_world_graph(areas, connections), f, indent=2)
    print(f"Written: {world_path}")

    # Write analysis.json
    analysis_path = output_dir / 'analysis.json'
    with open(analysis_path, 'w') as f:
        json.dump(build_analysis_json(areas), f, indent=2)
    print(f"Written: {analysis_path}")

    # Write mob_rankings.json
    mobs_path = output_dir / 'mob_rankings.json'
    with open(mobs_path, 'w') as f:
        json.dump(build_mob_rankings(areas), f)
    print(f"Written: {mobs_path}")

    # Write object_rankings.json
    objects_path = output_dir / 'object_rankings.json'
    with open(objects_path, 'w') as f:
        json.dump(build_object_rankings(areas), f)
    print(f"Written: {objects_path}")

    # Write shops.json
    shops_path = output_dir / 'shops.json'
    with open(shops_path, 'w') as f:
        json.dump(build_shop_listings(areas), f, indent=2)
    print(f"Written: {shops_path}")
