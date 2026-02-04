"""
Class Display editor panel.

Provides interface for editing class brackets and generation titles
with live ANSI color preview using the ColorTextEditor widget.
"""

import tkinter as tk
from tkinter import ttk, messagebox
from typing import Callable, Optional

from ..widgets.color_text import ColorTextEditor
from ..db.repository import ClassBracketsRepository


class ClassDisplayPanel(ttk.Frame):
    """
    Editor panel for class brackets and generation titles.

    Uses a notebook with two tabs:
    - Class Brackets: Edit decorative brackets for each class
    - Generation Titles: Edit rank/generation names for each class
    """

    def __init__(
        self,
        parent,
        brackets_repo,
        generations_repo,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.brackets_repo = brackets_repo
        self.generations_repo = generations_repo
        self.on_status = on_status or (lambda msg: None)

        self.current_bracket_id: Optional[int] = None
        self.current_gen_id: Optional[int] = None
        self.unsaved_brackets = False
        self.unsaved_gens = False

        self._build_ui()
        self._load_brackets()
        self._load_generations()

    def _build_ui(self):
        """Build the panel UI with tabbed interface."""
        notebook = ttk.Notebook(self)
        notebook.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Tab 1: Class Brackets
        brackets_frame = ttk.Frame(notebook)
        notebook.add(brackets_frame, text="Class Brackets")
        self._build_brackets_ui(brackets_frame)

        # Tab 2: Generation Titles
        generations_frame = ttk.Frame(notebook)
        notebook.add(generations_frame, text="Generation Titles")
        self._build_generations_ui(generations_frame)

    def _build_brackets_ui(self, parent):
        """Build the brackets editor UI."""
        paned = ttk.PanedWindow(parent, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True)

        # Left: class list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.brackets_tree = ttk.Treeview(
            tree_frame,
            columns=('class_id', 'class_name'),
            show='headings',
            selectmode='browse'
        )
        self.brackets_tree.heading('class_id', text='ID')
        self.brackets_tree.heading('class_name', text='Class')
        self.brackets_tree.column('class_id', width=60, stretch=False)
        self.brackets_tree.column('class_name', width=120)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.brackets_tree.yview)
        self.brackets_tree.configure(yscrollcommand=scrollbar.set)
        self.brackets_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.brackets_tree.bind('<<TreeviewSelect>>', self._on_bracket_select)

        # Right: bracket editor with ColorTextEditor
        right_frame = ttk.LabelFrame(paned, text="Edit Brackets")
        paned.add(right_frame, weight=2)

        # Class info
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Class:", width=14).pack(side=tk.LEFT)
        self.bracket_class_label = ttk.Label(row, text="-", font=('Consolas', 10, 'bold'))
        self.bracket_class_label.pack(side=tk.LEFT)

        # Open bracket
        ttk.Label(right_frame, text="Open Bracket (with color codes):").pack(anchor=tk.W, padx=8, pady=(8, 2))
        self.open_bracket_editor = ColorTextEditor(right_frame, show_preview=True, on_change=self._mark_brackets_unsaved)
        self.open_bracket_editor.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)

        # Close bracket
        ttk.Label(right_frame, text="Close Bracket (with color codes):").pack(anchor=tk.W, padx=8, pady=(8, 2))
        self.close_bracket_editor = ColorTextEditor(right_frame, show_preview=True, on_change=self._mark_brackets_unsaved)
        self.close_bracket_editor.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save Bracket", command=self._save_bracket).pack(side=tk.LEFT, padx=4)

    def _build_generations_ui(self, parent):
        """Build the generations editor UI."""
        paned = ttk.PanedWindow(parent, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True)

        # Left: class/generation tree
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        # Class selector
        class_frame = ttk.Frame(left_frame)
        class_frame.pack(fill=tk.X, padx=4, pady=4)
        ttk.Label(class_frame, text="Class:").pack(side=tk.LEFT)
        self.class_combo = ttk.Combobox(class_frame, state='readonly', width=20)
        self.class_combo.pack(side=tk.LEFT, padx=4)
        self.class_combo.bind('<<ComboboxSelected>>', self._on_class_select)

        # Generation list
        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.gens_tree = ttk.Treeview(
            tree_frame,
            columns=('gen', 'title'),
            show='headings',
            selectmode='browse'
        )
        self.gens_tree.heading('gen', text='Gen')
        self.gens_tree.heading('title', text='Title (raw)')
        self.gens_tree.column('gen', width=50, stretch=False)
        self.gens_tree.column('title', width=150)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.gens_tree.yview)
        self.gens_tree.configure(yscrollcommand=scrollbar.set)
        self.gens_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.gens_tree.bind('<<TreeviewSelect>>', self._on_gen_select)

        # Right: title editor
        right_frame = ttk.LabelFrame(paned, text="Edit Generation Title")
        paned.add(right_frame, weight=2)

        # Generation info
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Generation:", width=14).pack(side=tk.LEFT)
        self.gen_label = ttk.Label(row, text="-", font=('Consolas', 10, 'bold'))
        self.gen_label.pack(side=tk.LEFT)

        # Title with color preview
        ttk.Label(right_frame, text="Title (with color codes):").pack(anchor=tk.W, padx=8, pady=(8, 2))
        self.title_editor = ColorTextEditor(right_frame, show_preview=True, on_change=self._mark_gens_unsaved)
        self.title_editor.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)

        # Help text
        help_text = "Gen 0 = default/fallback for any unspecified generation"
        ttk.Label(right_frame, text=help_text, foreground='gray').pack(anchor=tk.W, padx=8, pady=4)

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save Title", command=self._save_gen).pack(side=tk.LEFT, padx=4)
        ttk.Button(btn_frame, text="New Title", command=self._new_gen).pack(side=tk.LEFT, padx=4)
        ttk.Button(btn_frame, text="Delete", command=self._delete_gen).pack(side=tk.LEFT, padx=4)

    def _load_brackets(self):
        """Load brackets into the tree."""
        self.brackets_tree.delete(*self.brackets_tree.get_children())

        entries = self.brackets_repo.list_all()
        for entry in entries:
            self.brackets_tree.insert('', tk.END, iid=str(entry['class_id']),
                                       values=(entry['class_id'], entry['class_name']))

        self.on_status(f"Loaded {len(entries)} class brackets")

    def _load_generations(self):
        """Load class list into combo."""
        # Build class list from brackets (which has class names)
        brackets = self.brackets_repo.list_all()
        class_names = [(b['class_id'], b['class_name']) for b in brackets]
        class_names.sort(key=lambda x: x[1])

        self.class_list = class_names
        self.class_combo['values'] = [f"{name} ({cid})" for cid, name in class_names]

        if class_names:
            self.class_combo.current(0)
            self._on_class_select(None)

    def _on_bracket_select(self, event):
        """Handle bracket selection."""
        if self.unsaved_brackets:
            if not messagebox.askyesno("Unsaved Changes", "Discard unsaved bracket changes?"):
                return

        selection = self.brackets_tree.selection()
        if not selection:
            return

        class_id = int(selection[0])
        self.current_bracket_id = class_id

        entry = self.brackets_repo.get_by_id(class_id)
        if entry:
            self.bracket_class_label.config(text=f"{entry['class_name']} (ID: {class_id})")
            self.open_bracket_editor.set_text(entry['open_bracket'])
            self.close_bracket_editor.set_text(entry['close_bracket'])
            self.unsaved_brackets = False
            self.on_status(f"Editing brackets for {entry['class_name']}")

    def _on_class_select(self, event):
        """Handle class selection in generations tab."""
        idx = self.class_combo.current()
        if idx < 0 or idx >= len(self.class_list):
            return

        class_id = self.class_list[idx][0]
        self._load_gens_for_class(class_id)

    def _load_gens_for_class(self, class_id: int):
        """Load generation entries for a class."""
        self.gens_tree.delete(*self.gens_tree.get_children())

        entries = self.generations_repo.get_by_class(class_id)
        for entry in entries:
            gen_label = str(entry['generation']) if entry['generation'] > 0 else "0 (default)"
            # Strip color codes for display in tree (just show raw)
            title_preview = entry['title'][:30] + "..." if len(entry['title']) > 30 else entry['title']
            self.gens_tree.insert('', tk.END, iid=str(entry['id']),
                                   values=(gen_label, title_preview))

        self.on_status(f"Loaded {len(entries)} generation titles")

    def _on_gen_select(self, event):
        """Handle generation selection."""
        if self.unsaved_gens:
            if not messagebox.askyesno("Unsaved Changes", "Discard unsaved title changes?"):
                return

        selection = self.gens_tree.selection()
        if not selection:
            return

        gen_id = int(selection[0])
        self.current_gen_id = gen_id

        entry = self.generations_repo.get_by_id(gen_id)
        if entry:
            gen_label = str(entry['generation']) if entry['generation'] > 0 else "0 (default)"
            self.gen_label.config(text=gen_label)
            self.title_editor.set_text(entry['title'])
            self.unsaved_gens = False

    def _mark_brackets_unsaved(self):
        """Mark brackets as having unsaved changes."""
        self.unsaved_brackets = True

    def _mark_gens_unsaved(self):
        """Mark generations as having unsaved changes."""
        self.unsaved_gens = True

    def _save_bracket(self):
        """Save current bracket."""
        if self.current_bracket_id is None:
            messagebox.showwarning("No Selection", "Please select a class first.")
            return

        open_bracket = self.open_bracket_editor.get_text().strip()
        close_bracket = self.close_bracket_editor.get_text().strip()

        self.brackets_repo.update(self.current_bracket_id, {
            'open_bracket': open_bracket,
            'close_bracket': close_bracket
        })

        self.unsaved_brackets = False
        self.on_status("Bracket saved. Restart server to apply changes.")
        messagebox.showinfo("Saved", "Bracket saved. Restart server to apply changes.")

    def _save_gen(self):
        """Save current generation title."""
        if self.current_gen_id is None:
            messagebox.showwarning("No Selection", "Please select a generation first.")
            return

        title = self.title_editor.get_text().strip()
        if not title:
            messagebox.showwarning("Empty Title", "Title cannot be empty.")
            return

        self.generations_repo.update(self.current_gen_id, {
            'title': title
        })

        self.unsaved_gens = False

        # Refresh tree
        idx = self.class_combo.current()
        if idx >= 0:
            class_id = self.class_list[idx][0]
            self._load_gens_for_class(class_id)

        self.on_status("Generation title saved. Restart server to apply changes.")

    def _new_gen(self):
        """Create a new generation title."""
        idx = self.class_combo.current()
        if idx < 0:
            messagebox.showwarning("No Class", "Please select a class first.")
            return

        class_id = self.class_list[idx][0]

        # Ask for generation number
        from tkinter import simpledialog
        gen = simpledialog.askinteger("New Generation",
                                       "Generation number (1-8, or 0 for default):",
                                       minvalue=0, maxvalue=8)
        if gen is None:
            return

        # Check if exists
        existing = self.generations_repo.get_by_class_and_gen(class_id, gen)
        if existing:
            messagebox.showwarning("Exists", f"Generation {gen} already exists for this class.")
            return

        # Insert new entry
        new_id = self.generations_repo.insert({
            'class_id': class_id,
            'generation': gen,
            'title': '#nNew Title#n'
        })

        # Refresh and select new entry
        self._load_gens_for_class(class_id)
        self.gens_tree.selection_set(str(new_id))
        self._on_gen_select(None)

        self.on_status(f"Created generation {gen}. Edit the title and save.")

    def _delete_gen(self):
        """Delete current generation title."""
        if self.current_gen_id is None:
            messagebox.showwarning("No Selection", "Please select a generation first.")
            return

        if not messagebox.askyesno("Confirm Delete", "Delete this generation title?"):
            return

        self.generations_repo.delete(self.current_gen_id)
        self.current_gen_id = None

        # Refresh tree
        idx = self.class_combo.current()
        if idx >= 0:
            class_id = self.class_list[idx][0]
            self._load_gens_for_class(class_id)

        self.title_editor.set_text("")
        self.gen_label.config(text="-")
        self.unsaved_gens = False

        self.on_status("Generation title deleted.")

    def check_unsaved(self) -> bool:
        """Check for unsaved changes before closing."""
        if self.unsaved_brackets or self.unsaved_gens:
            return messagebox.askyesno("Unsaved Changes",
                                        "You have unsaved changes. Discard them?")
        return True
