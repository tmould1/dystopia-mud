"""
Ability configuration editor panel.

Provides hierarchical interface for editing per-ability configuration.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Dict, List, Optional, Tuple


class AbilityConfigPanel(ttk.Frame):
    """
    Editor panel for ability_config table.

    Displays a hierarchical tree: Class > Ability > Parameters
    """

    def __init__(
        self,
        parent,
        repository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.current_key: Optional[str] = None
        self.unsaved = False
        self._tree_data: Dict[str, Tuple[str, str, str]] = {}  # node_id -> (class, ability, param)

        self._build_ui()
        self._load_tree()

    def _build_ui(self):
        """Build the panel UI."""
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: hierarchical tree
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        # Search
        search_frame = ttk.Frame(left_frame)
        search_frame.pack(fill=tk.X, padx=2, pady=2)
        ttk.Label(search_frame, text="Filter:").pack(side=tk.LEFT)
        self.search_var = tk.StringVar()
        self.search_var.trace_add('write', lambda *_: self._filter_tree())
        ttk.Entry(search_frame, textvariable=self.search_var).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(4, 0))

        # Tree
        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(tree_frame, show='tree', selectmode='browse')

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 entries")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Tree buttons
        tree_btn_frame = ttk.Frame(left_frame)
        tree_btn_frame.pack(fill=tk.X, padx=2, pady=2)
        ttk.Button(tree_btn_frame, text="Expand All", command=self._expand_all).pack(side=tk.LEFT, padx=(0, 2))
        ttk.Button(tree_btn_frame, text="Collapse All", command=self._collapse_all).pack(side=tk.LEFT)

        # Right: editor panel
        right_frame = ttk.LabelFrame(paned, text="Edit Ability Parameter")
        paned.add(right_frame, weight=1)

        # Key display
        row1 = ttk.Frame(right_frame)
        row1.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row1, text="Key:").pack(side=tk.LEFT)
        self.key_label = ttk.Label(row1, text="-", font=('Consolas', 10, 'bold'))
        self.key_label.pack(side=tk.LEFT, padx=4)

        # Parsed key display
        row2 = ttk.Frame(right_frame)
        row2.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row2, text="Class:", width=10).pack(side=tk.LEFT)
        self.class_label = ttk.Label(row2, text="-", foreground='blue')
        self.class_label.pack(side=tk.LEFT, padx=(0, 16))

        ttk.Label(row2, text="Ability:", width=10).pack(side=tk.LEFT)
        self.ability_label = ttk.Label(row2, text="-", foreground='green')
        self.ability_label.pack(side=tk.LEFT, padx=(0, 16))

        ttk.Label(row2, text="Param:", width=10).pack(side=tk.LEFT)
        self.param_label = ttk.Label(row2, text="-", foreground='purple')
        self.param_label.pack(side=tk.LEFT)

        # Value editor
        row3 = ttk.Frame(right_frame)
        row3.pack(fill=tk.X, padx=8, pady=8)
        ttk.Label(row3, text="Value:", width=10).pack(side=tk.LEFT)
        self.value_var = tk.IntVar(value=0)
        self.value_spin = ttk.Spinbox(
            row3,
            from_=-999999,
            to=999999,
            width=15,
            textvariable=self.value_var,
            command=self._mark_unsaved
        )
        self.value_spin.pack(side=tk.LEFT)
        self.value_spin.bind('<KeyRelease>', lambda e: self._mark_unsaved())

        # Common param hints
        hints_frame = ttk.LabelFrame(right_frame, text="Common Parameter Names")
        hints_frame.pack(fill=tk.X, padx=8, pady=8)
        hints = [
            ("mana_cost", "Mana required to use ability"),
            ("blood_cost", "Blood required (vampires)"),
            ("cooldown", "Cooldown in game ticks"),
            ("damage", "Base damage value"),
            ("duration", "Effect duration in ticks"),
            ("instakill_threshold", "HP threshold for instant kill"),
        ]
        for param, desc in hints:
            ttk.Label(hints_frame, text=f"  {param}: {desc}", foreground='gray').pack(anchor=tk.W)

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)

        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT)

        # Bulk editor section
        bulk_frame = ttk.LabelFrame(right_frame, text="Bulk Operations")
        bulk_frame.pack(fill=tk.X, padx=8, pady=8)

        bulk_row = ttk.Frame(bulk_frame)
        bulk_row.pack(fill=tk.X, padx=4, pady=4)
        ttk.Button(bulk_row, text="Edit All in Class", command=self._edit_class).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(bulk_row, text="Edit All in Ability", command=self._edit_ability).pack(side=tk.LEFT)

    def _load_tree(self):
        """Load the hierarchical tree."""
        self.tree.delete(*self.tree.get_children())
        self._tree_data.clear()

        entries = self.repository.list_all()
        total = len(entries)

        # Organize by class > ability > param
        hierarchy: Dict[str, Dict[str, List[Tuple[str, int]]]] = {}

        for entry in entries:
            key = entry['key']
            value = entry['value']
            parts = key.split('.')
            if len(parts) >= 3:
                class_name, ability, param = parts[0], parts[1], '.'.join(parts[2:])
            elif len(parts) == 2:
                class_name, ability, param = parts[0], parts[1], ''
            else:
                class_name, ability, param = key, '', ''

            if class_name not in hierarchy:
                hierarchy[class_name] = {}
            if ability not in hierarchy[class_name]:
                hierarchy[class_name][ability] = []
            hierarchy[class_name][ability].append((key, value))

        # Build tree
        for class_name in sorted(hierarchy.keys()):
            class_node = self.tree.insert('', tk.END, text=f"{class_name} ({sum(len(v) for v in hierarchy[class_name].values())})")

            for ability in sorted(hierarchy[class_name].keys()):
                if not ability:
                    # Direct class parameters
                    for key, value in hierarchy[class_name][ability]:
                        param_node = self.tree.insert(class_node, tk.END, text=f"{key} = {value}")
                        self._tree_data[param_node] = (class_name, '', key)
                else:
                    ability_node = self.tree.insert(class_node, tk.END, text=f"{ability} ({len(hierarchy[class_name][ability])})")

                    for key, value in sorted(hierarchy[class_name][ability]):
                        param = key.split('.')[-1]
                        param_node = self.tree.insert(ability_node, tk.END, text=f"{param} = {value}")
                        self._tree_data[param_node] = (class_name, ability, key)

        self.count_var.set(f"{total} entries, {len(hierarchy)} classes")
        self.on_status(f"Loaded {total} ability config entries")

    def _filter_tree(self):
        """Filter tree based on search term."""
        term = self.search_var.get().lower()
        if not term:
            self._load_tree()
            return

        self.tree.delete(*self.tree.get_children())
        self._tree_data.clear()

        entries = self.repository.list_all()
        filtered = [e for e in entries if term in e['key'].lower()]

        # Organize filtered entries
        hierarchy: Dict[str, Dict[str, List[Tuple[str, int]]]] = {}

        for entry in filtered:
            key = entry['key']
            value = entry['value']
            parts = key.split('.')
            if len(parts) >= 3:
                class_name, ability, param = parts[0], parts[1], '.'.join(parts[2:])
            elif len(parts) == 2:
                class_name, ability, param = parts[0], parts[1], ''
            else:
                class_name, ability, param = key, '', ''

            if class_name not in hierarchy:
                hierarchy[class_name] = {}
            if ability not in hierarchy[class_name]:
                hierarchy[class_name][ability] = []
            hierarchy[class_name][ability].append((key, value))

        # Build filtered tree
        for class_name in sorted(hierarchy.keys()):
            class_node = self.tree.insert('', tk.END, text=f"{class_name}", open=True)

            for ability in sorted(hierarchy[class_name].keys()):
                if not ability:
                    for key, value in hierarchy[class_name][ability]:
                        param_node = self.tree.insert(class_node, tk.END, text=f"{key} = {value}")
                        self._tree_data[param_node] = (class_name, '', key)
                else:
                    ability_node = self.tree.insert(class_node, tk.END, text=f"{ability}", open=True)
                    for key, value in sorted(hierarchy[class_name][ability]):
                        param = key.split('.')[-1]
                        param_node = self.tree.insert(ability_node, tk.END, text=f"{param} = {value}")
                        self._tree_data[param_node] = (class_name, ability, key)

        self.count_var.set(f"{len(filtered)} matches")

    def _on_select(self, event):
        """Handle tree selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved Changes", "Discard changes?"):
                return

        selection = self.tree.selection()
        if not selection:
            return

        node_id = selection[0]
        if node_id in self._tree_data:
            class_name, ability, key = self._tree_data[node_id]
            self._load_entry(key)

    def _load_entry(self, key: str):
        """Load an entry into the editor."""
        entry = self.repository.get_by_id(key)
        if not entry:
            return

        self.current_key = key
        self.key_label.configure(text=key)

        # Parse key
        parts = key.split('.')
        if len(parts) >= 3:
            class_name, ability, param = parts[0], parts[1], '.'.join(parts[2:])
        elif len(parts) == 2:
            class_name, ability, param = parts[0], parts[1], ''
        else:
            class_name, ability, param = key, '', ''

        self.class_label.configure(text=class_name)
        self.ability_label.configure(text=ability or '-')
        self.param_label.configure(text=param or '-')

        self.value_var.set(entry['value'])
        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current entry."""
        if self.current_key is None:
            return

        try:
            value = self.value_var.get()
        except tk.TclError:
            messagebox.showerror("Error", "Value must be an integer.")
            return

        self.repository.set_value(self.current_key, value)
        self.unsaved = False

        # Update tree display
        self._load_tree()
        self.on_status(f"Saved {self.current_key}")

    def _new(self):
        """Create a new entry."""
        key = simpledialog.askstring(
            "New Ability Config",
            "Enter key (format: class.ability.param):\n"
            "Example: vampire.spiritgate.mana_cost",
            parent=self
        )
        if not key:
            return

        key = key.strip().lower()
        if not key:
            return

        if '.' not in key:
            messagebox.showwarning("Warning", "Key should use format: class.ability.param")

        if self.repository.get_by_id(key):
            messagebox.showerror("Error", f"Key '{key}' already exists.")
            return

        value = simpledialog.askinteger("Value", "Enter value:", parent=self, initialvalue=0)
        if value is None:
            return

        self.repository.insert({'key': key, 'value': value})

        self._load_tree()
        self.on_status(f"Created {key}")

    def _delete(self):
        """Delete current entry."""
        if self.current_key is None:
            return

        if not messagebox.askyesno("Confirm Delete", f"Delete '{self.current_key}'?"):
            return

        self.repository.delete(self.current_key)
        self.current_key = None
        self._clear_editor()
        self._load_tree()
        self.on_status("Entry deleted")

    def _clear_editor(self):
        """Clear the editor."""
        self.current_key = None
        self.key_label.configure(text="-")
        self.class_label.configure(text="-")
        self.ability_label.configure(text="-")
        self.param_label.configure(text="-")
        self.value_var.set(0)
        self.unsaved = False

    def _expand_all(self):
        """Expand all tree nodes."""
        def expand(item):
            self.tree.item(item, open=True)
            for child in self.tree.get_children(item):
                expand(child)
        for item in self.tree.get_children():
            expand(item)

    def _collapse_all(self):
        """Collapse all tree nodes."""
        def collapse(item):
            self.tree.item(item, open=False)
            for child in self.tree.get_children(item):
                collapse(child)
        for item in self.tree.get_children():
            collapse(item)

    def _edit_class(self):
        """Open bulk editor for all params in current class."""
        if not self.current_key:
            messagebox.showinfo("Info", "Select an entry first.")
            return

        parts = self.current_key.split('.')
        if not parts:
            return

        class_name = parts[0]
        entries = self.repository.get_by_class(class_name)

        self._open_bulk_editor(f"Class: {class_name}", entries)

    def _edit_ability(self):
        """Open bulk editor for all params in current ability."""
        if not self.current_key:
            messagebox.showinfo("Info", "Select an entry first.")
            return

        parts = self.current_key.split('.')
        if len(parts) < 2:
            messagebox.showinfo("Info", "Select an ability parameter first.")
            return

        class_name, ability = parts[0], parts[1]
        entries = self.repository.get_ability_params(class_name, ability)

        self._open_bulk_editor(f"{class_name}.{ability}", entries)

    def _open_bulk_editor(self, title: str, entries: List[Dict]):
        """Open a bulk editor dialog for multiple entries."""
        dialog = tk.Toplevel(self)
        dialog.title(f"Bulk Edit: {title}")
        dialog.geometry("500x400")
        dialog.transient(self)
        dialog.grab_set()

        # Create scrollable frame
        canvas = tk.Canvas(dialog)
        scrollbar = ttk.Scrollbar(dialog, orient=tk.VERTICAL, command=canvas.yview)
        scrollable = ttk.Frame(canvas)

        scrollable.bind(
            '<Configure>',
            lambda e: canvas.configure(scrollregion=canvas.bbox('all'))
        )
        canvas.create_window((0, 0), window=scrollable, anchor=tk.NW)
        canvas.configure(yscrollcommand=scrollbar.set)

        # Entry widgets
        entry_vars = {}
        for entry in sorted(entries, key=lambda e: e['key']):
            key = entry['key']
            param = key.split('.')[-1]

            row = ttk.Frame(scrollable)
            row.pack(fill=tk.X, padx=4, pady=2)

            ttk.Label(row, text=param, width=25).pack(side=tk.LEFT)
            var = tk.IntVar(value=entry['value'])
            spin = ttk.Spinbox(row, from_=-999999, to=999999, width=10, textvariable=var)
            spin.pack(side=tk.LEFT)
            entry_vars[key] = var

        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # Buttons
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)

        def save_all():
            for key, var in entry_vars.items():
                try:
                    self.repository.set_value(key, var.get())
                except tk.TclError:
                    pass
            self._load_tree()
            self.on_status(f"Saved {len(entry_vars)} entries")
            dialog.destroy()

        ttk.Button(btn_frame, text="Save All", command=save_all).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Cancel", command=dialog.destroy).pack(side=tk.LEFT)

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes", "Discard changes?")
        return True
