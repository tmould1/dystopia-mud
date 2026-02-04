"""
Entity list panel with search and treeview.

Provides a reusable list widget for displaying and selecting entities.
"""

import tkinter as tk
from tkinter import ttk
from typing import Any, Callable, Dict, List, Optional, Tuple


class EntityListPanel(ttk.Frame):
    """
    A panel with a search bar and treeview for entity lists.

    Features:
    - Live search filtering as you type
    - Configurable columns
    - Selection callback
    - Sortable columns (click header)
    """

    def __init__(
        self,
        parent,
        columns: List[Tuple[str, str, int]],
        on_select: Optional[Callable[[Any], None]] = None,
        search_columns: Optional[List[str]] = None,
        **kwargs
    ):
        """
        Initialize the entity list panel.

        Args:
            parent: Parent Tkinter widget
            columns: List of (column_id, heading, width) tuples
            on_select: Callback invoked with selected item ID when selection changes
            search_columns: Column IDs to search (defaults to all visible columns)
            **kwargs: Additional arguments passed to ttk.Frame
        """
        super().__init__(parent, **kwargs)

        self.columns = columns
        self.on_select = on_select
        self.search_columns = search_columns or [col[0] for col in columns]

        self._items: List[Dict] = []
        self._id_column: str = columns[0][0] if columns else 'id'

        self._build_ui()

    def _build_ui(self):
        """Build the panel UI."""
        # Search bar
        search_frame = ttk.Frame(self)
        search_frame.pack(fill=tk.X, padx=2, pady=(2, 4))

        ttk.Label(search_frame, text="Search:").pack(side=tk.LEFT)
        self.search_var = tk.StringVar()
        self.search_var.trace_add('write', lambda *_: self._filter_list())
        self.search_entry = ttk.Entry(search_frame, textvariable=self.search_var)
        self.search_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(4, 0))

        # Clear button
        ttk.Button(
            search_frame,
            text="X",
            width=2,
            command=self._clear_search
        ).pack(side=tk.LEFT, padx=(2, 0))

        # Treeview frame
        tree_frame = ttk.Frame(self)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        # Create treeview
        column_ids = [col[0] for col in self.columns]
        self.tree = ttk.Treeview(
            tree_frame,
            columns=column_ids,
            show='headings',
            selectmode='browse'
        )

        # Configure columns
        for col_id, heading, width in self.columns:
            self.tree.heading(col_id, text=heading, anchor=tk.W,
                            command=lambda c=col_id: self._sort_by_column(c))
            self.tree.column(col_id, width=width, minwidth=30)

        # Scrollbar
        scrollbar = ttk.Scrollbar(
            tree_frame,
            orient=tk.VERTICAL,
            command=self.tree.yview
        )
        self.tree.configure(yscrollcommand=scrollbar.set)

        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # Selection binding
        self.tree.bind('<<TreeviewSelect>>', self._on_tree_select)

        # Item count label
        self.count_var = tk.StringVar(value="0 items")
        ttk.Label(self, textvariable=self.count_var).pack(anchor=tk.W, padx=2)

        # Sort state
        self._sort_column = None
        self._sort_reverse = False

    def _clear_search(self):
        """Clear the search box."""
        self.search_var.set('')
        self.search_entry.focus_set()

    def _filter_list(self):
        """Re-filter the tree based on search box."""
        search = self.search_var.get().lower()
        self._populate_tree(search)

    def _populate_tree(self, search: str = ''):
        """Populate the treeview with items, applying search filter."""
        # Remember current selection
        selection = self.tree.selection()
        selected_id = selection[0] if selection else None

        # Clear tree
        self.tree.delete(*self.tree.get_children())

        visible_count = 0
        for item in self._items:
            # Apply search filter
            if search:
                match = False
                for col_id in self.search_columns:
                    value = str(item.get(col_id, '')).lower()
                    if search in value:
                        match = True
                        break
                if not match:
                    continue

            # Add to tree
            item_id = str(item.get(self._id_column, ''))
            values = [item.get(col[0], '') for col in self.columns]
            self.tree.insert('', tk.END, iid=item_id, values=values)
            visible_count += 1

        # Update count
        total = len(self._items)
        if visible_count < total:
            self.count_var.set(f"{visible_count} of {total} items")
        else:
            self.count_var.set(f"{total} items")

        # Restore selection if possible
        if selected_id and self.tree.exists(selected_id):
            self.tree.selection_set(selected_id)
            self.tree.see(selected_id)

    def _sort_by_column(self, column: str):
        """Sort items by the clicked column."""
        if self._sort_column == column:
            self._sort_reverse = not self._sort_reverse
        else:
            self._sort_column = column
            self._sort_reverse = False

        # Sort items
        def sort_key(item):
            val = item.get(column, '')
            # Try numeric sort first
            try:
                return (0, int(val))
            except (ValueError, TypeError):
                return (1, str(val).lower())

        self._items.sort(key=sort_key, reverse=self._sort_reverse)
        self._populate_tree(self.search_var.get().lower())

    def _on_tree_select(self, event):
        """Handle treeview selection."""
        if not self.on_select:
            return

        selection = self.tree.selection()
        if selection:
            # Try to convert to int if it looks like a number
            item_id = selection[0]
            try:
                item_id = int(item_id)
            except ValueError:
                pass
            self.on_select(item_id)

    def set_items(self, items: List[Dict], id_column: Optional[str] = None):
        """
        Set the list of items to display.

        Args:
            items: List of dictionaries with column values
            id_column: Column to use as item ID (defaults to first column)
        """
        self._items = list(items)
        if id_column:
            self._id_column = id_column
        self._sort_column = None
        self._sort_reverse = False
        self._populate_tree(self.search_var.get().lower())

    def get_selected_id(self) -> Optional[Any]:
        """Get the ID of the currently selected item."""
        selection = self.tree.selection()
        if selection:
            item_id = selection[0]
            try:
                return int(item_id)
            except ValueError:
                return item_id
        return None

    def select_item(self, item_id: Any):
        """Select an item by ID and scroll to it."""
        str_id = str(item_id)
        if self.tree.exists(str_id):
            self.tree.selection_set(str_id)
            self.tree.see(str_id)
            self.tree.focus(str_id)

    def clear_selection(self):
        """Clear the current selection."""
        self.tree.selection_remove(*self.tree.selection())

    def refresh(self):
        """Refresh the display without changing data."""
        self._populate_tree(self.search_var.get().lower())

    def focus_search(self):
        """Set focus to the search entry."""
        self.search_entry.focus_set()
