"""
Quest editor panel for MUD editor.

Provides a master-detail interface for editing quest definitions, objectives, and prerequisites.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Dict, List, Optional


# Quest categories
QUEST_CATEGORIES = {
    'T': 'Tutorial',
    'M': 'Main Story',
    'C': 'Combat',
    'E': 'Exploration',
    'CL': 'Class',
    'CR': 'Crafting',
    'D': 'Daily',
    'A': 'Archaic',
}

# Quest tiers
QUEST_TIERS = {
    0: 'Tier 0 (Beginner)',
    1: 'Tier 1 (Novice)',
    2: 'Tier 2 (Intermediate)',
    3: 'Tier 3 (Advanced)',
    4: 'Tier 4 (Expert)',
    5: 'Tier 5 (Master)',
}

# Quest flags (bitmask)
QUEST_FLAGS = {
    1: 'Auto-Complete',
    2: 'Repeatable',
    4: 'FTUE Skip',
}

# Objective types
OBJECTIVE_TYPES = [
    'ARENA_WIN',
    'CLASS_POWER',
    'CLASS_TRAIN',
    'COMPLETE_QUEST',
    'EARN_QP',
    'FORGE_ITEM',
    'KILL_MOB',
    'KILL_PLAYER',
    'LEARN_DISCIPLINE',
    'LEARN_STANCE',
    'LEARN_SUPERSTANCE',
    'MASTERY',
    'QUEST_CREATE',
    'QUEST_MODIFY',
    'REACH_GEN',
    'REACH_PKSCORE',
    'REACH_STAT',
    'REACH_UPGRADE',
    'SPELL_SKILL',
    'USE_COMMAND',
    'VISIT_AREA',
    'WEAPON_SKILL',
]


class QuestEditorPanel(ttk.Frame):
    """Master-detail editor panel for quest definitions."""

    def __init__(
        self,
        parent,
        quest_repo,
        objectives_repo,
        prereqs_repo,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.quest_repo = quest_repo
        self.objectives_repo = objectives_repo
        self.prereqs_repo = prereqs_repo
        self.on_status = on_status or (lambda msg: None)

        self.current_id: Optional[str] = None
        self.unsaved = False
        # Cache all quest IDs/names for prerequisites picker
        self._all_quests: List[Dict] = []

        self._build_ui()
        self._load_quest_list()

    def _build_ui(self):
        """Build the panel UI."""
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # === Left side: quest list with filters ===
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        # Filter bar
        filter_frame = ttk.LabelFrame(left_frame, text="Filter")
        filter_frame.pack(fill=tk.X, padx=2, pady=(2, 4))

        row = ttk.Frame(filter_frame)
        row.pack(fill=tk.X, padx=4, pady=2)
        ttk.Label(row, text="Category:").pack(side=tk.LEFT)
        self.filter_cat_var = tk.StringVar(value='All')
        cat_values = ['All'] + [f"{k} - {v}" for k, v in QUEST_CATEGORIES.items()]
        ttk.Combobox(
            row, textvariable=self.filter_cat_var, values=cat_values,
            state='readonly', width=18
        ).pack(side=tk.LEFT, padx=(4, 8))

        ttk.Label(row, text="Tier:").pack(side=tk.LEFT)
        self.filter_tier_var = tk.StringVar(value='All')
        tier_values = ['All'] + [f"{k} - {v}" for k, v in QUEST_TIERS.items()]
        ttk.Combobox(
            row, textvariable=self.filter_tier_var, values=tier_values,
            state='readonly', width=20
        ).pack(side=tk.LEFT, padx=(4, 0))

        row2 = ttk.Frame(filter_frame)
        row2.pack(fill=tk.X, padx=4, pady=(0, 4))
        ttk.Label(row2, text="Search:").pack(side=tk.LEFT)
        self.filter_search_var = tk.StringVar()
        ttk.Entry(row2, textvariable=self.filter_search_var, width=25).pack(side=tk.LEFT, padx=(4, 4))
        ttk.Button(row2, text="Apply", command=self._apply_filter).pack(side=tk.LEFT)
        ttk.Button(row2, text="Clear", command=self._clear_filter).pack(side=tk.LEFT, padx=(4, 0))

        # Quest list tree
        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('id', 'name', 'category', 'tier'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('id', text='ID')
        self.tree.heading('name', text='Name')
        self.tree.heading('category', text='Cat')
        self.tree.heading('tier', text='Tier')
        self.tree.column('id', width=60, stretch=False)
        self.tree.column('name', width=180)
        self.tree.column('category', width=50, stretch=False)
        self.tree.column('tier', width=40, stretch=False)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 quests")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # List buttons
        list_btn_frame = ttk.Frame(left_frame)
        list_btn_frame.pack(fill=tk.X, padx=4, pady=4)
        ttk.Button(list_btn_frame, text="New Quest", command=self._new_quest).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(list_btn_frame, text="Delete Quest", command=self._delete_quest).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(list_btn_frame, text="Duplicate", command=self._duplicate_quest).pack(side=tk.LEFT)

        # === Right side: detail editor (scrollable) ===
        right_outer = ttk.Frame(paned)
        paned.add(right_outer, weight=2)

        # Canvas for scrolling
        canvas = tk.Canvas(right_outer, highlightthickness=0)
        vscroll = ttk.Scrollbar(right_outer, orient=tk.VERTICAL, command=canvas.yview)
        canvas.configure(yscrollcommand=vscroll.set)
        vscroll.pack(side=tk.RIGHT, fill=tk.Y)
        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self.detail_frame = ttk.Frame(canvas)
        self.detail_window = canvas.create_window((0, 0), window=self.detail_frame, anchor='nw')

        def _on_configure(event):
            canvas.configure(scrollregion=canvas.bbox('all'))
        self.detail_frame.bind('<Configure>', _on_configure)

        def _on_canvas_configure(event):
            canvas.itemconfig(self.detail_window, width=event.width)
        canvas.bind('<Configure>', _on_canvas_configure)

        # Mouse wheel scrolling
        def _on_mousewheel(event):
            canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")
        canvas.bind_all('<MouseWheel>', _on_mousewheel)
        self._canvas = canvas

        self._build_detail_form()

    def _build_detail_form(self):
        """Build the detail editor form inside the scrollable frame."""
        frame = self.detail_frame

        # === Basic Info ===
        info_frame = ttk.LabelFrame(frame, text="Quest Info")
        info_frame.pack(fill=tk.X, padx=8, pady=4)

        # ID
        row = ttk.Frame(info_frame)
        row.pack(fill=tk.X, padx=8, pady=3)
        ttk.Label(row, text="ID:", width=14).pack(side=tk.LEFT)
        self.id_var = tk.StringVar()
        self.id_entry = ttk.Entry(row, textvariable=self.id_var, width=12, font=('Consolas', 10))
        self.id_entry.pack(side=tk.LEFT)
        ttk.Label(row, text="(unique text key, e.g. T01, M03)", foreground='gray').pack(side=tk.LEFT, padx=(8, 0))

        # Name
        row = ttk.Frame(info_frame)
        row.pack(fill=tk.X, padx=8, pady=3)
        ttk.Label(row, text="Name:", width=14).pack(side=tk.LEFT)
        self.name_var = tk.StringVar()
        self.name_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.name_var, width=40).pack(side=tk.LEFT, fill=tk.X, expand=True)

        # Description
        row = ttk.Frame(info_frame)
        row.pack(fill=tk.X, padx=8, pady=3)
        ttk.Label(row, text="Description:", width=14).pack(side=tk.LEFT, anchor=tk.N)
        self.desc_text = tk.Text(row, wrap=tk.WORD, height=3, font=('Consolas', 10))
        self.desc_text.pack(side=tk.LEFT, fill=tk.X, expand=True)
        self.desc_text.bind('<KeyRelease>', lambda e: self._mark_unsaved())

        # Category + Tier row
        row = ttk.Frame(info_frame)
        row.pack(fill=tk.X, padx=8, pady=3)
        ttk.Label(row, text="Category:", width=14).pack(side=tk.LEFT)
        self.cat_var = tk.StringVar()
        self.cat_var.trace_add('write', lambda *_: self._mark_unsaved())
        cat_combo = ttk.Combobox(
            row, textvariable=self.cat_var,
            values=[f"{k} - {v}" for k, v in QUEST_CATEGORIES.items()],
            state='readonly', width=18
        )
        cat_combo.pack(side=tk.LEFT, padx=(0, 16))

        ttk.Label(row, text="Tier:").pack(side=tk.LEFT)
        self.tier_var = tk.IntVar()
        self.tier_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Spinbox(row, from_=0, to=5, width=4, textvariable=self.tier_var,
                    command=self._mark_unsaved).pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row, text="Sort:").pack(side=tk.LEFT)
        self.sort_var = tk.IntVar()
        self.sort_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Spinbox(row, from_=0, to=9999, width=6, textvariable=self.sort_var,
                    command=self._mark_unsaved).pack(side=tk.LEFT, padx=(4, 0))

        # === Flags ===
        flags_frame = ttk.LabelFrame(info_frame, text="Flags")
        flags_frame.pack(fill=tk.X, padx=8, pady=4)
        self.flag_vars = {}
        flag_row = ttk.Frame(flags_frame)
        flag_row.pack(fill=tk.X, padx=4, pady=2)
        for bit, label in QUEST_FLAGS.items():
            var = tk.BooleanVar()
            var.trace_add('write', lambda *_: self._mark_unsaved())
            ttk.Checkbutton(flag_row, text=label, variable=var).pack(side=tk.LEFT, padx=(0, 12))
            self.flag_vars[bit] = var

        # === Requirements ===
        req_frame = ttk.LabelFrame(frame, text="Requirements")
        req_frame.pack(fill=tk.X, padx=8, pady=4)

        row = ttk.Frame(req_frame)
        row.pack(fill=tk.X, padx=8, pady=3)
        ttk.Label(row, text="Min Exp Level:", width=14).pack(side=tk.LEFT)
        self.min_exp_var = tk.IntVar()
        self.min_exp_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Spinbox(row, from_=0, to=999, width=6, textvariable=self.min_exp_var,
                    command=self._mark_unsaved).pack(side=tk.LEFT, padx=(0, 16))

        ttk.Label(row, text="Max Exp Level:").pack(side=tk.LEFT)
        self.max_exp_var = tk.IntVar()
        self.max_exp_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Spinbox(row, from_=0, to=999, width=6, textvariable=self.max_exp_var,
                    command=self._mark_unsaved).pack(side=tk.LEFT, padx=(0, 16))

        ttk.Label(row, text="Req Class:").pack(side=tk.LEFT)
        self.req_class_var = tk.IntVar()
        self.req_class_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.req_class_var, width=10).pack(side=tk.LEFT)
        ttk.Label(row, text="(0=any, bitmask)", foreground='gray').pack(side=tk.LEFT, padx=(4, 0))

        # === Rewards ===
        reward_frame = ttk.LabelFrame(frame, text="Rewards")
        reward_frame.pack(fill=tk.X, padx=8, pady=4)

        row = ttk.Frame(reward_frame)
        row.pack(fill=tk.X, padx=8, pady=3)
        ttk.Label(row, text="QP:", width=14).pack(side=tk.LEFT)
        self.qp_var = tk.IntVar()
        self.qp_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Spinbox(row, from_=0, to=99999, width=8, textvariable=self.qp_var,
                    command=self._mark_unsaved).pack(side=tk.LEFT, padx=(0, 12))

        ttk.Label(row, text="EXP:").pack(side=tk.LEFT)
        self.exp_var = tk.IntVar()
        self.exp_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Spinbox(row, from_=0, to=9999999, width=10, textvariable=self.exp_var,
                    command=self._mark_unsaved).pack(side=tk.LEFT, padx=(0, 12))

        ttk.Label(row, text="Primal:").pack(side=tk.LEFT)
        self.primal_var = tk.IntVar()
        self.primal_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Spinbox(row, from_=0, to=99999, width=8, textvariable=self.primal_var,
                    command=self._mark_unsaved).pack(side=tk.LEFT, padx=(0, 12))

        ttk.Label(row, text="Item VNUM:").pack(side=tk.LEFT)
        self.item_vnum_var = tk.IntVar()
        self.item_vnum_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Spinbox(row, from_=0, to=999999, width=8, textvariable=self.item_vnum_var,
                    command=self._mark_unsaved).pack(side=tk.LEFT)

        # === Objectives ===
        obj_frame = ttk.LabelFrame(frame, text="Objectives")
        obj_frame.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)

        obj_tree_frame = ttk.Frame(obj_frame)
        obj_tree_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        self.obj_tree = ttk.Treeview(
            obj_tree_frame,
            columns=('index', 'type', 'target', 'threshold', 'description'),
            show='headings',
            selectmode='browse',
            height=6
        )
        self.obj_tree.heading('index', text='#')
        self.obj_tree.heading('type', text='Type')
        self.obj_tree.heading('target', text='Target')
        self.obj_tree.heading('threshold', text='Threshold')
        self.obj_tree.heading('description', text='Description')
        self.obj_tree.column('index', width=30, stretch=False)
        self.obj_tree.column('type', width=120)
        self.obj_tree.column('target', width=100)
        self.obj_tree.column('threshold', width=60, stretch=False)
        self.obj_tree.column('description', width=200)

        obj_scroll = ttk.Scrollbar(obj_tree_frame, orient=tk.VERTICAL, command=self.obj_tree.yview)
        self.obj_tree.configure(yscrollcommand=obj_scroll.set)
        self.obj_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        obj_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        obj_btn_frame = ttk.Frame(obj_frame)
        obj_btn_frame.pack(fill=tk.X, padx=4, pady=(0, 4))
        ttk.Button(obj_btn_frame, text="Add", command=self._add_objective).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(obj_btn_frame, text="Edit", command=self._edit_objective).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(obj_btn_frame, text="Remove", command=self._remove_objective).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(obj_btn_frame, text="Move Up", command=lambda: self._move_objective(-1)).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(obj_btn_frame, text="Move Down", command=lambda: self._move_objective(1)).pack(side=tk.LEFT)

        self.obj_tree.bind('<Double-1>', lambda e: self._edit_objective())

        # === Prerequisites ===
        prereq_frame = ttk.LabelFrame(frame, text="Prerequisites")
        prereq_frame.pack(fill=tk.X, padx=8, pady=4)

        prereq_inner = ttk.Frame(prereq_frame)
        prereq_inner.pack(fill=tk.X, padx=4, pady=4)

        self.prereq_tree = ttk.Treeview(
            prereq_inner,
            columns=('id', 'name'),
            show='headings',
            selectmode='browse',
            height=4
        )
        self.prereq_tree.heading('id', text='Quest ID')
        self.prereq_tree.heading('name', text='Name')
        self.prereq_tree.column('id', width=80, stretch=False)
        self.prereq_tree.column('name', width=250)

        prereq_scroll = ttk.Scrollbar(prereq_inner, orient=tk.VERTICAL, command=self.prereq_tree.yview)
        self.prereq_tree.configure(yscrollcommand=prereq_scroll.set)
        self.prereq_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        prereq_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        prereq_btn = ttk.Frame(prereq_frame)
        prereq_btn.pack(fill=tk.X, padx=4, pady=(0, 4))
        ttk.Button(prereq_btn, text="Add Prerequisite", command=self._add_prereq).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(prereq_btn, text="Remove", command=self._remove_prereq).pack(side=tk.LEFT)

        # === Save button ===
        save_frame = ttk.Frame(frame)
        save_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(save_frame, text="Save Quest", command=self._save).pack(side=tk.LEFT, padx=(0, 4))

    # =========================================================================
    # Quest list management
    # =========================================================================

    def _load_quest_list(self):
        """Load all quests into the tree and cache for prereq picking."""
        self._all_quests = self.quest_repo.list_all()
        self._populate_tree(self._all_quests)

    def _populate_tree(self, quests: List[Dict]):
        """Populate the quest list tree with given quests."""
        self.tree.delete(*self.tree.get_children())
        for q in quests:
            cat_label = QUEST_CATEGORIES.get(q.get('category', ''), q.get('category', ''))
            self.tree.insert('', tk.END, iid=q['id'],
                           values=(q['id'], q.get('name', ''), cat_label, q.get('tier', 0)))
        self.count_var.set(f"{len(quests)} quests")

    def _apply_filter(self):
        """Apply the filter criteria to the quest list."""
        quests = self._all_quests

        # Category filter
        cat_val = self.filter_cat_var.get()
        if cat_val != 'All':
            cat_code = cat_val.split(' - ')[0]
            quests = [q for q in quests if q.get('category') == cat_code]

        # Tier filter
        tier_val = self.filter_tier_var.get()
        if tier_val != 'All':
            tier_num = int(tier_val.split(' - ')[0])
            quests = [q for q in quests if q.get('tier') == tier_num]

        # Search filter
        search = self.filter_search_var.get().strip().lower()
        if search:
            quests = [q for q in quests
                     if search in q.get('id', '').lower()
                     or search in q.get('name', '').lower()
                     or search in q.get('description', '').lower()]

        self._populate_tree(quests)
        self.on_status(f"Showing {len(quests)} of {len(self._all_quests)} quests")

    def _clear_filter(self):
        """Reset all filters."""
        self.filter_cat_var.set('All')
        self.filter_tier_var.set('All')
        self.filter_search_var.set('')
        self._populate_tree(self._all_quests)

    def _on_select(self, event):
        """Handle quest selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved Changes", "Discard unsaved changes?"):
                if self.current_id:
                    self.tree.selection_set(self.current_id)
                return

        selection = self.tree.selection()
        if selection:
            self._load_quest(selection[0])

    # =========================================================================
    # Quest detail loading/saving
    # =========================================================================

    def _load_quest(self, quest_id: str):
        """Load a quest into the detail editor."""
        q = self.quest_repo.get_by_id(quest_id)
        if not q:
            return

        self.current_id = quest_id

        # Basic info
        self.id_var.set(q.get('id', ''))
        self.id_entry.configure(state='disabled')  # Can't change ID of existing quest
        self.name_var.set(q.get('name', ''))
        self.desc_text.delete('1.0', tk.END)
        self.desc_text.insert('1.0', q.get('description', ''))

        # Category
        cat = q.get('category', '')
        cat_label = QUEST_CATEGORIES.get(cat, cat)
        self.cat_var.set(f"{cat} - {cat_label}" if cat in QUEST_CATEGORIES else cat)

        self.tier_var.set(q.get('tier', 0))
        self.sort_var.set(q.get('sort_order', 0))

        # Flags
        flags = q.get('flags', 0)
        for bit, var in self.flag_vars.items():
            var.set(bool(flags & bit))

        # Requirements
        self.min_exp_var.set(q.get('min_explevel', 0))
        self.max_exp_var.set(q.get('max_explevel', 0))
        self.req_class_var.set(q.get('required_class', 0))

        # Rewards
        self.qp_var.set(q.get('qp_reward', 0))
        self.exp_var.set(q.get('exp_reward', 0))
        self.primal_var.set(q.get('primal_reward', 0))
        self.item_vnum_var.set(q.get('item_reward_vnum', 0))

        # Objectives
        self._load_objectives(quest_id)

        # Prerequisites
        self._load_prerequisites(quest_id)

        self.unsaved = False
        self.on_status(f"Loaded quest: {quest_id} - {q.get('name', '')}")

    def _load_objectives(self, quest_id: str):
        """Load objectives for a quest."""
        self.obj_tree.delete(*self.obj_tree.get_children())
        objectives = self.objectives_repo.get_for_quest(quest_id)
        for obj in objectives:
            self.obj_tree.insert('', tk.END, iid=f"obj_{obj['obj_index']}",
                               values=(obj['obj_index'], obj.get('type', ''),
                                      obj.get('target', ''), obj.get('threshold', 1),
                                      obj.get('description', '')))

    def _load_prerequisites(self, quest_id: str):
        """Load prerequisites for a quest."""
        self.prereq_tree.delete(*self.prereq_tree.get_children())
        prereq_ids = self.prereqs_repo.get_for_quest(quest_id)
        # Look up names from cached quest list
        name_map = {q['id']: q.get('name', '') for q in self._all_quests}
        for req_id in prereq_ids:
            self.prereq_tree.insert('', tk.END, iid=f"pre_{req_id}",
                                   values=(req_id, name_map.get(req_id, '(unknown)')))

    def _save(self):
        """Save the current quest, objectives, and prerequisites."""
        if self.current_id is None:
            return

        quest_id = self.current_id

        # Collect category code from combo value
        cat_combo = self.cat_var.get()
        cat_code = cat_combo.split(' - ')[0] if ' - ' in cat_combo else cat_combo

        # Collect flags bitmask
        flags = 0
        for bit, var in self.flag_vars.items():
            if var.get():
                flags |= bit

        data = {
            'name': self.name_var.get(),
            'description': self.desc_text.get('1.0', tk.END).rstrip('\n'),
            'category': cat_code,
            'tier': self.tier_var.get(),
            'flags': flags,
            'min_explevel': self.min_exp_var.get(),
            'max_explevel': self.max_exp_var.get(),
            'qp_reward': self.qp_var.get(),
            'exp_reward': self.exp_var.get(),
            'primal_reward': self.primal_var.get(),
            'item_reward_vnum': self.item_vnum_var.get(),
            'required_class': self.req_class_var.get(),
            'sort_order': self.sort_var.get(),
        }

        self.quest_repo.update(quest_id, data)

        # Save objectives
        objectives = self._collect_objectives()
        self.objectives_repo.save_for_quest(quest_id, objectives)

        # Save prerequisites
        prereq_ids = self._collect_prerequisites()
        self.prereqs_repo.save_for_quest(quest_id, prereq_ids)

        self.unsaved = False
        self._load_quest_list()  # Refresh list
        # Re-select current quest
        if self.tree.exists(quest_id):
            self.tree.selection_set(quest_id)
            self.tree.see(quest_id)
        self.on_status(f"Saved quest: {quest_id}")

    def _collect_objectives(self) -> List[Dict]:
        """Collect objectives from the tree widget."""
        objectives = []
        for item in self.obj_tree.get_children():
            vals = self.obj_tree.item(item, 'values')
            objectives.append({
                'obj_index': int(vals[0]),
                'type': vals[1],
                'target': vals[2],
                'threshold': int(vals[3]),
                'description': vals[4],
            })
        return objectives

    def _collect_prerequisites(self) -> List[str]:
        """Collect prerequisite IDs from the tree widget."""
        prereqs = []
        for item in self.prereq_tree.get_children():
            vals = self.prereq_tree.item(item, 'values')
            prereqs.append(vals[0])
        return prereqs

    # =========================================================================
    # New / Delete / Duplicate
    # =========================================================================

    def _new_quest(self):
        """Create a new quest."""
        quest_id = simpledialog.askstring("New Quest", "Enter quest ID (e.g. M10, CL05):", parent=self)
        if not quest_id:
            return
        quest_id = quest_id.strip().upper()

        # Check for duplicate
        if self.quest_repo.get_by_id(quest_id):
            messagebox.showerror("Error", f"Quest ID '{quest_id}' already exists.")
            return

        name = simpledialog.askstring("New Quest", "Enter quest name:", parent=self)
        if not name:
            return

        sort_order = self.quest_repo.get_next_sort_order()
        data = {
            'id': quest_id,
            'name': name,
            'description': '',
            'category': 'M',
            'tier': 0,
            'flags': 0,
            'min_explevel': 0,
            'max_explevel': 0,
            'qp_reward': 0,
            'exp_reward': 0,
            'primal_reward': 0,
            'item_reward_vnum': 0,
            'required_class': 0,
            'sort_order': sort_order,
        }
        self.quest_repo.insert(data)
        self._load_quest_list()
        self.tree.selection_set(quest_id)
        self.tree.see(quest_id)
        self._load_quest(quest_id)
        self.on_status(f"Created quest: {quest_id}")

    def _delete_quest(self):
        """Delete the selected quest."""
        if self.current_id is None:
            return

        if not messagebox.askyesno("Confirm Delete",
                                   f"Delete quest '{self.current_id}' and all its objectives/prerequisites?"):
            return

        self.quest_repo.delete(self.current_id)
        self.current_id = None
        self._clear_editor()
        self._load_quest_list()
        self.on_status("Quest deleted")

    def _duplicate_quest(self):
        """Duplicate the selected quest with a new ID."""
        if self.current_id is None:
            return

        new_id = simpledialog.askstring("Duplicate Quest",
                                        f"Enter new ID for copy of '{self.current_id}':", parent=self)
        if not new_id:
            return
        new_id = new_id.strip().upper()

        if self.quest_repo.get_by_id(new_id):
            messagebox.showerror("Error", f"Quest ID '{new_id}' already exists.")
            return

        # Copy quest def
        orig = self.quest_repo.get_by_id(self.current_id)
        if not orig:
            return

        data = dict(orig)
        data['id'] = new_id
        data['name'] = f"{data['name']} (copy)"
        data['sort_order'] = self.quest_repo.get_next_sort_order()
        self.quest_repo.insert(data)

        # Copy objectives
        objectives = self.objectives_repo.get_for_quest(self.current_id)
        self.objectives_repo.save_for_quest(new_id, objectives)

        # Copy prerequisites
        prereqs = self.prereqs_repo.get_for_quest(self.current_id)
        self.prereqs_repo.save_for_quest(new_id, prereqs)

        self._load_quest_list()
        self.tree.selection_set(new_id)
        self.tree.see(new_id)
        self._load_quest(new_id)
        self.on_status(f"Duplicated quest {self.current_id} -> {new_id}")

    # =========================================================================
    # Objectives CRUD
    # =========================================================================

    def _add_objective(self):
        """Add a new objective via dialog."""
        dialog = _ObjectiveDialog(self, "Add Objective")
        self.wait_window(dialog)
        if dialog.result:
            # Calculate next index
            children = self.obj_tree.get_children()
            next_idx = 0
            if children:
                last_vals = self.obj_tree.item(children[-1], 'values')
                next_idx = int(last_vals[0]) + 1

            obj = dialog.result
            obj['obj_index'] = next_idx
            self.obj_tree.insert('', tk.END, iid=f"obj_{next_idx}",
                               values=(next_idx, obj['type'], obj['target'],
                                      obj['threshold'], obj['description']))
            self._mark_unsaved()

    def _edit_objective(self):
        """Edit the selected objective."""
        selection = self.obj_tree.selection()
        if not selection:
            return

        vals = self.obj_tree.item(selection[0], 'values')
        current = {
            'type': vals[1],
            'target': vals[2],
            'threshold': int(vals[3]),
            'description': vals[4],
        }

        dialog = _ObjectiveDialog(self, "Edit Objective", current)
        self.wait_window(dialog)
        if dialog.result:
            obj = dialog.result
            self.obj_tree.item(selection[0],
                             values=(vals[0], obj['type'], obj['target'],
                                    obj['threshold'], obj['description']))
            self._mark_unsaved()

    def _remove_objective(self):
        """Remove the selected objective."""
        selection = self.obj_tree.selection()
        if not selection:
            return
        self.obj_tree.delete(selection[0])
        # Re-index remaining objectives
        self._reindex_objectives()
        self._mark_unsaved()

    def _reindex_objectives(self):
        """Re-number objective indices after removal/reorder."""
        items = self.obj_tree.get_children()
        for i, item in enumerate(items):
            vals = list(self.obj_tree.item(item, 'values'))
            vals[0] = i
            self.obj_tree.item(item, values=vals)
            # Update iid would require delete/re-insert, just keep the values correct

    def _move_objective(self, direction: int):
        """Move selected objective up (-1) or down (+1)."""
        selection = self.obj_tree.selection()
        if not selection:
            return

        items = list(self.obj_tree.get_children())
        idx = items.index(selection[0])
        new_idx = idx + direction

        if new_idx < 0 or new_idx >= len(items):
            return

        # Swap values
        vals_a = list(self.obj_tree.item(items[idx], 'values'))
        vals_b = list(self.obj_tree.item(items[new_idx], 'values'))
        # Swap all except index column
        vals_a[1:], vals_b[1:] = vals_b[1:], vals_a[1:]
        self.obj_tree.item(items[idx], values=vals_a)
        self.obj_tree.item(items[new_idx], values=vals_b)

        # Move selection to new position
        self.obj_tree.selection_set(items[new_idx])
        self._mark_unsaved()

    # =========================================================================
    # Prerequisites CRUD
    # =========================================================================

    def _add_prereq(self):
        """Add a prerequisite via a selection dialog."""
        # Build list of available quests (exclude self and already-added)
        existing = set(self._collect_prerequisites())
        available = [(q['id'], q.get('name', ''))
                     for q in self._all_quests
                     if q['id'] != self.current_id and q['id'] not in existing]

        if not available:
            messagebox.showinfo("Info", "No available quests to add as prerequisite.")
            return

        dialog = _PrereqPickerDialog(self, "Add Prerequisite", available)
        self.wait_window(dialog)
        if dialog.result:
            name_map = {q['id']: q.get('name', '') for q in self._all_quests}
            for req_id in dialog.result:
                self.prereq_tree.insert('', tk.END, iid=f"pre_{req_id}",
                                       values=(req_id, name_map.get(req_id, '')))
            self._mark_unsaved()

    def _remove_prereq(self):
        """Remove the selected prerequisite."""
        selection = self.prereq_tree.selection()
        if not selection:
            return
        self.prereq_tree.delete(selection[0])
        self._mark_unsaved()

    # =========================================================================
    # Helpers
    # =========================================================================

    def _mark_unsaved(self):
        self.unsaved = True

    def _clear_editor(self):
        """Clear all editor fields."""
        self.current_id = None
        self.id_var.set('')
        self.id_entry.configure(state='normal')
        self.name_var.set('')
        self.desc_text.delete('1.0', tk.END)
        self.cat_var.set('')
        self.tier_var.set(0)
        self.sort_var.set(0)
        for var in self.flag_vars.values():
            var.set(False)
        self.min_exp_var.set(0)
        self.max_exp_var.set(0)
        self.req_class_var.set(0)
        self.qp_var.set(0)
        self.exp_var.set(0)
        self.primal_var.set(0)
        self.item_vnum_var.set(0)
        self.obj_tree.delete(*self.obj_tree.get_children())
        self.prereq_tree.delete(*self.prereq_tree.get_children())
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes", "Discard unsaved changes?")
        return True


# =============================================================================
# Dialogs
# =============================================================================

class _ObjectiveDialog(tk.Toplevel):
    """Dialog for adding/editing a quest objective."""

    def __init__(self, parent, title: str, current: Optional[Dict] = None):
        super().__init__(parent)
        self.title(title)
        self.geometry("500x280")
        self.transient(parent)
        self.grab_set()
        self.result = None

        frame = ttk.Frame(self, padding=12)
        frame.pack(fill=tk.BOTH, expand=True)

        # Type
        row = ttk.Frame(frame)
        row.pack(fill=tk.X, pady=4)
        ttk.Label(row, text="Type:", width=12).pack(side=tk.LEFT)
        self.type_var = tk.StringVar(value=current.get('type', '') if current else '')
        ttk.Combobox(
            row, textvariable=self.type_var, values=OBJECTIVE_TYPES,
            state='readonly', width=24
        ).pack(side=tk.LEFT)

        # Target
        row = ttk.Frame(frame)
        row.pack(fill=tk.X, pady=4)
        ttk.Label(row, text="Target:", width=12).pack(side=tk.LEFT)
        self.target_var = tk.StringVar(value=current.get('target', '') if current else '')
        ttk.Entry(row, textvariable=self.target_var, width=30).pack(side=tk.LEFT, fill=tk.X, expand=True)

        # Threshold
        row = ttk.Frame(frame)
        row.pack(fill=tk.X, pady=4)
        ttk.Label(row, text="Threshold:", width=12).pack(side=tk.LEFT)
        self.threshold_var = tk.IntVar(value=current.get('threshold', 1) if current else 1)
        ttk.Spinbox(row, from_=1, to=999999, width=8, textvariable=self.threshold_var).pack(side=tk.LEFT)

        # Description
        row = ttk.Frame(frame)
        row.pack(fill=tk.BOTH, expand=True, pady=4)
        ttk.Label(row, text="Description:", width=12).pack(side=tk.LEFT, anchor=tk.N)
        self.desc_text = tk.Text(row, wrap=tk.WORD, height=4, font=('Consolas', 10))
        self.desc_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        if current and current.get('description'):
            self.desc_text.insert('1.0', current['description'])

        # Buttons
        btn = ttk.Frame(frame)
        btn.pack(fill=tk.X, pady=(8, 0))
        ttk.Button(btn, text="OK", command=self._ok).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn, text="Cancel", command=self.destroy).pack(side=tk.LEFT)

        self.bind('<Return>', lambda e: self._ok())
        self.bind('<Escape>', lambda e: self.destroy())

    def _ok(self):
        obj_type = self.type_var.get()
        if not obj_type:
            messagebox.showwarning("Validation", "Please select an objective type.", parent=self)
            return

        self.result = {
            'type': obj_type,
            'target': self.target_var.get(),
            'threshold': self.threshold_var.get(),
            'description': self.desc_text.get('1.0', tk.END).rstrip('\n'),
        }
        self.destroy()


class _PrereqPickerDialog(tk.Toplevel):
    """Dialog for picking prerequisite quests from a list."""

    def __init__(self, parent, title: str, available: List[tuple]):
        super().__init__(parent)
        self.title(title)
        self.geometry("450x400")
        self.transient(parent)
        self.grab_set()
        self.result = None

        frame = ttk.Frame(self, padding=8)
        frame.pack(fill=tk.BOTH, expand=True)

        ttk.Label(frame, text="Select prerequisite quest(s):").pack(anchor=tk.W, pady=(0, 4))

        # Filter
        filter_row = ttk.Frame(frame)
        filter_row.pack(fill=tk.X, pady=(0, 4))
        ttk.Label(filter_row, text="Search:").pack(side=tk.LEFT)
        self.search_var = tk.StringVar()
        self.search_var.trace_add('write', lambda *_: self._filter())
        ttk.Entry(filter_row, textvariable=self.search_var, width=25).pack(side=tk.LEFT, padx=(4, 0))

        tree_frame = ttk.Frame(frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('id', 'name'),
            show='headings',
            selectmode='extended'
        )
        self.tree.heading('id', text='ID')
        self.tree.heading('name', text='Name')
        self.tree.column('id', width=80, stretch=False)
        self.tree.column('name', width=300)

        scroll = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scroll.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self._available = available
        self._populate(available)

        # Buttons
        btn = ttk.Frame(frame)
        btn.pack(fill=tk.X, pady=(8, 0))
        ttk.Button(btn, text="Add Selected", command=self._ok).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn, text="Cancel", command=self.destroy).pack(side=tk.LEFT)

        self.tree.bind('<Double-1>', lambda e: self._ok())
        self.bind('<Escape>', lambda e: self.destroy())

    def _populate(self, items):
        self.tree.delete(*self.tree.get_children())
        for qid, name in items:
            self.tree.insert('', tk.END, iid=qid, values=(qid, name))

    def _filter(self):
        search = self.search_var.get().strip().lower()
        if not search:
            self._populate(self._available)
        else:
            filtered = [(qid, name) for qid, name in self._available
                       if search in qid.lower() or search in name.lower()]
            self._populate(filtered)

    def _ok(self):
        selection = self.tree.selection()
        if selection:
            self.result = list(selection)
        self.destroy()
