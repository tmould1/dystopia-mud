"""
Room editor panel with exit sub-editor.

Provides CRUD interface for room entities including exits, room texts,
sector column in list, and cross-references showing spawned entities.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Dict, List, Optional
import sys
from pathlib import Path

# Add mudlib to path for model imports
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from mudlib.models import SECTOR_NAMES, DIR_NAMES, ROOM_FLAGS, EXIT_FLAGS, WEAR_LOCATIONS

from ..db.repository import RoomRepository, ExitRepository, ResetRepository
from ..widgets import ColorTextEditor, EntityListPanel, strip_colors


class RoomEditorPanel(ttk.Frame):
    """
    Editor panel for room entities.

    Provides editing for:
    - Basic info (vnum, name, sector type)
    - Description with color preview
    - Room flags
    - Exits (6 directions with destination, door flags, key)
    - Cross-references (entities spawned in this room)
    """

    def __init__(
        self,
        parent,
        repository: RoomRepository,
        on_status: Optional[Callable[[str], None]] = None,
        reset_repo: Optional[ResetRepository] = None,
        mob_repo=None,
        obj_repo=None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.exit_repository = ExitRepository(repository.conn)
        self.reset_repo = reset_repo
        self.mob_repo = mob_repo
        self.obj_repo = obj_repo
        self.on_status = on_status or (lambda msg: None)

        self.current_vnum: Optional[int] = None
        self.unsaved = False

        # Cache room names for exit display
        self._room_cache: Dict[int, str] = {}
        self._mob_cache: Dict[int, str] = {}
        self._obj_cache: Dict[int, str] = {}

        self._build_ui()
        self._build_caches()
        self._load_entries()

    def _build_caches(self):
        """Build caches for entity name resolution."""
        rooms = self.repository.list_all()
        self._room_cache = {r['vnum']: r['name'] for r in rooms}

        if self.mob_repo:
            for mob in self.mob_repo.list_all():
                self._mob_cache[mob['vnum']] = mob.get('short_descr', f"mob#{mob['vnum']}")

        if self.obj_repo:
            for obj in self.obj_repo.list_all():
                self._obj_cache[obj['vnum']] = obj.get('short_descr', f"obj#{obj['vnum']}")

    def _build_ui(self):
        """Build the panel UI."""
        # Main horizontal pane
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left panel: entity list with sector column
        self.list_panel = EntityListPanel(
            paned,
            columns=[
                ('vnum', 'Vnum', 60),
                ('name', 'Name', 200),
                ('sector', 'Sector', 80),
            ],
            on_select=self._on_entry_select,
            search_columns=['name', 'description']
        )
        paned.add(self.list_panel, weight=1)

        # Right panel: editor (scrollable)
        right_container = ttk.Frame(paned)
        paned.add(right_container, weight=3)

        self._canvas = tk.Canvas(right_container, highlightthickness=0)
        scrollbar = ttk.Scrollbar(right_container, orient=tk.VERTICAL, command=self._canvas.yview)

        self.editor_frame = ttk.Frame(self._canvas)

        self._canvas.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self._canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        canvas_frame = self._canvas.create_window((0, 0), window=self.editor_frame, anchor=tk.NW)

        def configure_scroll(event):
            self._canvas.configure(scrollregion=self._canvas.bbox("all"))
            self._canvas.itemconfig(canvas_frame, width=event.width)

        self.editor_frame.bind('<Configure>', lambda e: self._canvas.configure(scrollregion=self._canvas.bbox("all")))
        self._canvas.bind('<Configure>', configure_scroll)

        # Mousewheel: bind only when hovering over this canvas
        def _on_enter(event):
            self._canvas.bind_all('<MouseWheel>',
                                  lambda e: self._canvas.yview_scroll(int(-1 * (e.delta / 120)), "units"))

        def _on_leave(event):
            self._canvas.unbind_all('<MouseWheel>')

        self._canvas.bind('<Enter>', _on_enter)
        self._canvas.bind('<Leave>', _on_leave)

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

        # === Room Contents (cross-references) ===
        refs_frame = ttk.LabelFrame(parent, text="Room Contents (from Resets)")
        refs_frame.pack(fill=tk.X, padx=4, pady=2)

        self.refs_tree = ttk.Treeview(
            refs_frame,
            columns=('type', 'vnum', 'name', 'detail'),
            show='headings',
            height=5,
            selectmode='browse'
        )
        self.refs_tree.heading('type', text='Type')
        self.refs_tree.heading('vnum', text='Vnum')
        self.refs_tree.heading('name', text='Name')
        self.refs_tree.heading('detail', text='Detail')
        self.refs_tree.column('type', width=50, stretch=False)
        self.refs_tree.column('vnum', width=60, stretch=False)
        self.refs_tree.column('name', width=200)
        self.refs_tree.column('detail', width=120)
        self.refs_tree.pack(fill=tk.X, padx=4, pady=4)

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
        """Load all rooms from database with sector column."""
        entries = self.repository.list_all()
        # Add sector display name for the list column
        for entry in entries:
            sector_id = entry.get('sector_type', 0)
            entry['sector'] = SECTOR_NAMES.get(sector_id, f'#{sector_id}')
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

        # Cross-references
        self._update_refs(vnum)

        self.unsaved = False
        self.on_status(f"Loaded room [{vnum}] {room.get('name', '')}")

    def _update_refs(self, vnum: int):
        """Populate room contents from resets."""
        self.refs_tree.delete(*self.refs_tree.get_children())

        if not self.reset_repo:
            return

        try:
            resets = self.reset_repo.list_all()
        except Exception:
            return

        # Walk resets to find what spawns in this room
        # M commands set the "current mob" context; G/E follow the last M
        last_mob_vnum = None
        last_mob_room = None

        for reset in resets:
            cmd = reset.get('command', '?')
            arg1 = reset.get('arg1', 0)
            arg2 = reset.get('arg2', 0)
            arg3 = reset.get('arg3', 0)

            if cmd == 'M':
                last_mob_vnum = arg1
                last_mob_room = arg3
                if arg3 == vnum:
                    mob_name = self._mob_cache.get(arg1, f'#{arg1}')
                    self.refs_tree.insert('', tk.END,
                                          values=('Mob', arg1, mob_name, f'max {arg2}'))

            elif cmd == 'O':
                last_mob_vnum = None
                last_mob_room = None
                if arg3 == vnum:
                    obj_name = self._obj_cache.get(arg1, f'#{arg1}')
                    self.refs_tree.insert('', tk.END,
                                          values=('Obj', arg1, obj_name, 'in room'))

            elif cmd == 'G':
                if last_mob_room == vnum and last_mob_vnum:
                    obj_name = self._obj_cache.get(arg1, f'#{arg1}')
                    mob_name = self._mob_cache.get(last_mob_vnum, f'#{last_mob_vnum}')
                    self.refs_tree.insert('', tk.END,
                                          values=('Give', arg1, obj_name,
                                                  f'to {mob_name[:20]}'))

            elif cmd == 'E':
                if last_mob_room == vnum and last_mob_vnum:
                    obj_name = self._obj_cache.get(arg1, f'#{arg1}')
                    wear_name = WEAR_LOCATIONS.get(arg3, f'loc_{arg3}')
                    self.refs_tree.insert('', tk.END,
                                          values=('Equip', arg1, obj_name, wear_name))

            elif cmd in ('D', 'R'):
                last_mob_vnum = None
                last_mob_room = None

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
        self._build_caches()
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

        self._build_caches()
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
        self._build_caches()
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
        self.refs_tree.delete(*self.refs_tree.get_children())
        self.unsaved = False

    def check_unsaved(self) -> bool:
        """Check for unsaved changes and prompt user."""
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes", "Discard unsaved changes?")
        return True
