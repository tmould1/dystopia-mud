"""
Unified configuration editor panel.

Provides hierarchical interface for editing the unified cfg system (config table in game.db).
Shows database overrides alongside compiled-in defaults from cfg_keys.py.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Dict, List, Optional, Tuple

try:
    from .. import cfg_keys
    _HAS_CFG_KEYS = bool(cfg_keys.ALL_KEYS)
except (ImportError, AttributeError):
    _HAS_CFG_KEYS = False
    cfg_keys = None


class UnifiedConfigPanel(ttk.Frame):
    """
    Editor panel for the unified config table (game.db).

    Displays a hierarchical tree grouped by key prefix (category.subcategory).
    Shows current override value alongside the compiled default.
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
        self._tree_data: Dict[str, str] = {}  # node_id -> key

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
        ttk.Entry(search_frame, textvariable=self.search_var).pack(
            side=tk.LEFT, fill=tk.X, expand=True, padx=(4, 0))

        # Tree
        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('value', 'default'),
            show='tree headings',
            selectmode='browse'
        )
        self.tree.heading('#0', text='Key', anchor=tk.W)
        self.tree.heading('value', text='Value')
        self.tree.heading('default', text='Default')
        self.tree.column('#0', width=250)
        self.tree.column('value', width=80, anchor=tk.E)
        self.tree.column('default', width=80, anchor=tk.E)

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
        ttk.Button(tree_btn_frame, text="Expand All", command=self._expand_all).pack(
            side=tk.LEFT, padx=(0, 2))
        ttk.Button(tree_btn_frame, text="Collapse All", command=self._collapse_all).pack(
            side=tk.LEFT)

        # Right: editor panel
        right_frame = ttk.LabelFrame(paned, text="Edit Config Value")
        paned.add(right_frame, weight=1)

        # Key display
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Key:").pack(side=tk.LEFT)
        self.key_label = ttk.Label(row, text="-", font=('Consolas', 10, 'bold'))
        self.key_label.pack(side=tk.LEFT, padx=4)

        # Category
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Category:", width=10).pack(side=tk.LEFT)
        self.category_label = ttk.Label(row, text="-", foreground='blue')
        self.category_label.pack(side=tk.LEFT)

        # Current value
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=8)
        ttk.Label(row, text="Value:", width=10).pack(side=tk.LEFT)
        self.value_var = tk.IntVar(value=0)
        self.value_spin = ttk.Spinbox(
            row,
            from_=-999999,
            to=999999,
            width=15,
            textvariable=self.value_var,
            command=self._mark_unsaved
        )
        self.value_spin.pack(side=tk.LEFT)
        self.value_spin.bind('<KeyRelease>', lambda e: self._mark_unsaved())

        # Default value display
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Default:", width=10).pack(side=tk.LEFT)
        self.default_label = ttk.Label(row, text="-", foreground='gray')
        self.default_label.pack(side=tk.LEFT)

        # Override indicator
        self.override_label = ttk.Label(right_frame, text="", foreground='orange')
        self.override_label.pack(anchor=tk.W, padx=20, pady=4)

        # Description
        if not _HAS_CFG_KEYS:
            desc_frame = ttk.Frame(right_frame)
            desc_frame.pack(fill=tk.X, padx=8, pady=(8, 4))
            ttk.Label(
                desc_frame,
                text="Note: cfg_keys.py has no keys populated.\n"
                     "Run 'python game/tools/generators/generate_cfg_keys.py'\n"
                     "after adding entries to balance.c / acfg_keys.h\n"
                     "to enable defaults and validation.",
                foreground='gray', font=('Consolas', 9), justify=tk.LEFT
            ).pack(anchor=tk.W, padx=12)

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)

        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Reset to Default", command=self._reset_to_default).pack(
            side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT)

    def _get_default(self, key: str) -> Optional[int]:
        """Get compiled default for a key."""
        if _HAS_CFG_KEYS:
            return cfg_keys.get_default(key)
        return None

    def _build_hierarchy(self, entries: List[Dict], open_nodes: bool = False):
        """Build tree from entries, grouped by dotted prefix."""
        self.tree.delete(*self.tree.get_children())
        self._tree_data.clear()

        # Group by top-level category
        categories: Dict[str, Dict[str, List[Tuple[str, int]]]] = {}

        for entry in entries:
            key = entry['key']
            value = entry['value']
            parts = key.split('.')
            cat = parts[0] if parts else key
            subcat = '.'.join(parts[1:-1]) if len(parts) > 2 else ''
            leaf = parts[-1] if len(parts) > 1 else key

            if cat not in categories:
                categories[cat] = {}
            group_key = subcat if subcat else '__direct__'
            if group_key not in categories[cat]:
                categories[cat][group_key] = []
            categories[cat][group_key].append((key, value))

        # Build tree
        for cat in sorted(categories.keys()):
            groups = categories[cat]
            total = sum(len(v) for v in groups.values())
            cat_node = self.tree.insert('', tk.END, text=f"{cat} ({total})", open=open_nodes)

            for group_key in sorted(groups.keys()):
                items = groups[group_key]

                if group_key == '__direct__':
                    parent = cat_node
                else:
                    parent = self.tree.insert(
                        cat_node, tk.END,
                        text=f"{group_key} ({len(items)})",
                        open=open_nodes
                    )

                for key, value in sorted(items, key=lambda x: x[0]):
                    leaf = key.split('.')[-1]
                    default = self._get_default(key)
                    default_str = str(default) if default is not None else ''
                    node = self.tree.insert(
                        parent, tk.END,
                        text=leaf,
                        values=(str(value), default_str)
                    )
                    self._tree_data[node] = key

        return len(entries)

    def _load_tree(self):
        """Load all config overrides into tree."""
        entries = self.repository.list_all()
        count = self._build_hierarchy(entries)
        self.count_var.set(f"{count} overrides")
        self.on_status(f"Loaded {count} config overrides")

    def _filter_tree(self):
        """Filter tree based on search term."""
        term = self.search_var.get().lower()
        if not term:
            self._load_tree()
            return

        entries = self.repository.list_all()
        filtered = [e for e in entries if term in e['key'].lower()]
        count = self._build_hierarchy(filtered, open_nodes=True)
        self.count_var.set(f"{count} matches")

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
            self._load_entry(self._tree_data[node_id])

    def _load_entry(self, key: str):
        """Load an entry into the editor."""
        entry = self.repository.get_by_id(key)
        if not entry:
            return

        self.current_key = key
        self.key_label.configure(text=key)

        # Category from first dot-segment
        parts = key.split('.')
        self.category_label.configure(text=parts[0] if parts else '-')

        self.value_var.set(entry['value'])

        # Show default
        default = self._get_default(key)
        if default is not None:
            self.default_label.configure(text=str(default))
            if entry['value'] != default:
                self.override_label.configure(text="(overridden from default)")
            else:
                self.override_label.configure(text="(matches default)")
        else:
            self.default_label.configure(text="(unknown)")
            self.override_label.configure(text="")

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
        self._load_tree()
        self.on_status(f"Saved {self.current_key} = {value}")

    def _new(self):
        """Create a new config override."""
        key = simpledialog.askstring(
            "New Config Override",
            "Enter key (format: category.subcategory.name):\n"
            "Example: combat.base_damcap",
            parent=self
        )
        if not key:
            return

        key = key.strip().lower()
        if not key:
            return

        # Validate against known keys if available
        if _HAS_CFG_KEYS and not cfg_keys.is_valid_key(key):
            if not messagebox.askyesno(
                "Unknown Key",
                f"'{key}' is not a recognized cfg key.\nAdd it anyway?"
            ):
                return

        existing = self.repository.get_by_id(key)
        if existing:
            messagebox.showerror("Error", f"Key '{key}' already has an override.")
            return

        default = self._get_default(key)
        initial = default if default is not None else 0

        value = simpledialog.askinteger("Value", "Enter value:", parent=self, initialvalue=initial)
        if value is None:
            return

        self.repository.set_value(key, value)
        self._load_tree()
        self.on_status(f"Created override: {key} = {value}")

    def _reset_to_default(self):
        """Remove override, reverting to compiled default."""
        if self.current_key is None:
            return

        default = self._get_default(self.current_key)
        if default is not None:
            msg = f"Reset '{self.current_key}' to default ({default})?\nThis removes the database override."
        else:
            msg = f"Remove override for '{self.current_key}'?\n(Compiled default is unknown)"

        if not messagebox.askyesno("Reset to Default", msg):
            return

        self.repository.delete_override(self.current_key)
        self.current_key = None
        self._clear_editor()
        self._load_tree()
        self.on_status("Override removed (reset to default)")

    def _delete(self):
        """Delete current override."""
        if self.current_key is None:
            return

        if not messagebox.askyesno("Confirm Delete", f"Delete override for '{self.current_key}'?"):
            return

        self.repository.delete_override(self.current_key)
        self.current_key = None
        self._clear_editor()
        self._load_tree()
        self.on_status("Config override deleted")

    def _clear_editor(self):
        """Clear the editor."""
        self.current_key = None
        self.key_label.configure(text="-")
        self.category_label.configure(text="-")
        self.default_label.configure(text="-")
        self.override_label.configure(text="")
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

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes", "Discard changes?")
        return True
