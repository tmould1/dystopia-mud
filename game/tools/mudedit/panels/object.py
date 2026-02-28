"""
Object/Item editor panel.

Provides CRUD interface for object entities with type-specific value editing,
computed power analysis, flags, and affects.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Dict, List, Optional
import sys
from pathlib import Path

# Add mudlib to path for model imports
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from mudlib.models import (
    ITEM_TYPES, EXTRA_FLAGS, WEAR_FLAGS, APPLY_TYPES, WEAPON_TYPES,
    WEAPON_SPELLS, WEAPON_AFFECTS, ARMOR_SPECIALS, CONTAINER_FLAGS,
    WEAR_LOCATIONS,
)

from ..db.repository import ObjectRepository
from ..widgets import ColorTextEditor, EntityListPanel, FlagEditor, strip_colors
from .stats_helpers import compute_object_stats

# Tier colors for power display
TIER_COLORS = {
    'junk': '#808080', 'common': '#c0c0c0', 'uncommon': '#00cc00',
    'rare': '#4488ff', 'epic': '#cc44ff', 'legendary': '#ff8800',
}


class ObjectEditorPanel(ttk.Frame):
    """
    Editor panel for object/item entities.

    Provides editing for:
    - Basic info (vnum, keywords, descriptions)
    - Item type with type-specific value widgets
    - Computed power analysis
    - Extra flags and wear flags
    - Stat affects
    - Cross-references (where this object spawns)
    """

    def __init__(
        self,
        parent,
        repository: ObjectRepository,
        on_status: Optional[Callable[[str], None]] = None,
        liquids_repo=None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)
        self.liquids_repo = liquids_repo

        self.current_vnum: Optional[int] = None
        self.unsaved = False
        self._stats_after_id = None

        # Build liquid type lookup from database
        self._liquid_types: Dict[int, str] = {0: '(none)'}
        if self.liquids_repo:
            try:
                for liq in self.liquids_repo.list_all():
                    self._liquid_types[liq['id']] = liq.get('name', f'liquid_{liq["id"]}')
            except Exception:
                pass

        # Current value widget references (rebuilt per item type)
        self._value_widgets: Dict[str, any] = {}

        # Build cross-reference spawn map from resets
        self._obj_spawn_map: Dict[int, List[Dict]] = {}
        self._mob_cache: Dict[int, str] = {}
        self._room_cache: Dict[int, str] = {}
        self._build_spawn_map()

        self._build_ui()
        self._load_entries()

    def _build_spawn_map(self):
        """Build object spawn map and entity caches from resets table."""
        try:
            # Cache mob and room names
            for row in self.repository.conn.execute(
                "SELECT vnum, short_descr FROM mobiles"
            ).fetchall():
                self._mob_cache[row[0]] = row[1]
            for row in self.repository.conn.execute(
                "SELECT vnum, name FROM rooms"
            ).fetchall():
                self._room_cache[row[0]] = row[1]

            # Walk resets to build spawn map
            resets = self.repository.conn.execute(
                "SELECT command, arg1, arg2, arg3 FROM resets ORDER BY sort_order"
            ).fetchall()

            current_mob = None
            current_room = None
            for r in resets:
                cmd, arg1, arg2, arg3 = r
                if cmd == 'M':
                    current_mob = arg1
                    current_room = arg3
                elif cmd == 'O':
                    self._obj_spawn_map.setdefault(arg1, []).append(
                        {'type': 'room', 'room_vnum': arg3})
                elif cmd == 'G':
                    self._obj_spawn_map.setdefault(arg1, []).append(
                        {'type': 'inventory', 'mob_vnum': current_mob,
                         'room_vnum': current_room})
                elif cmd == 'E':
                    wear_name = WEAR_LOCATIONS.get(arg3, f'loc {arg3}')
                    self._obj_spawn_map.setdefault(arg1, []).append(
                        {'type': 'equipped', 'mob_vnum': current_mob,
                         'room_vnum': current_room, 'wear_loc': wear_name})
                elif cmd == 'P':
                    self._obj_spawn_map.setdefault(arg1, []).append(
                        {'type': 'container', 'container_vnum': arg3})
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
                ('item_type', 'Type', 60),
            ],
            on_select=self._on_entry_select,
            search_columns=['name', 'short_descr']
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

        ttk.Label(row3, text="Ground Desc:").pack(side=tk.LEFT)
        self.long_var = tk.StringVar()
        self.long_var.trace_add('write', lambda *_: self._mark_unsaved())
        self.long_entry = ttk.Entry(row3, textvariable=self.long_var, width=60)
        self.long_entry.pack(side=tk.LEFT, padx=(4, 0), fill=tk.X, expand=True)

        row4 = ttk.Frame(basic_frame)
        row4.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(row4, text="Weight:").pack(side=tk.LEFT)
        self.weight_var = tk.IntVar(value=1)
        self.weight_spin = ttk.Spinbox(
            row4, from_=0, to=999999, width=8,
            textvariable=self.weight_var, command=self._mark_unsaved
        )
        self.weight_spin.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row4, text="Cost:").pack(side=tk.LEFT)
        self.cost_var = tk.IntVar(value=0)
        self.cost_spin = ttk.Spinbox(
            row4, from_=0, to=999999999, width=10,
            textvariable=self.cost_var, command=self._mark_unsaved
        )
        self.cost_spin.pack(side=tk.LEFT, padx=(4, 0))

        # === Item Type Section ===
        type_frame = ttk.LabelFrame(parent, text="Item Type & Values")
        type_frame.pack(fill=tk.X, padx=4, pady=2)

        type_row = ttk.Frame(type_frame)
        type_row.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(type_row, text="Item Type:").pack(side=tk.LEFT)

        self._type_list = [(v, k) for k, v in sorted(ITEM_TYPES.items())]
        type_names = [f"{t[1]} - {t[0]}" for t in self._type_list]

        self.type_var = tk.StringVar()
        self.type_combo = ttk.Combobox(
            type_row, textvariable=self.type_var,
            values=type_names, state='readonly', width=25
        )
        self.type_combo.pack(side=tk.LEFT, padx=(4, 16))
        self.type_combo.bind('<<ComboboxSelected>>', self._on_type_change)

        # Dynamic value widget container
        self.values_container = ttk.Frame(type_frame)
        self.values_container.pack(fill=tk.X, padx=4, pady=2)

        # Start with default layout
        self._rebuild_value_widgets(0)

        # === Analysis Section (computed stats, read-only) ===
        self.analysis_frame = ttk.LabelFrame(parent, text="Analysis")
        self.analysis_frame.pack(fill=tk.X, padx=4, pady=2)

        stats_row1 = ttk.Frame(self.analysis_frame)
        stats_row1.pack(fill=tk.X, padx=4, pady=1)

        ttk.Label(stats_row1, text="Power:").pack(side=tk.LEFT)
        self.power_tier_label = ttk.Label(
            stats_row1, text="-", font=('Consolas', 10, 'bold'))
        self.power_tier_label.pack(side=tk.LEFT, padx=(4, 8))
        self.power_score_label = ttk.Label(
            stats_row1, text="", foreground='gray')
        self.power_score_label.pack(side=tk.LEFT)

        stats_row2 = ttk.Frame(self.analysis_frame)
        stats_row2.pack(fill=tk.X, padx=4, pady=1)

        self.stats_detail_label = ttk.Label(
            stats_row2, text="", foreground='gray')
        self.stats_detail_label.pack(side=tk.LEFT)

        stats_row3 = ttk.Frame(self.analysis_frame)
        stats_row3.pack(fill=tk.X, padx=4, pady=1)

        self.stats_extra_label = ttk.Label(
            stats_row3, text="", foreground='gray')
        self.stats_extra_label.pack(side=tk.LEFT)

        # === Flags Section ===
        flags_frame = ttk.LabelFrame(parent, text="Flags")
        flags_frame.pack(fill=tk.X, padx=4, pady=2)

        self.extra_flags = FlagEditor(
            flags_frame, EXTRA_FLAGS,
            label="Extra Flags", columns=4,
            on_change=lambda v: self._mark_unsaved()
        )
        self.extra_flags.pack(fill=tk.X, padx=4, pady=2)

        self.wear_flags = FlagEditor(
            flags_frame, WEAR_FLAGS,
            label="Wear Locations", columns=4,
            on_change=lambda v: self._mark_unsaved()
        )
        self.wear_flags.pack(fill=tk.X, padx=4, pady=2)

        # === Affects Section ===
        affects_frame = ttk.LabelFrame(parent, text="Stat Affects")
        affects_frame.pack(fill=tk.X, padx=4, pady=2)

        self.affects_tree = ttk.Treeview(
            affects_frame,
            columns=('type', 'modifier'),
            show='headings',
            height=4
        )
        self.affects_tree.heading('type', text='Apply Type')
        self.affects_tree.heading('modifier', text='Modifier')
        self.affects_tree.column('type', width=150)
        self.affects_tree.column('modifier', width=80)
        self.affects_tree.pack(fill=tk.X, padx=4, pady=2)

        aff_btn_frame = ttk.Frame(affects_frame)
        aff_btn_frame.pack(fill=tk.X, padx=4, pady=2)

        ttk.Button(aff_btn_frame, text="Add Affect",
                   command=self._add_affect).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(aff_btn_frame, text="Remove Affect",
                   command=self._remove_affect).pack(side=tk.LEFT)

        # === Power Messages Section ===
        power_frame = ttk.LabelFrame(parent, text="Power Messages (optional)")
        power_frame.pack(fill=tk.X, padx=4, pady=2)

        for field_name, label in [
            ('chpoweron', 'Equip (to char):'),
            ('chpoweroff', 'Remove (to char):'),
            ('victpoweron', 'Equip (to room):'),
            ('victpoweroff', 'Remove (to room):'),
        ]:
            row = ttk.Frame(power_frame)
            row.pack(fill=tk.X, padx=4, pady=1)
            ttk.Label(row, text=label, width=18).pack(side=tk.LEFT)
            var = tk.StringVar()
            var.trace_add('write', lambda *_: self._mark_unsaved())
            setattr(self, f'{field_name}_var', var)
            entry = ttk.Entry(row, textvariable=var, width=50)
            entry.pack(side=tk.LEFT, fill=tk.X, expand=True)

        # === Spawn Locations Section (cross-references) ===
        refs_frame = ttk.LabelFrame(parent, text="Spawn Locations")
        refs_frame.pack(fill=tk.X, padx=4, pady=2)

        self.refs_tree = ttk.Treeview(
            refs_frame,
            columns=('info',),
            show='headings',
            height=3
        )
        self.refs_tree.heading('info', text='Location')
        self.refs_tree.column('info', width=400)
        self.refs_tree.pack(fill=tk.X, padx=4, pady=2)

        # === Button Bar ===
        btn_frame = ttk.Frame(parent)
        btn_frame.pack(fill=tk.X, padx=4, pady=4)

        ttk.Button(btn_frame, text="Save", command=self._save).pack(
            side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(
            side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(
            side=tk.LEFT)

    # =========================================================================
    # Type-specific value widgets
    # =========================================================================

    def _get_current_type_id(self) -> int:
        """Extract the current item type ID from the combobox."""
        type_str = self.type_var.get()
        if not type_str:
            return 0
        try:
            return int(type_str.split(' - ')[0])
        except (ValueError, IndexError):
            return 0

    def _rebuild_value_widgets(self, type_id: int):
        """Rebuild value widgets for the given item type."""
        # Destroy existing widgets
        for child in self.values_container.winfo_children():
            child.destroy()
        self._value_widgets.clear()

        if type_id == 5:
            self._build_weapon_values()
        elif type_id == 9:
            self._build_armor_values()
        elif type_id == 15:
            self._build_container_values()
        elif type_id == 17:
            self._build_drink_values()
        elif type_id == 19:
            self._build_food_values()
        elif type_id == 20:
            self._build_money_values()
        else:
            self._build_default_values()

    def _build_weapon_values(self):
        """Build weapon-specific value widgets."""
        f = self.values_container

        # Row 1: Spell + Affect (decoded from value0)
        row1 = ttk.Frame(f)
        row1.pack(fill=tk.X, pady=2)

        ttk.Label(row1, text="Spell:", width=10).pack(side=tk.LEFT)
        spell_items = ['0 - (none)'] + [
            f"{k} - {v}" for k, v in sorted(WEAPON_SPELLS.items())
            if k > 0 and v is not None
        ]
        spell_var = tk.StringVar()
        spell_combo = ttk.Combobox(
            row1, textvariable=spell_var, values=spell_items,
            state='readonly', width=30)
        spell_combo.pack(side=tk.LEFT, padx=(2, 0))
        spell_combo.bind('<<ComboboxSelected>>', lambda e: self._mark_unsaved())
        self._value_widgets['spell_var'] = spell_var

        row2 = ttk.Frame(f)
        row2.pack(fill=tk.X, pady=2)

        ttk.Label(row2, text="Affect:", width=10).pack(side=tk.LEFT)
        affect_items = ['0 - (none)'] + [
            f"{k} - {v}" for k, v in sorted(WEAPON_AFFECTS.items())
            if k > 0 and v is not None
        ]
        affect_var = tk.StringVar()
        affect_combo = ttk.Combobox(
            row2, textvariable=affect_var, values=affect_items,
            state='readonly', width=30)
        affect_combo.pack(side=tk.LEFT, padx=(2, 0))
        affect_combo.bind('<<ComboboxSelected>>', lambda e: self._mark_unsaved())
        self._value_widgets['affect_var'] = affect_var

        # Row 3: Min/Max damage
        row3 = ttk.Frame(f)
        row3.pack(fill=tk.X, pady=2)

        ttk.Label(row3, text="Min Dmg:", width=10).pack(side=tk.LEFT)
        min_var = tk.IntVar(value=0)
        ttk.Spinbox(row3, from_=0, to=99999, width=8,
                     textvariable=min_var, command=self._mark_unsaved
                     ).pack(side=tk.LEFT, padx=(2, 16))
        self._value_widgets['min_var'] = min_var

        ttk.Label(row3, text="Max Dmg:").pack(side=tk.LEFT)
        max_var = tk.IntVar(value=0)
        ttk.Spinbox(row3, from_=0, to=99999, width=8,
                     textvariable=max_var, command=self._mark_unsaved
                     ).pack(side=tk.LEFT, padx=(2, 0))
        self._value_widgets['max_var'] = max_var

        # Row 4: Weapon type
        row4 = ttk.Frame(f)
        row4.pack(fill=tk.X, pady=2)

        ttk.Label(row4, text="Type:", width=10).pack(side=tk.LEFT)
        wtype_items = [f"{k} - {v}" for k, v in sorted(WEAPON_TYPES.items())]
        wtype_var = tk.StringVar()
        wtype_combo = ttk.Combobox(
            row4, textvariable=wtype_var, values=wtype_items,
            state='readonly', width=20)
        wtype_combo.pack(side=tk.LEFT, padx=(2, 0))
        wtype_combo.bind('<<ComboboxSelected>>', lambda e: self._mark_unsaved())
        self._value_widgets['wtype_var'] = wtype_var

    def _build_armor_values(self):
        """Build armor-specific value widgets."""
        f = self.values_container

        row1 = ttk.Frame(f)
        row1.pack(fill=tk.X, pady=2)

        ttk.Label(row1, text="AC Bonus:", width=10).pack(side=tk.LEFT)
        ac_var = tk.IntVar(value=0)
        ttk.Spinbox(row1, from_=0, to=9999, width=8,
                     textvariable=ac_var, command=self._mark_unsaved
                     ).pack(side=tk.LEFT, padx=(2, 0))
        self._value_widgets['ac_var'] = ac_var

        row2 = ttk.Frame(f)
        row2.pack(fill=tk.X, pady=2)

        ttk.Label(row2, text="Special:", width=10).pack(side=tk.LEFT)
        special_items = ['0 - (none)'] + [
            f"{k} - {v}" for k, v in sorted(ARMOR_SPECIALS.items())
            if k > 0 and v is not None
        ]
        special_var = tk.StringVar()
        special_combo = ttk.Combobox(
            row2, textvariable=special_var, values=special_items,
            state='readonly', width=30)
        special_combo.pack(side=tk.LEFT, padx=(2, 0))
        special_combo.bind('<<ComboboxSelected>>', lambda e: self._mark_unsaved())
        self._value_widgets['special_var'] = special_var

    def _build_container_values(self):
        """Build container-specific value widgets."""
        f = self.values_container

        row1 = ttk.Frame(f)
        row1.pack(fill=tk.X, pady=2)

        ttk.Label(row1, text="Capacity:", width=10).pack(side=tk.LEFT)
        cap_var = tk.IntVar(value=0)
        ttk.Spinbox(row1, from_=0, to=9999, width=8,
                     textvariable=cap_var, command=self._mark_unsaved
                     ).pack(side=tk.LEFT, padx=(2, 0))
        self._value_widgets['cap_var'] = cap_var

        row2 = ttk.Frame(f)
        row2.pack(fill=tk.X, pady=2)

        ttk.Label(row2, text="Flags:", width=10).pack(side=tk.LEFT)
        flag_vars = {}
        for bit, name in sorted(CONTAINER_FLAGS.items()):
            var = tk.BooleanVar()
            cb = ttk.Checkbutton(row2, text=name, variable=var,
                                 command=self._mark_unsaved)
            cb.pack(side=tk.LEFT, padx=(0, 8))
            flag_vars[bit] = var
        self._value_widgets['cont_flag_vars'] = flag_vars

        row3 = ttk.Frame(f)
        row3.pack(fill=tk.X, pady=2)

        ttk.Label(row3, text="Key Vnum:", width=10).pack(side=tk.LEFT)
        key_var = tk.IntVar(value=0)
        ttk.Spinbox(row3, from_=-1, to=99999, width=8,
                     textvariable=key_var, command=self._mark_unsaved
                     ).pack(side=tk.LEFT, padx=(2, 0))
        self._value_widgets['key_var'] = key_var

    def _build_drink_values(self):
        """Build drink container-specific value widgets."""
        f = self.values_container

        row1 = ttk.Frame(f)
        row1.pack(fill=tk.X, pady=2)

        ttk.Label(row1, text="Max:", width=10).pack(side=tk.LEFT)
        max_var = tk.IntVar(value=0)
        ttk.Spinbox(row1, from_=0, to=9999, width=8,
                     textvariable=max_var, command=self._mark_unsaved
                     ).pack(side=tk.LEFT, padx=(2, 16))
        self._value_widgets['drink_max_var'] = max_var

        ttk.Label(row1, text="Current:").pack(side=tk.LEFT)
        cur_var = tk.IntVar(value=0)
        ttk.Spinbox(row1, from_=0, to=9999, width=8,
                     textvariable=cur_var, command=self._mark_unsaved
                     ).pack(side=tk.LEFT, padx=(2, 0))
        self._value_widgets['drink_cur_var'] = cur_var

        row2 = ttk.Frame(f)
        row2.pack(fill=tk.X, pady=2)

        ttk.Label(row2, text="Liquid:", width=10).pack(side=tk.LEFT)
        liq_items = [f"{k} - {v}" for k, v in sorted(self._liquid_types.items())]
        liq_var = tk.StringVar()
        liq_combo = ttk.Combobox(
            row2, textvariable=liq_var, values=liq_items,
            state='readonly', width=25)
        liq_combo.pack(side=tk.LEFT, padx=(2, 0))
        liq_combo.bind('<<ComboboxSelected>>', lambda e: self._mark_unsaved())
        self._value_widgets['liq_var'] = liq_var

        row3 = ttk.Frame(f)
        row3.pack(fill=tk.X, pady=2)

        poison_var = tk.BooleanVar()
        ttk.Checkbutton(row3, text="Poisoned", variable=poison_var,
                        command=self._mark_unsaved).pack(side=tk.LEFT)
        self._value_widgets['drink_poison_var'] = poison_var

    def _build_food_values(self):
        """Build food-specific value widgets."""
        f = self.values_container

        row1 = ttk.Frame(f)
        row1.pack(fill=tk.X, pady=2)

        ttk.Label(row1, text="Fill Hours:", width=10).pack(side=tk.LEFT)
        hours_var = tk.IntVar(value=0)
        ttk.Spinbox(row1, from_=0, to=48, width=6,
                     textvariable=hours_var, command=self._mark_unsaved
                     ).pack(side=tk.LEFT, padx=(2, 0))
        self._value_widgets['food_hours_var'] = hours_var

        row2 = ttk.Frame(f)
        row2.pack(fill=tk.X, pady=2)

        poison_var = tk.BooleanVar()
        ttk.Checkbutton(row2, text="Poisoned", variable=poison_var,
                        command=self._mark_unsaved).pack(side=tk.LEFT)
        self._value_widgets['food_poison_var'] = poison_var

    def _build_money_values(self):
        """Build money-specific value widgets."""
        f = self.values_container

        row1 = ttk.Frame(f)
        row1.pack(fill=tk.X, pady=2)

        ttk.Label(row1, text="Gold:", width=10).pack(side=tk.LEFT)
        gold_var = tk.IntVar(value=0)
        ttk.Spinbox(row1, from_=0, to=999999999, width=12,
                     textvariable=gold_var, command=self._mark_unsaved
                     ).pack(side=tk.LEFT, padx=(2, 0))
        self._value_widgets['gold_var'] = gold_var

    def _build_default_values(self):
        """Build generic value0-3 spinboxes (fallback)."""
        f = self.values_container

        row = ttk.Frame(f)
        row.pack(fill=tk.X, pady=2)

        for i in range(4):
            ttk.Label(row, text=f"Value{i}:").pack(side=tk.LEFT)
            var = tk.IntVar(value=0)
            ttk.Spinbox(row, from_=-999999, to=999999999, width=10,
                         textvariable=var, command=self._mark_unsaved
                         ).pack(side=tk.LEFT, padx=(2, 12))
            self._value_widgets[f'v{i}'] = var

    # =========================================================================
    # Read/write value widgets
    # =========================================================================

    def _populate_value_widgets(self, v0: int, v1: int, v2: int, v3: int):
        """Populate the current value widgets from raw values."""
        type_id = self._get_current_type_id()
        w = self._value_widgets

        if type_id == 5:  # Weapon
            spell_id = v0 % 1000
            affect_id = v0 // 1000
            spell_name = WEAPON_SPELLS.get(spell_id)
            affect_name = WEAPON_AFFECTS.get(affect_id)
            w['spell_var'].set(
                f"{spell_id} - {spell_name}" if spell_name else
                f"{spell_id} - (none)" if spell_id == 0 else
                f"{spell_id} - spell_{spell_id}")
            w['affect_var'].set(
                f"{affect_id} - {affect_name}" if affect_name else
                f"{affect_id} - (none)" if affect_id == 0 else
                f"{affect_id} - affect_{affect_id}")
            w['min_var'].set(v1)
            w['max_var'].set(v2)
            wtype_name = WEAPON_TYPES.get(v3, f'type_{v3}')
            w['wtype_var'].set(f"{v3} - {wtype_name}")

        elif type_id == 9:  # Armor
            w['ac_var'].set(v0)
            special_name = ARMOR_SPECIALS.get(v3)
            w['special_var'].set(
                f"{v3} - {special_name}" if special_name else
                f"{v3} - (none)" if v3 == 0 else
                f"{v3} - special_{v3}")

        elif type_id == 15:  # Container
            w['cap_var'].set(v0)
            for bit, var in w['cont_flag_vars'].items():
                var.set(bool(v1 & bit))
            w['key_var'].set(v2)

        elif type_id == 17:  # Drink
            w['drink_max_var'].set(v0)
            w['drink_cur_var'].set(v1)
            liq_name = self._liquid_types.get(v2, f'liquid_{v2}')
            w['liq_var'].set(f"{v2} - {liq_name}")
            w['drink_poison_var'].set(v3 != 0)

        elif type_id == 19:  # Food
            w['food_hours_var'].set(v0)
            w['food_poison_var'].set(v3 != 0)

        elif type_id == 20:  # Money
            w['gold_var'].set(v0)

        else:  # Default
            for i, v in enumerate([v0, v1, v2, v3]):
                if f'v{i}' in w:
                    w[f'v{i}'].set(v)

    def _read_value_widgets(self) -> tuple:
        """Read raw value0-3 from the current value widgets."""
        type_id = self._get_current_type_id()
        w = self._value_widgets

        if type_id == 5:  # Weapon
            spell_id = self._combo_id(w.get('spell_var'))
            affect_id = self._combo_id(w.get('affect_var'))
            v0 = (affect_id * 1000) + spell_id
            v1 = w.get('min_var', tk.IntVar()).get()
            v2 = w.get('max_var', tk.IntVar()).get()
            v3 = self._combo_id(w.get('wtype_var'))

        elif type_id == 9:  # Armor
            v0 = w.get('ac_var', tk.IntVar()).get()
            v1 = 0
            v2 = 0
            v3 = self._combo_id(w.get('special_var'))

        elif type_id == 15:  # Container
            v0 = w.get('cap_var', tk.IntVar()).get()
            v1 = 0
            for bit, var in w.get('cont_flag_vars', {}).items():
                if var.get():
                    v1 |= bit
            v2 = w.get('key_var', tk.IntVar()).get()
            v3 = 0

        elif type_id == 17:  # Drink
            v0 = w.get('drink_max_var', tk.IntVar()).get()
            v1 = w.get('drink_cur_var', tk.IntVar()).get()
            v2 = self._combo_id(w.get('liq_var'))
            v3 = 1 if w.get('drink_poison_var', tk.BooleanVar()).get() else 0

        elif type_id == 19:  # Food
            v0 = w.get('food_hours_var', tk.IntVar()).get()
            v1 = 0
            v2 = 0
            v3 = 1 if w.get('food_poison_var', tk.BooleanVar()).get() else 0

        elif type_id == 20:  # Money
            v0 = w.get('gold_var', tk.IntVar()).get()
            v1 = 0
            v2 = 0
            v3 = 0

        else:  # Default
            v0 = w.get('v0', tk.IntVar()).get()
            v1 = w.get('v1', tk.IntVar()).get()
            v2 = w.get('v2', tk.IntVar()).get()
            v3 = w.get('v3', tk.IntVar()).get()

        return v0, v1, v2, v3

    @staticmethod
    def _combo_id(var) -> int:
        """Extract the integer ID from a 'N - name' combobox StringVar."""
        if var is None:
            return 0
        try:
            return int(var.get().split(' - ')[0])
        except (ValueError, IndexError, AttributeError):
            return 0

    # =========================================================================
    # Computed stats
    # =========================================================================

    def _schedule_stats_refresh(self):
        """Schedule a debounced stats refresh."""
        if self._stats_after_id:
            self.after_cancel(self._stats_after_id)
        self._stats_after_id = self.after(200, self._refresh_computed_stats)

    def _refresh_computed_stats(self):
        """Refresh the computed stats display from current widget values."""
        self._stats_after_id = None

        if self.current_vnum is None:
            self.power_tier_label.configure(text="-", foreground='gray')
            self.power_score_label.configure(text="")
            self.stats_detail_label.configure(text="")
            self.stats_extra_label.configure(text="")
            return

        # Build a dict from current widget values
        type_id = self._get_current_type_id()
        v0, v1, v2, v3 = self._read_value_widgets()

        obj_dict = {
            'item_type': type_id,
            'value0': v0, 'value1': v1, 'value2': v2, 'value3': v3,
        }

        # Build affects list from treeview
        apply_reverse = {v: k for k, v in APPLY_TYPES.items()}
        affects = []
        for item in self.affects_tree.get_children():
            values = self.affects_tree.item(item)['values']
            apply_id = apply_reverse.get(str(values[0]), 0)
            affects.append({'location': apply_id, 'modifier': int(values[1])})

        stats = compute_object_stats(obj_dict, affects)

        # Update tier label
        tier = stats['power_tier']
        color = TIER_COLORS.get(tier, '#c0c0c0')
        self.power_tier_label.configure(
            text=f"[{tier.upper()}]", foreground=color)
        self.power_score_label.configure(
            text=f"(score: {stats['power_score']})")

        # Detail line: hitroll / damroll / AC
        parts = []
        if stats['hitroll']:
            parts.append(f"Hitroll: {stats['hitroll']:+d}")
        if stats['damroll']:
            parts.append(f"Damroll: {stats['damroll']:+d}")
        if stats['ac_mod']:
            parts.append(f"AC: {stats['ac_mod']:+d}")
        self.stats_detail_label.configure(text="  ".join(parts))

        # Extra line: weapon/armor specifics
        extra = ""
        if type_id == 5 and 'weapon_min' in stats:
            extra = (f"Damage: {stats['weapon_min']}-{stats['weapon_max']} "
                     f"(avg {stats['weapon_avg']})  "
                     f"Type: {stats.get('weapon_type', '?')}")
            if stats.get('weapon_spell'):
                extra += f"  Spell: {stats['weapon_spell']}"
            if stats.get('weapon_affect'):
                extra += f"  Affect: {stats['weapon_affect']}"
        elif type_id == 9 and 'armor_ac' in stats:
            extra = f"Base AC: {stats['armor_ac']}"
            if stats.get('armor_special'):
                extra += f"  Special: {stats['armor_special']}"
        self.stats_extra_label.configure(text=extra)

    # =========================================================================
    # Cross-references
    # =========================================================================

    def _update_refs(self, vnum: int):
        """Update the spawn locations treeview for the given object."""
        self.refs_tree.delete(*self.refs_tree.get_children())

        spawns = self._obj_spawn_map.get(vnum, [])
        for spawn in spawns:
            stype = spawn['type']
            if stype == 'room':
                rv = spawn['room_vnum']
                rname = self._room_cache.get(rv, f'#{rv}')
                info = f"In room [{rv}] {rname}"
            elif stype == 'inventory':
                mv = spawn.get('mob_vnum')
                mname = self._mob_cache.get(mv, f'#{mv}') if mv else '?'
                info = f"Given to [{mv}] {mname}"
            elif stype == 'equipped':
                mv = spawn.get('mob_vnum')
                mname = self._mob_cache.get(mv, f'#{mv}') if mv else '?'
                wear = spawn.get('wear_loc', '?')
                info = f"Equipped on [{mv}] {mname} ({wear})"
            elif stype == 'container':
                cv = spawn['container_vnum']
                # Container is another object
                cname = None
                try:
                    cobj = self.repository.get_by_id(cv)
                    if cobj:
                        cname = cobj.get('short_descr', f'#{cv}')
                except Exception:
                    pass
                cname = cname or f'#{cv}'
                info = f"In container [{cv}] {cname}"
            else:
                info = f"{stype}: {spawn}"

            self.refs_tree.insert('', tk.END, values=(info,))

    # =========================================================================
    # Event handlers
    # =========================================================================

    def _on_type_change(self, event=None):
        """Handle item type change - rebuild value widgets."""
        self._mark_unsaved()
        type_id = self._get_current_type_id()
        self._rebuild_value_widgets(type_id)
        self._schedule_stats_refresh()

    def _load_entries(self):
        """Load all objects from database."""
        entries = self.repository.list_all()

        for entry in entries:
            type_id = entry.get('item_type', 0)
            entry['item_type'] = ITEM_TYPES.get(type_id, str(type_id))

        self.list_panel.set_items(entries, id_column='vnum')
        self.on_status(f"Loaded {len(entries)} objects")

    def _on_entry_select(self, vnum: int):
        """Handle entry selection from list."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved Changes",
                                       "Discard unsaved changes?"):
                if self.current_vnum is not None:
                    self.list_panel.select_item(self.current_vnum)
                return

        self._load_entry(vnum)

    def _load_entry(self, vnum: int):
        """Load a specific object into the editor."""
        obj = self.repository.get_with_affects(vnum)
        if not obj:
            self.on_status(f"Object {vnum} not found")
            return

        self.current_vnum = vnum

        # Basic info
        self.vnum_label.configure(text=str(vnum))
        self.keywords_var.set(obj.get('name', ''))
        self.short_var.set(obj.get('short_descr', ''))
        self.long_var.set(obj.get('description', ''))
        self.weight_var.set(obj.get('weight', 1))
        self.cost_var.set(obj.get('cost', 0))

        # Item type
        type_id = obj.get('item_type', 0)
        type_name = ITEM_TYPES.get(type_id, 'unknown')
        self.type_var.set(f"{type_id} - {type_name}")

        # Rebuild and populate type-specific widgets
        self._rebuild_value_widgets(type_id)
        self._populate_value_widgets(
            obj.get('value0', 0), obj.get('value1', 0),
            obj.get('value2', 0), obj.get('value3', 0))

        # Flags
        self.extra_flags.set_value(obj.get('extra_flags', 0))
        self.wear_flags.set_value(obj.get('wear_flags', 0))

        # Affects
        self.affects_tree.delete(*self.affects_tree.get_children())
        for aff in obj.get('affects', []):
            apply_id = aff.get('location', 0)
            apply_name = APPLY_TYPES.get(apply_id, f'unknown_{apply_id}')
            self.affects_tree.insert(
                '', tk.END, values=(apply_name, aff.get('modifier', 0)))

        # Power messages
        self.chpoweron_var.set(obj.get('chpoweron', ''))
        self.chpoweroff_var.set(obj.get('chpoweroff', ''))
        self.victpoweron_var.set(obj.get('victpoweron', ''))
        self.victpoweroff_var.set(obj.get('victpoweroff', ''))

        # Cross-references
        self._update_refs(vnum)

        # Computed stats
        self._refresh_computed_stats()

        self.unsaved = False
        self.on_status(f"Loaded object [{vnum}] {obj.get('short_descr', '')}")

    def _mark_unsaved(self):
        """Mark the current entry as having unsaved changes."""
        self.unsaved = True
        self._schedule_stats_refresh()

    def _add_affect(self):
        """Add a new stat affect."""
        dialog = tk.Toplevel(self)
        dialog.title("Add Affect")
        dialog.geometry("300x120")
        dialog.transient(self)
        dialog.grab_set()

        ttk.Label(dialog, text="Apply Type:").pack(pady=(10, 2))
        type_var = tk.StringVar()
        type_names = [f"{k} - {v}" for k, v in sorted(APPLY_TYPES.items())]
        type_combo = ttk.Combobox(
            dialog, textvariable=type_var, values=type_names, state='readonly')
        type_combo.pack()
        type_combo.current(0)

        ttk.Label(dialog, text="Modifier:").pack(pady=(10, 2))
        mod_var = tk.IntVar(value=1)
        mod_spin = ttk.Spinbox(
            dialog, from_=-1000, to=1000, textvariable=mod_var, width=10)
        mod_spin.pack()

        def on_ok():
            type_str = type_var.get()
            try:
                apply_name = type_str.split(' - ')[1]
            except IndexError:
                apply_name = type_str
            self.affects_tree.insert(
                '', tk.END, values=(apply_name, mod_var.get()))
            self._mark_unsaved()
            dialog.destroy()

        ttk.Button(dialog, text="Add", command=on_ok).pack(pady=10)

    def _remove_affect(self):
        """Remove the selected affect."""
        selection = self.affects_tree.selection()
        if selection:
            self.affects_tree.delete(selection[0])
            self._mark_unsaved()

    def _save(self):
        """Save current object to database."""
        if self.current_vnum is None:
            self.on_status("No object selected")
            return

        keywords = self.keywords_var.get().strip()
        if not keywords:
            messagebox.showwarning("Warning", "Keywords cannot be empty.")
            return

        type_id = self._get_current_type_id()
        v0, v1, v2, v3 = self._read_value_widgets()

        data = {
            'name': keywords,
            'short_descr': self.short_var.get(),
            'description': self.long_var.get(),
            'weight': self.weight_var.get(),
            'cost': self.cost_var.get(),
            'item_type': type_id,
            'value0': v0,
            'value1': v1,
            'value2': v2,
            'value3': v3,
            'extra_flags': self.extra_flags.get_value(),
            'wear_flags': self.wear_flags.get_value(),
            'chpoweron': self.chpoweron_var.get(),
            'chpoweroff': self.chpoweroff_var.get(),
            'victpoweron': self.victpoweron_var.get(),
            'victpoweroff': self.victpoweroff_var.get(),
        }

        self.repository.update(self.current_vnum, data)

        # Update affects (delete old, insert new)
        self.repository.conn.execute(
            "DELETE FROM object_affects WHERE obj_vnum = ?",
            (self.current_vnum,)
        )

        apply_reverse = {v: k for k, v in APPLY_TYPES.items()}

        for item in self.affects_tree.get_children():
            values = self.affects_tree.item(item)['values']
            apply_name = values[0]
            modifier = values[1]
            apply_id = apply_reverse.get(apply_name, 0)

            self.repository.conn.execute(
                "INSERT INTO object_affects "
                "(obj_vnum, location, modifier, sort_order) "
                "VALUES (?, ?, ?, ?)",
                (self.current_vnum, apply_id, modifier, 0)
            )

        self.repository.conn.commit()

        # Refresh list
        self._load_entries()
        self.list_panel.select_item(self.current_vnum)

        self.unsaved = False
        self.on_status(
            f"Saved object [{self.current_vnum}] {self.short_var.get()}")

    def _new(self):
        """Create a new object."""
        entries = self.repository.list_all()
        if entries:
            max_vnum = max(e['vnum'] for e in entries)
            new_vnum = max_vnum + 1
        else:
            new_vnum = 1

        vnum = simpledialog.askinteger(
            "New Object", "Enter vnum:",
            initialvalue=new_vnum, parent=self)
        if not vnum:
            return

        if self.repository.get_by_id(vnum):
            messagebox.showerror("Error", f"Object {vnum} already exists.")
            return

        self.repository.insert({
            'vnum': vnum,
            'name': 'new object',
            'short_descr': 'a new object',
            'description': 'A new object lies here.',
            'item_type': 8,  # treasure
            'extra_flags': 0,
            'wear_flags': 1,  # take
            'value0': 0, 'value1': 0, 'value2': 0, 'value3': 0,
            'weight': 1,
            'cost': 0,
        })

        self._load_entries()
        self.list_panel.select_item(vnum)
        self._load_entry(vnum)
        self.on_status(f"Created object [{vnum}]")

    def _delete(self):
        """Delete the current object."""
        if self.current_vnum is None:
            self.on_status("No object selected")
            return

        short = self.short_var.get()
        if not messagebox.askyesno(
            "Confirm Delete",
            f"Delete object [{self.current_vnum}] {short}?"
        ):
            return

        # Delete affects first
        self.repository.conn.execute(
            "DELETE FROM object_affects WHERE obj_vnum = ?",
            (self.current_vnum,)
        )
        self.repository.delete(self.current_vnum)

        old_vnum = self.current_vnum
        self._clear_editor()
        self._load_entries()

        self.on_status(f"Deleted object [{old_vnum}] {short}")

    def _clear_editor(self):
        """Clear the editor fields."""
        self.current_vnum = None
        self.vnum_label.configure(text="-")
        self.keywords_var.set('')
        self.short_var.set('')
        self.long_var.set('')
        self.weight_var.set(1)
        self.cost_var.set(0)
        self.type_var.set('')
        self._rebuild_value_widgets(0)
        self.extra_flags.clear()
        self.wear_flags.clear()
        self.affects_tree.delete(*self.affects_tree.get_children())
        self.chpoweron_var.set('')
        self.chpoweroff_var.set('')
        self.victpoweron_var.set('')
        self.victpoweroff_var.set('')
        self.refs_tree.delete(*self.refs_tree.get_children())
        self._refresh_computed_stats()
        self.unsaved = False

    def check_unsaved(self) -> bool:
        """Check for unsaved changes and prompt user."""
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes",
                                       "Discard unsaved changes?")
        return True
