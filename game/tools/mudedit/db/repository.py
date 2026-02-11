"""
Repository classes for CRUD operations on MUD database entities.

Provides a generic BaseRepository and specialized repositories for each entity type.
"""

import sqlite3
from typing import Any, Dict, List, Optional, Tuple


# =============================================================================
# Shared Constants - Class ID to name mapping (fallback, prefer DB lookup)
# =============================================================================
CLASS_NAMES = {
    1: 'Demon',
    2: 'Mage',
    4: 'Werewolf',
    8: 'Vampire',
    16: 'Samurai',
    32: 'Drow',
    64: 'Monk',
    128: 'Ninja',
    256: 'Lich',
    512: 'Shapeshifter',
    1024: 'Tanarri',
    2048: 'Angel',
    4096: 'Undead Knight',
    8192: 'Spider Droid',
    16384: 'Dirgesinger',
    32768: 'Siren',
    65536: 'Psion',
    131072: 'Mindflayer',
    262144: 'Dragonkin',
    524288: 'Wyrm',
    1048576: 'Artificer',
    2097152: 'Mechanist',
}

# Cache for class names loaded from database
_class_name_cache: Dict[int, str] = {}

# Stat source enum mapping (matches C STAT_SOURCE enum in db_game.h)
STAT_SOURCES = {
    0: ('STAT_NONE', 'None'),
    1: ('STAT_BEAST', 'Beast (ch->beast)'),
    2: ('STAT_RAGE', 'Rage (ch->rage)'),
    3: ('STAT_CHI_CURRENT', 'Chi Current'),
    4: ('STAT_CHI_MAXIMUM', 'Chi Maximum'),
    5: ('STAT_GNOSIS_CURRENT', 'Gnosis Current'),
    6: ('STAT_GNOSIS_MAXIMUM', 'Gnosis Maximum'),
    7: ('STAT_MONKBLOCK', 'Monk Block'),
    8: ('STAT_SILTOL', 'Silver Tolerance'),
    9: ('STAT_SOULS', 'Souls'),
    10: ('STAT_DEMON_POWER', 'Demon Power (Current)'),
    11: ('STAT_DEMON_TOTAL', 'Demon Power (Total)'),
    12: ('STAT_DROID_POWER', 'Droid Power'),
    13: ('STAT_DROW_POWER', 'Drow Power'),
    14: ('STAT_DROW_MAGIC', 'Drow Magic'),
    15: ('STAT_TPOINTS', 'Tanarri Points'),
    16: ('STAT_ANGEL_JUSTICE', 'Angel Justice'),
    17: ('STAT_ANGEL_LOVE', 'Angel Love'),
    18: ('STAT_ANGEL_HARMONY', 'Angel Harmony'),
    19: ('STAT_ANGEL_PEACE', 'Angel Peace'),
    20: ('STAT_SHAPE_COUNTER', 'Shape Counter'),
    21: ('STAT_PHASE_COUNTER', 'Phase Counter'),
    22: ('STAT_HARA_KIRI', 'Hara Kiri'),
}


def get_class_name(class_id: int) -> str:
    """Get human-readable class name from class_id."""
    return CLASS_NAMES.get(class_id, f'Unknown ({class_id})')


def get_stat_source_name(stat_source: int) -> str:
    """Get human-readable stat source name."""
    if stat_source in STAT_SOURCES:
        return STAT_SOURCES[stat_source][1]
    return f'Unknown ({stat_source})'


# =============================================================================
# Repository Classes
# =============================================================================

class BaseRepository:
    """
    Base class for entity repositories.

    Provides generic CRUD operations that can be customized by subclasses.
    """

    def __init__(self, conn: sqlite3.Connection, table_name: str, primary_key: str = 'id'):
        """
        Initialize the repository.

        Args:
            conn: SQLite connection (should have row_factory = sqlite3.Row)
            table_name: Name of the database table
            primary_key: Name of the primary key column
        """
        self.conn = conn
        self.table_name = table_name
        self.primary_key = primary_key

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """
        Return all rows as dictionaries.

        Args:
            order_by: Optional column name to order by (defaults to primary key)
        """
        order_by = order_by or self.primary_key
        rows = self.conn.execute(
            f"SELECT * FROM {self.table_name} ORDER BY {order_by}"
        ).fetchall()
        return [dict(row) for row in rows]

    def get_by_id(self, id_value: Any) -> Optional[Dict]:
        """Get a single row by primary key."""
        row = self.conn.execute(
            f"SELECT * FROM {self.table_name} WHERE {self.primary_key} = ?",
            (id_value,)
        ).fetchone()
        return dict(row) if row else None

    def search(self, term: str, columns: List[str]) -> List[Dict]:
        """
        Search across specified columns.

        Args:
            term: Search term (case-insensitive LIKE match)
            columns: List of column names to search
        """
        if not columns:
            return []

        conditions = ' OR '.join(f"{col} LIKE ?" for col in columns)
        params = tuple(f"%{term}%" for _ in columns)

        rows = self.conn.execute(
            f"SELECT * FROM {self.table_name} WHERE {conditions} ORDER BY {self.primary_key}",
            params
        ).fetchall()
        return [dict(row) for row in rows]

    def insert(self, data: Dict) -> Any:
        """
        Insert a new row.

        Args:
            data: Dictionary of column_name -> value

        Returns:
            The new row's primary key value
        """
        columns = list(data.keys())
        placeholders = ', '.join('?' for _ in columns)
        column_names = ', '.join(columns)
        values = tuple(data.values())

        self.conn.execute(
            f"INSERT INTO {self.table_name} ({column_names}) VALUES ({placeholders})",
            values
        )
        self.conn.commit()

        # Return the new ID
        row = self.conn.execute("SELECT last_insert_rowid()").fetchone()
        return row[0]

    def update(self, id_value: Any, data: Dict) -> bool:
        """
        Update an existing row.

        Args:
            id_value: Primary key value of the row to update
            data: Dictionary of column_name -> new_value

        Returns:
            True if a row was updated, False if not found
        """
        if not data:
            return False

        set_clause = ', '.join(f"{col} = ?" for col in data.keys())
        values = tuple(data.values()) + (id_value,)

        cursor = self.conn.execute(
            f"UPDATE {self.table_name} SET {set_clause} WHERE {self.primary_key} = ?",
            values
        )
        self.conn.commit()
        return cursor.rowcount > 0

    def delete(self, id_value: Any) -> bool:
        """
        Delete a row by primary key.

        Returns:
            True if a row was deleted, False if not found
        """
        cursor = self.conn.execute(
            f"DELETE FROM {self.table_name} WHERE {self.primary_key} = ?",
            (id_value,)
        )
        self.conn.commit()
        return cursor.rowcount > 0

    def delete_many(self, id_values: List[Any]) -> int:
        """
        Delete multiple rows by primary key.

        Args:
            id_values: List of primary key values to delete

        Returns:
            Number of rows deleted
        """
        if not id_values:
            return 0

        placeholders = ', '.join('?' for _ in id_values)
        cursor = self.conn.execute(
            f"DELETE FROM {self.table_name} WHERE {self.primary_key} IN ({placeholders})",
            tuple(id_values)
        )
        self.conn.commit()
        return cursor.rowcount

    def count(self) -> int:
        """Return the total number of rows."""
        row = self.conn.execute(f"SELECT COUNT(*) FROM {self.table_name}").fetchone()
        return row[0]


class HelpRepository(BaseRepository):
    """Repository for help entries."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'helps', 'id')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all help entries ordered by ID."""
        return super().list_all(order_by or 'id')

    def search(self, term: str, columns: Optional[List[str]] = None) -> List[Dict]:
        """Search help entries by keyword or text content."""
        return super().search(term, columns or ['keyword', 'text'])

    def find_by_keyword(self, keyword: str) -> List[Dict]:
        """Find help entries matching a keyword (case-insensitive)."""
        rows = self.conn.execute(
            "SELECT * FROM helps WHERE keyword LIKE ? ORDER BY id",
            (f"%{keyword}%",)
        ).fetchall()
        return [dict(row) for row in rows]


class MobileRepository(BaseRepository):
    """Repository for mobile/NPC entities."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'mobiles', 'vnum')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all mobiles ordered by vnum."""
        return super().list_all(order_by or 'vnum')

    def search(self, term: str, columns: Optional[List[str]] = None) -> List[Dict]:
        """Search mobiles by name or description."""
        return super().search(
            term,
            columns or ['player_name', 'short_descr', 'description']
        )

    def get_with_special(self, vnum: int) -> Optional[Dict]:
        """Get a mobile with its special function (if any)."""
        mobile = self.get_by_id(vnum)
        if not mobile:
            return None

        # Check for special function
        row = self.conn.execute(
            "SELECT spec_fun_name FROM specials WHERE mob_vnum = ?",
            (vnum,)
        ).fetchone()

        if row:
            mobile['spec_fun_name'] = row['spec_fun_name']
        else:
            mobile['spec_fun_name'] = None

        return mobile

    def get_spawns(self, vnum: int) -> List[Dict]:
        """Get all reset entries that spawn this mobile."""
        rows = self.conn.execute(
            "SELECT * FROM resets WHERE command = 'M' AND arg1 = ? ORDER BY sort_order",
            (vnum,)
        ).fetchall()
        return [dict(row) for row in rows]


class ObjectRepository(BaseRepository):
    """Repository for object/item entities."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'objects', 'vnum')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all objects ordered by vnum."""
        return super().list_all(order_by or 'vnum')

    def search(self, term: str, columns: Optional[List[str]] = None) -> List[Dict]:
        """Search objects by name or description."""
        return super().search(
            term,
            columns or ['name', 'short_descr', 'description']
        )

    def get_with_affects(self, vnum: int) -> Optional[Dict]:
        """Get an object with its affects."""
        obj = self.get_by_id(vnum)
        if not obj:
            return None

        rows = self.conn.execute(
            "SELECT * FROM object_affects WHERE obj_vnum = ? ORDER BY sort_order",
            (vnum,)
        ).fetchall()
        obj['affects'] = [dict(row) for row in rows]

        return obj

    def get_extra_descs(self, vnum: int) -> List[Dict]:
        """Get extra descriptions for an object."""
        rows = self.conn.execute(
            "SELECT * FROM extra_descriptions WHERE owner_type = 'objects' AND owner_vnum = ? ORDER BY sort_order",
            (vnum,)
        ).fetchall()
        return [dict(row) for row in rows]


class RoomRepository(BaseRepository):
    """Repository for room entities."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'rooms', 'vnum')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all rooms ordered by vnum."""
        return super().list_all(order_by or 'vnum')

    def search(self, term: str, columns: Optional[List[str]] = None) -> List[Dict]:
        """Search rooms by name or description."""
        return super().search(
            term,
            columns or ['name', 'description']
        )

    def get_with_exits(self, vnum: int) -> Optional[Dict]:
        """Get a room with all its exits."""
        room = self.get_by_id(vnum)
        if not room:
            return None

        rows = self.conn.execute(
            "SELECT * FROM exits WHERE room_vnum = ? ORDER BY direction",
            (vnum,)
        ).fetchall()
        room['exits'] = [dict(row) for row in rows]

        return room

    def get_room_texts(self, vnum: int) -> List[Dict]:
        """Get room text triggers for a room."""
        rows = self.conn.execute(
            "SELECT * FROM room_texts WHERE room_vnum = ? ORDER BY sort_order",
            (vnum,)
        ).fetchall()
        return [dict(row) for row in rows]

    def get_extra_descs(self, vnum: int) -> List[Dict]:
        """Get extra descriptions for a room."""
        rows = self.conn.execute(
            "SELECT * FROM extra_descriptions WHERE owner_type = 'rooms' AND owner_vnum = ? ORDER BY sort_order",
            (vnum,)
        ).fetchall()
        return [dict(row) for row in rows]

    def get_resets_for_room(self, vnum: int) -> List[Dict]:
        """Get all reset entries for this room."""
        rows = self.conn.execute(
            "SELECT * FROM resets WHERE (command = 'M' AND arg3 = ?) OR (command = 'O' AND arg3 = ?) ORDER BY sort_order",
            (vnum, vnum)
        ).fetchall()
        return [dict(row) for row in rows]


class ResetRepository(BaseRepository):
    """Repository for area reset commands."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'resets', 'id')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all resets ordered by sort_order."""
        return super().list_all(order_by or 'sort_order')

    def get_by_command(self, command: str) -> List[Dict]:
        """Get all resets of a specific command type (M, O, G, E, P)."""
        rows = self.conn.execute(
            "SELECT * FROM resets WHERE command = ? ORDER BY sort_order",
            (command,)
        ).fetchall()
        return [dict(row) for row in rows]


class ShopRepository(BaseRepository):
    """Repository for shop configurations."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'shops', 'keeper_vnum')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all shops ordered by keeper vnum."""
        return super().list_all(order_by or 'keeper_vnum')

    def get_for_mobile(self, mob_vnum: int) -> Optional[Dict]:
        """Get shop configuration for a specific mobile."""
        return self.get_by_id(mob_vnum)


class AreaRepository(BaseRepository):
    """Repository for area metadata."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'area', 'name')

    def get_info(self) -> Optional[Dict]:
        """Get the area's metadata (there's only one row)."""
        row = self.conn.execute("SELECT * FROM area LIMIT 1").fetchone()
        return dict(row) if row else None

    def update_info(self, data: Dict) -> bool:
        """Update the area's metadata."""
        if not data:
            return False

        set_clause = ', '.join(f"{col} = ?" for col in data.keys())
        values = tuple(data.values())

        cursor = self.conn.execute(
            f"UPDATE area SET {set_clause}",
            values
        )
        self.conn.commit()
        return cursor.rowcount > 0


class ExitRepository(BaseRepository):
    """Repository for room exits."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'exits', 'id')

    def get_for_room(self, room_vnum: int) -> List[Dict]:
        """Get all exits for a specific room."""
        rows = self.conn.execute(
            "SELECT * FROM exits WHERE room_vnum = ? ORDER BY direction",
            (room_vnum,)
        ).fetchall()
        return [dict(row) for row in rows]

    def get_by_room_and_direction(self, room_vnum: int, direction: int) -> Optional[Dict]:
        """Get a specific exit by room and direction."""
        row = self.conn.execute(
            "SELECT * FROM exits WHERE room_vnum = ? AND direction = ?",
            (room_vnum, direction)
        ).fetchone()
        return dict(row) if row else None

    def delete_for_room(self, room_vnum: int) -> int:
        """Delete all exits for a room. Returns count of deleted exits."""
        cursor = self.conn.execute(
            "DELETE FROM exits WHERE room_vnum = ?",
            (room_vnum,)
        )
        self.conn.commit()
        return cursor.rowcount


class KeyValueRepository(BaseRepository):
    """Repository for key-value configuration tables (gameconfig, balance_config)."""

    def __init__(self, conn: sqlite3.Connection, table_name: str):
        super().__init__(conn, table_name, 'key')

    def get_value(self, key: str) -> Optional[str]:
        """Get a single configuration value."""
        row = self.conn.execute(
            f"SELECT value FROM {self.table_name} WHERE key = ?",
            (key,)
        ).fetchone()
        return row['value'] if row else None

    def set_value(self, key: str, value: str) -> bool:
        """Set a configuration value (insert or update)."""
        existing = self.get_by_id(key)
        if existing:
            return self.update(key, {'value': value})
        else:
            self.insert({'key': key, 'value': value})
            return True


class GameConfigRepository(KeyValueRepository):
    """Repository for gameconfig table."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'gameconfig')


class BalanceConfigRepository(KeyValueRepository):
    """Repository for balance_config table."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'balance_config')


class AbilityConfigRepository(BaseRepository):
    """Repository for ability_config table with hierarchical key support."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'ability_config', 'key')

    def get_value(self, key: str) -> Optional[int]:
        """Get a single ability config value."""
        row = self.conn.execute(
            "SELECT value FROM ability_config WHERE key = ?",
            (key,)
        ).fetchone()
        return row['value'] if row else None

    def set_value(self, key: str, value: int) -> bool:
        """Set an ability config value (insert or update)."""
        existing = self.get_by_id(key)
        if existing:
            return self.update(key, {'value': value})
        else:
            self.insert({'key': key, 'value': value})
            return True

    def get_by_class(self, class_name: str) -> List[Dict]:
        """Get all abilities for a specific class."""
        rows = self.conn.execute(
            "SELECT * FROM ability_config WHERE key LIKE ? ORDER BY key",
            (f"{class_name}.%",)
        ).fetchall()
        return [dict(row) for row in rows]

    def get_classes(self) -> List[str]:
        """Get list of unique class names."""
        rows = self.conn.execute(
            "SELECT DISTINCT substr(key, 1, instr(key, '.') - 1) as class_name "
            "FROM ability_config WHERE instr(key, '.') > 0 ORDER BY class_name"
        ).fetchall()
        return [row['class_name'] for row in rows if row['class_name']]

    def get_abilities_for_class(self, class_name: str) -> List[str]:
        """Get list of unique abilities for a class."""
        prefix = f"{class_name}."
        rows = self.conn.execute(
            "SELECT DISTINCT "
            "  substr(key, ?, instr(substr(key, ?), '.') - 1) as ability "
            "FROM ability_config "
            "WHERE key LIKE ? AND instr(substr(key, ?), '.') > 0 "
            "ORDER BY ability",
            (len(prefix) + 1, len(prefix) + 1, f"{prefix}%", len(prefix) + 1)
        ).fetchall()
        return [row['ability'] for row in rows if row['ability']]

    def get_ability_params(self, class_name: str, ability_name: str) -> List[Dict]:
        """Get all parameters for a specific class ability."""
        rows = self.conn.execute(
            "SELECT * FROM ability_config WHERE key LIKE ? ORDER BY key",
            (f"{class_name}.{ability_name}.%",)
        ).fetchall()
        return [dict(row) for row in rows]


class KingdomsRepository(BaseRepository):
    """Repository for kingdoms table."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'kingdoms', 'id')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all kingdoms ordered by ID."""
        return super().list_all(order_by or 'id')


class BansRepository(BaseRepository):
    """Repository for bans table."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'bans', 'id')

    def find_by_name(self, name: str) -> Optional[Dict]:
        """Find a ban by name."""
        row = self.conn.execute(
            "SELECT * FROM bans WHERE name = ?",
            (name,)
        ).fetchone()
        return dict(row) if row else None


class DisabledCommandsRepository(BaseRepository):
    """Repository for disabled_commands table."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'disabled_commands', 'command_name')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all disabled commands ordered by name."""
        return super().list_all(order_by or 'command_name')


class TopBoardRepository(BaseRepository):
    """Repository for topboard table."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'topboard', 'rank')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all topboard entries ordered by rank."""
        return super().list_all(order_by or 'rank')


class LeaderboardRepository(BaseRepository):
    """Repository for leaderboard table."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'leaderboard', 'category')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all leaderboard entries ordered by category."""
        return super().list_all(order_by or 'category')


class NotesRepository(BaseRepository):
    """Repository for notes table."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'notes', 'id')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all notes ordered by date_stamp descending."""
        return super().list_all(order_by or 'date_stamp DESC')

    def get_by_board(self, board_idx: int) -> List[Dict]:
        """Get all notes for a specific board."""
        rows = self.conn.execute(
            "SELECT * FROM notes WHERE board_idx = ? ORDER BY date_stamp DESC",
            (board_idx,)
        ).fetchall()
        return [dict(row) for row in rows]


class BugsRepository(BaseRepository):
    """Repository for bugs table."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'bugs', 'id')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all bug reports ordered by timestamp descending."""
        return super().list_all(order_by or 'timestamp DESC')


class PlayerRepository:
    """Repository for player database files."""

    def __init__(self, conn: sqlite3.Connection):
        self.conn = conn

    def get_player(self) -> Optional[Dict]:
        """Get player data (excluding password)."""
        row = self.conn.execute(
            "SELECT * FROM player LIMIT 1"
        ).fetchone()
        if not row:
            return None

        data = dict(row)
        # Mask password - never expose it
        if 'password' in data:
            data['password'] = '********'
        return data

    def update_player(self, data: Dict) -> bool:
        """Update player data. Password must be handled separately."""
        if not data:
            return False

        # Never allow password update through this method
        data = {k: v for k, v in data.items() if k != 'password'}
        if not data:
            return False

        set_clause = ', '.join(f"{col} = ?" for col in data.keys())
        values = tuple(data.values())

        cursor = self.conn.execute(
            f"UPDATE player SET {set_clause}",
            values
        )
        self.conn.commit()
        return cursor.rowcount > 0

    def set_password(self, password_hash: str) -> bool:
        """Set player password (write-only, expects pre-hashed value)."""
        cursor = self.conn.execute(
            "UPDATE player SET password = ?",
            (password_hash,)
        )
        self.conn.commit()
        return cursor.rowcount > 0

    def get_skills(self) -> List[Dict]:
        """Get all player skills."""
        rows = self.conn.execute(
            "SELECT * FROM skills ORDER BY skill_name"
        ).fetchall()
        return [dict(row) for row in rows]

    def update_skill(self, skill_name: str, value: int) -> bool:
        """Update or insert a skill value."""
        existing = self.conn.execute(
            "SELECT * FROM skills WHERE skill_name = ?",
            (skill_name,)
        ).fetchone()

        if existing:
            if value == 0:
                # Delete zero-value skills
                self.conn.execute("DELETE FROM skills WHERE skill_name = ?", (skill_name,))
            else:
                self.conn.execute(
                    "UPDATE skills SET value = ? WHERE skill_name = ?",
                    (value, skill_name)
                )
        else:
            if value != 0:
                self.conn.execute(
                    "INSERT INTO skills (skill_name, value) VALUES (?, ?)",
                    (skill_name, value)
                )
        self.conn.commit()
        return True

    def get_aliases(self) -> List[Dict]:
        """Get all player aliases."""
        rows = self.conn.execute(
            "SELECT * FROM aliases ORDER BY short_n"
        ).fetchall()
        return [dict(row) for row in rows]

    def add_alias(self, short_n: str, long_n: str) -> int:
        """Add a new alias."""
        cursor = self.conn.execute(
            "INSERT INTO aliases (short_n, long_n) VALUES (?, ?)",
            (short_n, long_n)
        )
        self.conn.commit()
        return cursor.lastrowid

    def delete_alias(self, id_val: int) -> bool:
        """Delete an alias."""
        cursor = self.conn.execute("DELETE FROM aliases WHERE id = ?", (id_val,))
        self.conn.commit()
        return cursor.rowcount > 0

    def get_affects(self) -> List[Dict]:
        """Get all active affects."""
        rows = self.conn.execute(
            "SELECT * FROM affects ORDER BY skill_name"
        ).fetchall()
        return [dict(row) for row in rows]

    def delete_affect(self, id_val: int) -> bool:
        """Delete an affect."""
        cursor = self.conn.execute("DELETE FROM affects WHERE id = ?", (id_val,))
        self.conn.commit()
        return cursor.rowcount > 0

    def get_objects(self) -> List[Dict]:
        """Get all player objects/inventory."""
        rows = self.conn.execute(
            "SELECT * FROM objects ORDER BY nest, id"
        ).fetchall()
        return [dict(row) for row in rows]

    def get_player_arrays(self) -> Dict[str, List[int]]:
        """Get all player arrays as a dictionary."""
        rows = self.conn.execute("SELECT * FROM player_arrays").fetchall()
        result = {}
        for row in rows:
            name = row['name']
            data_str = row['data']
            if data_str:
                result[name] = [int(x) for x in data_str.split()]
            else:
                result[name] = []
        return result

    def update_player_array(self, name: str, values: List[int]) -> bool:
        """Update or insert a player array."""
        data_str = ' '.join(str(v) for v in values)
        self.conn.execute(
            "INSERT OR REPLACE INTO player_arrays (name, data) VALUES (?, ?)",
            (name, data_str)
        )
        self.conn.commit()
        return True


class AudioConfigRepository(BaseRepository):
    """Repository for audio_config table - MCMP audio file mappings."""

    CATEGORIES = [
        'ambient', 'footstep', 'combat', 'weather',
        'channel', 'time', 'ui', 'spell', 'environment'
    ]

    MEDIA_TYPES = ['sound', 'music']

    TAGS = [
        'combat', 'environment', 'weather',
        'channel', 'ui', 'movement', 'spell'
    ]

    # Sector trigger keys for ambient/footstep categories
    SECTOR_KEYS = [
        'SECT_INSIDE', 'SECT_CITY', 'SECT_FIELD', 'SECT_FOREST',
        'SECT_HILLS', 'SECT_MOUNTAIN', 'SECT_WATER_SWIM',
        'SECT_WATER_NOSWIM', 'SECT_UNUSED', 'SECT_AIR', 'SECT_DESERT'
    ]

    WEATHER_KEYS = ['SKY_CLOUDY', 'SKY_RAINING', 'SKY_LIGHTNING']

    CHANNEL_KEYS = ['CHANNEL_TELL', 'CHANNEL_YELL', 'CHANNEL_IMMTALK', 'CHANNEL_CHAT']

    TIME_KEYS = ['hour_0', 'hour_5', 'hour_6', 'hour_19', 'hour_20']

    COMBAT_KEYS = ['combat_miss', 'combat_light_hit', 'combat_heavy_hit',
                   'combat_death', 'combat_engage', 'combat_victory']

    UI_KEYS = ['ui_login', 'ui_levelup', 'ui_death', 'ui_achievement']

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'audio_config', 'id')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all audio config entries ordered by category and trigger_key."""
        return super().list_all(order_by or 'category, trigger_key')

    def get_by_category(self, category: str) -> List[Dict]:
        """Get all entries for a specific category."""
        rows = self.conn.execute(
            "SELECT * FROM audio_config WHERE category = ? ORDER BY trigger_key",
            (category,)
        ).fetchall()
        return [dict(row) for row in rows]

    def find_by_trigger(self, category: str, trigger_key: str) -> Optional[Dict]:
        """Find a specific entry by category and trigger key."""
        row = self.conn.execute(
            "SELECT * FROM audio_config WHERE category = ? AND trigger_key = ?",
            (category, trigger_key)
        ).fetchone()
        return dict(row) if row else None

    def get_categories(self) -> List[str]:
        """Get list of categories that have entries."""
        rows = self.conn.execute(
            "SELECT DISTINCT category FROM audio_config ORDER BY category"
        ).fetchall()
        return [row['category'] for row in rows]

    def get_trigger_keys_for_category(self, category: str) -> List[str]:
        """Get suggested trigger keys based on category."""
        if category in ('ambient', 'footstep'):
            return self.SECTOR_KEYS
        elif category == 'weather':
            return self.WEATHER_KEYS
        elif category == 'channel':
            return self.CHANNEL_KEYS
        elif category == 'time':
            return self.TIME_KEYS
        elif category == 'combat':
            return self.COMBAT_KEYS
        elif category == 'ui':
            return self.UI_KEYS
        return []  # Free-form for other categories

    def search(self, term: str, columns: Optional[List[str]] = None) -> List[Dict]:
        """Search audio entries."""
        return super().search(
            term,
            columns or ['category', 'trigger_key', 'filename', 'caption']
        )


class SuperAdminsRepository(BaseRepository):
    """Repository for super_admins table."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'super_admins', 'name')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all super admins ordered by name."""
        return super().list_all(order_by or 'name')

    def exists(self, name: str) -> bool:
        """Check if a name is a super admin."""
        row = self.conn.execute(
            "SELECT 1 FROM super_admins WHERE name = ? COLLATE NOCASE",
            (name,)
        ).fetchone()
        return row is not None


class ImmortalPretitlesRepository(BaseRepository):
    """Repository for immortal_pretitles table."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'immortal_pretitles', 'immortal_name')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all immortal pretitles ordered by name."""
        return super().list_all(order_by or 'immortal_name')

    def get_by_name(self, name: str) -> Optional[Dict]:
        """Get pretitle for an immortal by name (case-insensitive)."""
        row = self.conn.execute(
            "SELECT * FROM immortal_pretitles WHERE immortal_name = ? COLLATE NOCASE",
            (name,)
        ).fetchone()
        return dict(row) if row else None


class ClassBracketsRepository(BaseRepository):
    """Repository for class_brackets table - who list brackets by class."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'class_brackets', 'class_id')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all class brackets with class_name from registry."""
        rows = self.conn.execute(
            "SELECT b.*, r.class_name FROM class_brackets b "
            "JOIN class_registry r ON b.class_id = r.class_id "
            "ORDER BY r.class_name"
        ).fetchall()
        return [dict(row) for row in rows]

    def get_by_id(self, class_id: int) -> Optional[Dict]:
        """Get bracket entry with class_name from registry."""
        row = self.conn.execute(
            "SELECT b.*, r.class_name FROM class_brackets b "
            "JOIN class_registry r ON b.class_id = r.class_id "
            "WHERE b.class_id = ?",
            (class_id,)
        ).fetchone()
        return dict(row) if row else None


class ClassGenerationsRepository(BaseRepository):
    """Repository for class_generations table - generation titles by class."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'class_generations', 'id')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all generation titles ordered by class_id and generation."""
        return super().list_all(order_by or 'class_id, generation DESC')

    def get_by_class(self, class_id: int) -> List[Dict]:
        """Get all generation titles for a specific class."""
        rows = self.conn.execute(
            "SELECT * FROM class_generations WHERE class_id = ? ORDER BY generation DESC",
            (class_id,)
        ).fetchall()
        return [dict(row) for row in rows]

    def get_by_class_and_gen(self, class_id: int, generation: int) -> Optional[Dict]:
        """Get a specific generation entry."""
        row = self.conn.execute(
            "SELECT * FROM class_generations WHERE class_id = ? AND generation = ?",
            (class_id, generation)
        ).fetchone()
        return dict(row) if row else None

    def get_classes_with_titles(self) -> List[int]:
        """Get list of class IDs that have generation titles."""
        rows = self.conn.execute(
            "SELECT DISTINCT class_id FROM class_generations ORDER BY class_id"
        ).fetchall()
        return [row['class_id'] for row in rows]


class ClassAurasRepository(BaseRepository):
    """Repository for class_auras table - room aura text by class."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'class_auras', 'class_id')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all class auras with class_name from registry."""
        rows = self.conn.execute(
            "SELECT a.*, r.class_name FROM class_auras a "
            "JOIN class_registry r ON a.class_id = r.class_id "
            "ORDER BY a.display_order, a.class_id"
        ).fetchall()
        return [dict(row) for row in rows]

    def get_by_id(self, class_id: int) -> Optional[Dict]:
        """Get aura entry with class_name from registry."""
        row = self.conn.execute(
            "SELECT a.*, r.class_name FROM class_auras a "
            "JOIN class_registry r ON a.class_id = r.class_id "
            "WHERE a.class_id = ?",
            (class_id,)
        ).fetchone()
        return dict(row) if row else None


class ClassArmorConfigRepository(BaseRepository):
    """Repository for class_armor_config table - armor creation settings per class."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'class_armor_config', 'class_id')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all armor configs ordered by class_id."""
        return super().list_all(order_by or 'class_id')


class ClassArmorPiecesRepository(BaseRepository):
    """Repository for class_armor_pieces table - armor pieces per class."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'class_armor_pieces', 'id')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all armor pieces ordered by class_id and keyword."""
        return super().list_all(order_by or 'class_id, keyword')

    def get_by_class(self, class_id: int) -> List[Dict]:
        """Get all armor pieces for a specific class."""
        rows = self.conn.execute(
            "SELECT * FROM class_armor_pieces WHERE class_id = ? ORDER BY keyword",
            (class_id,)
        ).fetchall()
        return [dict(row) for row in rows]

    def get_by_class_and_keyword(self, class_id: int, keyword: str) -> Optional[Dict]:
        """Get a specific armor piece by class and keyword."""
        row = self.conn.execute(
            "SELECT * FROM class_armor_pieces WHERE class_id = ? AND keyword = ?",
            (class_id, keyword)
        ).fetchone()
        return dict(row) if row else None

    def get_vnum_range_for_class(self, class_id: int) -> Tuple[int, int]:
        """Get the computed VNUM range for a class's armor pieces.

        Returns:
            Tuple of (min_vnum, max_vnum), or (0, 0) if no pieces
        """
        pieces = self.get_by_class(class_id)
        if not pieces:
            return (0, 0)
        vnums = [p['vnum'] for p in pieces]
        return (min(vnums), max(vnums))

    def get_all_vnums(self) -> List[int]:
        """Get all VNUMs used by any armor piece across all classes."""
        rows = self.conn.execute(
            "SELECT DISTINCT vnum FROM class_armor_pieces ORDER BY vnum"
        ).fetchall()
        return [row['vnum'] for row in rows]

    def check_vnum_overlap(self, vnum: int, exclude_class_id: Optional[int] = None) -> List[Dict]:
        """Check if a VNUM is already used by another class.

        Args:
            vnum: VNUM to check
            exclude_class_id: Class ID to exclude from check

        Returns:
            List of pieces that use this VNUM (from other classes)
        """
        if exclude_class_id is not None:
            rows = self.conn.execute(
                "SELECT * FROM class_armor_pieces WHERE vnum = ? AND class_id != ?",
                (vnum, exclude_class_id)
            ).fetchall()
        else:
            rows = self.conn.execute(
                "SELECT * FROM class_armor_pieces WHERE vnum = ?",
                (vnum,)
            ).fetchall()
        return [dict(row) for row in rows]

    def get_next_available_vnum(self, config_repo: 'ClassArmorConfigRepository', range_size: int = 20) -> int:
        """Get the next available VNUM that doesn't conflict with existing pieces.

        Considers both armor piece VNUMs and mastery VNUMs from config.

        Args:
            config_repo: ClassArmorConfigRepository to check mastery VNUMs
            range_size: Size of contiguous range needed (default 20)

        Returns:
            Start vnum for the next available range
        """
        # Get all armor piece VNUMs
        all_vnums = self.get_all_vnums()

        # Get all mastery VNUMs from configs
        configs = config_repo.list_all()
        for cfg in configs:
            if cfg.get('mastery_vnum') and cfg['mastery_vnum'] > 0:
                all_vnums.append(cfg['mastery_vnum'])

        if not all_vnums:
            return 33400  # Default starting point

        max_vnum = max(all_vnums)
        # Add padding between ranges
        return max_vnum + 20


class ClassStartingRepository(BaseRepository):
    """Repository for class_starting table - starting values for class selection."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'class_starting', 'class_id')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all starting configs ordered by class_id."""
        return super().list_all(order_by or 'class_id')


class ClassScoreStatsRepository(BaseRepository):
    """Repository for class_score_stats table - customizable score display per class."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'class_score_stats', 'id')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all score stats ordered by class_id and display_order."""
        return super().list_all(order_by or 'class_id, display_order')

    def get_by_class(self, class_id: int) -> List[Dict]:
        """Get all score stats for a specific class."""
        rows = self.conn.execute(
            "SELECT * FROM class_score_stats WHERE class_id = ? ORDER BY display_order",
            (class_id,)
        ).fetchall()
        return [dict(row) for row in rows]

    def get_classes_with_stats(self) -> List[int]:
        """Get list of class IDs that have score stats."""
        rows = self.conn.execute(
            "SELECT DISTINCT class_id FROM class_score_stats ORDER BY class_id"
        ).fetchall()
        return [row['class_id'] for row in rows]


class ClassRegistryRepository(BaseRepository):
    """Repository for class_registry table - centralized class metadata."""

    def __init__(self, conn: sqlite3.Connection):
        super().__init__(conn, 'class_registry', 'class_id')

    def list_all(self, order_by: Optional[str] = None) -> List[Dict]:
        """List all registry entries ordered by display_order, class_id."""
        return super().list_all(order_by or 'display_order, class_id')

    def get_by_keyword(self, keyword: str) -> Optional[Dict]:
        """Find class by keyword or alternate keyword."""
        row = self.conn.execute(
            "SELECT * FROM class_registry WHERE keyword = ? OR keyword_alt = ?",
            (keyword.lower(), keyword.lower())
        ).fetchone()
        return dict(row) if row else None

    def get_base_classes(self) -> List[Dict]:
        """Get all base classes (upgrade_class IS NULL)."""
        rows = self.conn.execute(
            "SELECT * FROM class_registry WHERE upgrade_class IS NULL ORDER BY display_order, class_id"
        ).fetchall()
        return [dict(row) for row in rows]

    def get_upgrade_classes(self) -> List[Dict]:
        """Get all upgrade classes (upgrade_class IS NOT NULL)."""
        rows = self.conn.execute(
            "SELECT * FROM class_registry WHERE upgrade_class IS NOT NULL ORDER BY display_order, class_id"
        ).fetchall()
        return [dict(row) for row in rows]


