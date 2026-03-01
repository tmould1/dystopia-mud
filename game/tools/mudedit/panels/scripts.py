"""
Script editor panel.

Provides interface for editing Lua scripts attached to mobs, objects, and rooms.
"""

import tkinter as tk
from tkinter import ttk, messagebox
from typing import Callable, Dict, List, Optional

from ..db.repository import ScriptRepository, MobileRepository, ObjectRepository, RoomRepository
from ..widgets.flags import FlagEditor

CUSTOM_LABEL = '(Custom)'


class ScriptEditorPanel(ttk.Frame):
    """
    Editor panel for Lua scripts.

    Displays scripts with owner info and allows editing trigger, code, pattern, etc.
    Supports library references: when a library script is selected, the code and
    trigger fields are read-only previews of the library template.
    """

    def __init__(
        self,
        parent,
        repository: ScriptRepository,
        mob_repository: MobileRepository,
        obj_repository: ObjectRepository,
        room_repository: RoomRepository,
        library_names: Optional[List[str]] = None,
        library_data: Optional[Dict[str, Dict]] = None,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.mob_repo = mob_repository
        self.obj_repo = obj_repository
        self.room_repo = room_repository
        self.on_status = on_status or (lambda msg: None)
        self._library_names = library_names or []
        self._library_data = library_data or {}

        self._mob_cache: Dict[int, str] = {}
        self._obj_cache: Dict[int, str] = {}
        self._room_cache: Dict[int, str] = {}
        self.current_id: Optional[int] = None
        self.unsaved = False

        self._build_caches()
        self._build_ui()
        self._load_entries()

    def _build_caches(self):
        """Build vnum -> name caches for all entity types."""
        for mob in self.mob_repo.list_all():
            self._mob_cache[mob['vnum']] = mob.get('short_descr', f"mob#{mob['vnum']}")
        for obj in self.obj_repo.list_all():
            self._obj_cache[obj['vnum']] = obj.get('short_descr', f"obj#{obj['vnum']}")
        for room in self.room_repo.list_all():
            self._room_cache[room['vnum']] = room.get('name', f"room#{room['vnum']}")

    def _build_ui(self):
        """Build the panel UI."""
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: script list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('id', 'owner', 'vnum', 'trigger', 'name'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('id', text='ID')
        self.tree.heading('owner', text='Owner')
        self.tree.heading('vnum', text='Vnum')
        self.tree.heading('trigger', text='Trigger')
        self.tree.heading('name', text='Name')
        self.tree.column('id', width=40, stretch=False)
        self.tree.column('owner', width=50, stretch=False)
        self.tree.column('vnum', width=160, stretch=False)
        self.tree.column('trigger', width=80, stretch=False)
        self.tree.column('name', width=150)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 scripts")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor (scrollable)
        right_outer = ttk.Frame(paned)
        paned.add(right_outer, weight=2)

        canvas = tk.Canvas(right_outer, highlightthickness=0)
        right_scroll = ttk.Scrollbar(right_outer, orient=tk.VERTICAL, command=canvas.yview)
        canvas.configure(yscrollcommand=right_scroll.set)
        right_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        right_frame = ttk.LabelFrame(canvas, text="Edit Script")
        canvas_window = canvas.create_window((0, 0), window=right_frame, anchor=tk.NW)

        def _on_right_configure(event):
            canvas.configure(scrollregion=canvas.bbox("all"))

        def _on_canvas_configure(event):
            canvas.itemconfig(canvas_window, width=event.width)

        right_frame.bind('<Configure>', _on_right_configure)
        canvas.bind('<Configure>', _on_canvas_configure)

        # ID
        row_id = ttk.Frame(right_frame)
        row_id.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row_id, text="ID:").pack(side=tk.LEFT)
        self.id_label = ttk.Label(row_id, text="-", font=('Consolas', 10, 'bold'))
        self.id_label.pack(side=tk.LEFT, padx=4)

        # Library selector
        row_lib = ttk.Frame(right_frame)
        row_lib.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row_lib, text="Library:").pack(side=tk.LEFT)
        self.library_var = tk.StringVar(value=CUSTOM_LABEL)
        lib_values = [CUSTOM_LABEL] + self._library_names
        self.library_combo = ttk.Combobox(
            row_lib, textvariable=self.library_var,
            values=lib_values, state='readonly', width=30)
        self.library_combo.pack(side=tk.LEFT, padx=4)
        self.library_combo.bind('<<ComboboxSelected>>', self._on_library_selected)

        # Name
        row_name = ttk.Frame(right_frame)
        row_name.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row_name, text="Name:").pack(side=tk.LEFT)
        self.name_var = tk.StringVar()
        self.name_var.trace_add('write', lambda *_: self._mark_unsaved())
        self.name_entry = ttk.Entry(row_name, textvariable=self.name_var, width=40)
        self.name_entry.pack(side=tk.LEFT, padx=4, fill=tk.X, expand=True)

        # Owner Type
        row_owner = ttk.Frame(right_frame)
        row_owner.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row_owner, text="Owner Type:").pack(side=tk.LEFT)
        self.owner_type_var = tk.StringVar(value='mob')
        owner_combo = ttk.Combobox(
            row_owner, textvariable=self.owner_type_var,
            values=['mob', 'obj', 'room'], state='readonly', width=8)
        owner_combo.pack(side=tk.LEFT, padx=4)
        owner_combo.bind('<<ComboboxSelected>>', lambda e: (self._mark_unsaved(), self._update_vnum_label()))

        # Owner Vnum
        row_vnum = ttk.Frame(right_frame)
        row_vnum.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row_vnum, text="Owner Vnum:").pack(side=tk.LEFT)
        self.vnum_var = tk.StringVar(value='0')
        self.vnum_var.trace_add('write', lambda *_: (self._mark_unsaved(), self._update_vnum_label()))
        ttk.Entry(row_vnum, textvariable=self.vnum_var, width=10).pack(side=tk.LEFT, padx=4)
        self.vnum_name_label = ttk.Label(row_vnum, text="", foreground='gray')
        self.vnum_name_label.pack(side=tk.LEFT, padx=4)

        # Trigger flags
        self.trigger_editor = FlagEditor(
            right_frame,
            flags={1: 'GREET', 2: 'SPEECH', 4: 'TICK', 8: 'KILL'},
            label="Trigger",
            columns=4,
            on_change=self._on_trigger_change
        )
        self.trigger_editor.pack(fill=tk.X, padx=8, pady=4)

        # Pattern
        row_pat = ttk.Frame(right_frame)
        row_pat.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row_pat, text="Pattern:").pack(side=tk.LEFT)
        self.pattern_var = tk.StringVar()
        self.pattern_var.trace_add('write', lambda *_: self._mark_unsaved())
        self.pattern_entry = ttk.Entry(row_pat, textvariable=self.pattern_var, width=30, state='disabled')
        self.pattern_entry.pack(side=tk.LEFT, padx=4, fill=tk.X, expand=True)
        ttk.Label(row_pat, text="(SPEECH only)", foreground='gray').pack(side=tk.LEFT, padx=4)

        # Chance + Sort Order
        row_misc = ttk.Frame(right_frame)
        row_misc.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row_misc, text="Chance:").pack(side=tk.LEFT)
        self.chance_var = tk.IntVar(value=0)
        ttk.Spinbox(row_misc, from_=0, to=100, width=5, textvariable=self.chance_var,
                    command=self._mark_unsaved).pack(side=tk.LEFT, padx=(2, 4))
        ttk.Label(row_misc, text="(0 = always)", foreground='gray').pack(side=tk.LEFT, padx=(0, 16))

        ttk.Label(row_misc, text="Sort Order:").pack(side=tk.LEFT)
        self.sort_order_var = tk.IntVar(value=0)
        ttk.Spinbox(row_misc, from_=0, to=99, width=5, textvariable=self.sort_order_var,
                    command=self._mark_unsaved).pack(side=tk.LEFT, padx=(2, 0))

        # Code editor
        code_frame = ttk.LabelFrame(right_frame, text="Lua Code")
        code_frame.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)

        code_inner = ttk.Frame(code_frame)
        code_inner.pack(fill=tk.BOTH, expand=True)

        self.code_text = tk.Text(
            code_inner, font=('Consolas', 10), wrap=tk.NONE,
            height=20, undo=True)
        code_xscroll = ttk.Scrollbar(code_inner, orient=tk.HORIZONTAL, command=self.code_text.xview)
        code_yscroll = ttk.Scrollbar(code_inner, orient=tk.VERTICAL, command=self.code_text.yview)
        self.code_text.configure(xscrollcommand=code_xscroll.set, yscrollcommand=code_yscroll.set)

        self.code_text.grid(row=0, column=0, sticky='nsew')
        code_yscroll.grid(row=0, column=1, sticky='ns')
        code_xscroll.grid(row=1, column=0, sticky='ew')
        code_inner.grid_rowconfigure(0, weight=1)
        code_inner.grid_columnconfigure(0, weight=1)

        self.code_text.bind('<<Modified>>', self._on_code_modified)

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)

        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT)

    def _on_code_modified(self, event):
        """Handle code text modification."""
        if self.code_text.edit_modified():
            self._mark_unsaved()
            self.code_text.edit_modified(False)

    def _on_library_selected(self, event=None):
        """Handle library combobox selection."""
        self._mark_unsaved()
        lib_name = self.library_var.get()
        is_library = (lib_name != CUSTOM_LABEL)

        if is_library and lib_name in self._library_data:
            # Preview library code/trigger/pattern/chance (read-only)
            lib = self._library_data[lib_name]
            self.name_var.set(lib_name)
            self.trigger_editor.set_value(lib.get('trigger', 0))
            self.pattern_var.set(lib.get('pattern', '') or '')
            self.chance_var.set(lib.get('chance', 0))
            self.code_text.delete('1.0', tk.END)
            self.code_text.insert('1.0', lib.get('code', ''))
            self.code_text.edit_modified(False)

        self._set_library_mode(is_library)

    def _set_library_mode(self, is_library: bool):
        """Enable/disable editor fields based on library vs custom mode."""
        state = 'disabled' if is_library else 'normal'
        self.name_entry.configure(state=state)
        self.trigger_editor.set_enabled(not is_library)
        self.pattern_entry.configure(state=state)
        self.code_text.configure(
            state=state,
            background='#e0e0e0' if is_library else 'white'
        )

    def _on_trigger_change(self, value: int):
        """Handle trigger flag change — enable/disable pattern field."""
        self._mark_unsaved()
        if self.library_var.get() != CUSTOM_LABEL:
            return  # Library mode — don't change pattern state
        if value & ScriptRepository.TRIG_SPEECH:
            self.pattern_entry.configure(state='normal')
        else:
            self.pattern_entry.configure(state='disabled')

    def _update_vnum_label(self):
        """Update the resolved name label next to vnum entry."""
        try:
            vnum = int(self.vnum_var.get())
        except (ValueError, tk.TclError):
            self.vnum_name_label.configure(text="")
            return

        name = self._resolve_owner(self.owner_type_var.get(), vnum)
        self.vnum_name_label.configure(text=name)

    def _resolve_owner(self, owner_type: str, vnum: int) -> str:
        """Look up owner name from cache."""
        if owner_type == 'mob':
            return self._mob_cache.get(vnum, '')
        elif owner_type == 'obj':
            return self._obj_cache.get(vnum, '')
        elif owner_type == 'room':
            return self._room_cache.get(vnum, '')
        return ''

    def _load_entries(self):
        """Load all scripts into the tree."""
        entries = self.repository.list_all()

        self.tree.delete(*self.tree.get_children())
        for script in entries:
            sid = script['id']
            owner = script['owner_type']
            vnum = script['owner_vnum']
            owner_name = self._resolve_owner(owner, vnum)
            vnum_display = f"{vnum} {owner_name}" if owner_name else str(vnum)

            # For library scripts, show trigger from library data
            lib_name = script.get('library_name')
            if lib_name and lib_name in self._library_data:
                trigger_str = ScriptRepository.trigger_display(
                    self._library_data[lib_name].get('trigger', 0))
            else:
                trigger_str = ScriptRepository.trigger_display(script['trigger'])

            name = lib_name if lib_name else (script.get('name', '') or '')
            self.tree.insert('', tk.END, iid=str(sid),
                             values=(sid, owner, vnum_display, trigger_str, name))

        self.count_var.set(f"{len(entries)} scripts")
        self.on_status(f"Loaded {len(entries)} scripts")

    def _on_select(self, event):
        """Handle script selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved", "Discard changes?"):
                if self.current_id is not None:
                    self.tree.selection_set(str(self.current_id))
                return

        selection = self.tree.selection()
        if selection:
            self._load_entry(int(selection[0]))

    def _load_entry(self, script_id: int):
        """Load a script into the editor."""
        script = self.repository.get_by_id(script_id)
        if not script:
            return

        self.current_id = script_id
        self.id_label.configure(text=str(script_id))
        self.owner_type_var.set(script.get('owner_type', 'mob'))
        self.vnum_var.set(str(script.get('owner_vnum', 0)))
        self.sort_order_var.set(script.get('sort_order', 0))

        lib_name = script.get('library_name')
        is_library = bool(lib_name and lib_name in self._library_data)

        if is_library:
            self.library_var.set(lib_name)
            lib = self._library_data[lib_name]
            self.name_var.set(lib_name)
            self.trigger_editor.set_value(lib.get('trigger', 0))
            self.pattern_var.set(lib.get('pattern', '') or '')
            self.chance_var.set(lib.get('chance', 0))
            self.code_text.configure(state='normal')
            self.code_text.delete('1.0', tk.END)
            self.code_text.insert('1.0', lib.get('code', ''))
            self.code_text.edit_modified(False)
        else:
            self.library_var.set(CUSTOM_LABEL)
            self.name_var.set(script.get('name', ''))
            self.trigger_editor.set_value(script.get('trigger', 0))
            self.pattern_var.set(script.get('pattern', '') or '')
            self.chance_var.set(script.get('chance', 0))
            self.code_text.configure(state='normal')
            self.code_text.delete('1.0', tk.END)
            self.code_text.insert('1.0', script.get('code', ''))
            self.code_text.edit_modified(False)

        self._set_library_mode(is_library)

        # Enable/disable pattern based on trigger (only for custom scripts)
        if not is_library:
            trigger = script.get('trigger', 0)
            if trigger & ScriptRepository.TRIG_SPEECH:
                self.pattern_entry.configure(state='normal')
            else:
                self.pattern_entry.configure(state='disabled')

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current script."""
        if self.current_id is None:
            return

        try:
            vnum = int(self.vnum_var.get())
        except (ValueError, tk.TclError):
            messagebox.showwarning("Warning", "Owner vnum must be a number.")
            return

        lib_name = self.library_var.get()
        is_library = (lib_name != CUSTOM_LABEL)

        if is_library:
            # Library reference — save minimal data
            data = {
                'name':         '',
                'owner_type':   self.owner_type_var.get(),
                'owner_vnum':   vnum,
                'trigger':      0,
                'code':         '',
                'pattern':      None,
                'chance':       0,
                'sort_order':   self.sort_order_var.get(),
                'library_name': lib_name,
            }
        else:
            # Custom inline script
            code = self.code_text.get('1.0', tk.END).rstrip('\n')
            if not code.strip():
                messagebox.showwarning("Warning", "Script code cannot be empty.")
                return

            trigger = self.trigger_editor.get_value()
            if trigger == 0:
                messagebox.showwarning("Warning", "Select at least one trigger type.")
                return

            pattern = self.pattern_var.get().strip() or None
            if not (trigger & ScriptRepository.TRIG_SPEECH):
                pattern = None

            data = {
                'name':         self.name_var.get().strip(),
                'owner_type':   self.owner_type_var.get(),
                'owner_vnum':   vnum,
                'trigger':      trigger,
                'code':         code,
                'pattern':      pattern,
                'chance':       self.chance_var.get(),
                'sort_order':   self.sort_order_var.get(),
                'library_name': None,
            }

        self.repository.update(self.current_id, data)
        self.unsaved = False
        self._load_entries()
        self.tree.selection_set(str(self.current_id))
        self.on_status(f"Saved script [{self.current_id}]")

    def _new(self):
        """Create a new script with defaults."""
        new_id = self.repository.insert({
            'owner_type': 'mob',
            'owner_vnum': 0,
            'trigger': ScriptRepository.TRIG_TICK,
            'name': 'New Script',
            'code': '-- New Lua script\nfunction on_tick(mob)\n    return false\nend\n',
            'chance': 0,
            'sort_order': 0,
        })

        self._load_entries()
        self.tree.selection_set(str(new_id))
        self._load_entry(new_id)
        self.on_status(f"Created script [{new_id}]")

    def _delete(self):
        """Delete current script."""
        if self.current_id is None:
            return

        script = self.repository.get_by_id(self.current_id)
        name = script.get('name', '') if script else str(self.current_id)
        if not messagebox.askyesno("Confirm", f"Delete script '{name}'?"):
            return

        self.repository.delete(self.current_id)
        self.current_id = None
        self._clear_editor()
        self._load_entries()
        self.on_status("Script deleted")

    def _clear_editor(self):
        """Clear the editor fields."""
        self.current_id = None
        self.id_label.configure(text="-")
        self.library_var.set(CUSTOM_LABEL)
        self._set_library_mode(False)
        self.name_var.set('')
        self.owner_type_var.set('mob')
        self.vnum_var.set('0')
        self.trigger_editor.set_value(0)
        self.pattern_var.set('')
        self.pattern_entry.configure(state='disabled')
        self.chance_var.set(0)
        self.sort_order_var.set(0)
        self.code_text.delete('1.0', tk.END)
        self.code_text.edit_modified(False)
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved", "Discard changes?")
        return True
