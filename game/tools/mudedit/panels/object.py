"""
Object/Item editor panel.

Provides CRUD interface for object entities with item types, flags, and affects.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Optional
import sys
from pathlib import Path

# Add mudlib to path for model imports
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from mudlib.models import ITEM_TYPES, EXTRA_FLAGS, WEAR_FLAGS, APPLY_TYPES, WEAPON_TYPES

from ..db.repository import ObjectRepository
from ..widgets import ColorTextEditor, EntityListPanel, FlagEditor, strip_colors


class ObjectEditorPanel(ttk.Frame):
    """
    Editor panel for object/item entities.

    Provides editing for:
    - Basic info (vnum, keywords, descriptions)
    - Item type and type-specific values
    - Extra flags and wear flags
    - Stat affects
    """

    def __init__(
        self,
        parent,
        repository: ObjectRepository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        """
        Initialize the object editor panel.

        Args:
            parent: Parent Tkinter widget
            repository: ObjectRepository for database operations
            on_status: Callback for status messages
            **kwargs: Additional arguments passed to ttk.Frame
        """
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.current_vnum: Optional[int] = None
        self.unsaved = False

        self._build_ui()
        self._load_entries()

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
                ('short_descr', 'Name', 200),
                ('item_type', 'Type', 60),
            ],
            on_select=self._on_entry_select,
            search_columns=['name', 'short_descr']
        )
        paned.add(self.list_panel, weight=1)

        # Right panel: editor (scrollable)
        right_container = ttk.Frame(paned)
        paned.add(right_container, weight=3)

        # Create a canvas with scrollbar for the editor
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

        # Mouse wheel scrolling
        def on_mousewheel(event):
            canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")
        canvas.bind_all('<MouseWheel>', on_mousewheel)

        self._build_editor(self.editor_frame)

    def _build_editor(self, parent):
        """Build the editor form."""
        # === Basic Info Section ===
        basic_frame = ttk.LabelFrame(parent, text="Basic Info")
        basic_frame.pack(fill=tk.X, padx=4, pady=(4, 2))

        # Row 1: Vnum, Keywords
        row1 = ttk.Frame(basic_frame)
        row1.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(row1, text="Vnum:").pack(side=tk.LEFT)
        self.vnum_label = ttk.Label(row1, text="-", font=('Consolas', 10, 'bold'))
        self.vnum_label.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row1, text="Keywords:").pack(side=tk.LEFT)
        self.keywords_var = tk.StringVar()
        self.keywords_var.trace_add('write', lambda *_: self._mark_unsaved())
        self.keywords_entry = ttk.Entry(row1, textvariable=self.keywords_var, width=40)
        self.keywords_entry.pack(side=tk.LEFT, padx=(4, 0), fill=tk.X, expand=True)

        # Row 2: Short description
        row2 = ttk.Frame(basic_frame)
        row2.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(row2, text="Short Desc:").pack(side=tk.LEFT)
        self.short_var = tk.StringVar()
        self.short_var.trace_add('write', lambda *_: self._mark_unsaved())
        self.short_entry = ttk.Entry(row2, textvariable=self.short_var, width=60)
        self.short_entry.pack(side=tk.LEFT, padx=(4, 0), fill=tk.X, expand=True)

        # Row 3: Long description (ground desc)
        row3 = ttk.Frame(basic_frame)
        row3.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(row3, text="Ground Desc:").pack(side=tk.LEFT)
        self.long_var = tk.StringVar()
        self.long_var.trace_add('write', lambda *_: self._mark_unsaved())
        self.long_entry = ttk.Entry(row3, textvariable=self.long_var, width=60)
        self.long_entry.pack(side=tk.LEFT, padx=(4, 0), fill=tk.X, expand=True)

        # Row 4: Weight, Cost
        row4 = ttk.Frame(basic_frame)
        row4.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(row4, text="Weight:").pack(side=tk.LEFT)
        self.weight_var = tk.IntVar(value=1)
        self.weight_spin = ttk.Spinbox(
            row4, from_=0, to=999999, width=8,
            textvariable=self.weight_var, command=self._mark_unsaved
        )
        self.weight_spin.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row4, text="Cost:").pack(side=tk.LEFT)
        self.cost_var = tk.IntVar(value=0)
        self.cost_spin = ttk.Spinbox(
            row4, from_=0, to=999999999, width=10,
            textvariable=self.cost_var, command=self._mark_unsaved
        )
        self.cost_spin.pack(side=tk.LEFT, padx=(4, 0))

        # === Item Type Section ===
        type_frame = ttk.LabelFrame(parent, text="Item Type & Values")
        type_frame.pack(fill=tk.X, padx=4, pady=2)

        # Type selector
        type_row = ttk.Frame(type_frame)
        type_row.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(type_row, text="Item Type:").pack(side=tk.LEFT)

        # Build type list for combobox
        self._type_list = [(v, k) for k, v in sorted(ITEM_TYPES.items())]
        type_names = [f"{t[1]} - {t[0]}" for t in self._type_list]

        self.type_var = tk.StringVar()
        self.type_combo = ttk.Combobox(
            type_row, textvariable=self.type_var,
            values=type_names, state='readonly', width=25
        )
        self.type_combo.pack(side=tk.LEFT, padx=(4, 16))
        self.type_combo.bind('<<ComboboxSelected>>', self._on_type_change)

        # Value fields (type-specific)
        values_frame = ttk.Frame(type_frame)
        values_frame.pack(fill=tk.X, padx=4, pady=2)

        self.value_vars = []
        self.value_labels = []
        self.value_spins = []

        for i in range(4):
            lbl = ttk.Label(values_frame, text=f"Value{i}:")
            lbl.pack(side=tk.LEFT)
            self.value_labels.append(lbl)

            var = tk.IntVar(value=0)
            self.value_vars.append(var)

            spin = ttk.Spinbox(
                values_frame, from_=-999999, to=999999999, width=10,
                textvariable=var, command=self._mark_unsaved
            )
            spin.pack(side=tk.LEFT, padx=(2, 12))
            self.value_spins.append(spin)

        # === Flags Section ===
        flags_frame = ttk.LabelFrame(parent, text="Flags")
        flags_frame.pack(fill=tk.X, padx=4, pady=2)

        # Extra Flags
        self.extra_flags = FlagEditor(
            flags_frame, EXTRA_FLAGS,
            label="Extra Flags", columns=4,
            on_change=lambda v: self._mark_unsaved()
        )
        self.extra_flags.pack(fill=tk.X, padx=4, pady=2)

        # Wear Flags
        self.wear_flags = FlagEditor(
            flags_frame, WEAR_FLAGS,
            label="Wear Locations", columns=4,
            on_change=lambda v: self._mark_unsaved()
        )
        self.wear_flags.pack(fill=tk.X, padx=4, pady=2)

        # === Affects Section ===
        affects_frame = ttk.LabelFrame(parent, text="Stat Affects")
        affects_frame.pack(fill=tk.X, padx=4, pady=2)

        # Affects list
        self.affects_tree = ttk.Treeview(
            affects_frame,
            columns=('type', 'modifier'),
            show='headings',
            height=4
        )
        self.affects_tree.heading('type', text='Apply Type')
        self.affects_tree.heading('modifier', text='Modifier')
        self.affects_tree.column('type', width=150)
        self.affects_tree.column('modifier', width=80)
        self.affects_tree.pack(fill=tk.X, padx=4, pady=2)

        # Affects buttons
        aff_btn_frame = ttk.Frame(affects_frame)
        aff_btn_frame.pack(fill=tk.X, padx=4, pady=2)

        ttk.Button(aff_btn_frame, text="Add Affect", command=self._add_affect).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(aff_btn_frame, text="Remove Affect", command=self._remove_affect).pack(side=tk.LEFT)

        # === Power Messages Section ===
        power_frame = ttk.LabelFrame(parent, text="Power Messages (optional)")
        power_frame.pack(fill=tk.X, padx=4, pady=2)

        for field_name, label in [
            ('chpoweron', 'Equip (to char):'),
            ('chpoweroff', 'Remove (to char):'),
            ('victpoweron', 'Equip (to room):'),
            ('victpoweroff', 'Remove (to room):'),
        ]:
            row = ttk.Frame(power_frame)
            row.pack(fill=tk.X, padx=4, pady=1)
            ttk.Label(row, text=label, width=18).pack(side=tk.LEFT)
            var = tk.StringVar()
            var.trace_add('write', lambda *_: self._mark_unsaved())
            setattr(self, f'{field_name}_var', var)
            entry = ttk.Entry(row, textvariable=var, width=50)
            entry.pack(side=tk.LEFT, fill=tk.X, expand=True)

        # === Button Bar ===
        btn_frame = ttk.Frame(parent)
        btn_frame.pack(fill=tk.X, padx=4, pady=4)

        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT)

    def _on_type_change(self, event=None):
        """Handle item type change - update value labels."""
        self._mark_unsaved()
        self._update_value_labels()

    def _update_value_labels(self):
        """Update value field labels based on item type."""
        type_str = self.type_var.get()
        if not type_str:
            return

        # Extract type ID from "id - name" format
        try:
            type_id = int(type_str.split(' - ')[0])
        except (ValueError, IndexError):
            return

        # Type-specific labels
        labels = {
            5: ['Affects', 'Dice Num', 'Dice Size', 'Weapon Type'],  # weapon
            9: ['Armor Class', '-', '-', 'Special Power'],  # armor (only v0 and v3 used)
            15: ['Capacity', 'Flags', 'Key Vnum', '-'],  # container
            17: ['Max Amount', 'Current', 'Liquid', 'Poisoned'],  # drink
            19: ['Hours', '-', '-', 'Poisoned'],  # food
            20: ['Gold', '-', '-', '-'],  # money
        }

        type_labels = labels.get(type_id, ['Value0', 'Value1', 'Value2', 'Value3'])
        for i, lbl in enumerate(type_labels):
            self.value_labels[i].configure(text=f"{lbl}:")

    def _load_entries(self):
        """Load all objects from database."""
        entries = self.repository.list_all()

        # Add type names for display
        for entry in entries:
            type_id = entry.get('item_type', 0)
            entry['item_type'] = ITEM_TYPES.get(type_id, str(type_id))

        self.list_panel.set_items(entries, id_column='vnum')
        self.on_status(f"Loaded {len(entries)} objects")

    def _on_entry_select(self, vnum: int):
        """Handle entry selection from list."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved Changes", "Discard unsaved changes?"):
                if self.current_vnum is not None:
                    self.list_panel.select_item(self.current_vnum)
                return

        self._load_entry(vnum)

    def _load_entry(self, vnum: int):
        """Load a specific object into the editor."""
        obj = self.repository.get_with_affects(vnum)
        if not obj:
            self.on_status(f"Object {vnum} not found")
            return

        self.current_vnum = vnum

        # Basic info
        self.vnum_label.configure(text=str(vnum))
        self.keywords_var.set(obj.get('name', ''))
        self.short_var.set(obj.get('short_descr', ''))
        self.long_var.set(obj.get('description', ''))
        self.weight_var.set(obj.get('weight', 1))
        self.cost_var.set(obj.get('cost', 0))

        # Item type
        type_id = obj.get('item_type', 0)
        type_name = ITEM_TYPES.get(type_id, 'unknown')
        self.type_var.set(f"{type_id} - {type_name}")
        self._update_value_labels()

        # Values
        for i in range(4):
            self.value_vars[i].set(obj.get(f'value{i}', 0))

        # Flags
        self.extra_flags.set_value(obj.get('extra_flags', 0))
        self.wear_flags.set_value(obj.get('wear_flags', 0))

        # Affects
        self.affects_tree.delete(*self.affects_tree.get_children())
        for aff in obj.get('affects', []):
            apply_id = aff.get('location', 0)
            apply_name = APPLY_TYPES.get(apply_id, f'unknown_{apply_id}')
            self.affects_tree.insert('', tk.END, values=(apply_name, aff.get('modifier', 0)))

        # Power messages
        self.chpoweron_var.set(obj.get('chpoweron', ''))
        self.chpoweroff_var.set(obj.get('chpoweroff', ''))
        self.victpoweron_var.set(obj.get('victpoweron', ''))
        self.victpoweroff_var.set(obj.get('victpoweroff', ''))

        self.unsaved = False
        self.on_status(f"Loaded object [{vnum}] {obj.get('short_descr', '')}")

    def _mark_unsaved(self):
        """Mark the current entry as having unsaved changes."""
        self.unsaved = True

    def _add_affect(self):
        """Add a new stat affect."""
        # Simple dialog to add affect
        dialog = tk.Toplevel(self)
        dialog.title("Add Affect")
        dialog.geometry("300x120")
        dialog.transient(self)
        dialog.grab_set()

        ttk.Label(dialog, text="Apply Type:").pack(pady=(10, 2))
        type_var = tk.StringVar()
        type_names = [f"{k} - {v}" for k, v in sorted(APPLY_TYPES.items())]
        type_combo = ttk.Combobox(dialog, textvariable=type_var, values=type_names, state='readonly')
        type_combo.pack()
        type_combo.current(0)

        ttk.Label(dialog, text="Modifier:").pack(pady=(10, 2))
        mod_var = tk.IntVar(value=1)
        mod_spin = ttk.Spinbox(dialog, from_=-1000, to=1000, textvariable=mod_var, width=10)
        mod_spin.pack()

        def on_ok():
            type_str = type_var.get()
            try:
                apply_name = type_str.split(' - ')[1]
            except IndexError:
                apply_name = type_str
            self.affects_tree.insert('', tk.END, values=(apply_name, mod_var.get()))
            self._mark_unsaved()
            dialog.destroy()

        ttk.Button(dialog, text="Add", command=on_ok).pack(pady=10)

    def _remove_affect(self):
        """Remove the selected affect."""
        selection = self.affects_tree.selection()
        if selection:
            self.affects_tree.delete(selection[0])
            self._mark_unsaved()

    def _save(self):
        """Save current object to database."""
        if self.current_vnum is None:
            self.on_status("No object selected")
            return

        keywords = self.keywords_var.get().strip()
        if not keywords:
            messagebox.showwarning("Warning", "Keywords cannot be empty.")
            return

        # Get type ID from combo
        type_str = self.type_var.get()
        try:
            type_id = int(type_str.split(' - ')[0])
        except (ValueError, IndexError):
            type_id = 0

        data = {
            'name': keywords,
            'short_descr': self.short_var.get(),
            'description': self.long_var.get(),
            'weight': self.weight_var.get(),
            'cost': self.cost_var.get(),
            'item_type': type_id,
            'value0': self.value_vars[0].get(),
            'value1': self.value_vars[1].get(),
            'value2': self.value_vars[2].get(),
            'value3': self.value_vars[3].get(),
            'extra_flags': self.extra_flags.get_value(),
            'wear_flags': self.wear_flags.get_value(),
            'chpoweron': self.chpoweron_var.get(),
            'chpoweroff': self.chpoweroff_var.get(),
            'victpoweron': self.victpoweron_var.get(),
            'victpoweroff': self.victpoweroff_var.get(),
        }

        self.repository.update(self.current_vnum, data)

        # Update affects (delete old, insert new)
        self.repository.conn.execute(
            "DELETE FROM object_affects WHERE obj_vnum = ?",
            (self.current_vnum,)
        )

        # Reverse lookup for apply types
        apply_reverse = {v: k for k, v in APPLY_TYPES.items()}

        for item in self.affects_tree.get_children():
            values = self.affects_tree.item(item)['values']
            apply_name = values[0]
            modifier = values[1]
            apply_id = apply_reverse.get(apply_name, 0)

            self.repository.conn.execute(
                "INSERT INTO object_affects (obj_vnum, location, modifier, sort_order) VALUES (?, ?, ?, ?)",
                (self.current_vnum, apply_id, modifier, 0)
            )

        self.repository.conn.commit()

        # Refresh list
        self._load_entries()
        self.list_panel.select_item(self.current_vnum)

        self.unsaved = False
        self.on_status(f"Saved object [{self.current_vnum}] {self.short_var.get()}")

    def _new(self):
        """Create a new object."""
        entries = self.repository.list_all()
        if entries:
            max_vnum = max(e['vnum'] for e in entries)
            new_vnum = max_vnum + 1
        else:
            new_vnum = 1

        vnum = simpledialog.askinteger("New Object", "Enter vnum:", initialvalue=new_vnum, parent=self)
        if not vnum:
            return

        if self.repository.get_by_id(vnum):
            messagebox.showerror("Error", f"Object {vnum} already exists.")
            return

        self.repository.insert({
            'vnum': vnum,
            'name': 'new object',
            'short_descr': 'a new object',
            'description': 'A new object lies here.',
            'item_type': 8,  # treasure
            'extra_flags': 0,
            'wear_flags': 1,  # take
            'value0': 0, 'value1': 0, 'value2': 0, 'value3': 0,
            'weight': 1,
            'cost': 0,
        })

        self._load_entries()
        self.list_panel.select_item(vnum)
        self._load_entry(vnum)
        self.on_status(f"Created object [{vnum}]")

    def _delete(self):
        """Delete the current object."""
        if self.current_vnum is None:
            self.on_status("No object selected")
            return

        short = self.short_var.get()
        if not messagebox.askyesno("Confirm Delete", f"Delete object [{self.current_vnum}] {short}?"):
            return

        # Delete affects first
        self.repository.conn.execute(
            "DELETE FROM object_affects WHERE obj_vnum = ?",
            (self.current_vnum,)
        )
        self.repository.delete(self.current_vnum)

        old_vnum = self.current_vnum
        self._clear_editor()
        self._load_entries()

        self.on_status(f"Deleted object [{old_vnum}] {short}")

    def _clear_editor(self):
        """Clear the editor fields."""
        self.current_vnum = None
        self.vnum_label.configure(text="-")
        self.keywords_var.set('')
        self.short_var.set('')
        self.long_var.set('')
        self.weight_var.set(1)
        self.cost_var.set(0)
        self.type_var.set('')
        for var in self.value_vars:
            var.set(0)
        self.extra_flags.clear()
        self.wear_flags.clear()
        self.affects_tree.delete(*self.affects_tree.get_children())
        self.chpoweron_var.set('')
        self.chpoweroff_var.set('')
        self.victpoweron_var.set('')
        self.victpoweroff_var.set('')
        self.unsaved = False

    def check_unsaved(self) -> bool:
        """Check for unsaved changes and prompt user."""
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes", "Discard unsaved changes?")
        return True
