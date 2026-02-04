"""
Navigation tree for MUD database browser.

Displays a hierarchical view of all available databases and their contents.
"""

import tkinter as tk
from tkinter import ttk
from pathlib import Path
from typing import Callable, Dict, List, Optional, Tuple

from ..db.manager import DatabaseManager


class NavigationTree(ttk.Frame):
    """
    Navigation tree for browsing MUD databases.

    Displays a hierarchical tree of:
    - Help Files
    - Areas (with sub-items for each entity type)
    - Game Config
    - Players
    """

    def __init__(
        self,
        parent,
        db_manager: DatabaseManager,
        on_select: Optional[Callable[[str, Path, str], None]] = None,
        **kwargs
    ):
        """
        Initialize the navigation tree.

        Args:
            parent: Parent Tkinter widget
            db_manager: DatabaseManager for path discovery
            on_select: Callback invoked with (category, db_path, entity_type)
                      when an item is selected
            **kwargs: Additional arguments passed to ttk.Frame
        """
        super().__init__(parent, **kwargs)

        self.db_manager = db_manager
        self.on_select = on_select

        # Track node -> (category, db_path, entity_type) mapping
        self._node_data: Dict[str, Tuple[str, Path, str]] = {}

        self._build_ui()
        self._populate_tree()

    def _build_ui(self):
        """Build the tree UI."""
        # Create treeview
        self.tree = ttk.Treeview(self, show='tree', selectmode='browse')

        # Scrollbar
        scrollbar = ttk.Scrollbar(
            self,
            orient=tk.VERTICAL,
            command=self.tree.yview
        )
        self.tree.configure(yscrollcommand=scrollbar.set)

        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # Selection binding
        self.tree.bind('<<TreeviewSelect>>', self._on_tree_select)

        # Double-click to expand/collapse
        self.tree.bind('<Double-1>', self._on_double_click)

    def _populate_tree(self):
        """Populate the tree with database structure."""
        # Clear existing items
        self.tree.delete(*self.tree.get_children())
        self._node_data.clear()

        # Help Files
        help_node = self.tree.insert('', tk.END, text='Help Files', open=True)
        for db_path in self.db_manager.list_help_dbs():
            node_id = self.tree.insert(help_node, tk.END, text=db_path.name)
            self._node_data[node_id] = ('help', db_path, 'helps')

        # Areas
        areas_node = self.tree.insert('', tk.END, text='Areas', open=False)
        for db_path in self.db_manager.list_area_dbs():
            area_name = self.db_manager.get_area_name(db_path)
            area_node = self.tree.insert(
                areas_node,
                tk.END,
                text=f"{db_path.stem} ({area_name})"
            )

            # Add entity type sub-items
            for entity_type, label in [
                ('area', 'Area Info'),
                ('mobiles', 'Mobiles'),
                ('objects', 'Objects'),
                ('rooms', 'Rooms'),
                ('resets', 'Resets'),
                ('shops', 'Shops'),
            ]:
                node_id = self.tree.insert(area_node, tk.END, text=label)
                self._node_data[node_id] = ('area', db_path, entity_type)

        # Game Config
        game_node = self.tree.insert('', tk.END, text='Game Config', open=False)
        game_db = self.db_manager.get_game_db_path()
        if game_db.exists():
            for table_name, label in [
                ('gameconfig', 'Configuration'),
                ('balance_config', 'Balance Config'),
                ('ability_config', 'Ability Config'),
                ('audio_config', 'Audio Config'),
                ('kingdoms', 'Kingdoms'),
                ('topboard', 'Top Board'),
                ('leaderboard', 'Leaderboard'),
                ('notes', 'Notes/Boards'),
                ('bugs', 'Bug Reports'),
                ('bans', 'Bans'),
                ('disabled_commands', 'Disabled Commands'),
                ('super_admins', 'Super Admins'),
                ('immortal_pretitles', 'Immortal Pretitles'),
                ('class_display', 'Class Display'),
            ]:
                node_id = self.tree.insert(game_node, tk.END, text=label)
                self._node_data[node_id] = ('game', game_db, table_name)

        # Players
        players_node = self.tree.insert('', tk.END, text='Players', open=False)
        for db_path in self.db_manager.list_player_dbs():
            player_name = self.db_manager.get_player_name(db_path)
            node_id = self.tree.insert(players_node, tk.END, text=player_name)
            self._node_data[node_id] = ('player', db_path, 'player')

    def _on_tree_select(self, event):
        """Handle tree selection."""
        if not self.on_select:
            return

        selection = self.tree.selection()
        if not selection:
            return

        node_id = selection[0]
        if node_id in self._node_data:
            category, db_path, entity_type = self._node_data[node_id]
            self.on_select(category, db_path, entity_type)

    def _on_double_click(self, event):
        """Handle double-click to expand/collapse."""
        item = self.tree.identify('item', event.x, event.y)
        if item and self.tree.get_children(item):
            # Has children, toggle open/close
            if self.tree.item(item, 'open'):
                self.tree.item(item, open=False)
            else:
                self.tree.item(item, open=True)

    def refresh(self):
        """Refresh the tree contents."""
        self._populate_tree()

    def expand_all(self):
        """Expand all tree nodes."""
        def expand(item):
            self.tree.item(item, open=True)
            for child in self.tree.get_children(item):
                expand(child)

        for item in self.tree.get_children():
            expand(item)

    def collapse_all(self):
        """Collapse all tree nodes."""
        def collapse(item):
            self.tree.item(item, open=False)
            for child in self.tree.get_children(item):
                collapse(child)

        for item in self.tree.get_children():
            collapse(item)
