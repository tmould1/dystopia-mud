"""
Class Equipment editor panel.

Unified editor for class armor configurations, armor pieces, and mastery items.
Combines the former class_armor and class_vnum_ranges panels into a single view.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Optional

from ..db.repository import get_class_name, CLASS_NAMES


class ClassEquipmentPanel(ttk.Frame):
    """
    Unified editor panel for class equipment (armor config, pieces, mastery).

    Shows armor configs on left, with config details, pieces, and computed
    VNUM range on right.
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
        left_frame = ttk.LabelFrame(main_paned, text="Classes with Equipment")
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

        # Row 4: Mastery vnum
        row4 = ttk.Frame(config_frame)
        row4.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row4, text="Mastery VNUM:").pack(side=tk.LEFT)
        self.mastery_var = tk.StringVar()
        self.mastery_var.trace_add('write', lambda *_: self._on_config_change())
        self.mastery_var.trace_add('write', lambda *_: self._update_mastery_preview())
        self.mastery_entry = ttk.Entry(
            row4, textvariable=self.mastery_var, width=10, font=('Consolas', 10)
        )
        self.mastery_entry.pack(side=tk.LEFT, padx=(4, 4))

        self.mastery_preview_var = tk.StringVar(value="")
        ttk.Label(
            row4, textvariable=self.mastery_preview_var,
            font=('Consolas', 9), foreground='#0066cc'
        ).pack(side=tk.LEFT, padx=(0, 16))

        ttk.Label(row4, text="(0 for none)", foreground='gray').pack(side=tk.LEFT)

        ttk.Button(
            row4, text="Save Config", command=self._save_config
        ).pack(side=tk.RIGHT, padx=8)

        # Pieces section with computed range
        pieces_outer = ttk.Frame(right_frame)
        pieces_outer.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left side: pieces
        pieces_frame = ttk.LabelFrame(pieces_outer, text="Armor Pieces")
        pieces_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

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
        self.pieces_tree.column('object_name', width=180)

        pieces_scroll = ttk.Scrollbar(
            pieces_list_frame, orient=tk.VERTICAL, command=self.pieces_tree.yview
        )
        self.pieces_tree.configure(yscrollcommand=pieces_scroll.set)
        self.pieces_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        pieces_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.pieces_tree.bind('<<TreeviewSelect>>', self._on_piece_select)

        # Custom armor note (shown when no pieces defined)
        self.custom_armor_label = ttk.Label(
            pieces_frame,
            text="This class uses a custom armor system.\nArmor pieces are not managed here.",
            foreground='#666666',
            font=('TkDefaultFont', 9, 'italic'),
            justify=tk.CENTER
        )
        # Hidden by default, shown when no pieces

        # Piece edit row
        piece_edit = ttk.Frame(pieces_frame)
        piece_edit.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(piece_edit, text="Keyword:").pack(side=tk.LEFT)
        self.piece_kw_var = tk.StringVar()
        self.piece_kw_var.trace_add('write', lambda *_: self._on_piece_change())
        self.piece_kw_entry = ttk.Entry(
            piece_edit, textvariable=self.piece_kw_var, width=12, font=('Consolas', 10)
        )
        self.piece_kw_entry.pack(side=tk.LEFT, padx=(4, 12))

        ttk.Label(piece_edit, text="VNUM:").pack(side=tk.LEFT)
        self.piece_vnum_var = tk.StringVar()
        self.piece_vnum_var.trace_add('write', lambda *_: self._on_piece_change())
        self.piece_vnum_var.trace_add('write', lambda *_: self._update_object_preview())
        self.piece_vnum_entry = ttk.Entry(
            piece_edit, textvariable=self.piece_vnum_var, width=8, font=('Consolas', 10)
        )
        self.piece_vnum_entry.pack(side=tk.LEFT, padx=(4, 4))

        self.object_name_var = tk.StringVar(value="")
        self.object_name_label = ttk.Label(
            piece_edit, textvariable=self.object_name_var,
            font=('Consolas', 9), foreground='#0066cc', width=20
        )
        self.object_name_label.pack(side=tk.LEFT, padx=(0, 8))

        # Piece buttons
        piece_btn_frame = ttk.Frame(pieces_frame)
        piece_btn_frame.pack(fill=tk.X, padx=8, pady=(0, 4))

        ttk.Button(
            piece_btn_frame, text="Save Piece", command=self._save_piece
        ).pack(side=tk.LEFT, padx=2)
        ttk.Button(
            piece_btn_frame, text="New", command=self._new_piece
        ).pack(side=tk.LEFT, padx=2)
        ttk.Button(
            piece_btn_frame, text="Delete", command=self._delete_piece
        ).pack(side=tk.LEFT, padx=2)

        # Right side: computed range and utilities
        range_frame = ttk.LabelFrame(pieces_outer, text="VNUM Range")
        range_frame.pack(side=tk.RIGHT, fill=tk.Y, padx=(8, 0))

        # Computed range display
        self.range_label = ttk.Label(
            range_frame, text="-", font=('Consolas', 11, 'bold'),
            foreground='#006600'
        )
        self.range_label.pack(padx=12, pady=8)

        self.piece_count_label = ttk.Label(
            range_frame, text="(0 pieces)", foreground='gray'
        )
        self.piece_count_label.pack(padx=12, pady=(0, 8))

        ttk.Separator(range_frame, orient=tk.HORIZONTAL).pack(fill=tk.X, padx=8)

        ttk.Button(
            range_frame, text="Check Overlap", command=self._check_overlap
        ).pack(padx=12, pady=8)

        ttk.Separator(range_frame, orient=tk.HORIZONTAL).pack(fill=tk.X, padx=8)

        ttk.Label(range_frame, text="Next Available:", foreground='gray').pack(padx=12, pady=(8, 0))
        self.next_avail_label = ttk.Label(
            range_frame, text="-", font=('Consolas', 10, 'bold')
        )
        self.next_avail_label.pack(padx=12, pady=(0, 8))

        # Help text
        ttk.Label(
            right_frame, text="Changes require server restart to take effect.",
            foreground='gray'
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

        self._update_next_available()
        self.on_status(f"Loaded {len(entries)} class equipment configurations")

    def _update_next_available(self):
        """Update the next available VNUM display."""
        try:
            next_vnum = self.pieces_repo.get_next_available_vnum(self.config_repo)
            self.next_avail_label.config(text=str(next_vnum))
        except Exception:
            self.next_avail_label.config(text="-")

    def _update_computed_range(self):
        """Update the computed VNUM range display."""
        if self.current_class_id is None:
            self.range_label.config(text="-")
            self.piece_count_label.config(text="(0 pieces)")
            return

        vnum_range = self.pieces_repo.get_vnum_range_for_class(self.current_class_id)
        if vnum_range[0] == 0 and vnum_range[1] == 0:
            self.range_label.config(text="No pieces")
        else:
            self.range_label.config(text=f"{vnum_range[0]} - {vnum_range[1]}")

        piece_count = len(self._pieces)
        self.piece_count_label.config(text=f"({piece_count} pieces)")

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
            mastery = entry.get('mastery_vnum', 0) or 0
            self.mastery_var.set(str(mastery))
            self.unsaved_config = False

        self._load_pieces_for_class(class_id)
        self._update_computed_range()
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

        # Show/hide custom armor note based on whether there are pieces
        if not self._pieces:
            self.custom_armor_label.pack(padx=8, pady=8)
        else:
            self.custom_armor_label.pack_forget()

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
                self.object_name_var.set(f"-> {obj_name}")
            else:
                self.object_name_var.set("(not found)")
        except ValueError:
            self.object_name_var.set("")

    def _update_mastery_preview(self):
        """Update the mastery object name preview."""
        vnum_str = self.mastery_var.get().strip()
        if not vnum_str or vnum_str == '0':
            self.mastery_preview_var.set("")
            return

        try:
            vnum = int(vnum_str)
            obj_name = self._lookup_object_name(vnum)
            if obj_name:
                self.mastery_preview_var.set(f"-> {obj_name}")
            else:
                self.mastery_preview_var.set("(not found)")
        except ValueError:
            self.mastery_preview_var.set("")

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

        try:
            mastery = int(self.mastery_var.get().strip())
        except ValueError:
            mastery = 0

        if not cost_key or not usage:
            messagebox.showwarning(
                "Required Fields", "Cost key and usage message are required."
            )
            return

        self.config_repo.update(self.current_class_id, {
            'acfg_cost_key': cost_key,
            'usage_message': usage,
            'act_to_char': to_char or "$p appears in your hands.",
            'act_to_room': to_room or "$p appears in $n's hands.",
            'mastery_vnum': mastery if mastery > 0 else 0
        })

        self.unsaved_config = False
        self._update_next_available()
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
            self._update_computed_range()
            self._update_next_available()
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
        self._update_computed_range()
        self._update_next_available()
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
            self._update_computed_range()
            self._update_next_available()

        self.piece_kw_var.set("")
        self.piece_vnum_var.set("")
        self.unsaved_piece = False

        self.on_status("Piece deleted.")

    def _check_overlap(self):
        """Check for overlapping VNUMs with other classes."""
        if self.current_class_id is None:
            messagebox.showwarning("No Selection", "Please select a class first.")
            return

        overlaps = []
        # Check each piece VNUM
        for piece in self._pieces:
            conflicts = self.pieces_repo.check_vnum_overlap(
                piece['vnum'], self.current_class_id
            )
            for conflict in conflicts:
                overlaps.append(
                    f"  VNUM {piece['vnum']} ({piece['keyword']}) conflicts with "
                    f"{get_class_name(conflict['class_id'])} ({conflict['keyword']})"
                )

        # Check mastery VNUM
        try:
            mastery = int(self.mastery_var.get().strip())
            if mastery > 0:
                mastery_conflicts = self.pieces_repo.check_vnum_overlap(
                    mastery, self.current_class_id
                )
                for conflict in mastery_conflicts:
                    overlaps.append(
                        f"  Mastery VNUM {mastery} conflicts with "
                        f"{get_class_name(conflict['class_id'])} ({conflict['keyword']})"
                    )
        except ValueError:
            pass

        if overlaps:
            messagebox.showwarning(
                "Overlaps Found",
                f"Found {len(overlaps)} VNUM conflict(s):\n\n" + "\n".join(overlaps)
            )
        else:
            messagebox.showinfo(
                "No Overlaps",
                "No VNUM conflicts found with other classes."
            )

    def check_unsaved(self) -> bool:
        """Check for unsaved changes before closing."""
        if self.unsaved_config or self.unsaved_piece:
            return messagebox.askyesno(
                "Unsaved Changes", "You have unsaved changes. Discard them?"
            )
        return True
