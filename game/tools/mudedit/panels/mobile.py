"""
Mobile/NPC editor panel.

Provides CRUD interface for mobile entities with flags, dice, and descriptions.
"""

import tkinter as tk
from tkinter import ttk, messagebox
from typing import Callable, Optional
import sys
from pathlib import Path

# Add mudlib to path for model imports
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from mudlib.models import ACT_FLAGS, AFF_FLAGS

from ..db.repository import MobileRepository
from ..widgets import ColorTextEditor, EntityListPanel, FlagEditor, DiceEditor, strip_colors


class MobileEditorPanel(ttk.Frame):
    """
    Editor panel for mobile/NPC entities.

    Provides editing for:
    - Basic info (vnum, keywords, short/long descriptions)
    - Combat stats (level, hitroll, AC, hit dice, damage dice)
    - Flags (act flags, affected_by flags)
    - Full description with color preview
    """

    def __init__(
        self,
        parent,
        repository: MobileRepository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        """
        Initialize the mobile editor panel.

        Args:
            parent: Parent Tkinter widget
            repository: MobileRepository for database operations
            on_status: Callback for status messages
            **kwargs: Additional arguments passed to ttk.Frame
        """
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.current_vnum: Optional[int] = None
        self.unsaved = False

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        # Main horizontal pane
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

        # Create a canvas with scrollbar for the editor
        canvas = tk.Canvas(right_container, highlightthickness=0)
        scrollbar = ttk.Scrollbar(right_container, orient=tk.VERTICAL, command=canvas.yview)

        self.editor_frame = ttk.Frame(canvas)

        canvas.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        canvas_frame = canvas.create_window((0, 0), window=self.editor_frame, anchor=tk.NW)

        def configure_scroll(event):
            canvas.configure(scrollregion=canvas.bbox("all"))
            canvas.itemconfig(canvas_frame, width=event.width)

        self.editor_frame.bind('<Configure>', lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.bind('<Configure>', configure_scroll)

        # Mouse wheel scrolling
        def on_mousewheel(event):
            canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")
        canvas.bind_all('<MouseWheel>', on_mousewheel)

        self._build_editor(self.editor_frame)

    def _build_editor(self, parent):
        """Build the editor form."""
        # === Basic Info Section ===
        basic_frame = ttk.LabelFrame(parent, text="Basic Info")
        basic_frame.pack(fill=tk.X, padx=4, pady=(4, 2))

        # Row 1: Vnum, Keywords
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

        # Row 2: Short description
        row2 = ttk.Frame(basic_frame)
        row2.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(row2, text="Short Desc:").pack(side=tk.LEFT)
        self.short_var = tk.StringVar()
        self.short_var.trace_add('write', lambda *_: self._mark_unsaved())
        self.short_entry = ttk.Entry(row2, textvariable=self.short_var, width=60)
        self.short_entry.pack(side=tk.LEFT, padx=(4, 0), fill=tk.X, expand=True)

        # Row 3: Long description
        row3 = ttk.Frame(basic_frame)
        row3.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(row3, text="Long Desc:").pack(side=tk.LEFT)
        self.long_var = tk.StringVar()
        self.long_var.trace_add('write', lambda *_: self._mark_unsaved())
        self.long_entry = ttk.Entry(row3, textvariable=self.long_var, width=60)
        self.long_entry.pack(side=tk.LEFT, padx=(4, 0), fill=tk.X, expand=True)

        # Row 4: Sex, Alignment, Gold
        row4 = ttk.Frame(basic_frame)
        row4.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(row4, text="Sex:").pack(side=tk.LEFT)
        self.sex_var = tk.StringVar(value="Neutral")
        self.sex_combo = ttk.Combobox(
            row4,
            textvariable=self.sex_var,
            values=["Neutral", "Male", "Female"],
            state='readonly',
            width=8
        )
        self.sex_combo.pack(side=tk.LEFT, padx=(4, 16))
        self.sex_combo.bind('<<ComboboxSelected>>', lambda e: self._mark_unsaved())

        ttk.Label(row4, text="Alignment:").pack(side=tk.LEFT)
        self.align_var = tk.IntVar(value=0)
        self.align_spin = ttk.Spinbox(
            row4,
            from_=-1000,
            to=1000,
            width=6,
            textvariable=self.align_var,
            command=self._mark_unsaved
        )
        self.align_spin.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row4, text="Gold:").pack(side=tk.LEFT)
        self.gold_var = tk.IntVar(value=0)
        self.gold_spin = ttk.Spinbox(
            row4,
            from_=0,
            to=999999999,
            width=10,
            textvariable=self.gold_var,
            command=self._mark_unsaved
        )
        self.gold_spin.pack(side=tk.LEFT, padx=(4, 0))

        # === Combat Stats Section ===
        combat_frame = ttk.LabelFrame(parent, text="Combat Stats")
        combat_frame.pack(fill=tk.X, padx=4, pady=2)

        # Row 1: Level, Hitroll, AC
        crow1 = ttk.Frame(combat_frame)
        crow1.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(crow1, text="Level:").pack(side=tk.LEFT)
        self.level_var = tk.IntVar(value=1)
        self.level_spin = ttk.Spinbox(
            crow1,
            from_=1,
            to=5000,
            width=6,
            textvariable=self.level_var,
            command=self._mark_unsaved
        )
        self.level_spin.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(crow1, text="Hitroll:").pack(side=tk.LEFT)
        self.hitroll_var = tk.IntVar(value=0)
        self.hitroll_spin = ttk.Spinbox(
            crow1,
            from_=-100,
            to=10000,
            width=6,
            textvariable=self.hitroll_var,
            command=self._mark_unsaved
        )
        self.hitroll_spin.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(crow1, text="AC:").pack(side=tk.LEFT)
        self.ac_var = tk.IntVar(value=0)
        self.ac_spin = ttk.Spinbox(
            crow1,
            from_=-10000,
            to=10000,
            width=6,
            textvariable=self.ac_var,
            command=self._mark_unsaved
        )
        self.ac_spin.pack(side=tk.LEFT, padx=(4, 0))

        # Row 2: Hit Dice
        crow2 = ttk.Frame(combat_frame)
        crow2.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(crow2, text="Hit Points:").pack(side=tk.LEFT)
        self.hit_dice = DiceEditor(crow2, on_change=lambda v: self._mark_unsaved())
        self.hit_dice.pack(side=tk.LEFT, padx=(4, 0))

        # Row 3: Damage Dice
        crow3 = ttk.Frame(combat_frame)
        crow3.pack(fill=tk.X, padx=4, pady=2)

        ttk.Label(crow3, text="Damage:").pack(side=tk.LEFT)
        self.dam_dice = DiceEditor(crow3, on_change=lambda v: self._mark_unsaved())
        self.dam_dice.pack(side=tk.LEFT, padx=(4, 0))

        # === Flags Section ===
        flags_frame = ttk.LabelFrame(parent, text="Flags")
        flags_frame.pack(fill=tk.X, padx=4, pady=2)

        # Act Flags
        self.act_flags = FlagEditor(
            flags_frame,
            ACT_FLAGS,
            label="Act Flags",
            columns=4,
            on_change=lambda v: self._mark_unsaved()
        )
        self.act_flags.pack(fill=tk.X, padx=4, pady=2)

        # Affected By Flags
        self.aff_flags = FlagEditor(
            flags_frame,
            AFF_FLAGS,
            label="Affected By",
            columns=4,
            on_change=lambda v: self._mark_unsaved()
        )
        self.aff_flags.pack(fill=tk.X, padx=4, pady=2)

        # === Description Section ===
        desc_frame = ttk.LabelFrame(parent, text="Full Description")
        desc_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=2)

        self.desc_editor = ColorTextEditor(
            desc_frame,
            show_preview=True,
            on_change=self._mark_unsaved
        )
        self.desc_editor.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # === Button Bar ===
        btn_frame = ttk.Frame(parent)
        btn_frame.pack(fill=tk.X, padx=4, pady=4)

        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(
            btn_frame,
            text="Refresh Preview",
            command=self.desc_editor.refresh_preview
        ).pack(side=tk.LEFT, padx=(0, 4))

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
        sex_map = {0: "Neutral", 1: "Male", 2: "Female"}
        self.sex_var.set(sex_map.get(mobile.get('sex', 0), "Neutral"))

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

        self.unsaved = False
        self.on_status(f"Loaded mobile [{vnum}] {mobile.get('short_descr', '')}")

    def _mark_unsaved(self):
        """Mark the current entry as having unsaved changes."""
        self.unsaved = True

    def _save(self):
        """Save current mobile to database."""
        if self.current_vnum is None:
            self.on_status("No mobile selected")
            return

        keywords = self.keywords_var.get().strip()
        if not keywords:
            messagebox.showwarning("Warning", "Keywords cannot be empty.")
            return

        # Map sex back to integer
        sex_map = {"Neutral": 0, "Male": 1, "Female": 2}
        sex = sex_map.get(self.sex_var.get(), 0)

        # Get dice values
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

        # Refresh list
        self._load_entries()
        self.list_panel.select_item(self.current_vnum)

        self.unsaved = False
        self.on_status(f"Saved mobile [{self.current_vnum}] {self.short_var.get()}")

    def _new(self):
        """Create a new mobile."""
        # Find next available vnum
        entries = self.repository.list_all()
        if entries:
            max_vnum = max(e['vnum'] for e in entries)
            new_vnum = max_vnum + 1
        else:
            new_vnum = 1

        # Prompt for vnum
        from tkinter import simpledialog
        vnum = simpledialog.askinteger(
            "New Mobile",
            "Enter vnum:",
            initialvalue=new_vnum,
            parent=self
        )
        if not vnum:
            return

        # Check if vnum exists
        if self.repository.get_by_id(vnum):
            messagebox.showerror("Error", f"Mobile {vnum} already exists.")
            return

        # Insert new mobile with defaults
        self.repository.insert({
            'vnum': vnum,
            'player_name': 'new mobile',
            'short_descr': 'a new mobile',
            'long_descr': 'A new mobile stands here.',
            'description': '',
            'act': 1,  # NPC flag
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
        self.unsaved = False

    def check_unsaved(self) -> bool:
        """Check for unsaved changes and prompt user."""
        if self.unsaved:
            return messagebox.askyesno(
                "Unsaved Changes",
                "You have unsaved changes. Discard them?"
            )
        return True
