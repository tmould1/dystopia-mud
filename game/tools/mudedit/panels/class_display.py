"""
Class Display editor panel.

Provides interface for editing class brackets and generation titles
with live ANSI color preview showing the combined result.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Optional

from ..widgets.colors import parse_colored_segments, DEFAULT_FG, PREVIEW_BG, xterm256_to_hex


class ClassDisplayPanel(ttk.Frame):
    """
    Editor panel for class brackets and generation titles.

    Shows a unified view with class list on left, and brackets + generations
    on the right with a combined preview at the bottom.
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

        self.current_class_id: Optional[int] = None
        self.current_gen_id: Optional[int] = None
        self.unsaved_brackets = False
        self.unsaved_gens = False
        self._gen_entries = []

        self._build_ui()
        self._load_classes()

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
            columns=('class_id', 'class_name'),
            show='headings',
            selectmode='browse'
        )
        self.class_tree.heading('class_id', text='ID')
        self.class_tree.heading('class_name', text='Class')
        self.class_tree.column('class_id', width=50, stretch=False)
        self.class_tree.column('class_name', width=100)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.class_tree.yview)
        self.class_tree.configure(yscrollcommand=scrollbar.set)
        self.class_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.class_tree.bind('<<TreeviewSelect>>', self._on_class_select)

        # Right: editors and preview (vertical layout)
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=4)

        # Top section: Compact editors
        edit_frame = ttk.LabelFrame(right_frame, text="Edit")
        edit_frame.pack(fill=tk.X, padx=4, pady=4)

        # Row 1: Class + Brackets (inline)
        row1 = ttk.Frame(edit_frame)
        row1.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row1, text="Class:").pack(side=tk.LEFT)
        self.class_label = ttk.Label(row1, text="-", font=('Consolas', 10, 'bold'), width=15)
        self.class_label.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row1, text="Open:").pack(side=tk.LEFT)
        self.open_var = tk.StringVar()
        self.open_var.trace_add('write', lambda *_: self._on_bracket_change())
        self.open_entry = ttk.Entry(row1, textvariable=self.open_var, width=15, font=('Consolas', 10))
        self.open_entry.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row1, text="Close:").pack(side=tk.LEFT)
        self.close_var = tk.StringVar()
        self.close_var.trace_add('write', lambda *_: self._on_bracket_change())
        self.close_entry = ttk.Entry(row1, textvariable=self.close_var, width=15, font=('Consolas', 10))
        self.close_entry.pack(side=tk.LEFT, padx=(4, 8))

        ttk.Button(row1, text="Save Brackets", command=self._save_brackets).pack(side=tk.LEFT, padx=8)

        # Row 2: Generation selection + title edit (inline)
        row2 = ttk.Frame(edit_frame)
        row2.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row2, text="Gen:").pack(side=tk.LEFT)
        self.gen_combo = ttk.Combobox(row2, state='readonly', width=10)
        self.gen_combo.pack(side=tk.LEFT, padx=(4, 16))
        self.gen_combo.bind('<<ComboboxSelected>>', self._on_gen_combo_select)

        ttk.Label(row2, text="Title:").pack(side=tk.LEFT)
        self.title_var = tk.StringVar()
        self.title_var.trace_add('write', lambda *_: self._on_title_change())
        self.title_entry = ttk.Entry(row2, textvariable=self.title_var, width=40, font=('Consolas', 10))
        self.title_entry.pack(side=tk.LEFT, padx=(4, 8), fill=tk.X, expand=True)

        ttk.Button(row2, text="Save Title", command=self._save_gen).pack(side=tk.LEFT, padx=2)
        ttk.Button(row2, text="New", command=self._new_gen).pack(side=tk.LEFT, padx=2)
        ttk.Button(row2, text="Del", command=self._delete_gen).pack(side=tk.LEFT, padx=2)

        # Main section: Combined Preview (takes most space)
        preview_frame = ttk.LabelFrame(right_frame, text="Preview (all generations with brackets)")
        preview_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        self.preview_text = tk.Text(
            preview_frame,
            wrap=tk.NONE,
            state=tk.DISABLED,
            bg=PREVIEW_BG,
            fg=DEFAULT_FG,
            font=('Consolas', 11),
            relief=tk.SUNKEN,
            borderwidth=2
        )
        preview_scroll = ttk.Scrollbar(preview_frame, orient=tk.VERTICAL, command=self.preview_text.yview)
        self.preview_text.configure(yscrollcommand=preview_scroll.set)
        self.preview_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=8, pady=8)
        preview_scroll.pack(side=tk.RIGHT, fill=tk.Y, pady=8)

        # Help text at bottom
        help_text = "Gen 0 = default/fallback. Changes require server restart."
        ttk.Label(preview_frame, text=help_text, foreground='gray').pack(anchor=tk.W, padx=8, pady=(0, 4))

    def _load_classes(self):
        """Load classes into the tree."""
        self.class_tree.delete(*self.class_tree.get_children())

        entries = self.brackets_repo.list_all()
        for entry in entries:
            self.class_tree.insert('', tk.END, iid=str(entry['class_id']),
                                    values=(entry['class_id'], entry['class_name']))

        self.on_status(f"Loaded {len(entries)} classes")

    def _on_class_select(self, event):
        """Handle class selection."""
        if self.unsaved_brackets or self.unsaved_gens:
            if not messagebox.askyesno("Unsaved Changes", "Discard unsaved changes?"):
                return

        selection = self.class_tree.selection()
        if not selection:
            return

        class_id = int(selection[0])
        self.current_class_id = class_id
        self.current_gen_id = None

        # Load bracket info
        entry = self.brackets_repo.get_by_id(class_id)
        if entry:
            self.class_label.config(text=entry['class_name'])
            self.open_var.set(entry['open_bracket'])
            self.close_var.set(entry['close_bracket'])
            self.unsaved_brackets = False

        # Load generations into combo (this also selects first gen and sets title)
        self._load_generations_for_class(class_id)
        self.unsaved_gens = False

        # Update preview
        self._update_preview()

        self.on_status(f"Editing {entry['class_name']}")

    def _load_generations_for_class(self, class_id: int):
        """Load generation entries for a class into the combo."""
        self._gen_entries = self.generations_repo.get_by_class(class_id)

        # Build combo values
        combo_values = []
        for entry in self._gen_entries:
            gen_label = f"Gen {entry['generation']}" if entry['generation'] > 0 else "Default (0)"
            combo_values.append(gen_label)

        self.gen_combo['values'] = combo_values
        if combo_values:
            self.gen_combo.current(0)
            self._on_gen_combo_select(None)

    def _on_gen_combo_select(self, event):
        """Handle generation combo selection."""
        if self.unsaved_gens and event is not None:
            if not messagebox.askyesno("Unsaved Changes", "Discard unsaved title changes?"):
                return

        idx = self.gen_combo.current()
        if idx < 0 or idx >= len(self._gen_entries):
            return

        entry = self._gen_entries[idx]
        self.current_gen_id = entry['id']
        self.title_var.set(entry['title'])
        self.unsaved_gens = False

        self._update_preview()

    def _on_bracket_change(self):
        """Called when bracket text changes."""
        self.unsaved_brackets = True
        self._update_preview()

    def _on_title_change(self):
        """Called when title text changes."""
        self.unsaved_gens = True
        self._update_preview()

    def _update_preview(self):
        """Update the combined preview."""
        open_b = self.open_var.get().strip()
        close_b = self.close_var.get().strip()
        title = self.title_var.get().strip() or "#nNo Title#n"

        # Build preview lines for ALL generations
        preview_lines = []

        if self.current_class_id:
            entries = self.generations_repo.get_by_class(self.current_class_id)
            # Show all generations
            for entry in entries:
                gen = entry['generation']
                gen_title = entry['title']
                # If this is the currently selected generation, use the editor value
                if self.current_gen_id and entry['id'] == self.current_gen_id:
                    gen_title = title
                combined = f"{open_b}{gen_title}{close_b}"
                gen_label = f"Gen {gen}" if gen > 0 else "Default"
                preview_lines.append((f"{gen_label:>8}: ", combined))

            if not entries:
                combined = f"{open_b}{title}{close_b}"
                preview_lines.append(("Preview: ", combined))
        else:
            combined = f"{open_b}{title}{close_b}"
            preview_lines.append(("Preview: ", combined))

        # Render preview
        self.preview_text.config(state=tk.NORMAL)
        self.preview_text.delete('1.0', tk.END)

        for label, colored_text in preview_lines:
            # Insert label
            self.preview_text.insert(tk.END, label)
            # Insert colored text
            self._insert_colored_text(colored_text)
            self.preview_text.insert(tk.END, "\n")

        self.preview_text.config(state=tk.DISABLED)

    def _insert_colored_text(self, text: str):
        """Insert text with MUD color codes into preview widget."""
        segments = parse_colored_segments(text)

        for plain_text, tag_name in segments:
            if not plain_text:
                continue

            # tag_name is like 'color_R', 'color_y', 'color_x123', 'color_n'
            # Extract the actual color code
            code = tag_name[6:] if tag_name.startswith('color_') else tag_name

            if code == 'n':
                # Reset - use default color
                self.preview_text.insert(tk.END, plain_text)
            elif code.startswith('x') and len(code) > 1:
                # 256-color code like 'x123'
                try:
                    color_num = int(code[1:])
                    hex_color = xterm256_to_hex(color_num)
                    self.preview_text.tag_configure(tag_name, foreground=hex_color)
                    self.preview_text.insert(tk.END, plain_text, tag_name)
                except:
                    self.preview_text.insert(tk.END, plain_text)
            else:
                # Standard color code
                color_map = {
                    'R': '#ff5555', 'r': '#aa0000',
                    'G': '#55ff55', 'g': '#00aa00',
                    'Y': '#ffff55', 'y': '#aaaa00',
                    'B': '#5555ff', 'b': '#0000aa',
                    'P': '#ff55ff', 'p': '#aa00aa',
                    'C': '#55ffff', 'c': '#00aaaa',
                    'W': '#ffffff', 'w': '#aaaaaa',
                    'L': '#55ffff', 'l': '#555555',
                    '0': '#555555', '7': '#c0c0c0', '9': '#ffffff',
                    'o': '#ffaa00', 'O': '#ffaa00',
                }
                if code in color_map:
                    self.preview_text.tag_configure(tag_name, foreground=color_map[code])
                    self.preview_text.insert(tk.END, plain_text, tag_name)
                else:
                    self.preview_text.insert(tk.END, plain_text)

    def _save_brackets(self):
        """Save current brackets."""
        if self.current_class_id is None:
            messagebox.showwarning("No Selection", "Please select a class first.")
            return

        open_bracket = self.open_var.get().strip()
        close_bracket = self.close_var.get().strip()

        self.brackets_repo.update(self.current_class_id, {
            'open_bracket': open_bracket,
            'close_bracket': close_bracket
        })

        self.unsaved_brackets = False
        self.on_status("Brackets saved. Restart server to apply.")

    def _save_gen(self):
        """Save current generation title."""
        if self.current_gen_id is None:
            messagebox.showwarning("No Selection", "Please select a generation first.")
            return

        title = self.title_var.get().strip()
        if not title:
            messagebox.showwarning("Empty Title", "Title cannot be empty.")
            return

        self.generations_repo.update(self.current_gen_id, {'title': title})
        self.unsaved_gens = False

        # Refresh combo and re-select
        if self.current_class_id:
            idx = self.gen_combo.current()
            self._load_generations_for_class(self.current_class_id)
            if idx >= 0:
                self.gen_combo.current(idx)

        self._update_preview()
        self.on_status("Title saved. Restart server to apply.")

    def _new_gen(self):
        """Create a new generation title."""
        if self.current_class_id is None:
            messagebox.showwarning("No Class", "Please select a class first.")
            return

        gen = simpledialog.askinteger("New Generation",
                                       "Generation number (1-8, or 0 for default):",
                                       minvalue=0, maxvalue=8)
        if gen is None:
            return

        existing = self.generations_repo.get_by_class_and_gen(self.current_class_id, gen)
        if existing:
            messagebox.showwarning("Exists", f"Generation {gen} already exists for this class.")
            return

        self.generations_repo.insert({
            'class_id': self.current_class_id,
            'generation': gen,
            'title': '#nNew Title#n'
        })

        self._load_generations_for_class(self.current_class_id)
        # Select the newly created one (will be in the list somewhere)
        for i, entry in enumerate(self._gen_entries):
            if entry['generation'] == gen:
                self.gen_combo.current(i)
                self._on_gen_combo_select(None)
                break

        self.on_status(f"Created generation {gen}. Edit and save.")

    def _delete_gen(self):
        """Delete current generation title."""
        if self.current_gen_id is None:
            messagebox.showwarning("No Selection", "Please select a generation first.")
            return

        if not messagebox.askyesno("Confirm Delete", "Delete this generation title?"):
            return

        self.generations_repo.delete(self.current_gen_id)
        self.current_gen_id = None

        if self.current_class_id:
            self._load_generations_for_class(self.current_class_id)

        self.title_var.set("")
        self.unsaved_gens = False
        self._update_preview()

        self.on_status("Generation title deleted.")

    def check_unsaved(self) -> bool:
        """Check for unsaved changes before closing."""
        if self.unsaved_brackets or self.unsaved_gens:
            return messagebox.askyesno("Unsaved Changes",
                                        "You have unsaved changes. Discard them?")
        return True
