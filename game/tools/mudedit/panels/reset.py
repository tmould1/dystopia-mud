"""
Reset editor panel.

Provides interface for editing area reset commands (mob/object spawning)
with hierarchical grouping: G/E commands nest under their parent M command.
"""

import tkinter as tk
from tkinter import ttk, messagebox
from typing import Callable, Dict, Optional

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from mudlib.models import WEAR_LOCATIONS

from ..db.repository import ResetRepository, MobileRepository, ObjectRepository, RoomRepository


# Reset command descriptions
RESET_COMMANDS = {
    'M': 'Load Mobile',
    'O': 'Load Object (room)',
    'G': 'Give Object (to mob)',
    'E': 'Equip Object (on mob)',
    'P': 'Put Object (in container)',
    'D': 'Set Door State',
    'R': 'Randomize Exits',
}


class ResetEditorPanel(ttk.Frame):
    """
    Editor panel for area reset commands.

    Displays resets with resolved entity names. G/E commands are shown as
    children of their preceding M command in the treeview hierarchy.
    """

    def __init__(
        self,
        parent,
        repository: ResetRepository,
        mob_repository: MobileRepository,
        obj_repository: ObjectRepository,
        room_repository: RoomRepository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.mob_repo = mob_repository
        self.obj_repo = obj_repository
        self.room_repo = room_repository
        self.on_status = on_status or (lambda msg: None)

        # Caches for entity names
        self._mob_cache: Dict[int, str] = {}
        self._obj_cache: Dict[int, str] = {}
        self._room_cache: Dict[int, str] = {}

        self.current_id: Optional[int] = None
        self.unsaved = False

        self._build_caches()
        self._build_ui()
        self._load_entries()

    def _build_caches(self):
        """Build name caches for entity resolution."""
        for mob in self.mob_repo.list_all():
            self._mob_cache[mob['vnum']] = mob.get('short_descr', f"mob#{mob['vnum']}")

        for obj in self.obj_repo.list_all():
            self._obj_cache[obj['vnum']] = obj.get('short_descr', f"obj#{obj['vnum']}")

        for room in self.room_repo.list_all():
            self._room_cache[room['vnum']] = room.get('name', f"room#{room['vnum']}")

    def _build_ui(self):
        """Build the panel UI."""
        # Main horizontal pane
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: reset list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        # Treeview for resets (tree mode for hierarchy)
        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('cmd', 'description'),
            show='tree headings',
            selectmode='browse'
        )
        self.tree.heading('cmd', text='Cmd')
        self.tree.heading('description', text='Description')
        self.tree.column('#0', width=20, stretch=False)
        self.tree.column('cmd', width=40, stretch=False)
        self.tree.column('description', width=350)

        # Row color tags
        self.tree.tag_configure('mob', background='#2a2a3a')
        self.tree.tag_configure('give', background='#1a2a1a')
        self.tree.tag_configure('obj', background='#2a2a1a')
        self.tree.tag_configure('door', background='#2a1a1a')

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        # Count label
        self.count_var = tk.StringVar(value="0 resets")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor
        right_frame = ttk.LabelFrame(paned, text="Edit Reset")
        paned.add(right_frame, weight=1)

        # Command type
        row1 = ttk.Frame(right_frame)
        row1.pack(fill=tk.X, padx=4, pady=4)

        ttk.Label(row1, text="Command:").pack(side=tk.LEFT)
        self.cmd_var = tk.StringVar()
        cmd_list = [f"{k} - {v}" for k, v in RESET_COMMANDS.items()]
        self.cmd_combo = ttk.Combobox(
            row1, textvariable=self.cmd_var,
            values=cmd_list, state='readonly', width=25
        )
        self.cmd_combo.pack(side=tk.LEFT, padx=(4, 0))
        self.cmd_combo.bind('<<ComboboxSelected>>', self._on_cmd_change)

        # Arguments
        self.arg_frame = ttk.Frame(right_frame)
        self.arg_frame.pack(fill=tk.X, padx=4, pady=4)

        self.arg_labels = []
        self.arg_vars = []
        self.arg_entries = []
        self.arg_name_labels = []

        for i in range(4):
            row = ttk.Frame(self.arg_frame)
            row.pack(fill=tk.X, pady=2)

            lbl = ttk.Label(row, text=f"Arg{i+1}:", width=10)
            lbl.pack(side=tk.LEFT)
            self.arg_labels.append(lbl)

            var = tk.StringVar()
            var.trace_add('write', lambda *_, idx=i: self._on_arg_change(idx))
            self.arg_vars.append(var)

            entry = ttk.Entry(row, textvariable=var, width=10)
            entry.pack(side=tk.LEFT, padx=(2, 8))
            self.arg_entries.append(entry)

            name_lbl = ttk.Label(row, text="", foreground='gray', width=30)
            name_lbl.pack(side=tk.LEFT)
            self.arg_name_labels.append(name_lbl)

        # Button bar
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=4, pady=8)

        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Move Up", command=self._move_up).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Move Down", command=self._move_down).pack(side=tk.LEFT)

    def _on_cmd_change(self, event=None):
        """Update argument labels based on command type."""
        self._mark_unsaved()
        self._update_arg_labels()

    def _update_arg_labels(self):
        """Update argument labels based on current command."""
        cmd_str = self.cmd_var.get()
        if not cmd_str:
            return

        cmd = cmd_str.split(' - ')[0]

        # Set labels based on command type
        labels = {
            'M': ['Mob Vnum', 'Max Count', 'Room Vnum', ''],
            'O': ['Obj Vnum', 'Unused', 'Room Vnum', ''],
            'G': ['Obj Vnum', 'Unused', '', ''],
            'E': ['Obj Vnum', 'Unused', 'Wear Loc', ''],
            'P': ['Obj Vnum', 'Unused', 'Container Vnum', ''],
            'D': ['Room Vnum', 'Direction', 'State', ''],
            'R': ['Room Vnum', 'Last Dir', '', ''],
        }

        arg_labels = labels.get(cmd, ['Arg1', 'Arg2', 'Arg3', 'Arg4'])
        for i, lbl in enumerate(arg_labels):
            self.arg_labels[i].configure(text=f"{lbl}:" if lbl else f"Arg{i+1}:")

    def _on_arg_change(self, arg_idx: int):
        """Update entity name display when arg changes."""
        self._mark_unsaved()

        cmd_str = self.cmd_var.get()
        if not cmd_str:
            return
        cmd = cmd_str.split(' - ')[0]

        try:
            vnum = int(self.arg_vars[arg_idx].get())
        except ValueError:
            self.arg_name_labels[arg_idx].configure(text='')
            return

        name = ''
        if cmd == 'M':
            if arg_idx == 0:
                name = self._mob_cache.get(vnum, '(unknown mob)')
            elif arg_idx == 2:
                name = self._room_cache.get(vnum, '(unknown room)')
        elif cmd in ('O', 'G', 'E', 'P'):
            if arg_idx == 0:
                name = self._obj_cache.get(vnum, '(unknown obj)')
            elif arg_idx == 2 and cmd == 'O':
                name = self._room_cache.get(vnum, '(unknown room)')
            elif arg_idx == 2 and cmd == 'P':
                name = self._obj_cache.get(vnum, '(unknown container)')
            elif arg_idx == 2 and cmd == 'E':
                name = WEAR_LOCATIONS.get(vnum, f'loc_{vnum}')
        elif cmd in ('D', 'R'):
            if arg_idx == 0:
                name = self._room_cache.get(vnum, '(unknown room)')

        self.arg_name_labels[arg_idx].configure(text=name[:30] if name else '')

    def _format_reset(self, reset: Dict) -> str:
        """Format a reset for display."""
        cmd = reset.get('command', '?')
        arg1 = reset.get('arg1', 0)
        arg2 = reset.get('arg2', 0)
        arg3 = reset.get('arg3', 0)

        if cmd == 'M':
            mob = self._mob_cache.get(arg1, f'#{arg1}')
            room = self._room_cache.get(arg3, f'#{arg3}')
            return f"Load {mob} in {room} (max {arg2})"
        elif cmd == 'O':
            obj = self._obj_cache.get(arg1, f'#{arg1}')
            room = self._room_cache.get(arg3, f'#{arg3}')
            return f"Load {obj} in {room}"
        elif cmd == 'G':
            obj = self._obj_cache.get(arg1, f'#{arg1}')
            return f"Give {obj}"
        elif cmd == 'E':
            obj = self._obj_cache.get(arg1, f'#{arg1}')
            wear_name = WEAR_LOCATIONS.get(arg3, f'loc_{arg3}')
            return f"Equip {obj} ({wear_name})"
        elif cmd == 'P':
            obj = self._obj_cache.get(arg1, f'#{arg1}')
            cont = self._obj_cache.get(arg3, f'#{arg3}')
            return f"Put {obj} in {cont}"
        elif cmd == 'D':
            room = self._room_cache.get(arg1, f'#{arg1}')
            dirs = ['n', 'e', 's', 'w', 'u', 'd']
            dir_name = dirs[arg2] if 0 <= arg2 < 6 else '?'
            return f"Set door {dir_name} in {room} to {arg3}"
        elif cmd == 'R':
            room = self._room_cache.get(arg1, f'#{arg1}')
            return f"Randomize exits in {room}"
        else:
            return f"{cmd} {arg1} {arg2} {arg3}"

    def _load_entries(self):
        """Load all resets with hierarchical grouping."""
        entries = self.repository.list_all()

        self.tree.delete(*self.tree.get_children())

        current_mob_iid = ''  # iid of the last M command

        for reset in entries:
            cmd = reset.get('command', '?')
            desc = self._format_reset(reset)
            iid = str(reset['id'])

            if cmd == 'M':
                # Root-level mob spawn
                self.tree.insert('', tk.END, iid=iid,
                                 values=(cmd, desc), tags=('mob',), open=True)
                current_mob_iid = iid
            elif cmd in ('G', 'E'):
                # Child of last M command
                parent = current_mob_iid if current_mob_iid else ''
                tag = 'give'
                self.tree.insert(parent, tk.END, iid=iid,
                                 values=(cmd, desc), tags=(tag,))
            elif cmd == 'O':
                # Root-level object spawn
                self.tree.insert('', tk.END, iid=iid,
                                 values=(cmd, desc), tags=('obj',))
                current_mob_iid = ''  # O breaks mob chain
            elif cmd in ('D', 'R'):
                # Root-level door/random
                self.tree.insert('', tk.END, iid=iid,
                                 values=(cmd, desc), tags=('door',))
                current_mob_iid = ''
            else:
                self.tree.insert('', tk.END, iid=iid,
                                 values=(cmd, desc))
                current_mob_iid = ''

        self.count_var.set(f"{len(entries)} resets")
        self.on_status(f"Loaded {len(entries)} resets")

    def _on_select(self, event):
        """Handle reset selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved Changes", "Discard?"):
                if self.current_id:
                    self.tree.selection_set(str(self.current_id))
                return

        selection = self.tree.selection()
        if selection:
            self._load_entry(int(selection[0]))

    def _load_entry(self, reset_id: int):
        """Load a reset into the editor."""
        reset = self.repository.get_by_id(reset_id)
        if not reset:
            return

        self.current_id = reset_id

        cmd = reset.get('command', 'M')
        cmd_desc = RESET_COMMANDS.get(cmd, 'Unknown')
        self.cmd_var.set(f"{cmd} - {cmd_desc}")
        self._update_arg_labels()

        self.arg_vars[0].set(str(reset.get('arg1', 0)))
        self.arg_vars[1].set(str(reset.get('arg2', 0)))
        self.arg_vars[2].set(str(reset.get('arg3', 0)))
        self.arg_vars[3].set('0')

        # Trigger name updates
        for i in range(4):
            self._on_arg_change(i)

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current reset."""
        if self.current_id is None:
            self.on_status("No reset selected")
            return

        cmd_str = self.cmd_var.get()
        cmd = cmd_str.split(' - ')[0] if cmd_str else 'M'

        try:
            arg1 = int(self.arg_vars[0].get() or 0)
            arg2 = int(self.arg_vars[1].get() or 0)
            arg3 = int(self.arg_vars[2].get() or 0)
        except ValueError:
            messagebox.showwarning("Warning", "Arguments must be numbers.")
            return

        self.repository.update(self.current_id, {
            'command': cmd,
            'arg1': arg1,
            'arg2': arg2,
            'arg3': arg3,
        })

        self._load_entries()
        self.tree.selection_set(str(self.current_id))
        self.unsaved = False
        self.on_status(f"Saved reset [{self.current_id}]")

    def _new(self):
        """Create a new reset."""
        # Get max sort_order
        entries = self.repository.list_all()
        max_sort = max((e.get('sort_order', 0) for e in entries), default=0)

        new_id = self.repository.insert({
            'command': 'M',
            'arg1': 0,
            'arg2': 1,
            'arg3': 0,
            'sort_order': max_sort + 1,
        })

        self._load_entries()
        self.tree.selection_set(str(new_id))
        self._load_entry(new_id)
        self.on_status(f"Created reset [{new_id}]")

    def _delete(self):
        """Delete current reset."""
        if self.current_id is None:
            return

        if not messagebox.askyesno("Confirm", f"Delete reset [{self.current_id}]?"):
            return

        self.repository.delete(self.current_id)
        self.current_id = None
        self._clear_editor()
        self._load_entries()
        self.on_status("Reset deleted")

    def _move_up(self):
        """Move current reset up in order."""
        self._move_reset(-1)

    def _move_down(self):
        """Move current reset down in order."""
        self._move_reset(1)

    def _move_reset(self, direction: int):
        """Move reset in sort order."""
        if self.current_id is None:
            return

        entries = self.repository.list_all()
        ids = [e['id'] for e in entries]

        try:
            idx = ids.index(self.current_id)
        except ValueError:
            return

        new_idx = idx + direction
        if new_idx < 0 or new_idx >= len(ids):
            return

        # Swap sort_order values
        other_id = ids[new_idx]
        my_order = entries[idx].get('sort_order', idx)
        other_order = entries[new_idx].get('sort_order', new_idx)

        self.repository.update(self.current_id, {'sort_order': other_order})
        self.repository.update(other_id, {'sort_order': my_order})

        self._load_entries()
        self.tree.selection_set(str(self.current_id))

    def _clear_editor(self):
        """Clear the editor."""
        self.current_id = None
        self.cmd_var.set('')
        for var in self.arg_vars:
            var.set('')
        for lbl in self.arg_name_labels:
            lbl.configure(text='')
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes", "Discard?")
        return True
