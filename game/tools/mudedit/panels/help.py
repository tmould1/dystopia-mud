"""
Help entry editor panel.

Provides CRUD interface for help file entries with color preview.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Optional

from ..db.repository import HelpRepository
from ..widgets import ColorTextEditor, EntityListPanel, strip_colors


class HelpEditorPanel(ttk.Frame):
    """
    Editor panel for help file entries.

    Provides a split view with entity list on the left and editor on the right.
    Supports create, read, update, delete operations with color preview.
    """

    def __init__(
        self,
        parent,
        repository: HelpRepository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        """
        Initialize the help editor panel.

        Args:
            parent: Parent Tkinter widget
            repository: HelpRepository for database operations
            on_status: Callback for status messages
            **kwargs: Additional arguments passed to ttk.Frame
        """
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.current_id: Optional[int] = None
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
                ('id', 'ID', 50),
                ('keyword', 'Keyword', 200),
            ],
            on_select=self._on_entry_select,
            search_columns=['keyword', 'text']
        )
        paned.add(self.list_panel, weight=1)

        # Right panel: editor
        right = ttk.Frame(paned)
        paned.add(right, weight=3)

        # Header: keyword, level, id
        header = ttk.Frame(right)
        header.pack(fill=tk.X, padx=2, pady=(2, 4))

        ttk.Label(header, text="Keyword:").grid(row=0, column=0, sticky=tk.W)
        self.keyword_var = tk.StringVar()
        self.keyword_var.trace_add('write', lambda *_: self._mark_unsaved())
        self.keyword_entry = ttk.Entry(header, textvariable=self.keyword_var, width=40)
        self.keyword_entry.grid(row=0, column=1, sticky=tk.W, padx=(4, 12))

        ttk.Label(header, text="Level:").grid(row=0, column=2, sticky=tk.W)
        self.level_var = tk.IntVar(value=0)
        self.level_var.trace_add('write', lambda *_: self._mark_unsaved())
        self.level_spin = ttk.Spinbox(
            header,
            from_=-1,
            to=100,
            width=5,
            textvariable=self.level_var
        )
        self.level_spin.grid(row=0, column=3, sticky=tk.W, padx=(4, 12))

        ttk.Label(header, text="ID:").grid(row=0, column=4, sticky=tk.W)
        self.id_label = ttk.Label(header, text="-")
        self.id_label.grid(row=0, column=5, sticky=tk.W, padx=(4, 0))

        # Text editor with preview
        self.text_editor = ColorTextEditor(
            right,
            show_preview=True,
            on_change=self._mark_unsaved
        )
        self.text_editor.pack(fill=tk.BOTH, expand=True, padx=2, pady=(0, 4))

        # Button bar
        btn_frame = ttk.Frame(right)
        btn_frame.pack(fill=tk.X, padx=2, pady=(0, 2))

        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(
            btn_frame,
            text="Refresh Preview",
            command=self.text_editor.refresh_preview
        ).pack(side=tk.LEFT, padx=(0, 4))

        # Keyboard shortcuts
        self.bind_all('<Control-s>', lambda e: self._save())
        self.bind_all('<Control-n>', lambda e: self._new())
        self.bind_all('<F5>', lambda e: self.text_editor.refresh_preview())

    def _load_entries(self):
        """Load all entries from database."""
        entries = self.repository.list_all()

        # Add stripped text for search
        for entry in entries:
            entry['text'] = strip_colors(entry.get('text', ''))

        self.list_panel.set_items(entries, id_column='id')
        self.on_status(f"Loaded {len(entries)} help entries")

    def _on_entry_select(self, entry_id: int):
        """Handle entry selection from list."""
        if self.unsaved:
            if not messagebox.askyesno(
                "Unsaved Changes",
                "You have unsaved changes. Discard them?"
            ):
                # Re-select the current entry
                if self.current_id is not None:
                    self.list_panel.select_item(self.current_id)
                return

        self._load_entry(entry_id)

    def _load_entry(self, entry_id: int):
        """Load a specific entry into the editor."""
        entry = self.repository.get_by_id(entry_id)
        if not entry:
            self.on_status(f"Entry {entry_id} not found")
            return

        self.current_id = entry_id

        # Populate fields
        self.keyword_var.set(entry['keyword'])
        self.level_var.set(entry['level'])
        self.id_label.configure(text=str(entry_id))

        # Load text
        self.text_editor.set_text(entry['text'])

        self.unsaved = False
        self.on_status(f"Loaded [{entry_id}] {entry['keyword']}")

    def _mark_unsaved(self):
        """Mark the current entry as having unsaved changes."""
        self.unsaved = True

    def _save(self):
        """Save current entry to database."""
        if self.current_id is None:
            self.on_status("No entry selected")
            return

        keyword = self.keyword_var.get().strip()
        if not keyword:
            messagebox.showwarning("Warning", "Keyword cannot be empty.")
            return

        try:
            level = self.level_var.get()
        except tk.TclError:
            level = 0

        text = self.text_editor.get_text()

        self.repository.update(self.current_id, {
            'keyword': keyword,
            'level': level,
            'text': text
        })

        # Refresh list
        self._load_entries()
        self.list_panel.select_item(self.current_id)

        self.unsaved = False
        self.on_status(f"Saved [{self.current_id}] {keyword}")

    def _new(self):
        """Create a new help entry."""
        keyword = simpledialog.askstring(
            "New Help Entry",
            "Keyword(s):",
            parent=self
        )
        if not keyword:
            return

        keyword = keyword.upper().strip()

        new_id = self.repository.insert({
            'level': 0,
            'keyword': keyword,
            'text': ''
        })

        self._load_entries()
        self.list_panel.select_item(new_id)
        self._load_entry(new_id)
        self.on_status(f"Created [{new_id}] {keyword}")

    def _delete(self):
        """Delete the current entry."""
        if self.current_id is None:
            self.on_status("No entry selected")
            return

        keyword = self.keyword_var.get()
        if not messagebox.askyesno(
            "Confirm Delete",
            f"Delete [{self.current_id}] {keyword}?"
        ):
            return

        self.repository.delete(self.current_id)

        old_id = self.current_id
        self._clear_editor()
        self._load_entries()

        self.on_status(f"Deleted [{old_id}] {keyword}")

    def _clear_editor(self):
        """Clear the editor fields."""
        self.current_id = None
        self.keyword_var.set('')
        self.level_var.set(0)
        self.id_label.configure(text='-')
        self.text_editor.clear()
        self.unsaved = False

    def check_unsaved(self) -> bool:
        """
        Check for unsaved changes and prompt user.

        Returns:
            True if it's safe to proceed, False if user cancelled
        """
        if self.unsaved:
            return messagebox.askyesno(
                "Unsaved Changes",
                "You have unsaved changes. Discard them?"
            )
        return True
