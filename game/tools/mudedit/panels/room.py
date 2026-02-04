"""
Room editor panel with exit sub-editor.

Provides CRUD interface for room entities including exits and room texts.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Dict, List, Optional
import sys
from pathlib import Path

# Add mudlib to path for model imports
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from mudlib.models import SECTOR_NAMES, DIR_NAMES

from ..db.repository import RoomRepository, ExitRepository
from ..widgets import ColorTextEditor, EntityListPanel, strip_colors


# Room flags (from merc.h)
ROOM_FLAGS = {
    1: 'dark',
    2: 'no_mob',
    4: 'indoors',
    8: 'private',
    16: 'safe',
    32: 'solitary',
    64: 'pet_shop',
    128: 'no_recall',
    256: 'no_teleport',
    512: 'total_darkness',
    1024: 'blade_barrier',
    2048: 'arena',
    4096: 'flaming',
    8192: 'silence',
    16384: 'no_gate',
    32768: 'prototype',
}

# Exit flags
EXIT_FLAGS = {
    1: 'door',
    2: 'closed',
    4: 'locked',
    8: 'pickproof',
    16: 'bashproof',
    32: 'passproof',
    64: 'easy',
    128: 'hard',
    256: 'infuriating',
}


class RoomEditorPanel(ttk.Frame):
    """
    Editor panel for room entities.

    Provides editing for:
    - Basic info (vnum, name, sector type)
    - Description with color preview
    - Room flags
    - Exits (6 directions with destination, door flags, key)
    """

    def __init__(
        self,
        parent,
        repository: RoomRepository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        """
        Initialize the room editor panel.

        Args:
            parent: Parent Tkinter widget
            repository: RoomRepository for database operations
            on_status: Callback for status messages
            **kwargs: Additional arguments passed to ttk.Frame
        """
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.exit_repository = ExitRepository(repository.conn)
        self.on_status = on_status or (lambda msg: None)

        self.current_vnum: Optional[int] = None
        self.unsaved = False

        # Cache room names for exit display
        self._room_cache: Dict[int, str] = {}

        self._build_ui()
        self._build_room_cache()
        self._load_entries()

    def _build_room_cache(self):
        """Build cache of room vnum -> name for exit display."""
        rooms = self.repository.list_all()
        self._room_cache = {r['vnum']: r['name'] for r in rooms}

    def _build_ui(self):
        """Build the panel UI."""
        # Main horizontal pane
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left panel: entity list
        self.list_panel = EntityListPanel(
            paned,
            columns=[
                ('vnum', 'Vnum', 60),
                ('name', 'Name', 200),
            ],
            on_select=self._on_entry_select,
            search_columns=['name', 'description']
        )
        paned.add(self.list_panel, weight=1)

        # Right panel: editor (scrollable)
        right_container = ttk.Frame(paned)
        paned.add(right_container, weight=3)

        canvas = tk.Canvas(right_container, highlightthickness=0)
        scrollbar = ttk.Scrollbar(right_container, orient=tk.VERTICAL, command=canvas.yview)

        self.editor_frame = ttk.Frame(canvas)

        canvas.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        canvas_frame = canvas.create_window((0, 0), window=self.editor_frame, anchor=tk.NW)

        def configure_scroll(event):
            canvas.configure(scrollregion=canvas.bbox("all"))
            canvas.itemconfig(canvas_frame, width=event.width)

        self.editor_frame.bind('<Configure>', lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.bind('<Configure>', configure_scroll)

        def on_mousewheel(event):
            canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")
        canvas.bind_all('<MouseWheel>', on_mousewheel)

        self._build_editor(self.editor_frame)

    def _build_editor(self, parent):
        """Build the editor form."""
        # === Basic Info Section ===
        basic_frame = ttk.LabelFrame(parent, text="Basic Info")
        basic_frame.pack(fill=tk.X, padx=4, pady=(4, 2))

        # Row 1: Vnum, Name
        row1 = ttk.Frame(basic_frame)
        row1.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(row1, text="Vnum:").pack(side=tk.LEFT)
        self.vnum_label = ttk.Label(row1, text="-", font=('Consolas', 10, 'bold'))
        self.vnum_label.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row1, text="Name:").pack(side=tk.LEFT)
        self.name_var = tk.StringVar()
        self.name_var.trace_add('write', lambda *_: self._mark_unsaved())
        self.name_entry = ttk.Entry(row1, textvariable=self.name_var, width=50)
        self.name_entry.pack(side=tk.LEFT, padx=(4, 0), fill=tk.X, expand=True)

        # Row 2: Sector type
        row2 = ttk.Frame(basic_frame)
        row2.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(row2, text="Sector:").pack(side=tk.LEFT)
        sector_list = [f"{k} - {v}" for k, v in sorted(SECTOR_NAMES.items())]
        self.sector_var = tk.StringVar()
        self.sector_combo = ttk.Combobox(
            row2, textvariable=self.sector_var,
            values=sector_list, state='readonly', width=20
        )
        self.sector_combo.pack(side=tk.LEFT, padx=(4, 0))
        self.sector_combo.bind('<<ComboboxSelected>>', lambda e: self._mark_unsaved())

        # === Room Flags Section ===
        flags_frame = ttk.LabelFrame(parent, text="Room Flags")
        flags_frame.pack(fill=tk.X, padx=4, pady=2)

        self.flag_vars: Dict[int, tk.BooleanVar] = {}
        flags_grid = ttk.Frame(flags_frame)
        flags_grid.pack(fill=tk.X, padx=4, pady=2)

        col = 0
        row = 0
        for bit, name in sorted(ROOM_FLAGS.items()):
            var = tk.BooleanVar()
            self.flag_vars[bit] = var
            cb = ttk.Checkbutton(
                flags_grid, text=name, variable=var,
                command=self._mark_unsaved
            )
            cb.grid(row=row, column=col, sticky=tk.W, padx=2)
            col += 1
            if col >= 4:
                col = 0
                row += 1

        # === Description Section ===
        desc_frame = ttk.LabelFrame(parent, text="Description")
        desc_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=2)

        self.desc_editor = ColorTextEditor(
            desc_frame, show_preview=True,
            on_change=self._mark_unsaved
        )
        self.desc_editor.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # === Exits Section ===
        exits_frame = ttk.LabelFrame(parent, text="Exits")
        exits_frame.pack(fill=tk.X, padx=4, pady=2)

        self.exit_widgets: Dict[int, Dict] = {}

        for direction, dir_name in enumerate(DIR_NAMES):
            exit_row = ttk.Frame(exits_frame)
            exit_row.pack(fill=tk.X, padx=4, pady=2)

            # Direction label
            ttk.Label(exit_row, text=f"{dir_name.capitalize():6}", width=8).pack(side=tk.LEFT)

            # Destination vnum
            ttk.Label(exit_row, text="To:").pack(side=tk.LEFT)
            dest_var = tk.StringVar()
            dest_entry = ttk.Entry(exit_row, textvariable=dest_var, width=8)
            dest_entry.pack(side=tk.LEFT, padx=(2, 8))
            dest_var.trace_add('write', lambda *_, d=direction: self._on_exit_change(d))

            # Destination name label
            dest_name = ttk.Label(exit_row, text="", foreground='gray', width=25)
            dest_name.pack(side=tk.LEFT, padx=(0, 8))

            # Door checkbox
            door_var = tk.BooleanVar()
            door_cb = ttk.Checkbutton(exit_row, text="Door", variable=door_var)
            door_cb.pack(side=tk.LEFT, padx=(0, 4))
            door_var.trace_add('write', lambda *_: self._mark_unsaved())

            # Key vnum
            ttk.Label(exit_row, text="Key:").pack(side=tk.LEFT)
            key_var = tk.StringVar()
            key_entry = ttk.Entry(exit_row, textvariable=key_var, width=8)
            key_entry.pack(side=tk.LEFT, padx=(2, 8))
            key_var.trace_add('write', lambda *_: self._mark_unsaved())

            # Clear button
            clear_btn = ttk.Button(
                exit_row, text="X", width=2,
                command=lambda d=direction: self._clear_exit(d)
            )
            clear_btn.pack(side=tk.LEFT)

            self.exit_widgets[direction] = {
                'dest_var': dest_var,
                'dest_name': dest_name,
                'door_var': door_var,
                'key_var': key_var,
            }

        # === Button Bar ===
        btn_frame = ttk.Frame(parent)
        btn_frame.pack(fill=tk.X, padx=4, pady=4)

        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Refresh Preview", command=self.desc_editor.refresh_preview).pack(side=tk.LEFT)

    def _on_exit_change(self, direction: int):
        """Update exit destination name when vnum changes."""
        self._mark_unsaved()
        widgets = self.exit_widgets[direction]
        try:
            vnum = int(widgets['dest_var'].get())
            name = self._room_cache.get(vnum, '(unknown/external)')
            widgets['dest_name'].configure(text=name[:25])
        except ValueError:
            widgets['dest_name'].configure(text='')

    def _clear_exit(self, direction: int):
        """Clear an exit."""
        widgets = self.exit_widgets[direction]
        widgets['dest_var'].set('')
        widgets['door_var'].set(False)
        widgets['key_var'].set('')
        widgets['dest_name'].configure(text='')
        self._mark_unsaved()

    def _load_entries(self):
        """Load all rooms from database."""
        entries = self.repository.list_all()
        self.list_panel.set_items(entries, id_column='vnum')
        self.on_status(f"Loaded {len(entries)} rooms")

    def _on_entry_select(self, vnum: int):
        """Handle entry selection from list."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved Changes", "Discard unsaved changes?"):
                if self.current_vnum is not None:
                    self.list_panel.select_item(self.current_vnum)
                return

        self._load_entry(vnum)

    def _load_entry(self, vnum: int):
        """Load a specific room into the editor."""
        room = self.repository.get_with_exits(vnum)
        if not room:
            self.on_status(f"Room {vnum} not found")
            return

        self.current_vnum = vnum

        # Basic info
        self.vnum_label.configure(text=str(vnum))
        self.name_var.set(room.get('name', ''))

        # Sector
        sector_id = room.get('sector_type', 0)
        sector_name = SECTOR_NAMES.get(sector_id, 'unknown')
        self.sector_var.set(f"{sector_id} - {sector_name}")

        # Flags
        room_flags = room.get('room_flags', 0)
        for bit, var in self.flag_vars.items():
            var.set(bool(room_flags & bit))

        # Description
        self.desc_editor.set_text(room.get('description', ''))

        # Exits
        for direction in range(6):
            self._clear_exit(direction)

        for exit_data in room.get('exits', []):
            direction = exit_data.get('direction', 0)
            if direction in self.exit_widgets:
                widgets = self.exit_widgets[direction]
                dest = exit_data.get('to_vnum', 0)
                if dest:
                    widgets['dest_var'].set(str(dest))
                    name = self._room_cache.get(dest, '(unknown/external)')
                    widgets['dest_name'].configure(text=name[:25])

                exit_info = exit_data.get('exit_info', 0)
                widgets['door_var'].set(bool(exit_info & 1))  # EX_DOOR

                key = exit_data.get('key_vnum', 0)
                if key and key > 0:
                    widgets['key_var'].set(str(key))

        self.unsaved = False
        self.on_status(f"Loaded room [{vnum}] {room.get('name', '')}")

    def _mark_unsaved(self):
        """Mark the current entry as having unsaved changes."""
        self.unsaved = True

    def _save(self):
        """Save current room to database."""
        if self.current_vnum is None:
            self.on_status("No room selected")
            return

        name = self.name_var.get().strip()
        if not name:
            messagebox.showwarning("Warning", "Room name cannot be empty.")
            return

        # Get sector ID
        sector_str = self.sector_var.get()
        try:
            sector_id = int(sector_str.split(' - ')[0])
        except (ValueError, IndexError):
            sector_id = 0

        # Calculate flags
        room_flags = 0
        for bit, var in self.flag_vars.items():
            if var.get():
                room_flags |= bit

        data = {
            'name': name,
            'sector_type': sector_id,
            'room_flags': room_flags,
            'description': self.desc_editor.get_text(),
        }

        self.repository.update(self.current_vnum, data)

        # Update exits
        self.exit_repository.delete_for_room(self.current_vnum)

        for direction, widgets in self.exit_widgets.items():
            dest_str = widgets['dest_var'].get().strip()
            if not dest_str:
                continue

            try:
                dest_vnum = int(dest_str)
            except ValueError:
                continue

            exit_info = 1 if widgets['door_var'].get() else 0

            key_str = widgets['key_var'].get().strip()
            try:
                key_vnum = int(key_str) if key_str else 0
            except ValueError:
                key_vnum = 0

            self.repository.conn.execute(
                """INSERT INTO exits (room_vnum, direction, to_vnum, exit_info, key_vnum, description, keyword)
                   VALUES (?, ?, ?, ?, ?, '', '')""",
                (self.current_vnum, direction, dest_vnum, exit_info, key_vnum)
            )

        self.repository.conn.commit()

        # Refresh
        self._build_room_cache()
        self._load_entries()
        self.list_panel.select_item(self.current_vnum)

        self.unsaved = False
        self.on_status(f"Saved room [{self.current_vnum}] {name}")

    def _new(self):
        """Create a new room."""
        entries = self.repository.list_all()
        if entries:
            max_vnum = max(e['vnum'] for e in entries)
            new_vnum = max_vnum + 1
        else:
            new_vnum = 1

        vnum = simpledialog.askinteger("New Room", "Enter vnum:", initialvalue=new_vnum, parent=self)
        if not vnum:
            return

        if self.repository.get_by_id(vnum):
            messagebox.showerror("Error", f"Room {vnum} already exists.")
            return

        self.repository.insert({
            'vnum': vnum,
            'name': 'A New Room',
            'description': '',
            'room_flags': 0,
            'sector_type': 0,
        })

        self._build_room_cache()
        self._load_entries()
        self.list_panel.select_item(vnum)
        self._load_entry(vnum)
        self.on_status(f"Created room [{vnum}]")

    def _delete(self):
        """Delete the current room."""
        if self.current_vnum is None:
            self.on_status("No room selected")
            return

        name = self.name_var.get()
        if not messagebox.askyesno("Confirm Delete", f"Delete room [{self.current_vnum}] {name}?"):
            return

        # Delete exits first
        self.exit_repository.delete_for_room(self.current_vnum)
        self.repository.delete(self.current_vnum)

        old_vnum = self.current_vnum
        self._clear_editor()
        self._build_room_cache()
        self._load_entries()

        self.on_status(f"Deleted room [{old_vnum}] {name}")

    def _clear_editor(self):
        """Clear the editor fields."""
        self.current_vnum = None
        self.vnum_label.configure(text="-")
        self.name_var.set('')
        self.sector_var.set('')
        for var in self.flag_vars.values():
            var.set(False)
        self.desc_editor.clear()
        for direction in range(6):
            self._clear_exit(direction)
        self.unsaved = False

    def check_unsaved(self) -> bool:
        """Check for unsaved changes and prompt user."""
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes", "Discard unsaved changes?")
        return True
