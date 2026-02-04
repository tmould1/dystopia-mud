"""
Area info editor panel.

Provides interface for editing area metadata.
"""

import tkinter as tk
from tkinter import ttk, messagebox
from typing import Callable, Optional

from ..db.repository import AreaRepository


class AreaInfoPanel(ttk.Frame):
    """
    Editor panel for area metadata.

    Displays and allows editing of:
    - Area name
    - Builder names
    - Vnum range
    - Security level
    - Recall room
    - Area flags
    """

    def __init__(
        self,
        parent,
        repository: AreaRepository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.unsaved = False

        self._build_ui()
        self._load_area()

    def _build_ui(self):
        """Build the panel UI."""
        main_frame = ttk.Frame(self)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=20)

        ttk.Label(
            main_frame,
            text="Area Information",
            font=('Consolas', 14, 'bold')
        ).pack(anchor=tk.W, pady=(0, 16))

        # Area name
        row1 = ttk.Frame(main_frame)
        row1.pack(fill=tk.X, pady=4)
        ttk.Label(row1, text="Area Name:", width=15).pack(side=tk.LEFT)
        self.name_var = tk.StringVar()
        self.name_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row1, textvariable=self.name_var, width=40).pack(side=tk.LEFT)

        # Builders
        row2 = ttk.Frame(main_frame)
        row2.pack(fill=tk.X, pady=4)
        ttk.Label(row2, text="Builders:", width=15).pack(side=tk.LEFT)
        self.builders_var = tk.StringVar()
        self.builders_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row2, textvariable=self.builders_var, width=40).pack(side=tk.LEFT)
        ttk.Label(row2, text="(space-separated, 'All' for everyone)", foreground='gray').pack(side=tk.LEFT, padx=8)

        # Vnum range
        row3 = ttk.Frame(main_frame)
        row3.pack(fill=tk.X, pady=4)
        ttk.Label(row3, text="Lower Vnum:", width=15).pack(side=tk.LEFT)
        self.lvnum_var = tk.IntVar(value=0)
        ttk.Spinbox(row3, from_=0, to=999999, width=10, textvariable=self.lvnum_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT, padx=(0, 16))

        ttk.Label(row3, text="Upper Vnum:").pack(side=tk.LEFT)
        self.uvnum_var = tk.IntVar(value=0)
        ttk.Spinbox(row3, from_=0, to=999999, width=10, textvariable=self.uvnum_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT)

        # Security
        row4 = ttk.Frame(main_frame)
        row4.pack(fill=tk.X, pady=4)
        ttk.Label(row4, text="Security:", width=15).pack(side=tk.LEFT)
        self.security_var = tk.IntVar(value=3)
        ttk.Spinbox(row4, from_=1, to=10, width=5, textvariable=self.security_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT)
        ttk.Label(row4, text="(1-10, higher = more restricted)", foreground='gray').pack(side=tk.LEFT, padx=8)

        # Recall
        row5 = ttk.Frame(main_frame)
        row5.pack(fill=tk.X, pady=4)
        ttk.Label(row5, text="Recall Vnum:", width=15).pack(side=tk.LEFT)
        self.recall_var = tk.IntVar(value=0)
        ttk.Spinbox(row5, from_=0, to=999999, width=10, textvariable=self.recall_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT)
        ttk.Label(row5, text="(room vnum for recall)", foreground='gray').pack(side=tk.LEFT, padx=8)

        # Hidden
        row6 = ttk.Frame(main_frame)
        row6.pack(fill=tk.X, pady=4)
        self.hidden_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(
            row6, text="Hidden (not shown in area list)",
            variable=self.hidden_var,
            command=self._mark_unsaved
        ).pack(side=tk.LEFT)

        # Stats (read-only)
        stats_frame = ttk.LabelFrame(main_frame, text="Statistics")
        stats_frame.pack(fill=tk.X, pady=(16, 4))

        self.stats_label = ttk.Label(stats_frame, text="Loading...")
        self.stats_label.pack(padx=8, pady=8, anchor=tk.W)

        # Buttons
        btn_frame = ttk.Frame(main_frame)
        btn_frame.pack(fill=tk.X, pady=(16, 0))

        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Reload", command=self._load_area).pack(side=tk.LEFT)

    def _load_area(self):
        """Load area info from database."""
        info = self.repository.get_info()
        if not info:
            self.on_status("No area info found")
            return

        self.name_var.set(info.get('name', ''))
        self.builders_var.set(info.get('builders', ''))
        self.lvnum_var.set(info.get('lvnum', 0))
        self.uvnum_var.set(info.get('uvnum', 0))
        self.security_var.set(info.get('security', 3))
        self.recall_var.set(info.get('recall', 0))
        self.hidden_var.set(bool(info.get('is_hidden', 0)))

        # Get stats
        conn = self.repository.conn
        mob_count = conn.execute("SELECT COUNT(*) FROM mobiles").fetchone()[0]
        obj_count = conn.execute("SELECT COUNT(*) FROM objects").fetchone()[0]
        room_count = conn.execute("SELECT COUNT(*) FROM rooms").fetchone()[0]
        reset_count = conn.execute("SELECT COUNT(*) FROM resets").fetchone()[0]

        self.stats_label.configure(
            text=f"Mobiles: {mob_count}    Objects: {obj_count}    "
                 f"Rooms: {room_count}    Resets: {reset_count}"
        )

        self.unsaved = False
        self.on_status(f"Loaded area: {info.get('name', 'Unknown')}")

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save area info."""
        name = self.name_var.get().strip()
        if not name:
            messagebox.showwarning("Warning", "Area name cannot be empty.")
            return

        data = {
            'name': name,
            'builders': self.builders_var.get(),
            'lvnum': self.lvnum_var.get(),
            'uvnum': self.uvnum_var.get(),
            'security': self.security_var.get(),
            'recall': self.recall_var.get(),
            'is_hidden': 1 if self.hidden_var.get() else 0,
        }

        self.repository.update_info(data)
        self.unsaved = False
        self.on_status(f"Saved area: {name}")

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes", "Discard?")
        return True
