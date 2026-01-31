#!/usr/bin/env python3
"""
Restore color codes to area SQLite databases.

Extracts old .are files from git history (before they were removed),
parses them with color codes preserved, and runs UPDATE statements
on existing .db files to restore the color-coded text fields.

This is a targeted update -- it only touches text fields in mobiles,
objects, rooms, and extra_descriptions. Schema additions and other
post-migration changes (death_teleport columns, candyland, etc.)
are fully preserved.

Usage:
    python restore_color_codes.py
"""

import os
import sqlite3
import subprocess
import sys
import tempfile
from pathlib import Path

# Add parent so we can import mudlib
sys.path.insert(0, str(Path(__file__).resolve().parent))

from mudlib.area_parser import parse_area_file

# Last commit before .are files were removed in the migration
COMMIT_REF = "25d6a60^"


def extract_are_files(repo_root: Path, dest_dir: Path) -> list:
    """Extract .are files from git history into dest_dir."""
    # Get area.lst
    result = subprocess.run(
        ["git", "show", f"{COMMIT_REF}:gamedata/area/area.lst"],
        capture_output=True, text=True, cwd=repo_root
    )
    if result.returncode != 0:
        print(f"Error getting area.lst from git: {result.stderr}")
        sys.exit(1)

    area_lst = dest_dir / "area.lst"
    area_lst.write_text(result.stdout)

    area_files = [l.strip() for l in result.stdout.split("\n")
                  if l.strip() and not l.startswith("$")]

    extracted = []
    for fname in area_files:
        r = subprocess.run(
            ["git", "show", f"{COMMIT_REF}:gamedata/area/{fname}"],
            capture_output=True, cwd=repo_root
        )
        if r.returncode == 0:
            (dest_dir / fname).write_bytes(r.stdout)
            extracted.append(fname)
        else:
            print(f"  SKIP {fname} (not in git history)")

    return extracted


def update_area_db(area, db_path: Path) -> dict:
    """Update text fields in an existing .db file with color-coded values."""
    counts = {"mobiles": 0, "objects": 0, "rooms": 0, "extra_descs": 0}

    conn = sqlite3.connect(str(db_path))

    # Update mobiles
    for vnum, mob in area.mobiles.items():
        cur = conn.execute(
            "UPDATE mobiles SET player_name=?, short_descr=?, long_descr=?, description=? "
            "WHERE vnum=?",
            (mob.name, mob.short_descr, mob.long_descr, mob.description, vnum)
        )
        counts["mobiles"] += cur.rowcount

    # Update objects -- only name, short_descr, description (NOT power strings)
    for vnum, obj in area.objects.items():
        cur = conn.execute(
            "UPDATE objects SET name=?, short_descr=?, description=? "
            "WHERE vnum=?",
            (obj.name, obj.short_descr, obj.long_descr, vnum)
        )
        counts["objects"] += cur.rowcount

    # Update rooms
    for vnum, room in area.rooms.items():
        cur = conn.execute(
            "UPDATE rooms SET name=?, description=? WHERE vnum=?",
            (room.name, room.description, vnum)
        )
        counts["rooms"] += cur.rowcount

    # Update extra_descriptions by matching owner_vnum + sort_order
    # Rebuild from parsed data
    for vnum, obj in area.objects.items():
        for sort_idx, ed in enumerate(obj.extra_descs):
            cur = conn.execute(
                "UPDATE extra_descriptions SET keyword=?, description=? "
                "WHERE owner_type='object' AND owner_vnum=? AND sort_order=?",
                (ed.keyword, ed.description, vnum, sort_idx)
            )
            counts["extra_descs"] += cur.rowcount

    for vnum, room in area.rooms.items():
        for sort_idx, ed in enumerate(room.extra_descs):
            cur = conn.execute(
                "UPDATE extra_descriptions SET keyword=?, description=? "
                "WHERE owner_type='room' AND owner_vnum=? AND sort_order=?",
                (ed.keyword, ed.description, vnum, sort_idx)
            )
            counts["extra_descs"] += cur.rowcount

    conn.commit()
    conn.close()
    return counts


def main():
    repo_root = Path(__file__).resolve().parent.parent.parent
    db_dir = repo_root / "gamedata" / "db" / "areas"

    if not db_dir.exists():
        print(f"Error: database directory not found: {db_dir}")
        sys.exit(1)

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)

        print(f"Extracting .are files from git ref {COMMIT_REF}...")
        extracted = extract_are_files(repo_root, tmp_path)
        print(f"  Extracted {len(extracted)} area files\n")

        total = {"mobiles": 0, "objects": 0, "rooms": 0, "extra_descs": 0}
        errors = []
        updated = 0

        for fname in extracted:
            filepath = tmp_path / fname
            basename = filepath.stem
            db_path = db_dir / f"{basename}.db"

            if not db_path.exists():
                continue

            try:
                area = parse_area_file(filepath)
                if area is None:
                    errors.append(f"{fname}: parser returned None")
                    print(f"  FAIL  {fname}")
                    continue

                counts = update_area_db(area, db_path)
                updated += 1

                parts = []
                for key, val in counts.items():
                    if val > 0:
                        parts.append(f"{val} {key}")
                summary = ", ".join(parts) if parts else "no changes"
                print(f"  OK    {basename:30s}  [{summary}]")

                for k, v in counts.items():
                    total[k] += v

            except Exception as e:
                errors.append(f"{fname}: {e}")
                print(f"  FAIL  {fname}: {e}")

    print(f"\nDone! Updated {updated} area databases.")
    print(f"  Mobiles updated: {total['mobiles']}")
    print(f"  Objects updated: {total['objects']}")
    print(f"  Rooms updated:   {total['rooms']}")
    print(f"  Extra descs updated: {total['extra_descs']}")

    if errors:
        print(f"\n{len(errors)} error(s):")
        for e in errors:
            print(f"  - {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
