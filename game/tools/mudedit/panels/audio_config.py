"""
Audio configuration editor panel.

Provides an interface for editing MCMP audio file mappings in game.db.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Dict, List, Optional

from ..db.repository import AudioConfigRepository


class AudioConfigPanel(ttk.Frame):
    """
    Editor panel for audio_config table.

    Displays categories on the left, entries list in the middle,
    and editor form on the right.
    """

    def __init__(
        self,
        parent,
        repository: AudioConfigRepository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.current_entry: Optional[Dict] = None
        self.unsaved = False
        self._current_entries: List[Dict] = []

        self._build_ui()
        self._load_data()

    def _build_ui(self):
        """Build the panel UI with three-pane layout."""
        main_paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: Category tree
        left_frame = ttk.LabelFrame(main_paned, text="Categories")
        main_paned.add(left_frame, weight=0)

        self.category_tree = ttk.Treeview(left_frame, show='tree', selectmode='browse')
        self.category_tree.pack(fill=tk.BOTH, expand=True, padx=2, pady=2)
        self.category_tree.bind('<<TreeviewSelect>>', self._on_category_select)

        # Middle: Entries list
        middle_frame = ttk.LabelFrame(main_paned, text="Audio Entries")
        main_paned.add(middle_frame, weight=1)

        # Search
        search_frame = ttk.Frame(middle_frame)
        search_frame.pack(fill=tk.X, padx=2, pady=2)
        ttk.Label(search_frame, text="Filter:").pack(side=tk.LEFT)
        self.search_var = tk.StringVar()
        self.search_var.trace_add('write', lambda *_: self._filter_entries())
        ttk.Entry(search_frame, textvariable=self.search_var).pack(
            side=tk.LEFT, fill=tk.X, expand=True, padx=(4, 0)
        )

        # Entries treeview
        tree_frame = ttk.Frame(middle_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.entries_tree = ttk.Treeview(
            tree_frame,
            columns=('trigger', 'filename', 'volume'),
            show='headings',
            selectmode='browse'
        )
        self.entries_tree.heading('trigger', text='Trigger Key')
        self.entries_tree.heading('filename', text='Filename')
        self.entries_tree.heading('volume', text='Vol')
        self.entries_tree.column('trigger', width=140)
        self.entries_tree.column('filename', width=200)
        self.entries_tree.column('volume', width=40)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.entries_tree.yview)
        self.entries_tree.configure(yscrollcommand=scrollbar.set)
        self.entries_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.entries_tree.bind('<<TreeviewSelect>>', self._on_entry_select)

        self.count_var = tk.StringVar(value="0 entries")
        ttk.Label(middle_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Entry buttons
        btn_frame = ttk.Frame(middle_frame)
        btn_frame.pack(fill=tk.X, padx=2, pady=2)
        ttk.Button(btn_frame, text="New Entry", command=self._new_entry).pack(side=tk.LEFT, padx=(0, 2))
        ttk.Button(btn_frame, text="Delete", command=self._delete_entry).pack(side=tk.LEFT)

        # Right: Editor form
        right_frame = ttk.LabelFrame(main_paned, text="Edit Audio Entry")
        main_paned.add(right_frame, weight=1)

        self._build_editor(right_frame)

    def _build_editor(self, parent):
        """Build the editor form."""
        # Category
        row = ttk.Frame(parent)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Category:", width=12).pack(side=tk.LEFT)
        self.category_var = tk.StringVar()
        self.category_combo = ttk.Combobox(
            row, textvariable=self.category_var,
            values=self.repository.CATEGORIES, state='readonly', width=15
        )
        self.category_combo.pack(side=tk.LEFT)
        self.category_combo.bind('<<ComboboxSelected>>', self._on_category_change)

        # Trigger Key
        row = ttk.Frame(parent)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Trigger Key:", width=12).pack(side=tk.LEFT)
        self.trigger_var = tk.StringVar()
        self.trigger_combo = ttk.Combobox(row, textvariable=self.trigger_var, width=20)
        self.trigger_combo.pack(side=tk.LEFT)
        self.trigger_combo.bind('<KeyRelease>', lambda e: self._mark_unsaved())
        self.trigger_combo.bind('<<ComboboxSelected>>', lambda e: self._mark_unsaved())

        # Filename
        row = ttk.Frame(parent)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Filename:", width=12).pack(side=tk.LEFT)
        self.filename_var = tk.StringVar()
        self.filename_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.filename_var, width=30).pack(side=tk.LEFT, fill=tk.X, expand=True)

        # Volume / Priority / Loops (row)
        row = ttk.Frame(parent)
        row.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row, text="Volume:", width=8).pack(side=tk.LEFT)
        self.volume_var = tk.IntVar(value=50)
        ttk.Spinbox(row, from_=0, to=100, width=5, textvariable=self.volume_var,
                    command=self._mark_unsaved).pack(side=tk.LEFT, padx=(0, 12))

        ttk.Label(row, text="Priority:", width=8).pack(side=tk.LEFT)
        self.priority_var = tk.IntVar(value=50)
        ttk.Spinbox(row, from_=0, to=100, width=5, textvariable=self.priority_var,
                    command=self._mark_unsaved).pack(side=tk.LEFT, padx=(0, 12))

        ttk.Label(row, text="Loops:", width=6).pack(side=tk.LEFT)
        self.loops_var = tk.IntVar(value=1)
        ttk.Spinbox(row, from_=-1, to=100, width=5, textvariable=self.loops_var,
                    command=self._mark_unsaved).pack(side=tk.LEFT)
        ttk.Label(row, text="(-1=loop)", foreground='gray').pack(side=tk.LEFT, padx=4)

        # Media Type / Tag (row)
        row = ttk.Frame(parent)
        row.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row, text="Media Type:", width=12).pack(side=tk.LEFT)
        self.media_type_var = tk.StringVar(value='sound')
        media_combo = ttk.Combobox(row, textvariable=self.media_type_var,
                                   values=self.repository.MEDIA_TYPES, state='readonly', width=8)
        media_combo.pack(side=tk.LEFT, padx=(0, 12))
        media_combo.bind('<<ComboboxSelected>>', lambda e: self._mark_unsaved())

        ttk.Label(row, text="Tag:", width=4).pack(side=tk.LEFT)
        self.tag_var = tk.StringVar()
        tag_combo = ttk.Combobox(row, textvariable=self.tag_var,
                                 values=self.repository.TAGS, width=12)
        tag_combo.pack(side=tk.LEFT)
        tag_combo.bind('<KeyRelease>', lambda e: self._mark_unsaved())
        tag_combo.bind('<<ComboboxSelected>>', lambda e: self._mark_unsaved())

        # Use Key / Continue (row)
        row = ttk.Frame(parent)
        row.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row, text="Binding Key:", width=12).pack(side=tk.LEFT)
        self.use_key_var = tk.StringVar()
        self.use_key_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.use_key_var, width=12).pack(side=tk.LEFT, padx=(0, 12))

        self.continue_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(row, text="Continue (don't restart)",
                        variable=self.continue_var, command=self._mark_unsaved).pack(side=tk.LEFT)

        # Caption
        row = ttk.Frame(parent)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Caption:", width=12).pack(side=tk.LEFT, anchor=tk.N)
        self.caption_var = tk.StringVar()
        self.caption_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.caption_var, width=35).pack(side=tk.LEFT, fill=tk.X, expand=True)

        # Hint text
        hint_frame = ttk.LabelFrame(parent, text="Hints")
        hint_frame.pack(fill=tk.X, padx=8, pady=8)
        hints = [
            "Loops: 1=once, -1=infinite",
            "Binding Key: same key sounds replace each other",
            "Caption: accessibility text for hearing-impaired",
        ]
        for hint in hints:
            ttk.Label(hint_frame, text=f"  {hint}", foreground='gray').pack(anchor=tk.W)

        # Save button
        btn_frame = ttk.Frame(parent)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save Entry", command=self._save).pack(side=tk.LEFT)

    def _load_data(self):
        """Load categories and entries."""
        # Populate category tree
        self.category_tree.delete(*self.category_tree.get_children())

        self.category_tree.insert('', tk.END, text='All Categories', iid='all')

        for category in self.repository.CATEGORIES:
            entries = self.repository.get_by_category(category)
            label = f"{category.title()} ({len(entries)})"
            self.category_tree.insert('', tk.END, text=label, iid=category)

        # Select "All Categories" by default
        self.category_tree.selection_set('all')
        self._load_entries_for_category(None)

        total = len(self.repository.list_all())
        self.on_status(f"Loaded {total} audio config entries")

    def _load_entries_for_category(self, category: Optional[str]):
        """Load entries for selected category."""
        if category == 'all' or category is None:
            entries = self.repository.list_all()
        else:
            entries = self.repository.get_by_category(category)

        self._current_entries = entries
        self._populate_entries_tree(entries)

    def _populate_entries_tree(self, entries: List[Dict]):
        """Populate entries treeview."""
        self.entries_tree.delete(*self.entries_tree.get_children())

        for entry in entries:
            self.entries_tree.insert(
                '', tk.END,
                iid=str(entry['id']),
                values=(entry['trigger_key'], entry['filename'], entry['volume'])
            )

        self.count_var.set(f"{len(entries)} entries")

    def _on_category_select(self, event):
        """Handle category selection."""
        selection = self.category_tree.selection()
        if selection:
            category = selection[0]
            self._load_entries_for_category(None if category == 'all' else category)

    def _on_entry_select(self, event):
        """Handle entry selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved Changes", "Discard changes?"):
                return

        selection = self.entries_tree.selection()
        if selection:
            entry_id = int(selection[0])
            self._load_entry(entry_id)

    def _load_entry(self, entry_id: int):
        """Load entry into editor."""
        entry = self.repository.get_by_id(entry_id)
        if not entry:
            return

        self.current_entry = entry

        self.category_var.set(entry['category'])
        self.trigger_var.set(entry['trigger_key'])
        self.filename_var.set(entry['filename'])
        self.volume_var.set(entry['volume'])
        self.priority_var.set(entry['priority'])
        self.loops_var.set(entry['loops'])
        self.media_type_var.set(entry['media_type'])
        self.tag_var.set(entry['tag'] or '')
        self.use_key_var.set(entry['use_key'] or '')
        self.continue_var.set(bool(entry['use_continue']))
        self.caption_var.set(entry['caption'] or '')

        # Update trigger key suggestions
        self._update_trigger_suggestions()

        self.unsaved = False

    def _on_category_change(self, event=None):
        """Handle category combo change - update trigger suggestions."""
        self._update_trigger_suggestions()
        self._mark_unsaved()

    def _update_trigger_suggestions(self):
        """Update trigger key combobox suggestions based on category."""
        category = self.category_var.get()
        suggestions = self.repository.get_trigger_keys_for_category(category)
        self.trigger_combo['values'] = suggestions

    def _filter_entries(self):
        """Filter entries based on search."""
        term = self.search_var.get().lower()
        if not term:
            self._populate_entries_tree(self._current_entries)
        else:
            filtered = [
                e for e in self._current_entries
                if term in e['trigger_key'].lower() or term in e['filename'].lower()
            ]
            self._populate_entries_tree(filtered)

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current entry."""
        if not self.current_entry:
            messagebox.showinfo("Info", "Select an entry first, or create a new one.")
            return

        data = {
            'category': self.category_var.get(),
            'trigger_key': self.trigger_var.get(),
            'filename': self.filename_var.get(),
            'volume': self.volume_var.get(),
            'priority': self.priority_var.get(),
            'loops': self.loops_var.get(),
            'media_type': self.media_type_var.get(),
            'tag': self.tag_var.get() or '',
            'caption': self.caption_var.get() or '',
            'use_key': self.use_key_var.get() or None,
            'use_continue': 1 if self.continue_var.get() else 0,
        }

        if not data['trigger_key'] or not data['filename']:
            messagebox.showerror("Error", "Trigger key and filename are required.")
            return

        self.repository.update(self.current_entry['id'], data)
        self.unsaved = False

        self._load_data()
        # Re-select the current entry
        self.entries_tree.selection_set(str(self.current_entry['id']))
        self.on_status(f"Saved {data['category']}/{data['trigger_key']}")

    def _new_entry(self):
        """Create a new entry."""
        category = simpledialog.askstring(
            "New Audio Entry",
            "Enter category:\n(ambient, footstep, combat, weather, channel, time, ui, spell, environment)",
            parent=self
        )
        if not category or category not in self.repository.CATEGORIES:
            if category:
                messagebox.showerror("Error", "Invalid category.")
            return

        trigger = simpledialog.askstring(
            "New Audio Entry",
            f"Enter trigger key for {category}:",
            parent=self
        )
        if not trigger:
            return

        # Check if already exists
        if self.repository.find_by_trigger(category, trigger):
            messagebox.showerror("Error", f"Entry {category}/{trigger} already exists.")
            return

        # Insert with defaults
        new_id = self.repository.insert({
            'category': category,
            'trigger_key': trigger,
            'filename': f"{category}/new_sound.mp3",
            'volume': 50,
            'priority': 50,
            'loops': 1,
            'media_type': 'sound',
            'tag': category,
            'caption': '',
            'use_key': None,
            'use_continue': 0,
        })

        self._load_data()
        self.entries_tree.selection_set(str(new_id))
        self._load_entry(new_id)
        self.on_status(f"Created {category}/{trigger}")

    def _delete_entry(self):
        """Delete current entry."""
        if not self.current_entry:
            return

        if not messagebox.askyesno("Confirm Delete",
                                   f"Delete {self.current_entry['category']}/{self.current_entry['trigger_key']}?"):
            return

        self.repository.delete(self.current_entry['id'])
        self.current_entry = None
        self._clear_editor()
        self._load_data()
        self.on_status("Entry deleted")

    def _clear_editor(self):
        """Clear the editor form."""
        self.current_entry = None
        self.category_var.set('')
        self.trigger_var.set('')
        self.filename_var.set('')
        self.volume_var.set(50)
        self.priority_var.set(50)
        self.loops_var.set(1)
        self.media_type_var.set('sound')
        self.tag_var.set('')
        self.use_key_var.set('')
        self.continue_var.set(False)
        self.caption_var.set('')
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes", "Discard changes?")
        return True
