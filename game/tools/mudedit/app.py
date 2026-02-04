"""
Main MUD Editor application.

Provides the main window with navigation tree and tabbed editor panels.
"""

import tkinter as tk
from tkinter import ttk, messagebox
from pathlib import Path
from typing import Dict, Optional, Tuple

from .db.manager import DatabaseManager
from .db.repository import (
    HelpRepository, MobileRepository, ObjectRepository, RoomRepository,
    ResetRepository, ShopRepository, AreaRepository,
    GameConfigRepository, BalanceConfigRepository, AbilityConfigRepository,
    AudioConfigRepository,
    KingdomsRepository, BansRepository, DisabledCommandsRepository,
    TopBoardRepository, LeaderboardRepository, NotesRepository, BugsRepository,
    SuperAdminsRepository, ImmortalPretitlesRepository,
    PlayerRepository
)
from .nav.tree import NavigationTree
from .panels import (
    HelpEditorPanel, MobileEditorPanel, ObjectEditorPanel, RoomEditorPanel,
    ResetEditorPanel, AreaInfoPanel, ShopEditorPanel,
    GameConfigPanel, BalanceConfigPanel, AbilityConfigPanel, AudioConfigPanel,
    KingdomsPanel, BansPanel, DisabledCommandsPanel,
    LeaderboardPanel, NotesPanel, BugsPanel, SuperAdminsPanel, ImmortalPretitlesPanel,
    PlayerEditorPanel
)


class MudEditorApp:
    """
    Main MUD database editor application.

    Provides a navigation tree on the left and tabbed editor panels on the right.
    """

    def __init__(self, root: tk.Tk, gamedata_path: Optional[Path] = None):
        """
        Initialize the MUD editor application.

        Args:
            root: Tkinter root window
            gamedata_path: Optional path to gamedata directory
        """
        self.root = root
        self.root.title("Dystopia MUD Database Editor")
        self.root.geometry("1400x800")
        self.root.minsize(1000, 600)

        # Initialize database manager
        self.db_manager = DatabaseManager(gamedata_path)

        # Track open tabs: tab_id -> (category, db_path, entity_type, panel)
        self._open_tabs: Dict[str, Tuple[str, Path, str, ttk.Frame]] = {}

        self._build_ui()
        self._setup_menu()

    def _build_ui(self):
        """Build the main UI."""
        # Main horizontal pane
        paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left panel: navigation tree
        nav_frame = ttk.LabelFrame(paned, text="Database Browser", width=280)
        paned.add(nav_frame, weight=0)

        self.nav_tree = NavigationTree(
            nav_frame,
            self.db_manager,
            on_select=self._on_nav_select
        )
        self.nav_tree.pack(fill=tk.BOTH, expand=True, padx=2, pady=2)

        # Navigation buttons
        nav_btn_frame = ttk.Frame(nav_frame)
        nav_btn_frame.pack(fill=tk.X, padx=2, pady=2)

        ttk.Button(
            nav_btn_frame,
            text="Refresh",
            command=self.nav_tree.refresh
        ).pack(side=tk.LEFT, padx=(0, 2))

        ttk.Button(
            nav_btn_frame,
            text="Expand All",
            command=self.nav_tree.expand_all
        ).pack(side=tk.LEFT, padx=(0, 2))

        ttk.Button(
            nav_btn_frame,
            text="Collapse All",
            command=self.nav_tree.collapse_all
        ).pack(side=tk.LEFT)

        # Right panel: tabbed editor area
        right_frame = ttk.Frame(paned)
        paned.add(right_frame, weight=1)

        self.notebook = ttk.Notebook(right_frame)
        self.notebook.pack(fill=tk.BOTH, expand=True)

        # Welcome tab
        welcome = ttk.Frame(self.notebook)
        self.notebook.add(welcome, text="Welcome")

        welcome_label = ttk.Label(
            welcome,
            text="Dystopia MUD Database Editor\n\n"
                 "Select a database item from the tree on the left to begin editing.\n\n"
                 "Available databases:\n"
                 "  - Help Files: Edit in-game help documentation\n"
                 "  - Areas: Edit rooms, mobiles, objects, and resets\n"
                 "  - Game Config: Edit server configuration\n"
                 "  - Players: View player data (read-only recommended)",
            justify=tk.LEFT,
            font=('Consolas', 11)
        )
        welcome_label.pack(padx=20, pady=20, anchor=tk.NW)

        # Status bar
        self.status_var = tk.StringVar(value="Ready")
        status = ttk.Label(
            self.root,
            textvariable=self.status_var,
            relief=tk.SUNKEN,
            anchor=tk.W
        )
        status.pack(fill=tk.X, side=tk.BOTTOM, padx=4, pady=(0, 4))

        # Enable tab closing with middle-click and right-click menu
        self.notebook.bind('<Button-2>', self._on_tab_middle_click)
        self.notebook.bind('<Button-3>', self._on_tab_right_click)

        # Keyboard shortcut to close current tab
        self.root.bind('<Control-w>', self._close_current_tab)

        # Create tab context menu
        self._tab_menu = tk.Menu(self.root, tearoff=0)
        self._tab_menu.add_command(label="Close Tab", command=self._close_menu_tab)
        self._tab_menu.add_command(label="Close Other Tabs", command=self._close_other_tabs)
        self._tab_menu.add_separator()
        self._tab_menu.add_command(label="Close All Tabs", command=self._close_all_tabs)
        self._menu_tab_index = None

        # Handle window close
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    def _setup_menu(self):
        """Setup the menu bar."""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)

        # File menu
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="Refresh Tree", command=self.nav_tree.refresh)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self._on_close)

        # View menu
        view_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="View", menu=view_menu)
        view_menu.add_command(label="Expand All", command=self.nav_tree.expand_all)
        view_menu.add_command(label="Collapse All", command=self.nav_tree.collapse_all)

        # Help menu
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Help", menu=help_menu)
        help_menu.add_command(label="About", command=self._show_about)

    def _on_nav_select(self, category: str, db_path: Path, entity_type: str):
        """Handle navigation tree selection."""
        # Create a unique tab ID
        tab_id = f"{category}:{db_path.stem}:{entity_type}"

        # Check if tab already exists
        if tab_id in self._open_tabs:
            # Switch to existing tab
            _, _, _, panel = self._open_tabs[tab_id]
            self.notebook.select(panel)
            return

        # Create new tab based on category and entity type
        try:
            panel = self._create_editor_panel(category, db_path, entity_type)
            if panel:
                # Generate tab title
                title = self._get_tab_title(category, db_path, entity_type)

                self.notebook.add(panel, text=title)
                self.notebook.select(panel)
                self._open_tabs[tab_id] = (category, db_path, entity_type, panel)

                self.status_var.set(f"Opened {title}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to open editor: {e}")
            self.status_var.set(f"Error: {e}")

    def _create_editor_panel(
        self,
        category: str,
        db_path: Path,
        entity_type: str
    ) -> Optional[ttk.Frame]:
        """Create an editor panel for the given database item."""
        if category == 'help':
            conn = self.db_manager.get_connection(db_path)
            repository = HelpRepository(conn)
            return HelpEditorPanel(
                self.notebook,
                repository,
                on_status=self._set_status
            )

        elif category == 'area':
            conn = self.db_manager.get_connection(db_path)

            if entity_type == 'area':
                repository = AreaRepository(conn)
                return AreaInfoPanel(
                    self.notebook,
                    repository,
                    on_status=self._set_status
                )

            elif entity_type == 'mobiles':
                repository = MobileRepository(conn)
                return MobileEditorPanel(
                    self.notebook,
                    repository,
                    on_status=self._set_status
                )

            elif entity_type == 'objects':
                repository = ObjectRepository(conn)
                return ObjectEditorPanel(
                    self.notebook,
                    repository,
                    on_status=self._set_status
                )

            elif entity_type == 'rooms':
                repository = RoomRepository(conn)
                return RoomEditorPanel(
                    self.notebook,
                    repository,
                    on_status=self._set_status
                )

            elif entity_type == 'resets':
                reset_repo = ResetRepository(conn)
                mob_repo = MobileRepository(conn)
                obj_repo = ObjectRepository(conn)
                room_repo = RoomRepository(conn)
                return ResetEditorPanel(
                    self.notebook,
                    reset_repo,
                    mob_repo,
                    obj_repo,
                    room_repo,
                    on_status=self._set_status
                )

            elif entity_type == 'shops':
                shop_repo = ShopRepository(conn)
                mob_repo = MobileRepository(conn)
                return ShopEditorPanel(
                    self.notebook,
                    shop_repo,
                    mob_repo,
                    on_status=self._set_status
                )

            else:
                return self._create_placeholder_panel(
                    f"Area Editor: {entity_type}",
                    f"Database: {db_path.name}\n"
                    f"Entity Type: {entity_type}\n\n"
                    "This entity type editor is not yet implemented."
                )

        elif category == 'game':
            conn = self.db_manager.get_connection(db_path)

            if entity_type == 'gameconfig':
                repository = GameConfigRepository(conn)
                return GameConfigPanel(
                    self.notebook,
                    repository,
                    on_status=self._set_status
                )

            elif entity_type == 'balance_config':
                repository = BalanceConfigRepository(conn)
                return BalanceConfigPanel(
                    self.notebook,
                    repository,
                    on_status=self._set_status
                )

            elif entity_type == 'ability_config':
                repository = AbilityConfigRepository(conn)
                return AbilityConfigPanel(
                    self.notebook,
                    repository,
                    on_status=self._set_status
                )

            elif entity_type == 'audio_config':
                repository = AudioConfigRepository(conn)
                return AudioConfigPanel(
                    self.notebook,
                    repository,
                    on_status=self._set_status
                )

            elif entity_type == 'kingdoms':
                repository = KingdomsRepository(conn)
                return KingdomsPanel(
                    self.notebook,
                    repository,
                    on_status=self._set_status
                )

            elif entity_type == 'bans':
                repository = BansRepository(conn)
                return BansPanel(
                    self.notebook,
                    repository,
                    on_status=self._set_status
                )

            elif entity_type == 'disabled_commands':
                repository = DisabledCommandsRepository(conn)
                return DisabledCommandsPanel(
                    self.notebook,
                    repository,
                    on_status=self._set_status
                )

            elif entity_type == 'topboard' or entity_type == 'leaderboard':
                topboard_repo = TopBoardRepository(conn)
                leaderboard_repo = LeaderboardRepository(conn)
                return LeaderboardPanel(
                    self.notebook,
                    topboard_repo,
                    leaderboard_repo,
                    on_status=self._set_status
                )

            elif entity_type == 'notes':
                repository = NotesRepository(conn)
                return NotesPanel(
                    self.notebook,
                    repository,
                    on_status=self._set_status
                )

            elif entity_type == 'bugs':
                repository = BugsRepository(conn)
                return BugsPanel(
                    self.notebook,
                    repository,
                    on_status=self._set_status
                )

            elif entity_type == 'super_admins':
                repository = SuperAdminsRepository(conn)
                return SuperAdminsPanel(
                    self.notebook,
                    repository,
                    on_status=self._set_status
                )

            elif entity_type == 'immortal_pretitles':
                repository = ImmortalPretitlesRepository(conn)
                return ImmortalPretitlesPanel(
                    self.notebook,
                    repository,
                    on_status=self._set_status
                )

            else:
                return self._create_placeholder_panel(
                    f"Game Config: {entity_type}",
                    f"Table: {entity_type}\n\n"
                    "This editor is not yet implemented."
                )

        elif category == 'player':
            conn = self.db_manager.get_connection(db_path)
            repository = PlayerRepository(conn)
            player_name = db_path.stem
            return PlayerEditorPanel(
                self.notebook,
                repository,
                player_name,
                on_status=self._set_status
            )

        return None

    def _create_placeholder_panel(self, title: str, message: str) -> ttk.Frame:
        """Create a placeholder panel for unimplemented editors."""
        panel = ttk.Frame(self.notebook)

        label = ttk.Label(
            panel,
            text=f"{title}\n\n{message}",
            justify=tk.LEFT,
            font=('Consolas', 11)
        )
        label.pack(padx=20, pady=20, anchor=tk.NW)

        return panel

    def _get_tab_title(self, category: str, db_path: Path, entity_type: str) -> str:
        """Generate a title for a tab."""
        if category == 'help':
            return f"Help: {db_path.stem}"
        elif category == 'area':
            return f"{db_path.stem}: {entity_type.title()}"
        elif category == 'game':
            # Friendly names for game config tables
            names = {
                'gameconfig': 'Configuration',
                'balance_config': 'Balance Config',
                'ability_config': 'Ability Config',
                'audio_config': 'Audio Config',
                'kingdoms': 'Kingdoms',
                'topboard': 'Leaderboards',
                'leaderboard': 'Leaderboards',
                'notes': 'Notes',
                'bugs': 'Bug Reports',
                'bans': 'Bans',
                'disabled_commands': 'Disabled Cmds',
                'super_admins': 'Super Admins',
                'immortal_pretitles': 'Imm Pretitles',
            }
            return names.get(entity_type, entity_type.title())
        elif category == 'player':
            return f"Player: {db_path.stem}"
        return f"{category}: {entity_type}"

    def _on_tab_middle_click(self, event):
        """Handle middle-click to close tab."""
        try:
            # Identify which tab was clicked
            clicked_tab = self.notebook.tk.call(
                self.notebook._w, "identify", "tab", event.x, event.y
            )
            if clicked_tab is not None and clicked_tab != '':
                tab_index = int(clicked_tab)
                # Don't close the welcome tab (index 0)
                if tab_index > 0:
                    self._close_tab(tab_index)
        except (tk.TclError, ValueError):
            pass

    def _close_tab(self, tab_index: int):
        """Close a tab by index."""
        try:
            tab_widget = self.notebook.tabs()[tab_index]
            panel = self.notebook.nametowidget(tab_widget)

            # Check for unsaved changes
            if hasattr(panel, 'check_unsaved'):
                if not panel.check_unsaved():
                    return

            # Find and remove from tracking
            for tab_id, (_, _, _, tracked_panel) in list(self._open_tabs.items()):
                if tracked_panel == panel:
                    del self._open_tabs[tab_id]
                    break

            self.notebook.forget(tab_index)
        except (tk.TclError, IndexError):
            pass

    def _on_tab_right_click(self, event):
        """Handle right-click to show context menu."""
        try:
            clicked_tab = self.notebook.tk.call(
                self.notebook._w, "identify", "tab", event.x, event.y
            )
            if clicked_tab is not None and clicked_tab != '':
                tab_index = int(clicked_tab)
                # Don't show menu for welcome tab
                if tab_index > 0:
                    self._menu_tab_index = tab_index
                    self._tab_menu.tk_popup(event.x_root, event.y_root)
        except (tk.TclError, ValueError):
            pass

    def _close_menu_tab(self):
        """Close the tab that was right-clicked."""
        if self._menu_tab_index is not None and self._menu_tab_index > 0:
            self._close_tab(self._menu_tab_index)
            self._menu_tab_index = None

    def _close_current_tab(self, event=None):
        """Close the currently selected tab (Ctrl+W)."""
        try:
            current = self.notebook.index(self.notebook.select())
            if current > 0:  # Don't close welcome tab
                self._close_tab(current)
        except tk.TclError:
            pass

    def _close_other_tabs(self):
        """Close all tabs except the right-clicked one."""
        if self._menu_tab_index is None:
            return

        # Close tabs from highest index to lowest (to avoid index shifting)
        tabs = list(range(len(self.notebook.tabs())))
        for i in sorted(tabs, reverse=True):
            if i != 0 and i != self._menu_tab_index:  # Keep welcome and right-clicked tab
                self._close_tab(i)

        self._menu_tab_index = None

    def _close_all_tabs(self):
        """Close all tabs except Welcome."""
        # Close from highest index to lowest
        for i in range(len(self.notebook.tabs()) - 1, 0, -1):
            self._close_tab(i)

    def _set_status(self, message: str):
        """Set the status bar message."""
        self.status_var.set(message)

    def _show_about(self):
        """Show about dialog."""
        messagebox.showinfo(
            "About",
            "Dystopia MUD Database Editor\n\n"
            "A GUI tool for editing MUD database files.\n\n"
            "Supports:\n"
            "- Help files\n"
            "- Area data (rooms, mobs, objects)\n"
            "- Game configuration\n"
            "- Player data"
        )

    def _on_close(self):
        """Handle window close."""
        # Check all open tabs for unsaved changes
        for tab_id, (_, _, _, panel) in self._open_tabs.items():
            if hasattr(panel, 'check_unsaved'):
                if not panel.check_unsaved():
                    return

        # Close database connections
        self.db_manager.close_all()

        self.root.destroy()

    def run(self):
        """Run the application main loop."""
        self.root.mainloop()
