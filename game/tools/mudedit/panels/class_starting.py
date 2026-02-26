"""
Class Starting editor panel.

Provides interface for editing class starting values (beast, level, disciplines)
that are applied when a player selects a class via selfclass command.
"""

import tkinter as tk
from tkinter import ttk, messagebox
from typing import Callable, Optional

from ..db.repository import load_class_names


class ClassStartingPanel(ttk.Frame):
    """
    Editor panel for class starting configuration.

    Shows a list of classes with their starting values and allows editing
    beast, level, and whether disciplines are initialized.
    """

    def __init__(
        self,
        parent,
        starting_repo,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.starting_repo = starting_repo
        self.on_status = on_status or (lambda msg: None)
        self.class_names = load_class_names(self.starting_repo.conn)

        self.current_class_id: Optional[int] = None
        self.unsaved = False

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        main_paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: class list
        left_frame = ttk.LabelFrame(main_paned, text="Classes")
        main_paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.class_tree = ttk.Treeview(
            tree_frame,
            columns=('class_id', 'class_name', 'beast', 'level', 'disciplines'),
            show='headings',
            selectmode='browse'
        )
        self.class_tree.heading('class_id', text='ID')
        self.class_tree.heading('class_name', text='Class')
        self.class_tree.heading('beast', text='Beast')
        self.class_tree.heading('level', text='Level')
        self.class_tree.heading('disciplines', text='Disciplines')
        self.class_tree.column('class_id', width=50, stretch=False)
        self.class_tree.column('class_name', width=100)
        self.class_tree.column('beast', width=50, stretch=False)
        self.class_tree.column('level', width=50, stretch=False)
        self.class_tree.column('disciplines', width=70, stretch=False)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.class_tree.yview)
        self.class_tree.configure(yscrollcommand=scrollbar.set)
        self.class_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.class_tree.bind('<<TreeviewSelect>>', self._on_class_select)

        # Right: editor
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=2)

        # Edit section
        edit_frame = ttk.LabelFrame(right_frame, text="Edit Starting Values")
        edit_frame.pack(fill=tk.X, padx=4, pady=4)

        # Row 1: Class name
        row1 = ttk.Frame(edit_frame)
        row1.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row1, text="Class:").pack(side=tk.LEFT)
        self.class_label = ttk.Label(row1, text="-", font=('Consolas', 10, 'bold'), width=15)
        self.class_label.pack(side=tk.LEFT, padx=(4, 16))

        # Row 2: Beast value
        row2 = ttk.Frame(edit_frame)
        row2.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row2, text="Starting Beast:").pack(side=tk.LEFT)
        self.beast_var = tk.StringVar()
        self.beast_var.trace_add('write', lambda *_: self._on_change())
        self.beast_entry = ttk.Entry(row2, textvariable=self.beast_var, width=8, font=('Consolas', 10))
        self.beast_entry.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row2, text="(Default: 15, Vampire: 30)", foreground='gray').pack(side=tk.LEFT)

        # Row 3: Level value
        row3 = ttk.Frame(edit_frame)
        row3.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row3, text="Starting Level:").pack(side=tk.LEFT)
        self.level_var = tk.StringVar()
        self.level_var.trace_add('write', lambda *_: self._on_change())
        self.level_entry = ttk.Entry(row3, textvariable=self.level_var, width=8, font=('Consolas', 10))
        self.level_entry.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row3, text="(Default: 1, Monk/Mage: 3)", foreground='gray').pack(side=tk.LEFT)

        # Row 4: Disciplines checkbox
        row4 = ttk.Frame(edit_frame)
        row4.pack(fill=tk.X, padx=8, pady=4)

        self.disciplines_var = tk.BooleanVar()
        self.disciplines_check = ttk.Checkbutton(
            row4,
            text="Initialize Disciplines (calls set_learnable_disciplines())",
            variable=self.disciplines_var,
            command=self._on_change
        )
        self.disciplines_check.pack(side=tk.LEFT)

        # Row 5: Save button
        row5 = ttk.Frame(edit_frame)
        row5.pack(fill=tk.X, padx=8, pady=8)

        ttk.Button(row5, text="Save", command=self._save_entry).pack(side=tk.LEFT)

        # Info section
        info_frame = ttk.LabelFrame(right_frame, text="Information")
        info_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        info_text = (
            "These values are applied when a player uses the 'selfclass' command.\n\n"
            "• Beast: Initial beast value (affects aggression/frenzy mechanics)\n"
            "• Level: Character level set immediately on class selection\n"
            "• Disciplines: Whether to call set_learnable_disciplines() for this class\n\n"
            "Changes require server restart to take effect."
        )
        info_label = ttk.Label(info_frame, text=info_text, wraplength=400, justify=tk.LEFT)
        info_label.pack(anchor=tk.W, padx=8, pady=8)

    def _load_entries(self):
        """Load entries into the tree."""
        self.class_tree.delete(*self.class_tree.get_children())

        entries = self.starting_repo.list_all()
        for entry in entries:
            class_name = self.class_names.get(entry['class_id'], f"Unknown ({entry['class_id']})")
            disciplines = "Yes" if entry['has_disciplines'] else "No"
            self.class_tree.insert('', tk.END, iid=str(entry['class_id']),
                                   values=(entry['class_id'], class_name,
                                           entry['starting_beast'], entry['starting_level'],
                                           disciplines))

        self.on_status(f"Loaded {len(entries)} class starting configs")

    def _on_class_select(self, event):
        """Handle class selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved Changes", "Discard unsaved changes?"):
                return

        selection = self.class_tree.selection()
        if not selection:
            return

        class_id = int(selection[0])
        self.current_class_id = class_id

        entry = self.starting_repo.get_by_id(class_id)
        if entry:
            class_name = self.class_names.get(class_id, f'Unknown ({class_id})')
            self.class_label.config(text=class_name)
            self.beast_var.set(str(entry['starting_beast']))
            self.level_var.set(str(entry['starting_level']))
            self.disciplines_var.set(bool(entry['has_disciplines']))
            self.unsaved = False

        self.on_status(f"Editing {class_name}")

    def _on_change(self):
        """Called when any field changes."""
        self.unsaved = True

    def _save_entry(self):
        """Save current entry."""
        if self.current_class_id is None:
            messagebox.showwarning("No Selection", "Please select a class first.")
            return

        try:
            beast = int(self.beast_var.get().strip())
        except ValueError:
            messagebox.showwarning("Invalid Beast", "Beast must be a number.")
            return

        try:
            level = int(self.level_var.get().strip())
        except ValueError:
            messagebox.showwarning("Invalid Level", "Level must be a number.")
            return

        if beast < 0:
            messagebox.showwarning("Invalid Beast", "Beast cannot be negative.")
            return

        if level < 1:
            messagebox.showwarning("Invalid Level", "Level must be at least 1.")
            return

        self.starting_repo.update(self.current_class_id, {
            'starting_beast': beast,
            'starting_level': level,
            'has_disciplines': 1 if self.disciplines_var.get() else 0
        })

        self.unsaved = False
        self._load_entries()  # Refresh list

        # Re-select current item
        self.class_tree.selection_set(str(self.current_class_id))

        self.on_status("Starting config saved. Restart server to apply.")

    def check_unsaved(self) -> bool:
        """Check for unsaved changes before closing."""
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes",
                                        "You have unsaved changes. Discard them?")
        return True
