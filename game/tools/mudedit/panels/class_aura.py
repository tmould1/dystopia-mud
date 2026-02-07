"""
Class Aura editor panel.

Provides interface for editing class room aura text (displayed when looking
at characters in a room) with live ANSI color preview.
"""

import tkinter as tk
from tkinter import ttk, messagebox
from typing import Callable, Optional

from ..widgets.colors import parse_colored_segments, DEFAULT_FG, PREVIEW_BG, xterm256_to_hex


class ClassAuraPanel(ttk.Frame):
    """
    Editor panel for class room aura text.

    Shows a list of classes with their aura text and MXP tooltip,
    with live color preview.
    """

    def __init__(
        self,
        parent,
        auras_repo,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.auras_repo = auras_repo
        self.on_status = on_status or (lambda msg: None)

        self.current_class_id: Optional[int] = None
        self.unsaved = False

        self._build_ui()
        self._load_auras()

    def _build_ui(self):
        """Build the panel UI."""
        main_paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: aura list
        left_frame = ttk.LabelFrame(main_paned, text="Class Auras")
        main_paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.aura_tree = ttk.Treeview(
            tree_frame,
            columns=('class_id', 'class_name', 'order'),
            show='headings',
            selectmode='browse'
        )
        self.aura_tree.heading('class_id', text='ID')
        self.aura_tree.heading('class_name', text='Class')
        self.aura_tree.heading('order', text='Order')
        self.aura_tree.column('class_id', width=50, stretch=False)
        self.aura_tree.column('class_name', width=100)
        self.aura_tree.column('order', width=50, stretch=False)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.aura_tree.yview)
        self.aura_tree.configure(yscrollcommand=scrollbar.set)
        self.aura_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.aura_tree.bind('<<TreeviewSelect>>', self._on_aura_select)

        # Right: editor and preview
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=3)

        # Edit section
        edit_frame = ttk.LabelFrame(right_frame, text="Edit Aura")
        edit_frame.pack(fill=tk.X, padx=4, pady=4)

        # Row 1: Class name
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

        # Row 2: Aura text
        row2 = ttk.Frame(edit_frame)
        row2.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row2, text="Aura Text:").pack(side=tk.LEFT)
        self.aura_var = tk.StringVar()
        self.aura_var.trace_add('write', lambda *_: self._on_change())
        self.aura_entry = ttk.Entry(row2, textvariable=self.aura_var, width=50, font=('Consolas', 10))
        self.aura_entry.pack(side=tk.LEFT, padx=(4, 8), fill=tk.X, expand=True)

        # Row 3: MXP tooltip
        row3 = ttk.Frame(edit_frame)
        row3.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row3, text="MXP Tooltip:").pack(side=tk.LEFT)
        self.tooltip_var = tk.StringVar()
        self.tooltip_var.trace_add('write', lambda *_: self._on_change())
        self.tooltip_entry = ttk.Entry(row3, textvariable=self.tooltip_var, width=30, font=('Consolas', 10))
        self.tooltip_entry.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Button(row3, text="Save", command=self._save_aura).pack(side=tk.LEFT, padx=8)

        # Preview section
        preview_frame = ttk.LabelFrame(right_frame, text="Preview (how it appears in room)")
        preview_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        self.preview_text = tk.Text(
            preview_frame,
            wrap=tk.NONE,
            state=tk.DISABLED,
            bg=PREVIEW_BG,
            fg=DEFAULT_FG,
            font=('Consolas', 11),
            relief=tk.SUNKEN,
            borderwidth=2,
            height=3
        )
        self.preview_text.pack(fill=tk.BOTH, expand=True, padx=8, pady=8)

        # Help text
        help_text = "Aura text shows before character name when looking in room. Use #X color codes. Changes require server restart."
        ttk.Label(preview_frame, text=help_text, foreground='gray', wraplength=500).pack(anchor=tk.W, padx=8, pady=(0, 4))

    def _load_auras(self):
        """Load auras into the tree."""
        self.aura_tree.delete(*self.aura_tree.get_children())

        entries = self.auras_repo.list_all()
        for entry in entries:
            self.aura_tree.insert('', tk.END, iid=str(entry['class_id']),
                                   values=(entry['class_id'], entry['class_name'], entry['display_order']))

        self.on_status(f"Loaded {len(entries)} class auras")

    def _on_aura_select(self, event):
        """Handle aura selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved Changes", "Discard unsaved changes?"):
                return

        selection = self.aura_tree.selection()
        if not selection:
            return

        class_id = int(selection[0])
        self.current_class_id = class_id

        entry = self.auras_repo.get_by_id(class_id)
        if entry:
            self.class_label.config(text=entry['class_name'])
            self.aura_var.set(entry['aura_text'])
            self.tooltip_var.set(entry['mxp_tooltip'])
            self.order_var.set(str(entry['display_order']))
            self.unsaved = False

            self._update_preview()
            self.on_status(f"Editing {entry['class_name']}")

    def _on_change(self):
        """Called when any field changes."""
        self.unsaved = True
        self._update_preview()

    def _update_preview(self):
        """Update the preview."""
        aura_text = self.aura_var.get() or "#nNo Aura#n"

        self.preview_text.config(state=tk.NORMAL)
        self.preview_text.delete('1.0', tk.END)

        # Show example: aura + character name
        self.preview_text.insert(tk.END, "Example: ")
        self._insert_colored_text(aura_text)
        self.preview_text.insert(tk.END, "SomePlayer is standing here.\n")

        self.preview_text.config(state=tk.DISABLED)

    def _insert_colored_text(self, text: str):
        """Insert text with MUD color codes into preview widget."""
        segments = parse_colored_segments(text)

        for plain_text, tag_name in segments:
            if not plain_text:
                continue

            code = tag_name[6:] if tag_name.startswith('color_') else tag_name

            if code == 'n':
                self.preview_text.insert(tk.END, plain_text)
            elif code.startswith('x') and len(code) > 1:
                try:
                    color_num = int(code[1:])
                    hex_color = xterm256_to_hex(color_num)
                    self.preview_text.tag_configure(tag_name, foreground=hex_color)
                    self.preview_text.insert(tk.END, plain_text, tag_name)
                except:
                    self.preview_text.insert(tk.END, plain_text)
            else:
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

    def _save_aura(self):
        """Save current aura."""
        if self.current_class_id is None:
            messagebox.showwarning("No Selection", "Please select a class first.")
            return

        aura_text = self.aura_var.get().strip()
        tooltip = self.tooltip_var.get().strip()

        try:
            order = int(self.order_var.get().strip())
        except ValueError:
            messagebox.showwarning("Invalid Order", "Display order must be a number.")
            return

        if not aura_text:
            messagebox.showwarning("Empty Aura", "Aura text cannot be empty.")
            return

        if not tooltip:
            messagebox.showwarning("Empty Tooltip", "MXP tooltip cannot be empty.")
            return

        self.auras_repo.update(self.current_class_id, {
            'aura_text': aura_text,
            'mxp_tooltip': tooltip,
            'display_order': order
        })

        self.unsaved = False
        self._load_auras()  # Refresh list to show updated order

        # Re-select current item
        self.aura_tree.selection_set(str(self.current_class_id))

        self.on_status("Aura saved. Restart server to apply.")

    def check_unsaved(self) -> bool:
        """Check for unsaved changes before closing."""
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes",
                                        "You have unsaved changes. Discard them?")
        return True
