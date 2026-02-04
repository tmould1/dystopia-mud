"""
Shop editor panel.

Provides interface for editing shop configurations.
"""

import tkinter as tk
from tkinter import ttk, messagebox
from typing import Callable, Dict, Optional
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from mudlib.models import ITEM_TYPES

from ..db.repository import ShopRepository, MobileRepository


class ShopEditorPanel(ttk.Frame):
    """
    Editor panel for shop configurations.

    Displays shops with keeper names and allows editing buy types, profits, hours.
    """

    def __init__(
        self,
        parent,
        repository: ShopRepository,
        mob_repository: MobileRepository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.mob_repo = mob_repository
        self.on_status = on_status or (lambda msg: None)

        self._mob_cache: Dict[int, str] = {}
        self.current_vnum: Optional[int] = None
        self.unsaved = False

        self._build_cache()
        self._build_ui()
        self._load_entries()

    def _build_cache(self):
        """Build mob name cache."""
        for mob in self.mob_repo.list_all():
            self._mob_cache[mob['vnum']] = mob.get('short_descr', f"mob#{mob['vnum']}")

    def _build_ui(self):
        """Build the panel UI."""
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: shop list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('vnum', 'keeper'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('vnum', text='Vnum')
        self.tree.heading('keeper', text='Shopkeeper')
        self.tree.column('vnum', width=60, stretch=False)
        self.tree.column('keeper', width=200)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 shops")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor
        right_frame = ttk.LabelFrame(paned, text="Edit Shop")
        paned.add(right_frame, weight=1)

        # Keeper
        row1 = ttk.Frame(right_frame)
        row1.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row1, text="Keeper Vnum:").pack(side=tk.LEFT)
        self.keeper_label = ttk.Label(row1, text="-", font=('Consolas', 10, 'bold'))
        self.keeper_label.pack(side=tk.LEFT, padx=4)
        self.keeper_name = ttk.Label(row1, text="", foreground='gray')
        self.keeper_name.pack(side=tk.LEFT, padx=4)

        # Buy types
        buy_frame = ttk.LabelFrame(right_frame, text="Items Shop Will Buy")
        buy_frame.pack(fill=tk.X, padx=8, pady=4)

        self.buy_type_vars = []
        type_list = ["(none)"] + [f"{k} - {v}" for k, v in sorted(ITEM_TYPES.items())]

        for i in range(5):
            row = ttk.Frame(buy_frame)
            row.pack(fill=tk.X, padx=4, pady=2)
            ttk.Label(row, text=f"Type {i+1}:", width=8).pack(side=tk.LEFT)
            var = tk.StringVar()
            combo = ttk.Combobox(row, textvariable=var, values=type_list, state='readonly', width=20)
            combo.pack(side=tk.LEFT)
            combo.bind('<<ComboboxSelected>>', lambda e: self._mark_unsaved())
            self.buy_type_vars.append(var)

        # Profits
        profit_frame = ttk.Frame(right_frame)
        profit_frame.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(profit_frame, text="Profit Buy:").pack(side=tk.LEFT)
        self.profit_buy_var = tk.IntVar(value=100)
        ttk.Spinbox(profit_frame, from_=1, to=1000, width=5, textvariable=self.profit_buy_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT, padx=(2, 16))
        ttk.Label(profit_frame, text="%").pack(side=tk.LEFT, padx=(0, 16))

        ttk.Label(profit_frame, text="Profit Sell:").pack(side=tk.LEFT)
        self.profit_sell_var = tk.IntVar(value=100)
        ttk.Spinbox(profit_frame, from_=1, to=1000, width=5, textvariable=self.profit_sell_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT, padx=(2, 0))
        ttk.Label(profit_frame, text="%").pack(side=tk.LEFT)

        # Hours
        hours_frame = ttk.Frame(right_frame)
        hours_frame.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(hours_frame, text="Open Hour:").pack(side=tk.LEFT)
        self.open_var = tk.IntVar(value=0)
        ttk.Spinbox(hours_frame, from_=0, to=23, width=4, textvariable=self.open_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT, padx=(2, 16))

        ttk.Label(hours_frame, text="Close Hour:").pack(side=tk.LEFT)
        self.close_var = tk.IntVar(value=23)
        ttk.Spinbox(hours_frame, from_=0, to=23, width=4, textvariable=self.close_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT, padx=(2, 0))

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)

        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT)

    def _load_entries(self):
        """Load all shops."""
        entries = self.repository.list_all()

        self.tree.delete(*self.tree.get_children())
        for shop in entries:
            vnum = shop['keeper_vnum']
            name = self._mob_cache.get(vnum, f"#{vnum}")
            self.tree.insert('', tk.END, iid=str(vnum), values=(vnum, name))

        self.count_var.set(f"{len(entries)} shops")
        self.on_status(f"Loaded {len(entries)} shops")

    def _on_select(self, event):
        """Handle shop selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved", "Discard?"):
                if self.current_vnum:
                    self.tree.selection_set(str(self.current_vnum))
                return

        selection = self.tree.selection()
        if selection:
            self._load_entry(int(selection[0]))

    def _load_entry(self, vnum: int):
        """Load a shop into the editor."""
        shop = self.repository.get_by_id(vnum)
        if not shop:
            return

        self.current_vnum = vnum
        self.keeper_label.configure(text=str(vnum))
        self.keeper_name.configure(text=self._mob_cache.get(vnum, ''))

        # Buy types
        for i in range(5):
            type_id = shop.get(f'buy_type{i}', 0)
            if type_id and type_id in ITEM_TYPES:
                self.buy_type_vars[i].set(f"{type_id} - {ITEM_TYPES[type_id]}")
            else:
                self.buy_type_vars[i].set("(none)")

        self.profit_buy_var.set(shop.get('profit_buy', 100))
        self.profit_sell_var.set(shop.get('profit_sell', 100))
        self.open_var.set(shop.get('open_hour', 0))
        self.close_var.set(shop.get('close_hour', 23))

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current shop."""
        if self.current_vnum is None:
            return

        # Parse buy types
        buy_types = []
        for var in self.buy_type_vars:
            val = var.get()
            if val and val != "(none)":
                try:
                    type_id = int(val.split(' - ')[0])
                    buy_types.append(type_id)
                except ValueError:
                    buy_types.append(0)
            else:
                buy_types.append(0)

        data = {
            'buy_type0': buy_types[0],
            'buy_type1': buy_types[1],
            'buy_type2': buy_types[2],
            'buy_type3': buy_types[3],
            'buy_type4': buy_types[4],
            'profit_buy': self.profit_buy_var.get(),
            'profit_sell': self.profit_sell_var.get(),
            'open_hour': self.open_var.get(),
            'close_hour': self.close_var.get(),
        }

        self.repository.update(self.current_vnum, data)
        self.unsaved = False
        self.on_status(f"Saved shop [{self.current_vnum}]")

    def _new(self):
        """Create a new shop."""
        from tkinter import simpledialog
        vnum = simpledialog.askinteger("New Shop", "Enter keeper mob vnum:", parent=self)
        if not vnum:
            return

        if not self._mob_cache.get(vnum):
            messagebox.showwarning("Warning", f"Mobile {vnum} not found in this area.")

        if self.repository.get_by_id(vnum):
            messagebox.showerror("Error", f"Shop for mob {vnum} already exists.")
            return

        self.repository.insert({
            'keeper_vnum': vnum,
            'buy_type0': 0, 'buy_type1': 0, 'buy_type2': 0, 'buy_type3': 0, 'buy_type4': 0,
            'profit_buy': 100,
            'profit_sell': 100,
            'open_hour': 0,
            'close_hour': 23,
        })

        self._load_entries()
        self.tree.selection_set(str(vnum))
        self._load_entry(vnum)
        self.on_status(f"Created shop for mob [{vnum}]")

    def _delete(self):
        """Delete current shop."""
        if self.current_vnum is None:
            return

        name = self._mob_cache.get(self.current_vnum, str(self.current_vnum))
        if not messagebox.askyesno("Confirm", f"Delete shop for {name}?"):
            return

        self.repository.delete(self.current_vnum)
        self.current_vnum = None
        self._clear_editor()
        self._load_entries()
        self.on_status("Shop deleted")

    def _clear_editor(self):
        """Clear editor."""
        self.current_vnum = None
        self.keeper_label.configure(text="-")
        self.keeper_name.configure(text="")
        for var in self.buy_type_vars:
            var.set("(none)")
        self.profit_buy_var.set(100)
        self.profit_sell_var.set(100)
        self.open_var.set(0)
        self.close_var.set(23)
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved", "Discard?")
        return True
