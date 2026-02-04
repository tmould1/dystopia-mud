"""
Game configuration editor panels.

Provides interfaces for editing key-value configuration tables.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Optional

from ..db.repository import GameConfigRepository, BalanceConfigRepository


class KeyValueEditorPanel(ttk.Frame):
    """
    Generic editor panel for key-value configuration tables.

    Displays a list of keys on the left, editor on the right.
    """

    def __init__(
        self,
        parent,
        repository,
        title: str,
        on_status: Optional[Callable[[str], None]] = None,
        value_type: str = 'text',  # 'text' or 'number'
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.title = title
        self.on_status = on_status or (lambda msg: None)
        self.value_type = value_type

        self.current_key: Optional[str] = None
        self.unsaved = False

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: key list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        # Search
        search_frame = ttk.Frame(left_frame)
        search_frame.pack(fill=tk.X, padx=2, pady=2)
        ttk.Label(search_frame, text="Filter:").pack(side=tk.LEFT)
        self.search_var = tk.StringVar()
        self.search_var.trace_add('write', lambda *_: self._filter_list())
        ttk.Entry(search_frame, textvariable=self.search_var).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(4, 0))

        # Tree
        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('key', 'value'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('key', text='Key')
        self.tree.heading('value', text='Value')
        self.tree.column('key', width=200)
        self.tree.column('value', width=150)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 entries")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor
        right_frame = ttk.LabelFrame(paned, text=f"Edit {self.title}")
        paned.add(right_frame, weight=1)

        # Key (read-only display)
        row1 = ttk.Frame(right_frame)
        row1.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row1, text="Key:").pack(side=tk.LEFT)
        self.key_label = ttk.Label(row1, text="-", font=('Consolas', 10, 'bold'))
        self.key_label.pack(side=tk.LEFT, padx=4)

        # Value editor
        row2 = ttk.Frame(right_frame)
        row2.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)
        ttk.Label(row2, text="Value:").pack(anchor=tk.NW)

        if self.value_type == 'number':
            self.value_var = tk.StringVar()
            self.value_var.trace_add('write', lambda *_: self._mark_unsaved())
            self.value_entry = ttk.Entry(row2, textvariable=self.value_var, width=20)
            self.value_entry.pack(anchor=tk.NW, pady=(4, 0))
        else:
            self.value_text = tk.Text(row2, wrap=tk.WORD, height=10, font=('Consolas', 10))
            self.value_text.pack(fill=tk.BOTH, expand=True, pady=(4, 0))
            self.value_text.bind('<KeyRelease>', lambda e: self._mark_unsaved())

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)

        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT)

        # Store all entries for filtering
        self._all_entries = []

    def _load_entries(self):
        """Load all entries."""
        self._all_entries = self.repository.list_all()
        self._populate_tree(self._all_entries)
        self.on_status(f"Loaded {len(self._all_entries)} {self.title.lower()} entries")

    def _populate_tree(self, entries):
        """Populate tree with given entries."""
        self.tree.delete(*self.tree.get_children())
        for entry in entries:
            key = entry['key']
            value = str(entry['value'])[:50]  # Truncate for display
            self.tree.insert('', tk.END, iid=key, values=(key, value))
        self.count_var.set(f"{len(entries)} entries")

    def _filter_list(self):
        """Filter the list based on search term."""
        term = self.search_var.get().lower()
        if not term:
            self._populate_tree(self._all_entries)
        else:
            filtered = [e for e in self._all_entries if term in e['key'].lower() or term in str(e['value']).lower()]
            self._populate_tree(filtered)

    def _on_select(self, event):
        """Handle entry selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved Changes", "Discard changes?"):
                if self.current_key:
                    self.tree.selection_set(self.current_key)
                return

        selection = self.tree.selection()
        if selection:
            self._load_entry(selection[0])

    def _load_entry(self, key: str):
        """Load an entry into the editor."""
        entry = self.repository.get_by_id(key)
        if not entry:
            return

        self.current_key = key
        self.key_label.configure(text=key)

        if self.value_type == 'number':
            self.value_var.set(str(entry['value']))
        else:
            self.value_text.delete('1.0', tk.END)
            self.value_text.insert('1.0', str(entry['value']))

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current entry."""
        if self.current_key is None:
            return

        if self.value_type == 'number':
            try:
                value = self.value_var.get()
                # Allow integers and floats
                if '.' in value:
                    float(value)
                else:
                    int(value)
            except ValueError:
                messagebox.showerror("Error", "Value must be a number.")
                return
        else:
            value = self.value_text.get('1.0', tk.END).rstrip('\n')

        self.repository.set_value(self.current_key, value)
        self.unsaved = False

        # Refresh the list
        self._load_entries()
        self.tree.selection_set(self.current_key)
        self.on_status(f"Saved {self.current_key}")

    def _new(self):
        """Create a new entry."""
        key = simpledialog.askstring("New Entry", "Enter key name:", parent=self)
        if not key:
            return

        key = key.strip()
        if not key:
            return

        if self.repository.get_by_id(key):
            messagebox.showerror("Error", f"Key '{key}' already exists.")
            return

        default_value = "0" if self.value_type == 'number' else ""
        self.repository.insert({'key': key, 'value': default_value})

        self._load_entries()
        self.tree.selection_set(key)
        self._load_entry(key)
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
        self._load_entries()
        self.on_status("Entry deleted")

    def _clear_editor(self):
        """Clear the editor."""
        self.current_key = None
        self.key_label.configure(text="-")
        if self.value_type == 'number':
            self.value_var.set("")
        else:
            self.value_text.delete('1.0', tk.END)
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes", "Discard changes?")
        return True


class GameConfigPanel(KeyValueEditorPanel):
    """Editor panel for gameconfig table."""

    def __init__(
        self,
        parent,
        repository: GameConfigRepository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(
            parent,
            repository,
            title="Configuration",
            on_status=on_status,
            value_type='text',
            **kwargs
        )


class BalanceConfigPanel(KeyValueEditorPanel):
    """Editor panel for balance_config table."""

    def __init__(
        self,
        parent,
        repository: BalanceConfigRepository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(
            parent,
            repository,
            title="Balance Config",
            on_status=on_status,
            value_type='text',  # Values can be integers or floats stored as text
            **kwargs
        )
