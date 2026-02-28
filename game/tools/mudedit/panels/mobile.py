"""
Mobile/NPC editor panel.

Provides CRUD interface for mobile entities with flags, dice, descriptions,
computed difficulty stats, and spawn location cross-references.
"""

import tkinter as tk
from tkinter import ttk, messagebox
from typing import Callable, Dict, Optional
import sys
from pathlib import Path

# Add mudlib to path for model imports
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from mudlib.models import ACT_FLAGS, AFF_FLAGS, SEX_TYPES

from ..db.repository import MobileRepository
from ..widgets import ColorTextEditor, EntityListPanel, FlagEditor, DiceEditor, strip_colors
from .stats_helpers import compute_mob_stats

# Tier colors for difficulty display
TIER_COLORS = {
    'trivial': '#808080', 'easy': '#00cc00', 'normal': '#c0c0c0',
    'hard': '#ff8800', 'deadly': '#ff4444',
}


class MobileEditorPanel(ttk.Frame):
    """
    Editor panel for mobile/NPC entities.

    Provides editing for:
    - Basic info (vnum, keywords, short/long descriptions)
    - Combat stats (level, hitroll, AC, hit dice, damage dice)
    - Computed difficulty analysis
    - Flags (act flags, affected_by flags)
    - Full description with color preview
    - Spawn location cross-references
    """

    def __init__(
        self,
        parent,
        repository: MobileRepository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.current_vnum: Optional[int] = None
        self.unsaved = False
        self._stats_after_id = None

        # Cache room names for spawn location display
        self._room_cache: Dict[int, str] = {}
        self._build_room_cache()

        self._build_ui()
        self._load_entries()

    def _build_room_cache(self):
        """Build cache of room vnum -> name."""
        try:
            for row in self.repository.conn.execute(
                "SELECT vnum, name FROM rooms"
            ).fetchall():
                self._room_cache[row[0]] = row[1]
        except Exception:
            pass

    def _build_ui(self):
        """Build the panel UI."""
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left panel: entity list
        self.list_panel = EntityListPanel(
            paned,
            columns=[
                ('vnum', 'Vnum', 60),
                ('short_descr', 'Name', 200),
                ('level', 'Lvl', 40),
            ],
            on_select=self._on_entry_select,
            search_columns=['player_name', 'short_descr']
        )
        paned.add(self.list_panel, weight=1)

        # Right panel: editor (scrollable)
        right_container = ttk.Frame(paned)
        paned.add(right_container, weight=3)

        self._canvas = tk.Canvas(right_container, highlightthickness=0)
        scrollbar = ttk.Scrollbar(right_container, orient=tk.VERTICAL,
                                  command=self._canvas.yview)

        self.editor_frame = ttk.Frame(self._canvas)

        self._canvas.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self._canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        canvas_frame = self._canvas.create_window(
            (0, 0), window=self.editor_frame, anchor=tk.NW)

        def configure_scroll(event):
            self._canvas.configure(scrollregion=self._canvas.bbox("all"))
            self._canvas.itemconfig(canvas_frame, width=event.width)

        self.editor_frame.bind(
            '<Configure>',
            lambda e: self._canvas.configure(
                scrollregion=self._canvas.bbox("all")))
        self._canvas.bind('<Configure>', configure_scroll)

        # Mousewheel: bind on enter, unbind on leave
        def on_mousewheel(event):
            self._canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")

        self._canvas.bind(
            '<Enter>', lambda e: self._canvas.bind_all('<MouseWheel>', on_mousewheel))
        self._canvas.bind(
            '<Leave>', lambda e: self._canvas.unbind_all('<MouseWheel>'))

        self._build_editor(self.editor_frame)

    def _build_editor(self, parent):
        """Build the editor form."""
        # === Basic Info Section ===
        basic_frame = ttk.LabelFrame(parent, text="Basic Info")
        basic_frame.pack(fill=tk.X, padx=4, pady=(4, 2))

        row1 = ttk.Frame(basic_frame)
        row1.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(row1, text="Vnum:").pack(side=tk.LEFT)
        self.vnum_label = ttk.Label(row1, text="-", font=('Consolas', 10, 'bold'))
        self.vnum_label.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row1, text="Keywords:").pack(side=tk.LEFT)
        self.keywords_var = tk.StringVar()
        self.keywords_var.trace_add('write', lambda *_: self._mark_unsaved())
        self.keywords_entry = ttk.Entry(row1, textvariable=self.keywords_var, width=40)
        self.keywords_entry.pack(side=tk.LEFT, padx=(4, 0), fill=tk.X, expand=True)

        row2 = ttk.Frame(basic_frame)
        row2.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(row2, text="Short Desc:").pack(side=tk.LEFT)
        self.short_var = tk.StringVar()
        self.short_var.trace_add('write', lambda *_: self._mark_unsaved())
        self.short_entry = ttk.Entry(row2, textvariable=self.short_var, width=60)
        self.short_entry.pack(side=tk.LEFT, padx=(4, 0), fill=tk.X, expand=True)

        row3 = ttk.Frame(basic_frame)
        row3.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(row3, text="Long Desc:").pack(side=tk.LEFT)
        self.long_var = tk.StringVar()
        self.long_var.trace_add('write', lambda *_: self._mark_unsaved())
        self.long_entry = ttk.Entry(row3, textvariable=self.long_var, width=60)
        self.long_entry.pack(side=tk.LEFT, padx=(4, 0), fill=tk.X, expand=True)

        row4 = ttk.Frame(basic_frame)
        row4.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(row4, text="Sex:").pack(side=tk.LEFT)
        self.sex_var = tk.StringVar(value="Neutral")
        self.sex_combo = ttk.Combobox(
            row4, textvariable=self.sex_var,
            values=["Neutral", "Male", "Female"],
            state='readonly', width=8
        )
        self.sex_combo.pack(side=tk.LEFT, padx=(4, 16))
        self.sex_combo.bind('<<ComboboxSelected>>', lambda e: self._mark_unsaved())

        ttk.Label(row4, text="Alignment:").pack(side=tk.LEFT)
        self.align_var = tk.IntVar(value=0)
        self.align_spin = ttk.Spinbox(
            row4, from_=-1000, to=1000, width=6,
            textvariable=self.align_var, command=self._mark_unsaved
        )
        self.align_spin.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row4, text="Gold:").pack(side=tk.LEFT)
        self.gold_var = tk.IntVar(value=0)
        self.gold_spin = ttk.Spinbox(
            row4, from_=0, to=999999999, width=10,
            textvariable=self.gold_var, command=self._mark_unsaved
        )
        self.gold_spin.pack(side=tk.LEFT, padx=(4, 0))

        # === Combat Stats Section ===
        combat_frame = ttk.LabelFrame(parent, text="Combat Stats")
        combat_frame.pack(fill=tk.X, padx=4, pady=2)

        crow1 = ttk.Frame(combat_frame)
        crow1.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(crow1, text="Level:").pack(side=tk.LEFT)
        self.level_var = tk.IntVar(value=1)
        self.level_spin = ttk.Spinbox(
            crow1, from_=1, to=5000, width=6,
            textvariable=self.level_var, command=self._mark_unsaved
        )
        self.level_spin.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(crow1, text="Hitroll:").pack(side=tk.LEFT)
        self.hitroll_var = tk.IntVar(value=0)
        self.hitroll_spin = ttk.Spinbox(
            crow1, from_=-100, to=10000, width=6,
            textvariable=self.hitroll_var, command=self._mark_unsaved
        )
        self.hitroll_spin.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(crow1, text="AC:").pack(side=tk.LEFT)
        self.ac_var = tk.IntVar(value=0)
        self.ac_spin = ttk.Spinbox(
            crow1, from_=-10000, to=10000, width=6,
            textvariable=self.ac_var, command=self._mark_unsaved
        )
        self.ac_spin.pack(side=tk.LEFT, padx=(4, 0))

        crow2 = ttk.Frame(combat_frame)
        crow2.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(crow2, text="Hit Points:").pack(side=tk.LEFT)
        self.hit_dice = DiceEditor(crow2, on_change=lambda v: self._mark_unsaved())
        self.hit_dice.pack(side=tk.LEFT, padx=(4, 0))

        crow3 = ttk.Frame(combat_frame)
        crow3.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(crow3, text="Damage:").pack(side=tk.LEFT)
        self.dam_dice = DiceEditor(crow3, on_change=lambda v: self._mark_unsaved())
        self.dam_dice.pack(side=tk.LEFT, padx=(4, 0))

        # === Computed Stats Section (read-only) ===
        stats_frame = ttk.LabelFrame(parent, text="Computed Stats")
        stats_frame.pack(fill=tk.X, padx=4, pady=2)

        srow1 = ttk.Frame(stats_frame)
        srow1.pack(fill=tk.X, padx=4, pady=1)

        ttk.Label(srow1, text="Difficulty:").pack(side=tk.LEFT)
        self.diff_tier_label = ttk.Label(
            srow1, text="-", font=('Consolas', 10, 'bold'))
        self.diff_tier_label.pack(side=tk.LEFT, padx=(4, 12))

        ttk.Label(srow1, text="Avg HP:").pack(side=tk.LEFT)
        self.avg_hp_label = ttk.Label(srow1, text="-", foreground='gray')
        self.avg_hp_label.pack(side=tk.LEFT, padx=(4, 12))

        ttk.Label(srow1, text="Avg Dmg/Round:").pack(side=tk.LEFT)
        self.avg_dmg_label = ttk.Label(srow1, text="-", foreground='gray')
        self.avg_dmg_label.pack(side=tk.LEFT, padx=(4, 0))

        srow2 = ttk.Frame(stats_frame)
        srow2.pack(fill=tk.X, padx=4, pady=1)

        ttk.Label(srow2, text="Attacks/Round:").pack(side=tk.LEFT)
        self.attacks_label = ttk.Label(srow2, text="-", foreground='gray')
        self.attacks_label.pack(side=tk.LEFT, padx=(4, 12))

        ttk.Label(srow2, text="HP/Level:").pack(side=tk.LEFT)
        self.hp_per_lvl_label = ttk.Label(srow2, text="-", foreground='gray')
        self.hp_per_lvl_label.pack(side=tk.LEFT, padx=(4, 0))

        # === Flags Section ===
        flags_frame = ttk.LabelFrame(parent, text="Flags")
        flags_frame.pack(fill=tk.X, padx=4, pady=2)

        self.act_flags = FlagEditor(
            flags_frame, ACT_FLAGS,
            label="Act Flags", columns=4,
            on_change=lambda v: self._mark_unsaved()
        )
        self.act_flags.pack(fill=tk.X, padx=4, pady=2)

        self.aff_flags = FlagEditor(
            flags_frame, AFF_FLAGS,
            label="Affected By", columns=4,
            on_change=lambda v: self._mark_unsaved()
        )
        self.aff_flags.pack(fill=tk.X, padx=4, pady=2)

        # === Description Section ===
        desc_frame = ttk.LabelFrame(parent, text="Full Description")
        desc_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=2)

        self.desc_editor = ColorTextEditor(
            desc_frame, show_preview=True,
            on_change=self._mark_unsaved
        )
        self.desc_editor.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # === Spawn Locations Section (cross-references) ===
        refs_frame = ttk.LabelFrame(parent, text="Spawn Locations")
        refs_frame.pack(fill=tk.X, padx=4, pady=2)

        self.refs_tree = ttk.Treeview(
            refs_frame,
            columns=('room', 'max'),
            show='headings',
            height=3
        )
        self.refs_tree.heading('room', text='Room')
        self.refs_tree.heading('max', text='Max')
        self.refs_tree.column('room', width=350)
        self.refs_tree.column('max', width=50)
        self.refs_tree.pack(fill=tk.X, padx=4, pady=2)

        # === Button Bar ===
        btn_frame = ttk.Frame(parent)
        btn_frame.pack(fill=tk.X, padx=4, pady=4)

        ttk.Button(btn_frame, text="Save", command=self._save).pack(
            side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(
            side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(
            side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Refresh Preview",
                   command=self.desc_editor.refresh_preview).pack(
            side=tk.LEFT, padx=(0, 4))

    # =========================================================================
    # Computed stats
    # =========================================================================

    def _schedule_stats_refresh(self):
        """Schedule a debounced stats refresh."""
        if self._stats_after_id:
            self.after_cancel(self._stats_after_id)
        self._stats_after_id = self.after(200, self._refresh_computed_stats)

    def _refresh_computed_stats(self):
        """Refresh the computed stats display."""
        self._stats_after_id = None

        if self.current_vnum is None:
            self.diff_tier_label.configure(text="-", foreground='gray')
            self.avg_hp_label.configure(text="-")
            self.avg_dmg_label.configure(text="-")
            self.attacks_label.configure(text="-")
            self.hp_per_lvl_label.configure(text="-")
            return

        try:
            hit_n, hit_s, hit_p = self.hit_dice.get_value()
        except (ValueError, tk.TclError):
            return

        mob_dict = {
            'level': self.level_var.get(),
            'hitnodice': hit_n,
            'hitsizedice': hit_s,
            'hitplus': hit_p,
            'hitroll': self.hitroll_var.get(),
            'ac': self.ac_var.get(),
            'act': self.act_flags.get_value(),
            'affected_by': self.aff_flags.get_value(),
        }

        stats = compute_mob_stats(mob_dict)

        tier = stats['difficulty_tier']
        color = TIER_COLORS.get(tier, '#c0c0c0')
        self.diff_tier_label.configure(
            text=f"[{tier.upper()}]", foreground=color)
        self.avg_hp_label.configure(text=f"{stats['avg_hp']:,}")
        self.avg_dmg_label.configure(text=f"{stats['avg_damage']:,}")
        self.attacks_label.configure(text=str(stats['num_attacks']))
        self.hp_per_lvl_label.configure(text=str(stats['hp_per_level']))

    # =========================================================================
    # Cross-references
    # =========================================================================

    def _update_refs(self, vnum: int):
        """Update spawn locations for the given mobile."""
        self.refs_tree.delete(*self.refs_tree.get_children())
        try:
            rows = self.repository.conn.execute(
                "SELECT arg2, arg3 FROM resets "
                "WHERE command = 'M' AND arg1 = ? ORDER BY sort_order",
                (vnum,)
            ).fetchall()
            for row in rows:
                max_count = row[0]
                room_vnum = row[1]
                room_name = self._room_cache.get(
                    room_vnum, f'(unknown #{room_vnum})')
                self.refs_tree.insert(
                    '', tk.END,
                    values=(f"[{room_vnum}] {room_name}", max_count))
        except Exception:
            pass

    # =========================================================================
    # Event handlers
    # =========================================================================

    def _load_entries(self):
        """Load all mobiles from database."""
        entries = self.repository.list_all()
        self.list_panel.set_items(entries, id_column='vnum')
        self.on_status(f"Loaded {len(entries)} mobiles")

    def _on_entry_select(self, vnum: int):
        """Handle entry selection from list."""
        if self.unsaved:
            if not messagebox.askyesno(
                "Unsaved Changes",
                "You have unsaved changes. Discard them?"
            ):
                if self.current_vnum is not None:
                    self.list_panel.select_item(self.current_vnum)
                return

        self._load_entry(vnum)

    def _load_entry(self, vnum: int):
        """Load a specific mobile into the editor."""
        mobile = self.repository.get_by_id(vnum)
        if not mobile:
            self.on_status(f"Mobile {vnum} not found")
            return

        self.current_vnum = vnum

        # Basic info
        self.vnum_label.configure(text=str(vnum))
        self.keywords_var.set(mobile.get('player_name', ''))
        self.short_var.set(mobile.get('short_descr', ''))
        self.long_var.set(mobile.get('long_descr', ''))

        # Sex
        self.sex_var.set(SEX_TYPES.get(mobile.get('sex', 0), "Neutral"))

        self.align_var.set(mobile.get('alignment', 0))
        self.gold_var.set(mobile.get('gold', 0))

        # Combat stats
        self.level_var.set(mobile.get('level', 1))
        self.hitroll_var.set(mobile.get('hitroll', 0))
        self.ac_var.set(mobile.get('ac', 0))

        # Dice
        self.hit_dice.set_value((
            mobile.get('hitnodice', 1),
            mobile.get('hitsizedice', 1),
            mobile.get('hitplus', 0)
        ))
        self.dam_dice.set_value((
            mobile.get('damnodice', 1),
            mobile.get('damsizedice', 1),
            mobile.get('damplus', 0)
        ))

        # Flags
        self.act_flags.set_value(mobile.get('act', 0))
        self.aff_flags.set_value(mobile.get('affected_by', 0))

        # Description
        self.desc_editor.set_text(mobile.get('description', ''))

        # Cross-references
        self._update_refs(vnum)

        # Computed stats
        self._refresh_computed_stats()

        self.unsaved = False
        self.on_status(f"Loaded mobile [{vnum}] {mobile.get('short_descr', '')}")

    def _mark_unsaved(self):
        """Mark the current entry as having unsaved changes."""
        self.unsaved = True
        self._schedule_stats_refresh()

    def _save(self):
        """Save current mobile to database."""
        if self.current_vnum is None:
            self.on_status("No mobile selected")
            return

        keywords = self.keywords_var.get().strip()
        if not keywords:
            messagebox.showwarning("Warning", "Keywords cannot be empty.")
            return

        sex_map = {"Neutral": 0, "Male": 1, "Female": 2}
        sex = sex_map.get(self.sex_var.get(), 0)

        hit_n, hit_s, hit_p = self.hit_dice.get_value()
        dam_n, dam_s, dam_p = self.dam_dice.get_value()

        data = {
            'player_name': keywords,
            'short_descr': self.short_var.get(),
            'long_descr': self.long_var.get(),
            'sex': sex,
            'alignment': self.align_var.get(),
            'gold': self.gold_var.get(),
            'level': self.level_var.get(),
            'hitroll': self.hitroll_var.get(),
            'ac': self.ac_var.get(),
            'hitnodice': hit_n,
            'hitsizedice': hit_s,
            'hitplus': hit_p,
            'damnodice': dam_n,
            'damsizedice': dam_s,
            'damplus': dam_p,
            'act': self.act_flags.get_value(),
            'affected_by': self.aff_flags.get_value(),
            'description': self.desc_editor.get_text(),
        }

        self.repository.update(self.current_vnum, data)

        self._load_entries()
        self.list_panel.select_item(self.current_vnum)

        self.unsaved = False
        self.on_status(
            f"Saved mobile [{self.current_vnum}] {self.short_var.get()}")

    def _new(self):
        """Create a new mobile."""
        entries = self.repository.list_all()
        if entries:
            max_vnum = max(e['vnum'] for e in entries)
            new_vnum = max_vnum + 1
        else:
            new_vnum = 1

        from tkinter import simpledialog
        vnum = simpledialog.askinteger(
            "New Mobile", "Enter vnum:",
            initialvalue=new_vnum, parent=self
        )
        if not vnum:
            return

        if self.repository.get_by_id(vnum):
            messagebox.showerror("Error", f"Mobile {vnum} already exists.")
            return

        self.repository.insert({
            'vnum': vnum,
            'player_name': 'new mobile',
            'short_descr': 'a new mobile',
            'long_descr': 'A new mobile stands here.',
            'description': '',
            'act': 1,
            'affected_by': 0,
            'alignment': 0,
            'level': 1,
            'hitroll': 0,
            'ac': 0,
            'hitnodice': 1,
            'hitsizedice': 8,
            'hitplus': 10,
            'damnodice': 1,
            'damsizedice': 4,
            'damplus': 0,
            'gold': 0,
            'sex': 0,
        })

        self._load_entries()
        self.list_panel.select_item(vnum)
        self._load_entry(vnum)
        self.on_status(f"Created mobile [{vnum}]")

    def _delete(self):
        """Delete the current mobile."""
        if self.current_vnum is None:
            self.on_status("No mobile selected")
            return

        short = self.short_var.get()
        if not messagebox.askyesno(
            "Confirm Delete",
            f"Delete mobile [{self.current_vnum}] {short}?"
        ):
            return

        self.repository.delete(self.current_vnum)

        old_vnum = self.current_vnum
        self._clear_editor()
        self._load_entries()

        self.on_status(f"Deleted mobile [{old_vnum}] {short}")

    def _clear_editor(self):
        """Clear the editor fields."""
        self.current_vnum = None
        self.vnum_label.configure(text="-")
        self.keywords_var.set('')
        self.short_var.set('')
        self.long_var.set('')
        self.sex_var.set("Neutral")
        self.align_var.set(0)
        self.gold_var.set(0)
        self.level_var.set(1)
        self.hitroll_var.set(0)
        self.ac_var.set(0)
        self.hit_dice.set_value((1, 1, 0))
        self.dam_dice.set_value((1, 1, 0))
        self.act_flags.clear()
        self.aff_flags.clear()
        self.desc_editor.clear()
        self.refs_tree.delete(*self.refs_tree.get_children())
        self._refresh_computed_stats()
        self.unsaved = False

    def check_unsaved(self) -> bool:
        """Check for unsaved changes and prompt user."""
        if self.unsaved:
            return messagebox.askyesno(
                "Unsaved Changes",
                "You have unsaved changes. Discard them?"
            )
        return True
