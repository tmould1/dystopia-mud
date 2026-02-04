"""
Database manager for MUD editor.

Handles connections to multiple SQLite database files across the MUD data directories.
"""

import sqlite3
from pathlib import Path
from typing import Dict, List, Optional


class DatabaseManager:
    """
    Manages connections to multiple SQLite database files.

    Provides lazy connection management and path discovery for:
    - Help databases (gamedata/db/game/*.db)
    - Area databases (gamedata/db/areas/*.db)
    - Player databases (gamedata/db/players/*.db)
    """

    def __init__(self, gamedata_path: Optional[Path] = None):
        """
        Initialize the database manager.

        Args:
            gamedata_path: Path to the gamedata directory. If None, auto-detects
                          based on this file's location (game/tools/mudedit/db/).
        """
        if gamedata_path is None:
            # Default: go up from game/tools/mudedit/db/ to repo root, then into gamedata
            repo_root = Path(__file__).resolve().parent.parent.parent.parent.parent
            gamedata_path = repo_root / 'gamedata'

        self.gamedata = Path(gamedata_path)
        self.db_root = self.gamedata / 'db'
        self._connections: Dict[Path, sqlite3.Connection] = {}

    def get_connection(self, db_path: Path) -> sqlite3.Connection:
        """
        Get or create a connection to a database file.

        Args:
            db_path: Path to the database file (can be relative to db_root or absolute).

        Returns:
            sqlite3.Connection to the database.

        Raises:
            FileNotFoundError: If the database file doesn't exist.
        """
        # Normalize the path
        if not db_path.is_absolute():
            db_path = self.db_root / db_path
        db_path = db_path.resolve()

        if not db_path.exists():
            raise FileNotFoundError(f"Database not found: {db_path}")

        if db_path not in self._connections:
            self._connections[db_path] = sqlite3.connect(str(db_path))
            self._connections[db_path].row_factory = sqlite3.Row

        return self._connections[db_path]

    def close_connection(self, db_path: Path) -> None:
        """Close a specific database connection."""
        if not db_path.is_absolute():
            db_path = self.db_root / db_path
        db_path = db_path.resolve()

        if db_path in self._connections:
            self._connections[db_path].close()
            del self._connections[db_path]

    def close_all(self) -> None:
        """Close all open database connections."""
        for conn in self._connections.values():
            conn.close()
        self._connections.clear()

    # --- Path Discovery Methods ---

    def get_help_db_path(self, live: bool = True) -> Path:
        """
        Get the path to a help database.

        Args:
            live: If True, return live_help.db; otherwise base_help.db
        """
        name = 'live_help.db' if live else 'base_help.db'
        return self.db_root / 'game' / name

    def get_game_db_path(self) -> Path:
        """Get the path to the game configuration database."""
        return self.db_root / 'game' / 'game.db'

    def list_area_dbs(self) -> List[Path]:
        """
        List all area database files.

        Returns:
            List of Path objects for each area database, sorted by name.
        """
        areas_dir = self.db_root / 'areas'
        if not areas_dir.exists():
            return []
        return sorted(areas_dir.glob('*.db'))

    def list_player_dbs(self) -> List[Path]:
        """
        List all player database files.

        Returns:
            List of Path objects for each player database, sorted by name.
        """
        players_dir = self.db_root / 'players'
        if not players_dir.exists():
            return []
        return sorted(players_dir.glob('*.db'))

    def list_help_dbs(self) -> List[Path]:
        """
        List all help database files.

        Returns:
            List of Path objects for help databases.
        """
        game_dir = self.db_root / 'game'
        if not game_dir.exists():
            return []
        return [p for p in game_dir.glob('*help*.db')]

    # --- Utility Methods ---

    def get_area_name(self, area_db_path: Path) -> str:
        """
        Get the display name of an area from its database.

        Args:
            area_db_path: Path to the area database.

        Returns:
            The area name from the database, or the filename stem if not found.
        """
        try:
            conn = self.get_connection(area_db_path)
            row = conn.execute("SELECT name FROM area LIMIT 1").fetchone()
            if row:
                return row['name']
        except Exception:
            pass
        return area_db_path.stem

    def get_player_name(self, player_db_path: Path) -> str:
        """
        Get the character name from a player database.

        Args:
            player_db_path: Path to the player database.

        Returns:
            The player name, or the filename stem if not found.
        """
        try:
            conn = self.get_connection(player_db_path)
            row = conn.execute(
                "SELECT value FROM meta WHERE key = 'name'"
            ).fetchone()
            if row:
                return row['value']
        except Exception:
            pass
        return player_db_path.stem

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close_all()
        return False
