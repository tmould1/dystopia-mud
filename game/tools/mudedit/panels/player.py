"""
Player editor panel.

Provides interface for viewing and editing player data.
Password is write-only for security.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
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
from mudlib.models import CLASS_TABLE


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
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.player_name = player_name
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

        self.unsaved = False
        self.on_status(f"Saved player: {self.player_name}")

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes", "Discard changes?")
        return True
