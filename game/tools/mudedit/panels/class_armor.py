"""
Class Armor editor panel.

Provides interface for editing class armor configurations and pieces
with the ability to manage armor cost settings and individual armor piece vnums.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Optional

from ..db.repository import get_class_name


class ClassArmorPanel(ttk.Frame):
    """
    Editor panel for class armor configuration and pieces.

    Shows armor configs on left, with config details and piece list on right.
    """

    def __init__(
        self,
        parent,
        config_repo,
        pieces_repo,
        ability_config_repo=None,
        db_manager=None,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.config_repo = config_repo
        self.pieces_repo = pieces_repo
        self.ability_config_repo = ability_config_repo
        self.db_manager = db_manager
        self.on_status = on_status or (lambda msg: None)

        self.current_class_id: Optional[int] = None
        self.current_piece_id: Optional[int] = None
        self.unsaved_config = False
        self.unsaved_piece = False
        self._pieces = []
        self._cost_keys = self._load_cost_keys()

        self._build_ui()
        self._load_configs()

    def _load_cost_keys(self) -> list:
        """Load ability config keys that are armor-related cost keys."""
        if not self.ability_config_repo:
            return []

        keys = []
        try:
            entries = self.ability_config_repo.list_all()
            for entry in entries:
                key = entry.get('key', '')
                # Match keys containing "armor" and ending with "cost"
                if 'armor' in key.lower() and key.endswith('_cost'):
                    keys.append(key)
        except Exception:
            pass

        return sorted(keys)

    def _lookup_object_name(self, vnum: int) -> Optional[str]:
        """Look up an object name from area databases by vnum."""
        if not self.db_manager:
            return None

        try:
            for area_db in self.db_manager.list_area_dbs():
                conn = self.db_manager.get_connection(area_db)
                row = conn.execute(
                    "SELECT short_descr FROM objects WHERE vnum = ?",
                    (vnum,)
                ).fetchone()
                if row:
                    return row['short_descr']
        except Exception:
            pass

        return None

    def _build_ui(self):
        """Build the panel UI."""
        main_paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: config list
        left_frame = ttk.LabelFrame(main_paned, text="Classes with Armor")
        main_paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.config_tree = ttk.Treeview(
            tree_frame,
            columns=('class_id', 'class_name'),
            show='headings',
            selectmode='browse'
        )
        self.config_tree.heading('class_id', text='ID')
        self.config_tree.heading('class_name', text='Class')
        self.config_tree.column('class_id', width=50, stretch=False)
        self.config_tree.column('class_name', width=100)

        scrollbar = ttk.Scrollbar(
            tree_frame, orient=tk.VERTICAL, command=self.config_tree.yview
        )
        self.config_tree.configure(yscrollcommand=scrollbar.set)
        self.config_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.config_tree.bind('<<TreeviewSelect>>', self._on_config_select)

        # Right: config editor and pieces
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=3)

        # Config edit section
        config_frame = ttk.LabelFrame(right_frame, text="Armor Configuration")
        config_frame.pack(fill=tk.X, padx=4, pady=4)

        # Row 1: Class name and cost key
        row1 = ttk.Frame(config_frame)
        row1.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row1, text="Class:").pack(side=tk.LEFT)
        self.class_label = ttk.Label(
            row1, text="-", font=('Consolas', 10, 'bold'), width=15
        )
        self.class_label.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row1, text="ACFG Cost Key:").pack(side=tk.LEFT)
        self.cost_key_var = tk.StringVar()
        self.cost_key_var.trace_add('write', lambda *_: self._on_config_change())
        self.cost_key_combo = ttk.Combobox(
            row1, textvariable=self.cost_key_var, width=40, font=('Consolas', 10),
            values=self._cost_keys
        )
        self.cost_key_combo.pack(side=tk.LEFT, padx=(4, 8), fill=tk.X, expand=True)

        # Row 2: Usage message
        row2 = ttk.Frame(config_frame)
        row2.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row2, text="Usage Message:").pack(side=tk.LEFT)
        self.usage_var = tk.StringVar()
        self.usage_var.trace_add('write', lambda *_: self._on_config_change())
        self.usage_entry = ttk.Entry(
            row2, textvariable=self.usage_var, width=60, font=('Consolas', 10)
        )
        self.usage_entry.pack(side=tk.LEFT, padx=(4, 8), fill=tk.X, expand=True)

        # Row 3: Act messages
        row3 = ttk.Frame(config_frame)
        row3.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row3, text="To Char:").pack(side=tk.LEFT)
        self.to_char_var = tk.StringVar()
        self.to_char_var.trace_add('write', lambda *_: self._on_config_change())
        self.to_char_entry = ttk.Entry(
            row3, textvariable=self.to_char_var, width=30, font=('Consolas', 10)
        )
        self.to_char_entry.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row3, text="To Room:").pack(side=tk.LEFT)
        self.to_room_var = tk.StringVar()
        self.to_room_var.trace_add('write', lambda *_: self._on_config_change())
        self.to_room_entry = ttk.Entry(
            row3, textvariable=self.to_room_var, width=30, font=('Consolas', 10)
        )
        self.to_room_entry.pack(side=tk.LEFT, padx=(4, 8))

        ttk.Button(
            row3, text="Save Config", command=self._save_config
        ).pack(side=tk.LEFT, padx=8)

        # Pieces section
        pieces_frame = ttk.LabelFrame(right_frame, text="Armor Pieces")
        pieces_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Pieces list
        pieces_list_frame = ttk.Frame(pieces_frame)
        pieces_list_frame.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)

        self.pieces_tree = ttk.Treeview(
            pieces_list_frame,
            columns=('keyword', 'vnum', 'object_name'),
            show='headings',
            selectmode='browse',
            height=8
        )
        self.pieces_tree.heading('keyword', text='Keyword')
        self.pieces_tree.heading('vnum', text='VNUM')
        self.pieces_tree.heading('object_name', text='Object Name')
        self.pieces_tree.column('keyword', width=100)
        self.pieces_tree.column('vnum', width=70)
        self.pieces_tree.column('object_name', width=200)

        pieces_scroll = ttk.Scrollbar(
            pieces_list_frame, orient=tk.VERTICAL, command=self.pieces_tree.yview
        )
        self.pieces_tree.configure(yscrollcommand=pieces_scroll.set)
        self.pieces_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        pieces_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.pieces_tree.bind('<<TreeviewSelect>>', self._on_piece_select)

        # Piece edit row
        piece_edit = ttk.Frame(pieces_frame)
        piece_edit.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(piece_edit, text="Keyword:").pack(side=tk.LEFT)
        self.piece_kw_var = tk.StringVar()
        self.piece_kw_var.trace_add('write', lambda *_: self._on_piece_change())
        self.piece_kw_entry = ttk.Entry(
            piece_edit, textvariable=self.piece_kw_var, width=15, font=('Consolas', 10)
        )
        self.piece_kw_entry.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(piece_edit, text="VNUM:").pack(side=tk.LEFT)
        self.piece_vnum_var = tk.StringVar()
        self.piece_vnum_var.trace_add('write', lambda *_: self._on_piece_change())
        self.piece_vnum_var.trace_add('write', lambda *_: self._update_object_preview())
        self.piece_vnum_entry = ttk.Entry(
            piece_edit, textvariable=self.piece_vnum_var, width=10, font=('Consolas', 10)
        )
        self.piece_vnum_entry.pack(side=tk.LEFT, padx=(4, 4))

        # Object name preview label
        self.object_name_var = tk.StringVar(value="")
        self.object_name_label = ttk.Label(
            piece_edit, textvariable=self.object_name_var,
            font=('Consolas', 9), foreground='#0066cc', width=30
        )
        self.object_name_label.pack(side=tk.LEFT, padx=(0, 8))

        ttk.Button(
            piece_edit, text="Save Piece", command=self._save_piece
        ).pack(side=tk.LEFT, padx=2)
        ttk.Button(
            piece_edit, text="New", command=self._new_piece
        ).pack(side=tk.LEFT, padx=2)
        ttk.Button(
            piece_edit, text="Del", command=self._delete_piece
        ).pack(side=tk.LEFT, padx=2)

        # Help text
        help_text = "Changes require server restart to take effect."
        ttk.Label(
            pieces_frame, text=help_text, foreground='gray'
        ).pack(anchor=tk.W, padx=8, pady=(0, 4))

    def _load_configs(self):
        """Load configs into the tree."""
        self.config_tree.delete(*self.config_tree.get_children())

        entries = self.config_repo.list_all()
        for entry in entries:
            class_name = get_class_name(entry['class_id'])
            self.config_tree.insert(
                '', tk.END, iid=str(entry['class_id']),
                values=(entry['class_id'], class_name)
            )

        self.on_status(f"Loaded {len(entries)} armor configurations")

    def _on_config_select(self, event):
        """Handle config selection."""
        if self.unsaved_config or self.unsaved_piece:
            if not messagebox.askyesno("Unsaved Changes", "Discard unsaved changes?"):
                return

        selection = self.config_tree.selection()
        if not selection:
            return

        class_id = int(selection[0])
        self.current_class_id = class_id
        self.current_piece_id = None

        entry = self.config_repo.get_by_id(class_id)
        if entry:
            class_name = get_class_name(class_id)
            self.class_label.config(text=class_name)
            self.cost_key_var.set(entry['acfg_cost_key'])
            self.usage_var.set(entry['usage_message'])
            self.to_char_var.set(entry['act_to_char'])
            self.to_room_var.set(entry['act_to_room'])
            self.unsaved_config = False

        self._load_pieces_for_class(class_id)
        self.on_status(f"Editing {class_name}")

    def _load_pieces_for_class(self, class_id: int):
        """Load pieces for a class into the tree."""
        self.pieces_tree.delete(*self.pieces_tree.get_children())
        self._pieces = self.pieces_repo.get_by_class(class_id)

        for piece in self._pieces:
            obj_name = self._lookup_object_name(piece['vnum']) or '(not found)'
            self.pieces_tree.insert(
                '', tk.END, iid=str(piece['id']),
                values=(piece['keyword'], piece['vnum'], obj_name)
            )

        # Clear piece editor
        self.piece_kw_var.set("")
        self.piece_vnum_var.set("")
        self.object_name_var.set("")
        self.unsaved_piece = False

    def _update_object_preview(self):
        """Update the object name preview when vnum changes."""
        vnum_str = self.piece_vnum_var.get().strip()
        if not vnum_str:
            self.object_name_var.set("")
            return

        try:
            vnum = int(vnum_str)
            obj_name = self._lookup_object_name(vnum)
            if obj_name:
                self.object_name_var.set(f"→ {obj_name}")
            else:
                self.object_name_var.set("(object not found)")
        except ValueError:
            self.object_name_var.set("")

    def _on_piece_select(self, event):
        """Handle piece selection."""
        if self.unsaved_piece:
            if not messagebox.askyesno("Unsaved Changes", "Discard piece changes?"):
                return

        selection = self.pieces_tree.selection()
        if not selection:
            return

        piece_id = int(selection[0])
        self.current_piece_id = piece_id

        piece = self.pieces_repo.get_by_id(piece_id)
        if piece:
            self.piece_kw_var.set(piece['keyword'])
            self.piece_vnum_var.set(str(piece['vnum']))
            # Trigger object preview update
            self._update_object_preview()
            self.unsaved_piece = False

    def _on_config_change(self):
        """Called when config fields change."""
        self.unsaved_config = True

    def _on_piece_change(self):
        """Called when piece fields change."""
        self.unsaved_piece = True

    def _save_config(self):
        """Save current config."""
        if self.current_class_id is None:
            messagebox.showwarning("No Selection", "Please select a class first.")
            return

        cost_key = self.cost_key_var.get().strip()
        usage = self.usage_var.get().strip()
        to_char = self.to_char_var.get().strip()
        to_room = self.to_room_var.get().strip()

        if not cost_key or not usage:
            messagebox.showwarning(
                "Required Fields", "Cost key and usage message are required."
            )
            return

        self.config_repo.update(self.current_class_id, {
            'acfg_cost_key': cost_key,
            'usage_message': usage,
            'act_to_char': to_char or "$p appears in your hands.",
            'act_to_room': to_room or "$p appears in $n's hands."
        })

        self.unsaved_config = False
        self.on_status("Config saved. Restart server to apply.")

    def _save_piece(self):
        """Save current piece."""
        if self.current_piece_id is None:
            messagebox.showwarning("No Selection", "Please select a piece first.")
            return

        keyword = self.piece_kw_var.get().strip().lower()
        try:
            vnum = int(self.piece_vnum_var.get().strip())
        except ValueError:
            messagebox.showwarning("Invalid VNUM", "VNUM must be a number.")
            return

        if not keyword:
            messagebox.showwarning("Empty Keyword", "Keyword cannot be empty.")
            return

        self.pieces_repo.update(self.current_piece_id, {
            'keyword': keyword,
            'vnum': vnum
        })

        self.unsaved_piece = False
        if self.current_class_id:
            self._load_pieces_for_class(self.current_class_id)
            # Re-select
            self.pieces_tree.selection_set(str(self.current_piece_id))

        self.on_status("Piece saved. Restart server to apply.")

    def _new_piece(self):
        """Create a new armor piece."""
        if self.current_class_id is None:
            messagebox.showwarning("No Class", "Please select a class first.")
            return

        keyword = simpledialog.askstring(
            "New Piece", "Keyword for new armor piece (e.g., 'ring', 'boots'):"
        )
        if not keyword:
            return

        keyword = keyword.strip().lower()
        existing = self.pieces_repo.get_by_class_and_keyword(
            self.current_class_id, keyword
        )
        if existing:
            messagebox.showwarning(
                "Exists", f"Keyword '{keyword}' already exists for this class."
            )
            return

        vnum = simpledialog.askinteger("VNUM", "Object VNUM for this piece:")
        if vnum is None:
            return

        new_id = self.pieces_repo.insert({
            'class_id': self.current_class_id,
            'keyword': keyword,
            'vnum': vnum
        })

        self._load_pieces_for_class(self.current_class_id)
        self.pieces_tree.selection_set(str(new_id))
        self._on_piece_select(None)

        self.on_status(f"Created piece '{keyword}'. Save to confirm.")

    def _delete_piece(self):
        """Delete current piece."""
        if self.current_piece_id is None:
            messagebox.showwarning("No Selection", "Please select a piece first.")
            return

        if not messagebox.askyesno("Confirm Delete", "Delete this armor piece?"):
            return

        self.pieces_repo.delete(self.current_piece_id)
        self.current_piece_id = None

        if self.current_class_id:
            self._load_pieces_for_class(self.current_class_id)

        self.piece_kw_var.set("")
        self.piece_vnum_var.set("")
        self.unsaved_piece = False

        self.on_status("Piece deleted.")

    def check_unsaved(self) -> bool:
        """Check for unsaved changes before closing."""
        if self.unsaved_config or self.unsaved_piece:
            return messagebox.askyesno(
                "Unsaved Changes", "You have unsaved changes. Discard them?"
            )
        return True
