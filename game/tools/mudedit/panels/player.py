"""
Player editor panel.

Provides interface for viewing and editing player data.
Password is write-only for security.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from pathlib import Path
from typing import Callable, Dict, List, Optional
import hashlib
import sys
from pathlib import Path

# Try to import crypt for Unix password hashing, fallback for Windows
try:
    import crypt
    HAS_CRYPT = True
except ImportError:
    HAS_CRYPT = False

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from mudlib.models import (
    CLASS_TABLE, DISCIPLINE_NAMES, PLR_FLAGS, EXTRA_FLAGS_PLAYER,
    NEWBITS_FLAGS, IMM_FLAGS, AFF_FLAGS, GIFT_NAMES
)


# Weapon proficiency names (index -> name)
WEAPON_NAMES = {
    0: 'Unarmed', 1: 'Slice', 2: 'Stab', 3: 'Slash', 4: 'Whip',
    5: 'Claw', 6: 'Blast', 7: 'Pound', 8: 'Crush', 9: 'Grep',
    10: 'Bite', 11: 'Pierce', 12: 'Suck',
}

# Spell proficiency names (index -> name)
SPELL_NAMES = {
    0: 'Purple (General)', 1: 'Red', 2: 'Blue', 3: 'Green', 4: 'Yellow',
}

# Stance proficiency names (index -> name) - required for mastery
STANCE_NAMES = {
    1: 'Viper', 2: 'Crane', 3: 'Crab', 4: 'Mongoose', 5: 'Bull',
    6: 'Mantis', 7: 'Dragon', 8: 'Tiger', 9: 'Monkey', 10: 'Swallow',
}

# Super stance names (optional display)
SUPER_STANCE_NAMES = {
    13: 'SS1', 14: 'SS2', 15: 'SS3', 16: 'SS4', 17: 'SS5',
}

MASTERY_THRESHOLD = 200


class PlayerEditorPanel(ttk.Frame):
    """
    Editor panel for player data.

    Features multiple tabs for different aspects of player data.
    Password field is write-only - can set but not read.
    """

    # Sex options
    SEXES = {0: 'Neutral', 1: 'Male', 2: 'Female'}

    def __init__(
        self,
        parent,
        repository,
        player_name: str,
        db_path: Optional[Path] = None,
        on_delete: Optional[Callable[[], None]] = None,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.player_name = player_name
        self.db_path = db_path
        self.on_delete = on_delete
        self.on_status = on_status or (lambda msg: None)

        self.unsaved = False

        self._build_ui()
        self._load_player()

    def _build_ui(self):
        """Build the panel UI."""
        # Header
        header = ttk.Frame(self)
        header.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(header, text=f"Player: {self.player_name}",
                 font=('Consolas', 14, 'bold')).pack(side=tk.LEFT)

        # Notebook for tabs
        self.notebook = ttk.Notebook(self)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Build tabs
        self._build_info_tab()
        self._build_stats_tab()
        self._build_mastery_tab()
        self._build_powers_tab()
        self._build_class_tab()
        self._build_flags_tab()
        self._build_messages_tab()
        self._build_skills_tab()
        self._build_aliases_tab()
        self._build_affects_tab()
        self._build_inventory_tab()
        self._build_password_tab()

        # Save button
        btn_frame = ttk.Frame(self)
        btn_frame.pack(fill=tk.X, padx=8, pady=4)
        ttk.Button(btn_frame, text="Save Changes", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Reload", command=self._load_player).pack(side=tk.LEFT)
        ttk.Button(btn_frame, text="Delete Player", command=self._delete_player).pack(side=tk.RIGHT)

    def _build_info_tab(self):
        """Build the basic info tab."""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Character Info")

        # Use canvas for scrolling
        canvas = tk.Canvas(frame)
        scrollbar = ttk.Scrollbar(frame, orient=tk.VERTICAL, command=canvas.yview)
        scrollable = ttk.Frame(canvas)

        scrollable.bind('<Configure>', lambda e: canvas.configure(scrollregion=canvas.bbox('all')))
        canvas.create_window((0, 0), window=scrollable, anchor=tk.NW)
        canvas.configure(yscrollcommand=scrollbar.set)

        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # Identity section
        identity = ttk.LabelFrame(scrollable, text="Identity")
        identity.pack(fill=tk.X, padx=8, pady=4)

        row = ttk.Frame(identity)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Name:", width=15).pack(side=tk.LEFT)
        self.name_var = tk.StringVar()
        ttk.Entry(row, textvariable=self.name_var, width=25).pack(side=tk.LEFT)

        row = ttk.Frame(identity)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Title:", width=15).pack(side=tk.LEFT)
        self.title_var = tk.StringVar()
        ttk.Entry(row, textvariable=self.title_var, width=40).pack(side=tk.LEFT)

        row = ttk.Frame(identity)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Sex:", width=15).pack(side=tk.LEFT)
        self.sex_var = tk.StringVar()
        sex_combo = ttk.Combobox(row, textvariable=self.sex_var, values=list(self.SEXES.values()), state='readonly', width=10)
        sex_combo.pack(side=tk.LEFT)

        row = ttk.Frame(identity)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Class:", width=15).pack(side=tk.LEFT)
        self.class_var = tk.StringVar()
        class_values = [f"{k} - {v}" for k, v in CLASS_TABLE.items()]
        ttk.Combobox(row, textvariable=self.class_var, values=class_values, state='readonly', width=25).pack(side=tk.LEFT)

        row = ttk.Frame(identity)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Level:", width=15).pack(side=tk.LEFT)
        self.level_var = tk.IntVar()
        ttk.Spinbox(row, from_=1, to=12, width=6, textvariable=self.level_var).pack(side=tk.LEFT)

        row = ttk.Frame(identity)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Trust:", width=15).pack(side=tk.LEFT)
        self.trust_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=12, width=6, textvariable=self.trust_var).pack(side=tk.LEFT)

        # Vitals section
        vitals = ttk.LabelFrame(scrollable, text="Vitals")
        vitals.pack(fill=tk.X, padx=8, pady=4)

        row = ttk.Frame(vitals)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="HP:", width=8).pack(side=tk.LEFT)
        self.hit_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=9999999, width=10, textvariable=self.hit_var).pack(side=tk.LEFT, padx=(0, 8))
        ttk.Label(row, text="/").pack(side=tk.LEFT)
        self.max_hit_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=9999999, width=10, textvariable=self.max_hit_var).pack(side=tk.LEFT, padx=8)

        row = ttk.Frame(vitals)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Mana:", width=8).pack(side=tk.LEFT)
        self.mana_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=9999999, width=10, textvariable=self.mana_var).pack(side=tk.LEFT, padx=(0, 8))
        ttk.Label(row, text="/").pack(side=tk.LEFT)
        self.max_mana_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=9999999, width=10, textvariable=self.max_mana_var).pack(side=tk.LEFT, padx=8)

        row = ttk.Frame(vitals)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Move:", width=8).pack(side=tk.LEFT)
        self.move_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=9999999, width=10, textvariable=self.move_var).pack(side=tk.LEFT, padx=(0, 8))
        ttk.Label(row, text="/").pack(side=tk.LEFT)
        self.max_move_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=9999999, width=10, textvariable=self.max_move_var).pack(side=tk.LEFT, padx=8)

        # Economy section
        economy = ttk.LabelFrame(scrollable, text="Economy & Progress")
        economy.pack(fill=tk.X, padx=8, pady=4)

        row = ttk.Frame(economy)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Gold:", width=15).pack(side=tk.LEFT)
        self.gold_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999999, width=15, textvariable=self.gold_var).pack(side=tk.LEFT)

        row = ttk.Frame(economy)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Experience:", width=15).pack(side=tk.LEFT)
        self.exp_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999999, width=15, textvariable=self.exp_var).pack(side=tk.LEFT)

        row = ttk.Frame(economy)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Quest Points:", width=15).pack(side=tk.LEFT)
        self.quest_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999999, width=15, textvariable=self.quest_var).pack(side=tk.LEFT)

        row = ttk.Frame(economy)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Practice:", width=15).pack(side=tk.LEFT)
        self.practice_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=99999, width=10, textvariable=self.practice_var).pack(side=tk.LEFT)

        # PvP/PvM stats
        pvp = ttk.LabelFrame(scrollable, text="Kill/Death Stats")
        pvp.pack(fill=tk.X, padx=8, pady=4)

        row = ttk.Frame(pvp)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Player Kills:", width=15).pack(side=tk.LEFT)
        self.pkill_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=10, textvariable=self.pkill_var).pack(side=tk.LEFT, padx=(0, 16))
        ttk.Label(row, text="Player Deaths:").pack(side=tk.LEFT)
        self.pdeath_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=10, textvariable=self.pdeath_var).pack(side=tk.LEFT)

        row = ttk.Frame(pvp)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Mob Kills:", width=15).pack(side=tk.LEFT)
        self.mkill_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=10, textvariable=self.mkill_var).pack(side=tk.LEFT, padx=(0, 16))
        ttk.Label(row, text="Mob Deaths:").pack(side=tk.LEFT)
        self.mdeath_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=10, textvariable=self.mdeath_var).pack(side=tk.LEFT)

        row = ttk.Frame(pvp)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Arena Wins:", width=15).pack(side=tk.LEFT)
        self.awins_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=10, textvariable=self.awins_var).pack(side=tk.LEFT, padx=(0, 16))
        ttk.Label(row, text="Arena Losses:").pack(side=tk.LEFT)
        self.alosses_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=10, textvariable=self.alosses_var).pack(side=tk.LEFT)

        # Location
        location = ttk.LabelFrame(scrollable, text="Location")
        location.pack(fill=tk.X, padx=8, pady=4)

        row = ttk.Frame(location)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Room Vnum:", width=15).pack(side=tk.LEFT)
        self.room_vnum_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=10, textvariable=self.room_vnum_var).pack(side=tk.LEFT)

        row = ttk.Frame(location)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Home Room:", width=15).pack(side=tk.LEFT)
        self.home_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=10, textvariable=self.home_var).pack(side=tk.LEFT)

    def _build_stats_tab(self):
        """Build the combat stats tab."""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Combat Stats")

        # Attributes
        attrs = ttk.LabelFrame(frame, text="Attributes (Permanent)")
        attrs.pack(fill=tk.X, padx=8, pady=4)

        self.attr_vars = {}
        attr_names = ['Strength', 'Intelligence', 'Wisdom', 'Dexterity', 'Constitution']
        for i, name in enumerate(attr_names):
            row = ttk.Frame(attrs)
            row.pack(fill=tk.X, padx=4, pady=2)
            ttk.Label(row, text=f"{name}:", width=15).pack(side=tk.LEFT)
            var = tk.IntVar()
            ttk.Spinbox(row, from_=0, to=250, width=6, textvariable=var).pack(side=tk.LEFT)
            self.attr_vars[i] = var

        # Combat stats
        combat = ttk.LabelFrame(frame, text="Combat Modifiers")
        combat.pack(fill=tk.X, padx=8, pady=4)

        row = ttk.Frame(combat)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Hitroll:", width=15).pack(side=tk.LEFT)
        self.hitroll_var = tk.IntVar()
        ttk.Spinbox(row, from_=-9999, to=9999, width=10, textvariable=self.hitroll_var).pack(side=tk.LEFT)

        row = ttk.Frame(combat)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Damroll:", width=15).pack(side=tk.LEFT)
        self.damroll_var = tk.IntVar()
        ttk.Spinbox(row, from_=-9999, to=9999, width=10, textvariable=self.damroll_var).pack(side=tk.LEFT)

        row = ttk.Frame(combat)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Armor:", width=15).pack(side=tk.LEFT)
        self.armor_var = tk.IntVar()
        ttk.Spinbox(row, from_=-9999, to=9999, width=10, textvariable=self.armor_var).pack(side=tk.LEFT)

        row = ttk.Frame(combat)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Saving Throw:", width=15).pack(side=tk.LEFT)
        self.saving_throw_var = tk.IntVar()
        ttk.Spinbox(row, from_=-9999, to=9999, width=10, textvariable=self.saving_throw_var).pack(side=tk.LEFT)

        row = ttk.Frame(combat)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Alignment:", width=15).pack(side=tk.LEFT)
        self.alignment_var = tk.IntVar()
        ttk.Spinbox(row, from_=-1000, to=1000, width=10, textvariable=self.alignment_var).pack(side=tk.LEFT)

    def _build_mastery_tab(self):
        """Build the mastery/level tab for weapon, spell, and stance proficiencies."""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Level/Mastery")

        # Use canvas for scrolling (pattern from _build_info_tab)
        canvas = tk.Canvas(frame)
        scrollbar = ttk.Scrollbar(frame, orient=tk.VERTICAL, command=canvas.yview)
        scrollable = ttk.Frame(canvas)

        scrollable.bind('<Configure>', lambda e: canvas.configure(scrollregion=canvas.bbox('all')))
        canvas.create_window((0, 0), window=scrollable, anchor=tk.NW)
        canvas.configure(yscrollcommand=scrollbar.set)

        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # Info banner
        info = ttk.Label(scrollable,
            text="Mastery requires all weapons, spells, and stances (1-10) >= 200",
            foreground='blue')
        info.pack(fill=tk.X, padx=8, pady=4)

        # Main content frame (horizontal layout for weapons and spells)
        main_frame = ttk.Frame(scrollable)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left side: Weapons
        weapons_frame = ttk.LabelFrame(main_frame, text="Weapon Proficiencies (wpn)")
        weapons_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=4, pady=4)

        self.wpn_vars = {}
        for idx, name in WEAPON_NAMES.items():
            row = ttk.Frame(weapons_frame)
            row.pack(fill=tk.X, padx=4, pady=1)
            ttk.Label(row, text=f"{name} ({idx}):", width=15).pack(side=tk.LEFT)
            var = tk.IntVar()
            ttk.Spinbox(row, from_=0, to=10000, width=8, textvariable=var).pack(side=tk.LEFT)
            self.wpn_vars[idx] = var

        # Right side: Spells
        spells_frame = ttk.LabelFrame(main_frame, text="Spell Proficiencies (spl)")
        spells_frame.pack(side=tk.LEFT, fill=tk.BOTH, padx=4, pady=4)

        self.spl_vars = {}
        for idx, name in SPELL_NAMES.items():
            row = ttk.Frame(spells_frame)
            row.pack(fill=tk.X, padx=4, pady=1)
            ttk.Label(row, text=f"{name} ({idx}):", width=18).pack(side=tk.LEFT)
            var = tk.IntVar()
            ttk.Spinbox(row, from_=0, to=10000, width=8, textvariable=var).pack(side=tk.LEFT)
            self.spl_vars[idx] = var

        # Stances section (below main_frame)
        stances_frame = ttk.LabelFrame(scrollable, text="Stance Proficiencies (stance) - Required for Mastery")
        stances_frame.pack(fill=tk.X, padx=8, pady=4)

        self.stance_vars = {}

        # Regular stances in a grid (2 rows of 5)
        stance_grid = ttk.Frame(stances_frame)
        stance_grid.pack(fill=tk.X, padx=4, pady=4)

        col = 0
        row_idx = 0
        for idx, name in STANCE_NAMES.items():
            cell = ttk.Frame(stance_grid)
            cell.grid(row=row_idx, column=col, padx=4, pady=2, sticky='w')
            ttk.Label(cell, text=f"{name} ({idx}):").pack(side=tk.LEFT)
            var = tk.IntVar()
            ttk.Spinbox(cell, from_=0, to=10000, width=6, textvariable=var).pack(side=tk.LEFT, padx=(2, 0))
            self.stance_vars[idx] = var
            col += 1
            if col >= 5:
                col = 0
                row_idx += 1

        # Super stances section
        super_stances_frame = ttk.LabelFrame(scrollable, text="Super Stances (optional)")
        super_stances_frame.pack(fill=tk.X, padx=8, pady=4)

        ss_grid = ttk.Frame(super_stances_frame)
        ss_grid.pack(fill=tk.X, padx=4, pady=4)

        col = 0
        for idx, name in SUPER_STANCE_NAMES.items():
            cell = ttk.Frame(ss_grid)
            cell.grid(row=0, column=col, padx=4, pady=2, sticky='w')
            ttk.Label(cell, text=f"{name} ({idx}):").pack(side=tk.LEFT)
            var = tk.IntVar()
            ttk.Spinbox(cell, from_=0, to=10000, width=6, textvariable=var).pack(side=tk.LEFT, padx=(2, 0))
            self.stance_vars[idx] = var
            col += 1

        # Buttons
        btn_frame = ttk.Frame(scrollable)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)

        ttk.Button(btn_frame, text="Set All to 200 (Mastery Ready)",
                   command=self._set_all_to_mastery).pack(side=tk.LEFT, padx=(0, 8))
        ttk.Button(btn_frame, text="Save Mastery Values",
                   command=self._save_mastery).pack(side=tk.LEFT)

    def _build_powers_tab(self):
        """Build the powers/disciplines tab."""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Powers")

        # Use canvas for scrolling
        canvas = tk.Canvas(frame)
        scrollbar = ttk.Scrollbar(frame, orient=tk.VERTICAL, command=canvas.yview)
        scrollable = ttk.Frame(canvas)

        scrollable.bind('<Configure>', lambda e: canvas.configure(scrollregion=canvas.bbox('all')))
        canvas.create_window((0, 0), window=scrollable, anchor=tk.NW)
        canvas.configure(yscrollcommand=scrollbar.set)

        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # Info banner
        info = ttk.Label(scrollable,
            text="Powers/Disciplines array (power[44]) - Class-specific abilities",
            foreground='blue')
        info.pack(fill=tk.X, padx=8, pady=4)

        # Organize disciplines by class type
        self.power_vars = {}

        # Vampire disciplines (2-16, 39-43)
        vamp_frame = ttk.LabelFrame(scrollable, text="Vampire Disciplines")
        vamp_frame.pack(fill=tk.X, padx=8, pady=4)

        vamp_indices = [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 39, 40, 41, 42, 43]
        self._build_power_grid(vamp_frame, vamp_indices)

        # Werewolf totems (18-29)
        were_frame = ttk.LabelFrame(scrollable, text="Werewolf Totems")
        were_frame.pack(fill=tk.X, padx=8, pady=4)

        were_indices = [18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29]
        self._build_power_grid(were_frame, were_indices)

        # Demon disciplines (30-38)
        demon_frame = ttk.LabelFrame(scrollable, text="Demon Disciplines")
        demon_frame.pack(fill=tk.X, padx=8, pady=4)

        demon_indices = [30, 31, 32, 33, 34, 35, 36, 37, 38]
        self._build_power_grid(demon_frame, demon_indices)

        # Other indices (0, 1, 17 - if used)
        other_frame = ttk.LabelFrame(scrollable, text="Other Powers")
        other_frame.pack(fill=tk.X, padx=8, pady=4)

        other_indices = [0, 1, 17]
        self._build_power_grid(other_frame, other_indices)

        # Save button for powers
        btn_frame = ttk.Frame(scrollable)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save Powers", command=self._save_powers).pack(side=tk.LEFT)

    def _build_power_grid(self, parent, indices):
        """Build a grid of power spinboxes."""
        grid = ttk.Frame(parent)
        grid.pack(fill=tk.X, padx=4, pady=4)

        col = 0
        row_idx = 0
        for idx in indices:
            if idx in DISCIPLINE_NAMES:
                const_name, display_name = DISCIPLINE_NAMES[idx]
            else:
                const_name = f'POWER_{idx}'
                display_name = f'Power {idx}'

            cell = ttk.Frame(grid)
            cell.grid(row=row_idx, column=col, padx=4, pady=2, sticky='w')
            ttk.Label(cell, text=f"{display_name} ({idx}):").pack(side=tk.LEFT)
            var = tk.IntVar()
            ttk.Spinbox(cell, from_=0, to=10000, width=6, textvariable=var).pack(side=tk.LEFT, padx=(2, 0))
            self.power_vars[idx] = var

            col += 1
            if col >= 4:
                col = 0
                row_idx += 1

    def _build_class_tab(self):
        """Build the class-specific tab with fields relevant to the player's class."""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Class Data")

        # Use canvas for scrolling
        canvas = tk.Canvas(frame)
        scrollbar = ttk.Scrollbar(frame, orient=tk.VERTICAL, command=canvas.yview)
        self.class_scrollable = ttk.Frame(canvas)

        self.class_scrollable.bind('<Configure>', lambda e: canvas.configure(scrollregion=canvas.bbox('all')))
        canvas.create_window((0, 0), window=self.class_scrollable, anchor=tk.NW)
        canvas.configure(yscrollcommand=scrollbar.set)

        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # Info banner
        self.class_info_label = ttk.Label(self.class_scrollable,
            text="Class-specific data. Content changes based on player's class.",
            foreground='blue')
        self.class_info_label.pack(fill=tk.X, padx=8, pady=4)

        # Class display
        class_display = ttk.LabelFrame(self.class_scrollable, text="Current Class")
        class_display.pack(fill=tk.X, padx=8, pady=4)

        row = ttk.Frame(class_display)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Class:").pack(side=tk.LEFT)
        self.class_display_var = tk.StringVar()
        ttk.Label(row, textvariable=self.class_display_var, font=('Consolas', 10, 'bold')).pack(side=tk.LEFT, padx=8)

        # Werewolf Gifts section
        self.gifts_frame = ttk.LabelFrame(self.class_scrollable, text="Werewolf Gifts")
        self.gifts_frame.pack(fill=tk.X, padx=8, pady=4)

        self.gift_vars = {}
        gifts_grid = ttk.Frame(self.gifts_frame)
        gifts_grid.pack(fill=tk.X, padx=4, pady=4)

        col = 0
        row_idx = 0
        for idx, name in GIFT_NAMES.items():
            cell = ttk.Frame(gifts_grid)
            cell.grid(row=row_idx, column=col, padx=4, pady=2, sticky='w')
            ttk.Label(cell, text=f"{name} ({idx}):").pack(side=tk.LEFT)
            var = tk.IntVar()
            ttk.Spinbox(cell, from_=0, to=10000, width=6, textvariable=var).pack(side=tk.LEFT, padx=(2, 0))
            self.gift_vars[idx] = var

            col += 1
            if col >= 4:
                col = 0
                row_idx += 1

        # Werewolf-specific fields
        self.were_frame = ttk.LabelFrame(self.class_scrollable, text="Werewolf Attributes")
        self.were_frame.pack(fill=tk.X, padx=8, pady=4)

        row = ttk.Frame(self.were_frame)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Gnosis (0):", width=12).pack(side=tk.LEFT)
        self.gnosis0_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=10000, width=8, textvariable=self.gnosis0_var).pack(side=tk.LEFT, padx=(0, 16))
        ttk.Label(row, text="Gnosis (1):", width=12).pack(side=tk.LEFT)
        self.gnosis1_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=10000, width=8, textvariable=self.gnosis1_var).pack(side=tk.LEFT)

        row = ttk.Frame(self.were_frame)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Rage:", width=12).pack(side=tk.LEFT)
        self.rage_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=1000, width=8, textvariable=self.rage_var).pack(side=tk.LEFT)

        # Vampire-specific fields
        self.vamp_frame = ttk.LabelFrame(self.class_scrollable, text="Vampire Attributes")
        self.vamp_frame.pack(fill=tk.X, padx=8, pady=4)

        row = ttk.Frame(self.vamp_frame)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Generation:", width=12).pack(side=tk.LEFT)
        self.generation_var = tk.IntVar()
        ttk.Spinbox(row, from_=1, to=15, width=6, textvariable=self.generation_var).pack(side=tk.LEFT, padx=(0, 16))
        ttk.Label(row, text="Blood:", width=12).pack(side=tk.LEFT)
        self.blood_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=100000, width=8, textvariable=self.blood_var).pack(side=tk.LEFT)

        # Demon-specific fields
        self.demon_frame = ttk.LabelFrame(self.class_scrollable, text="Demon Attributes")
        self.demon_frame.pack(fill=tk.X, padx=8, pady=4)

        row = ttk.Frame(self.demon_frame)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Corruption:", width=12).pack(side=tk.LEFT)
        self.corruption_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=10000, width=8, textvariable=self.corruption_var).pack(side=tk.LEFT)

        # Dragonkin-specific fields
        self.dragon_frame = ttk.LabelFrame(self.class_scrollable, text="Dragonkin Attributes")
        self.dragon_frame.pack(fill=tk.X, padx=8, pady=4)

        row = ttk.Frame(self.dragon_frame)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Attunement:", width=12).pack(side=tk.LEFT)
        self.attunement_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=10000, width=8, textvariable=self.attunement_var).pack(side=tk.LEFT, padx=(0, 16))
        ttk.Label(row, text="Essence:", width=12).pack(side=tk.LEFT)
        self.essence_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=10000, width=8, textvariable=self.essence_var).pack(side=tk.LEFT)

        # Mage-specific fields
        self.mage_frame = ttk.LabelFrame(self.class_scrollable, text="Mage Attributes")
        self.mage_frame.pack(fill=tk.X, padx=8, pady=4)

        row = ttk.Frame(self.mage_frame)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Arete:", width=12).pack(side=tk.LEFT)
        self.arete_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=10, width=6, textvariable=self.arete_var).pack(side=tk.LEFT, padx=(0, 16))
        ttk.Label(row, text="Quintessence:", width=12).pack(side=tk.LEFT)
        self.quintessence_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=10000, width=8, textvariable=self.quintessence_var).pack(side=tk.LEFT)

        # Generic class data (stored as class-specific integer fields)
        self.generic_frame = ttk.LabelFrame(self.class_scrollable, text="Other Class Fields")
        self.generic_frame.pack(fill=tk.X, padx=8, pady=4)

        row = ttk.Frame(self.generic_frame)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="OrigClass:", width=12).pack(side=tk.LEFT)
        self.origclass_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=10, textvariable=self.origclass_var).pack(side=tk.LEFT, padx=(0, 16))
        ttk.Label(row, text="Tier:", width=12).pack(side=tk.LEFT)
        self.tier_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=10, width=6, textvariable=self.tier_var).pack(side=tk.LEFT)

        # Save button
        btn_frame = ttk.Frame(self.class_scrollable)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save Class Data", command=self._save_class_data).pack(side=tk.LEFT)
        ttk.Button(btn_frame, text="Refresh View", command=self._refresh_class_tab).pack(side=tk.LEFT, padx=8)

    def _refresh_class_tab(self):
        """Refresh the class tab to show/hide relevant sections."""
        # Get current class
        try:
            class_val = int(self.class_var.get().split(' - ')[0])
        except (ValueError, IndexError):
            class_val = 0

        class_name = CLASS_TABLE.get(class_val, 'Unknown')
        self.class_display_var.set(f"{class_val} - {class_name}")

        # Show/hide relevant frames based on class
        # Werewolf = 4
        if class_val == 4:
            self.gifts_frame.pack(fill=tk.X, padx=8, pady=4)
            self.were_frame.pack(fill=tk.X, padx=8, pady=4)
        else:
            self.gifts_frame.pack_forget()
            self.were_frame.pack_forget()

        # Vampire = 8
        if class_val == 8:
            self.vamp_frame.pack(fill=tk.X, padx=8, pady=4)
        else:
            self.vamp_frame.pack_forget()

        # Demon = 1
        if class_val == 1:
            self.demon_frame.pack(fill=tk.X, padx=8, pady=4)
        else:
            self.demon_frame.pack_forget()

        # Mage = 2
        if class_val == 2:
            self.mage_frame.pack(fill=tk.X, padx=8, pady=4)
        else:
            self.mage_frame.pack_forget()

        # Generic always visible
        self.generic_frame.pack(fill=tk.X, padx=8, pady=4)

    def _load_class_data(self):
        """Load class-specific data from the database."""
        player = self.repository.get_player()
        arrays = self.repository.get_player_arrays()

        if not player:
            return

        # Update class display
        class_val = player.get('class', 0)
        class_name = CLASS_TABLE.get(class_val, 'Unknown')
        self.class_display_var.set(f"{class_val} - {class_name}")

        # Load gifts array (21 elements)
        gifts = arrays.get('gifts', [0] * 21)
        for idx in range(21):
            if idx in self.gift_vars and idx < len(gifts):
                self.gift_vars[idx].set(gifts[idx])

        # Load gnosis array (2 elements)
        gnosis = arrays.get('gnosis', [0, 0])
        if len(gnosis) >= 2:
            self.gnosis0_var.set(gnosis[0])
            self.gnosis1_var.set(gnosis[1])

        # Load werewolf rage
        self.rage_var.set(player.get('rage', 0))

        # Load vampire fields
        self.generation_var.set(player.get('generation', 13))
        # Blood is stored in condition array (index 2 = COND_THIRST)
        condition = arrays.get('condition', [0, 0, 0])
        if len(condition) >= 3:
            self.blood_var.set(condition[2])
        else:
            self.blood_var.set(0)

        # Note: These fields are displayed but not yet in the database schema
        # They will show 0 until schema is updated
        self.corruption_var.set(0)
        self.arete_var.set(0)
        self.quintessence_var.set(0)
        self.attunement_var.set(0)
        self.essence_var.set(0)
        self.origclass_var.set(0)
        self.tier_var.set(0)

        # Refresh visibility
        self._refresh_class_tab()

    def _save_class_data(self):
        """Save class-specific data to the database."""
        arrays = self.repository.get_player_arrays()

        # Save gifts array
        gifts = arrays.get('gifts', [0] * 21)
        while len(gifts) < 21:
            gifts.append(0)
        for idx, var in self.gift_vars.items():
            if idx < len(gifts):
                gifts[idx] = var.get()
        self.repository.update_player_array('gifts', gifts)

        # Save gnosis array
        gnosis = [self.gnosis0_var.get(), self.gnosis1_var.get()]
        self.repository.update_player_array('gnosis', gnosis)

        # Save blood via condition array (index 2 = COND_THIRST)
        condition = arrays.get('condition', [0, 0, 0])
        while len(condition) < 3:
            condition.append(0)
        condition[2] = self.blood_var.get()  # COND_THIRST
        self.repository.update_player_array('condition', condition)

        # Save player fields (only columns that exist in schema)
        data = {
            'rage': self.rage_var.get(),
            'generation': self.generation_var.get(),
        }
        self.repository.update_player(data)
        self.on_status("Class data saved")

    def _build_flags_tab(self):
        """Build the flags tab with checkboxes for all flag types."""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Flags")

        # Use canvas for scrolling
        canvas = tk.Canvas(frame)
        scrollbar = ttk.Scrollbar(frame, orient=tk.VERTICAL, command=canvas.yview)
        scrollable = ttk.Frame(canvas)

        scrollable.bind('<Configure>', lambda e: canvas.configure(scrollregion=canvas.bbox('all')))
        canvas.create_window((0, 0), window=scrollable, anchor=tk.NW)
        canvas.configure(yscrollcommand=scrollbar.set)

        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # Store all flag variables
        self.flag_vars = {
            'act': {},      # PLR_* flags
            'extra': {},    # EXTRA_* flags
            'newbits': {},  # NEW_* flags
            'immune': {},   # IMM_* flags
            'affected_by': {},  # AFF_* flags
        }

        # PLR flags (stored in 'act' field for players)
        plr_frame = ttk.LabelFrame(scrollable, text="Player Flags (act)")
        plr_frame.pack(fill=tk.X, padx=8, pady=4)
        self._build_flag_checkboxes(plr_frame, PLR_FLAGS, 'act')

        # Extra flags
        extra_frame = ttk.LabelFrame(scrollable, text="Extra Flags (extra)")
        extra_frame.pack(fill=tk.X, padx=8, pady=4)
        self._build_flag_checkboxes(extra_frame, EXTRA_FLAGS_PLAYER, 'extra')

        # Newbits flags
        newbits_frame = ttk.LabelFrame(scrollable, text="Newbits Flags (newbits)")
        newbits_frame.pack(fill=tk.X, padx=8, pady=4)
        self._build_flag_checkboxes(newbits_frame, NEWBITS_FLAGS, 'newbits')

        # Immunity flags
        imm_frame = ttk.LabelFrame(scrollable, text="Immunity Flags (immune)")
        imm_frame.pack(fill=tk.X, padx=8, pady=4)
        self._build_flag_checkboxes(imm_frame, IMM_FLAGS, 'immune')

        # Affected_by flags
        aff_frame = ttk.LabelFrame(scrollable, text="Affected By Flags (affected_by)")
        aff_frame.pack(fill=tk.X, padx=8, pady=4)
        self._build_flag_checkboxes(aff_frame, AFF_FLAGS, 'affected_by')

        # Save button
        btn_frame = ttk.Frame(scrollable)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save Flags", command=self._save_flags).pack(side=tk.LEFT)

    def _build_flag_checkboxes(self, parent, flag_dict, field_name):
        """Build checkboxes for a set of flags."""
        grid = ttk.Frame(parent)
        grid.pack(fill=tk.X, padx=4, pady=4)

        col = 0
        row_idx = 0
        for bit_value, flag_info in sorted(flag_dict.items()):
            # Handle both tuple format (const, display) and simple string format
            if isinstance(flag_info, tuple):
                const_name, display_name = flag_info
            else:
                display_name = flag_info

            var = tk.BooleanVar()
            cb = ttk.Checkbutton(grid, text=display_name, variable=var)
            cb.grid(row=row_idx, column=col, padx=4, pady=1, sticky='w')
            self.flag_vars[field_name][bit_value] = var

            col += 1
            if col >= 4:
                col = 0
                row_idx += 1

    def _build_messages_tab(self):
        """Build the custom messages tab."""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Messages")

        # Use canvas for scrolling
        canvas = tk.Canvas(frame)
        scrollbar = ttk.Scrollbar(frame, orient=tk.VERTICAL, command=canvas.yview)
        scrollable = ttk.Frame(canvas)

        scrollable.bind('<Configure>', lambda e: canvas.configure(scrollregion=canvas.bbox('all')))
        canvas.create_window((0, 0), window=scrollable, anchor=tk.NW)
        canvas.configure(yscrollcommand=scrollbar.set)

        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # Info
        ttk.Label(scrollable,
            text="Custom messages for player actions. Leave blank to use defaults.",
            foreground='blue').pack(fill=tk.X, padx=8, pady=4)

        # Message fields
        self.message_vars = {}

        messages = [
            ('bamfin', 'Bamf In', 'Message shown when immortal teleports in'),
            ('bamfout', 'Bamf Out', 'Message shown when immortal teleports out'),
            ('loginmessage', 'Login', 'Custom login message'),
            ('logoutmessage', 'Logout', 'Custom logout message'),
            ('avatarmessage', 'Avatar', 'Custom avatar message'),
            ('decapmessage', 'Decap', 'Custom decapitation message (use $n and $N)'),
            ('tiemessage', 'Tie', 'Custom tie message (use $n and $N)'),
        ]

        for field_name, label, tooltip in messages:
            msg_frame = ttk.LabelFrame(scrollable, text=label)
            msg_frame.pack(fill=tk.X, padx=8, pady=4)

            ttk.Label(msg_frame, text=tooltip, foreground='gray').pack(fill=tk.X, padx=4, pady=2)

            var = tk.StringVar()
            entry = ttk.Entry(msg_frame, textvariable=var, width=60)
            entry.pack(fill=tk.X, padx=4, pady=4)
            self.message_vars[field_name] = var

        # Description fields (multi-line)
        desc_frame = ttk.LabelFrame(scrollable, text="Descriptions")
        desc_frame.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)

        # Short description
        row = ttk.Frame(desc_frame)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Short Desc:", width=12).pack(side=tk.LEFT)
        self.short_descr_var = tk.StringVar()
        ttk.Entry(row, textvariable=self.short_descr_var, width=50).pack(side=tk.LEFT, fill=tk.X, expand=True)

        # Long description
        row = ttk.Frame(desc_frame)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Long Desc:", width=12).pack(side=tk.LEFT)
        self.long_descr_var = tk.StringVar()
        ttk.Entry(row, textvariable=self.long_descr_var, width=50).pack(side=tk.LEFT, fill=tk.X, expand=True)

        # Full description (multi-line)
        ttk.Label(desc_frame, text="Full Description (shown when looked at):").pack(fill=tk.X, padx=4, pady=2)
        self.description_text = tk.Text(desc_frame, height=6, width=60)
        self.description_text.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Save button
        btn_frame = ttk.Frame(scrollable)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save Messages", command=self._save_messages).pack(side=tk.LEFT)

    def _build_skills_tab(self):
        """Build the skills tab."""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Skills")

        # Search
        search_frame = ttk.Frame(frame)
        search_frame.pack(fill=tk.X, padx=4, pady=4)
        ttk.Label(search_frame, text="Filter:").pack(side=tk.LEFT)
        self.skill_search_var = tk.StringVar()
        self.skill_search_var.trace_add('write', lambda *_: self._filter_skills())
        ttk.Entry(search_frame, textvariable=self.skill_search_var).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=4)

        # Skills list
        tree_frame = ttk.Frame(frame)
        tree_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        self.skills_tree = ttk.Treeview(
            tree_frame,
            columns=('skill', 'value'),
            show='headings',
            selectmode='browse'
        )
        self.skills_tree.heading('skill', text='Skill')
        self.skills_tree.heading('value', text='Value')
        self.skills_tree.column('skill', width=200)
        self.skills_tree.column('value', width=80)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.skills_tree.yview)
        self.skills_tree.configure(yscrollcommand=scrollbar.set)
        self.skills_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.skills_tree.bind('<Double-1>', self._edit_skill)

        ttk.Label(frame, text="Double-click a skill to edit its value", foreground='gray').pack(padx=4, pady=2)

        self._all_skills = []

    def _build_aliases_tab(self):
        """Build the aliases tab."""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Aliases")

        # Alias list
        tree_frame = ttk.Frame(frame)
        tree_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        self.aliases_tree = ttk.Treeview(
            tree_frame,
            columns=('short', 'long'),
            show='headings',
            selectmode='browse'
        )
        self.aliases_tree.heading('short', text='Alias')
        self.aliases_tree.heading('long', text='Expands To')
        self.aliases_tree.column('short', width=100)
        self.aliases_tree.column('long', width=300)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.aliases_tree.yview)
        self.aliases_tree.configure(yscrollcommand=scrollbar.set)
        self.aliases_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # Buttons
        btn_frame = ttk.Frame(frame)
        btn_frame.pack(fill=tk.X, padx=4, pady=4)
        ttk.Button(btn_frame, text="Add Alias", command=self._add_alias).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete Selected", command=self._delete_alias).pack(side=tk.LEFT)

    def _build_affects_tab(self):
        """Build the affects tab."""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Affects")

        tree_frame = ttk.Frame(frame)
        tree_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        self.affects_tree = ttk.Treeview(
            tree_frame,
            columns=('skill', 'duration', 'modifier', 'location'),
            show='headings',
            selectmode='browse'
        )
        self.affects_tree.heading('skill', text='Effect')
        self.affects_tree.heading('duration', text='Duration')
        self.affects_tree.heading('modifier', text='Modifier')
        self.affects_tree.heading('location', text='Location')
        self.affects_tree.column('skill', width=150)
        self.affects_tree.column('duration', width=80)
        self.affects_tree.column('modifier', width=80)
        self.affects_tree.column('location', width=80)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.affects_tree.yview)
        self.affects_tree.configure(yscrollcommand=scrollbar.set)
        self.affects_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        btn_frame = ttk.Frame(frame)
        btn_frame.pack(fill=tk.X, padx=4, pady=4)
        ttk.Button(btn_frame, text="Remove Selected", command=self._delete_affect).pack(side=tk.LEFT)
        ttk.Label(btn_frame, text="(Affects are normally managed by game spells)", foreground='gray').pack(side=tk.LEFT, padx=8)

    def _build_inventory_tab(self):
        """Build the inventory tab."""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Inventory")

        tree_frame = ttk.Frame(frame)
        tree_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        self.inventory_tree = ttk.Treeview(
            tree_frame,
            columns=('id', 'vnum', 'name', 'location', 'type'),
            show='headings',
            selectmode='browse'
        )
        self.inventory_tree.heading('id', text='ID')
        self.inventory_tree.heading('vnum', text='Vnum')
        self.inventory_tree.heading('name', text='Name')
        self.inventory_tree.heading('location', text='Wear Loc')
        self.inventory_tree.heading('type', text='Type')
        self.inventory_tree.column('id', width=50)
        self.inventory_tree.column('vnum', width=60)
        self.inventory_tree.column('name', width=250)
        self.inventory_tree.column('location', width=80)
        self.inventory_tree.column('type', width=80)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.inventory_tree.yview)
        self.inventory_tree.configure(yscrollcommand=scrollbar.set)
        self.inventory_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        ttk.Label(frame, text="Inventory is read-only. Edit objects through Area editors.",
                 foreground='gray').pack(padx=4, pady=4)

    def _build_password_tab(self):
        """Build the password tab (write-only)."""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Password")

        warning = ttk.Label(
            frame,
            text="WARNING: Password changes take effect immediately.\n"
                 "The password is hashed and stored securely.\n"
                 "For security, passwords cannot be read - only set.",
            foreground='red',
            justify=tk.LEFT
        )
        warning.pack(padx=20, pady=20, anchor=tk.W)

        pw_frame = ttk.LabelFrame(frame, text="Set New Password")
        pw_frame.pack(fill=tk.X, padx=20, pady=10)

        row = ttk.Frame(pw_frame)
        row.pack(fill=tk.X, padx=8, pady=8)
        ttk.Label(row, text="New Password:", width=15).pack(side=tk.LEFT)
        self.new_password_var = tk.StringVar()
        self.pw_entry1 = ttk.Entry(row, textvariable=self.new_password_var, show='*', width=25)
        self.pw_entry1.pack(side=tk.LEFT)

        row = ttk.Frame(pw_frame)
        row.pack(fill=tk.X, padx=8, pady=8)
        ttk.Label(row, text="Confirm Password:", width=15).pack(side=tk.LEFT)
        self.confirm_password_var = tk.StringVar()
        self.pw_entry2 = ttk.Entry(row, textvariable=self.confirm_password_var, show='*', width=25)
        self.pw_entry2.pack(side=tk.LEFT)

        btn_frame = ttk.Frame(pw_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Set Password", command=self._set_password).pack(side=tk.LEFT)

    def _load_player(self):
        """Load player data from database."""
        player = self.repository.get_player()
        if not player:
            self.on_status("No player data found")
            return

        # Load identity
        self.name_var.set(player.get('name', ''))
        self.title_var.set(player.get('title', ''))
        self.sex_var.set(self.SEXES.get(player.get('sex', 0), 'Neutral'))
        self.level_var.set(player.get('level', 1))
        self.trust_var.set(player.get('trust', 0))

        # Class
        class_id = player.get('class', 0)
        if class_id in CLASS_TABLE:
            self.class_var.set(f"{class_id} - {CLASS_TABLE[class_id]}")
        else:
            self.class_var.set(f"{class_id} - Unknown")

        # Vitals
        self.hit_var.set(player.get('hit', 0))
        self.max_hit_var.set(player.get('max_hit', 0))
        self.mana_var.set(player.get('mana', 0))
        self.max_mana_var.set(player.get('max_mana', 0))
        self.move_var.set(player.get('move', 0))
        self.max_move_var.set(player.get('max_move', 0))

        # Economy
        self.gold_var.set(player.get('gold', 0))
        self.exp_var.set(player.get('exp', 0))
        self.quest_var.set(player.get('quest', 0))
        self.practice_var.set(player.get('practice', 0))

        # PvP stats
        self.pkill_var.set(player.get('pkill', 0))
        self.pdeath_var.set(player.get('pdeath', 0))
        self.mkill_var.set(player.get('mkill', 0))
        self.mdeath_var.set(player.get('mdeath', 0))
        self.awins_var.set(player.get('awins', 0))
        self.alosses_var.set(player.get('alosses', 0))

        # Location
        self.room_vnum_var.set(player.get('room_vnum', 3001))
        self.home_var.set(player.get('home', 3001))

        # Combat stats
        self.hitroll_var.set(player.get('hitroll', 0))
        self.damroll_var.set(player.get('damroll', 0))
        self.armor_var.set(player.get('armor', 0))
        self.saving_throw_var.set(player.get('saving_throw', 0))
        self.alignment_var.set(player.get('alignment', 0))

        # Load attributes from player_arrays
        arrays = self.repository.get_player_arrays()
        attr_perm = arrays.get('attr_perm', [0, 0, 0, 0, 0])
        for i, var in self.attr_vars.items():
            if i < len(attr_perm):
                var.set(attr_perm[i])

        # Load mastery data (wpn, spl, stance arrays)
        self._load_mastery_data()

        # Load powers, flags, messages, and class data
        self._load_powers()
        self._load_flags()
        self._load_messages()
        self._load_class_data()

        # Load skills
        self._load_skills()

        # Load aliases
        self._load_aliases()

        # Load affects
        self._load_affects()

        # Load inventory
        self._load_inventory()

        self.unsaved = False
        self.on_status(f"Loaded player: {self.player_name}")

    def _load_skills(self):
        """Load skills into the tree."""
        self._all_skills = self.repository.get_skills()
        self._filter_skills()

    def _filter_skills(self):
        """Filter skills based on search."""
        term = self.skill_search_var.get().lower()
        self.skills_tree.delete(*self.skills_tree.get_children())

        for skill in self._all_skills:
            if not term or term in skill['skill_name'].lower():
                self.skills_tree.insert('', tk.END, iid=skill['skill_name'],
                                       values=(skill['skill_name'], skill['value']))

    def _edit_skill(self, event):
        """Edit a skill value."""
        selection = self.skills_tree.selection()
        if not selection:
            return

        skill_name = selection[0]
        current = self.skills_tree.item(skill_name)['values'][1]

        value = simpledialog.askinteger(
            "Edit Skill",
            f"Enter new value for {skill_name}:",
            parent=self,
            initialvalue=current,
            minvalue=0,
            maxvalue=100
        )
        if value is not None:
            self.repository.update_skill(skill_name, value)
            self._load_skills()
            self.on_status(f"Updated skill: {skill_name} = {value}")

    def _load_mastery_data(self):
        """Load mastery-related arrays from the database."""
        arrays = self.repository.get_player_arrays()

        # Load weapon proficiencies (wpn array has 13 elements)
        wpn = arrays.get('wpn', [0] * 13)
        for idx in range(13):
            if idx in self.wpn_vars and idx < len(wpn):
                self.wpn_vars[idx].set(wpn[idx])

        # Load spell proficiencies (spl array has 5 elements)
        spl = arrays.get('spl', [0] * 5)
        for idx in range(5):
            if idx in self.spl_vars and idx < len(spl):
                self.spl_vars[idx].set(spl[idx])

        # Load stance proficiencies (stance array has 24 elements)
        stance = arrays.get('stance', [0] * 24)
        for idx in self.stance_vars:
            if idx < len(stance):
                self.stance_vars[idx].set(stance[idx])

    def _save_mastery(self):
        """Save mastery values to the database."""
        arrays = self.repository.get_player_arrays()

        # Build wpn array (13 elements)
        wpn = arrays.get('wpn', [0] * 13)
        for idx in range(13):
            if idx in self.wpn_vars:
                wpn[idx] = self.wpn_vars[idx].get()
        self.repository.update_player_array('wpn', wpn)

        # Build spl array (5 elements)
        spl = arrays.get('spl', [0] * 5)
        for idx in range(5):
            if idx in self.spl_vars:
                spl[idx] = self.spl_vars[idx].get()
        self.repository.update_player_array('spl', spl)

        # Build stance array (24 elements, preserve unedited indices)
        stance = arrays.get('stance', [0] * 24)
        for idx in self.stance_vars:
            stance[idx] = self.stance_vars[idx].get()
        self.repository.update_player_array('stance', stance)

        self.on_status("Mastery values saved")

    def _set_all_to_mastery(self):
        """Set all proficiency values to 200 (mastery threshold)."""
        # Set all weapons to 200
        for var in self.wpn_vars.values():
            var.set(MASTERY_THRESHOLD)

        # Set all spells to 200
        for var in self.spl_vars.values():
            var.set(MASTERY_THRESHOLD)

        # Set stances 1-10 to 200 (mastery requirement)
        for idx in range(1, 11):
            if idx in self.stance_vars:
                self.stance_vars[idx].set(MASTERY_THRESHOLD)

        self.on_status("All mastery-required values set to 200")

    def _load_powers(self):
        """Load power array from database."""
        arrays = self.repository.get_player_arrays()
        power = arrays.get('power', [0] * 44)
        for idx, var in self.power_vars.items():
            if idx < len(power):
                var.set(power[idx])

    def _save_powers(self):
        """Save power array to database."""
        arrays = self.repository.get_player_arrays()
        power = arrays.get('power', [0] * 44)

        # Ensure array is large enough
        while len(power) < 44:
            power.append(0)

        for idx, var in self.power_vars.items():
            if idx < len(power):
                power[idx] = var.get()

        self.repository.update_player_array('power', power)
        self.on_status("Powers saved")

    def _load_flags(self):
        """Load all flag values from database."""
        player = self.repository.get_player()
        if not player:
            return

        for field_name, flags in self.flag_vars.items():
            field_value = player.get(field_name, 0) or 0
            for bit_value, var in flags.items():
                var.set(bool(field_value & bit_value))

    def _save_flags(self):
        """Save all flag values to database."""
        data = {}

        for field_name, flags in self.flag_vars.items():
            value = 0
            for bit_value, var in flags.items():
                if var.get():
                    value |= bit_value
            data[field_name] = value

        self.repository.update_player(data)
        self.on_status("Flags saved")

    def _load_messages(self):
        """Load message fields from database."""
        player = self.repository.get_player()
        if not player:
            return

        for field_name, var in self.message_vars.items():
            var.set(player.get(field_name, '') or '')

        self.short_descr_var.set(player.get('short_descr', '') or '')
        self.long_descr_var.set(player.get('long_descr', '') or '')

        desc = player.get('description', '') or ''
        self.description_text.delete('1.0', tk.END)
        self.description_text.insert('1.0', desc)

    def _save_messages(self):
        """Save message fields to database."""
        data = {}
        for field_name, var in self.message_vars.items():
            data[field_name] = var.get()

        data['short_descr'] = self.short_descr_var.get()
        data['long_descr'] = self.long_descr_var.get()
        data['description'] = self.description_text.get('1.0', tk.END).strip()

        self.repository.update_player(data)
        self.on_status("Messages saved")

    def _load_aliases(self):
        """Load aliases into the tree."""
        self.aliases_tree.delete(*self.aliases_tree.get_children())
        for alias in self.repository.get_aliases():
            self.aliases_tree.insert('', tk.END, iid=str(alias['id']),
                                    values=(alias['short_n'], alias['long_n']))

    def _add_alias(self):
        """Add a new alias."""
        short = simpledialog.askstring("New Alias", "Enter alias:", parent=self)
        if not short:
            return
        long = simpledialog.askstring("New Alias", f"'{short}' expands to:", parent=self)
        if not long:
            return

        self.repository.add_alias(short.strip(), long.strip())
        self._load_aliases()
        self.on_status(f"Added alias: {short}")

    def _delete_alias(self):
        """Delete selected alias."""
        selection = self.aliases_tree.selection()
        if not selection:
            return

        if messagebox.askyesno("Confirm", "Delete this alias?"):
            self.repository.delete_alias(int(selection[0]))
            self._load_aliases()
            self.on_status("Alias deleted")

    def _load_affects(self):
        """Load affects into the tree."""
        self.affects_tree.delete(*self.affects_tree.get_children())
        for aff in self.repository.get_affects():
            self.affects_tree.insert('', tk.END, iid=str(aff['id']),
                                    values=(aff['skill_name'], aff['duration'],
                                           aff['modifier'], aff['location']))

    def _delete_affect(self):
        """Delete selected affect."""
        selection = self.affects_tree.selection()
        if not selection:
            return

        if messagebox.askyesno("Confirm", "Remove this affect?"):
            self.repository.delete_affect(int(selection[0]))
            self._load_affects()
            self.on_status("Affect removed")

    def _load_inventory(self):
        """Load inventory into the tree."""
        self.inventory_tree.delete(*self.inventory_tree.get_children())
        for obj in self.repository.get_objects():
            indent = "  " * obj.get('nest', 0)
            name = indent + (obj.get('short_descr', '') or obj.get('name', 'Unknown'))
            self.inventory_tree.insert('', tk.END, iid=str(obj['id']),
                                      values=(obj['id'], obj['vnum'], name,
                                             obj['wear_loc'], obj['item_type']))

    def _set_password(self):
        """Set a new password for the player."""
        pw1 = self.new_password_var.get()
        pw2 = self.confirm_password_var.get()

        if not pw1:
            messagebox.showwarning("Warning", "Password cannot be empty.")
            return

        if pw1 != pw2:
            messagebox.showerror("Error", "Passwords do not match.")
            return

        if len(pw1) < 5:
            messagebox.showwarning("Warning", "Password should be at least 5 characters.")
            return

        # Hash the password using crypt (same as the MUD server)
        # Note: On Windows, crypt is not available - use MD5 fallback
        if HAS_CRYPT:
            password_hash = crypt.crypt(pw1, pw1[:2])
        else:
            # Fallback for Windows - the MUD server uses traditional DES crypt
            # but MD5 hash is a reasonable cross-platform alternative
            password_hash = hashlib.md5(pw1.encode()).hexdigest()
            messagebox.showwarning(
                "Warning",
                "Password hashed with MD5 (Windows fallback).\n"
                "This may not match the MUD server's expected format.\n"
                "For full compatibility, set passwords on Linux."
            )

        if messagebox.askyesno("Confirm", f"Set new password for {self.player_name}?"):
            self.repository.set_password(password_hash)
            self.new_password_var.set("")
            self.confirm_password_var.set("")
            self.on_status(f"Password updated for {self.player_name}")
            messagebox.showinfo("Success", "Password has been changed.")

    def _save(self):
        """Save player changes."""
        # Get sex value
        sex_val = 0
        for k, v in self.SEXES.items():
            if v == self.sex_var.get():
                sex_val = k
                break

        # Get class value
        try:
            class_val = int(self.class_var.get().split(' - ')[0])
        except (ValueError, IndexError):
            class_val = 0

        data = {
            'name': self.name_var.get(),
            'title': self.title_var.get(),
            'sex': sex_val,
            'class': class_val,
            'level': self.level_var.get(),
            'trust': self.trust_var.get(),
            'hit': self.hit_var.get(),
            'max_hit': self.max_hit_var.get(),
            'mana': self.mana_var.get(),
            'max_mana': self.max_mana_var.get(),
            'move': self.move_var.get(),
            'max_move': self.max_move_var.get(),
            'gold': self.gold_var.get(),
            'exp': self.exp_var.get(),
            'quest': self.quest_var.get(),
            'practice': self.practice_var.get(),
            'pkill': self.pkill_var.get(),
            'pdeath': self.pdeath_var.get(),
            'mkill': self.mkill_var.get(),
            'mdeath': self.mdeath_var.get(),
            'awins': self.awins_var.get(),
            'alosses': self.alosses_var.get(),
            'room_vnum': self.room_vnum_var.get(),
            'home': self.home_var.get(),
            'hitroll': self.hitroll_var.get(),
            'damroll': self.damroll_var.get(),
            'armor': self.armor_var.get(),
            'saving_throw': self.saving_throw_var.get(),
            'alignment': self.alignment_var.get(),
        }

        self.repository.update_player(data)

        # Update attributes
        attr_perm = [self.attr_vars[i].get() for i in range(5)]
        self.repository.update_player_array('attr_perm', attr_perm)

        # Update mastery arrays (wpn, spl, stance)
        wpn = [self.wpn_vars[i].get() for i in range(13)]
        self.repository.update_player_array('wpn', wpn)

        spl = [self.spl_vars[i].get() for i in range(5)]
        self.repository.update_player_array('spl', spl)

        # Preserve existing stance values, update only the ones we're editing
        arrays = self.repository.get_player_arrays()
        stance = arrays.get('stance', [0] * 24)
        for idx in self.stance_vars:
            stance[idx] = self.stance_vars[idx].get()
        self.repository.update_player_array('stance', stance)

        # Update powers array
        power = arrays.get('power', [0] * 44)
        while len(power) < 44:
            power.append(0)
        for idx, var in self.power_vars.items():
            if idx < len(power):
                power[idx] = var.get()
        self.repository.update_player_array('power', power)

        # Update flags - build the flag values from checkboxes
        flag_data = {}
        for field_name, flags in self.flag_vars.items():
            value = 0
            for bit_value, var in flags.items():
                if var.get():
                    value |= bit_value
            flag_data[field_name] = value
        self.repository.update_player(flag_data)

        # Update messages
        msg_data = {}
        for field_name, var in self.message_vars.items():
            msg_data[field_name] = var.get()
        msg_data['short_descr'] = self.short_descr_var.get()
        msg_data['long_descr'] = self.long_descr_var.get()
        msg_data['description'] = self.description_text.get('1.0', tk.END).strip()
        self.repository.update_player(msg_data)

        # Update class-specific data
        gifts = arrays.get('gifts', [0] * 21)
        while len(gifts) < 21:
            gifts.append(0)
        for idx, var in self.gift_vars.items():
            if idx < len(gifts):
                gifts[idx] = var.get()
        self.repository.update_player_array('gifts', gifts)

        gnosis = [self.gnosis0_var.get(), self.gnosis1_var.get()]
        self.repository.update_player_array('gnosis', gnosis)

        # Save blood via condition array (index 2 = COND_THIRST)
        condition = arrays.get('condition', [0, 0, 0])
        while len(condition) < 3:
            condition.append(0)
        condition[2] = self.blood_var.get()
        self.repository.update_player_array('condition', condition)

        # Save class-specific fields (only those that exist in schema)
        class_data = {
            'rage': self.rage_var.get(),
            'generation': self.generation_var.get(),
        }
        self.repository.update_player(class_data)

        self.unsaved = False
        self.on_status(f"Saved player: {self.player_name}")

    def _delete_player(self):
        """Delete this player after confirmation."""
        if not self.on_delete:
            messagebox.showwarning("Warning", "Delete not available")
            return

        # Require typing player name for safety
        confirm = simpledialog.askstring(
            "Confirm Delete",
            f"This will permanently delete {self.player_name}.\n\n"
            f"Type the player name to confirm:",
            parent=self
        )

        if confirm and confirm.lower() == self.player_name.lower():
            self.on_delete()
        elif confirm is not None:
            messagebox.showerror("Error", "Name did not match. Deletion cancelled.")

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes", "Discard changes?")
        return True
