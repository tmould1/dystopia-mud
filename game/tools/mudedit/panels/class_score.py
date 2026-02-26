"""
Class Score Stats editor panel.

Provides interface for editing class-specific score display stats
with support for the STAT_SOURCE enum and format strings.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Optional

import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from mudlib.models import STAT_SOURCES, get_stat_source_name

from ..db.repository import load_class_names


class ClassScorePanel(ttk.Frame):
    """
    Editor panel for class score stat configuration.

    Shows stats grouped by class on left, with editor on right.
    """

    def __init__(
        self,
        parent,
        score_repo,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.score_repo = score_repo
        self.on_status = on_status or (lambda msg: None)
        self.class_names = load_class_names(self.score_repo.conn)

        self.current_stat_id: Optional[int] = None
        self.unsaved = False

        self._build_ui()
        self._load_stats()

    def _build_ui(self):
        """Build the panel UI."""
        main_paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: stats list grouped by class
        left_frame = ttk.LabelFrame(main_paned, text="Score Stats by Class")
        main_paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.stats_tree = ttk.Treeview(
            tree_frame,
            columns=('class', 'label', 'source'),
            show='headings',
            selectmode='browse'
        )
        self.stats_tree.heading('class', text='Class')
        self.stats_tree.heading('label', text='Label')
        self.stats_tree.heading('source', text='Source')
        self.stats_tree.column('class', width=100)
        self.stats_tree.column('label', width=150)
        self.stats_tree.column('source', width=100)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.stats_tree.yview)
        self.stats_tree.configure(yscrollcommand=scrollbar.set)
        self.stats_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.stats_tree.bind('<<TreeviewSelect>>', self._on_stat_select)

        # Buttons under tree
        btn_frame = ttk.Frame(left_frame)
        btn_frame.pack(fill=tk.X, padx=4, pady=4)

        ttk.Button(btn_frame, text="New Stat", command=self._new_stat).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Delete", command=self._delete_stat).pack(side=tk.LEFT, padx=2)

        # Right: editor
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=2)

        # Edit section
        edit_frame = ttk.LabelFrame(right_frame, text="Edit Score Stat")
        edit_frame.pack(fill=tk.X, padx=4, pady=4)

        # Row 1: Class ID
        row1 = ttk.Frame(edit_frame)
        row1.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row1, text="Class:").pack(side=tk.LEFT)
        self.class_label = ttk.Label(row1, text="-", font=('Consolas', 10, 'bold'), width=15)
        self.class_label.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row1, text="Display Order:").pack(side=tk.LEFT)
        self.order_var = tk.StringVar()
        self.order_var.trace_add('write', lambda *_: self._on_change())
        self.order_entry = ttk.Entry(row1, textvariable=self.order_var, width=5, font=('Consolas', 10))
        self.order_entry.pack(side=tk.LEFT, padx=(4, 8))

        # Row 2: Stat source
        row2 = ttk.Frame(edit_frame)
        row2.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row2, text="Stat Source:").pack(side=tk.LEFT)
        self.source_var = tk.StringVar()
        self.source_combo = ttk.Combobox(
            row2, textvariable=self.source_var, width=30, state='readonly'
        )
        # Populate stat sources
        source_values = [f"{k}: {v[1]}" for k, v in STAT_SOURCES.items()]
        self.source_combo['values'] = source_values
        self.source_combo.bind('<<ComboboxSelected>>', lambda e: self._on_change())
        self.source_combo.pack(side=tk.LEFT, padx=(4, 8))

        # Row 3: Label
        row3 = ttk.Frame(edit_frame)
        row3.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row3, text="Label:").pack(side=tk.LEFT)
        self.label_var = tk.StringVar()
        self.label_var.trace_add('write', lambda *_: self._on_change())
        self.label_entry = ttk.Entry(row3, textvariable=self.label_var, width=40, font=('Consolas', 10))
        self.label_entry.pack(side=tk.LEFT, padx=(4, 8), fill=tk.X, expand=True)

        # Row 4: Format string
        row4 = ttk.Frame(edit_frame)
        row4.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row4, text="Format:").pack(side=tk.LEFT)
        self.format_var = tk.StringVar()
        self.format_var.trace_add('write', lambda *_: self._on_change())
        self.format_entry = ttk.Entry(row4, textvariable=self.format_var, width=50, font=('Consolas', 10))
        self.format_entry.pack(side=tk.LEFT, padx=(4, 8), fill=tk.X, expand=True)

        # Row 5: Save button
        row5 = ttk.Frame(edit_frame)
        row5.pack(fill=tk.X, padx=8, pady=8)

        ttk.Button(row5, text="Save", command=self._save_stat).pack(side=tk.LEFT)

        # Info section
        info_frame = ttk.LabelFrame(right_frame, text="Format String Help")
        info_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        info_text = (
            "Format string uses printf-style placeholders:\n"
            "  %s = Label text\n"
            "  %d = Stat value\n\n"
            "Example: #R[#n%s: #C%d#R]\\n\\r\n"
            "Result:  [Your current beast is: 30]\n\n"
            "Use \\n for newline, \\r for carriage return.\n"
            "Use #X for MUD color codes (R=red, C=cyan, etc.)\n\n"
            "Changes require server restart to take effect."
        )
        info_label = ttk.Label(info_frame, text=info_text, wraplength=400, justify=tk.LEFT,
                               font=('Consolas', 9))
        info_label.pack(anchor=tk.W, padx=8, pady=8)

    def _load_stats(self):
        """Load stats into the tree."""
        self.stats_tree.delete(*self.stats_tree.get_children())

        entries = self.score_repo.list_all()
        for entry in entries:
            class_name = self.class_names.get(entry['class_id'], f"Unknown ({entry['class_id']})")
            source_name = get_stat_source_name(entry['stat_source'])
            self.stats_tree.insert('', tk.END, iid=str(entry['id']),
                                   values=(class_name, entry['stat_label'], source_name))

        self.on_status(f"Loaded {len(entries)} class score stats")

    def _on_stat_select(self, event):
        """Handle stat selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved Changes", "Discard unsaved changes?"):
                return

        selection = self.stats_tree.selection()
        if not selection:
            return

        stat_id = int(selection[0])
        self.current_stat_id = stat_id

        entry = self.score_repo.get_by_id(stat_id)
        if entry:
            class_name = self.class_names.get(entry['class_id'], f"Unknown ({entry['class_id']})")
            self.class_label.config(text=class_name)
            self.order_var.set(str(entry['display_order']))
            self.label_var.set(entry['stat_label'])
            self.format_var.set(entry['format_string'])

            # Set source combo
            source_idx = entry['stat_source']
            if source_idx in STAT_SOURCES:
                combo_val = f"{source_idx}: {STAT_SOURCES[source_idx][1]}"
                self.source_var.set(combo_val)

            self.unsaved = False

        self.on_status(f"Editing {class_name} stat")

    def _on_change(self):
        """Called when any field changes."""
        self.unsaved = True

    def _save_stat(self):
        """Save current stat."""
        if self.current_stat_id is None:
            messagebox.showwarning("No Selection", "Please select a stat first.")
            return

        try:
            order = int(self.order_var.get().strip())
        except ValueError:
            messagebox.showwarning("Invalid Order", "Display order must be a number.")
            return

        label = self.label_var.get().strip()
        format_str = self.format_var.get().strip()

        if not label:
            messagebox.showwarning("Empty Label", "Label cannot be empty.")
            return

        if not format_str:
            messagebox.showwarning("Empty Format", "Format string cannot be empty.")
            return

        # Parse source from combo
        source_str = self.source_var.get()
        try:
            source_id = int(source_str.split(':')[0])
        except (ValueError, IndexError):
            messagebox.showwarning("Invalid Source", "Please select a valid stat source.")
            return

        self.score_repo.update(self.current_stat_id, {
            'stat_source': source_id,
            'stat_label': label,
            'format_string': format_str,
            'display_order': order
        })

        self.unsaved = False
        self._load_stats()

        # Re-select current item
        self.stats_tree.selection_set(str(self.current_stat_id))

        self.on_status("Score stat saved. Restart server to apply.")

    def _new_stat(self):
        """Create a new score stat."""
        # Ask for class
        class_options = list(self.class_names.items())
        class_str = simpledialog.askstring(
            "New Score Stat",
            "Enter class ID (e.g., 8 for Vampire):"
        )
        if not class_str:
            return

        try:
            class_id = int(class_str.strip())
        except ValueError:
            messagebox.showwarning("Invalid", "Class ID must be a number.")
            return

        if class_id not in self.class_names:
            messagebox.showwarning("Invalid", f"Unknown class ID: {class_id}")
            return

        # Insert with default values
        new_id = self.score_repo.insert({
            'class_id': class_id,
            'stat_source': 0,
            'stat_source_max': 0,
            'stat_label': 'New Stat',
            'format_string': '#R[#n%s: #C%d#R]\\n\\r',
            'display_order': 100
        })

        self._load_stats()
        self.stats_tree.selection_set(str(new_id))
        self._on_stat_select(None)

        self.on_status(f"Created new stat for {self.class_names.get(class_id, f'Unknown ({class_id})')}")

    def _delete_stat(self):
        """Delete current stat."""
        if self.current_stat_id is None:
            messagebox.showwarning("No Selection", "Please select a stat first.")
            return

        if not messagebox.askyesno("Confirm Delete", "Delete this score stat?"):
            return

        self.score_repo.delete(self.current_stat_id)
        self.current_stat_id = None

        self._load_stats()

        # Clear editor
        self.class_label.config(text="-")
        self.order_var.set("")
        self.label_var.set("")
        self.format_var.set("")
        self.source_var.set("")
        self.unsaved = False

        self.on_status("Score stat deleted.")

    def check_unsaved(self) -> bool:
        """Check for unsaved changes before closing."""
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes",
                                        "You have unsaved changes. Discard them?")
        return True
