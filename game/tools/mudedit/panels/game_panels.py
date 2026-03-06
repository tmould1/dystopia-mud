"""
Game management panels.

Provides interfaces for editing kingdoms, bans, disabled commands, leaderboards, notes, and bugs.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Optional
from datetime import datetime


class KingdomsPanel(ttk.Frame):
    """Editor panel for kingdoms/guilds."""

    def __init__(
        self,
        parent,
        repository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.current_id: Optional[int] = None
        self.unsaved = False

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: kingdom list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('id', 'name', 'leader'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('id', text='ID')
        self.tree.heading('name', text='Name')
        self.tree.heading('leader', text='Leader')
        self.tree.column('id', width=40, stretch=False)
        self.tree.column('name', width=120)
        self.tree.column('leader', width=100)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        # Right: editor
        right_frame = ttk.LabelFrame(paned, text="Edit Kingdom")
        paned.add(right_frame, weight=2)

        # ID
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="ID:", width=12).pack(side=tk.LEFT)
        self.id_label = ttk.Label(row, text="-", font=('Consolas', 10, 'bold'))
        self.id_label.pack(side=tk.LEFT)

        # Name
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Name:", width=12).pack(side=tk.LEFT)
        self.name_var = tk.StringVar()
        self.name_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.name_var, width=30).pack(side=tk.LEFT)

        # Who Name (displayed in who list)
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Who Name:", width=12).pack(side=tk.LEFT)
        self.whoname_var = tk.StringVar()
        self.whoname_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.whoname_var, width=30).pack(side=tk.LEFT)

        # Leader
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Leader:", width=12).pack(side=tk.LEFT)
        self.leader_var = tk.StringVar()
        self.leader_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.leader_var, width=20).pack(side=tk.LEFT)

        # General
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="General:", width=12).pack(side=tk.LEFT)
        self.general_var = tk.StringVar()
        self.general_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.general_var, width=20).pack(side=tk.LEFT)

        # Stats frame
        stats_frame = ttk.LabelFrame(right_frame, text="Statistics")
        stats_frame.pack(fill=tk.X, padx=8, pady=8)

        row = ttk.Frame(stats_frame)
        row.pack(fill=tk.X, padx=4, pady=4)
        ttk.Label(row, text="Kills:", width=8).pack(side=tk.LEFT)
        self.kills_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=8, textvariable=self.kills_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT, padx=(0, 16))
        ttk.Label(row, text="Deaths:", width=8).pack(side=tk.LEFT)
        self.deaths_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=8, textvariable=self.deaths_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT, padx=(0, 16))
        ttk.Label(row, text="QPs:", width=6).pack(side=tk.LEFT)
        self.qps_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=8, textvariable=self.qps_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT)

        # Requirements frame
        req_frame = ttk.LabelFrame(right_frame, text="Membership Requirements")
        req_frame.pack(fill=tk.X, padx=8, pady=8)

        row = ttk.Frame(req_frame)
        row.pack(fill=tk.X, padx=4, pady=4)
        ttk.Label(row, text="Hit:", width=8).pack(side=tk.LEFT)
        self.req_hit_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=8, textvariable=self.req_hit_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT, padx=(0, 16))
        ttk.Label(row, text="Move:", width=8).pack(side=tk.LEFT)
        self.req_move_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=8, textvariable=self.req_move_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT)

        row = ttk.Frame(req_frame)
        row.pack(fill=tk.X, padx=4, pady=4)
        ttk.Label(row, text="Mana:", width=8).pack(side=tk.LEFT)
        self.req_mana_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=8, textvariable=self.req_mana_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT, padx=(0, 16))
        ttk.Label(row, text="QPs:", width=8).pack(side=tk.LEFT)
        self.req_qps_var = tk.IntVar()
        ttk.Spinbox(row, from_=0, to=999999, width=8, textvariable=self.req_qps_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT)

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Reload", command=self._load_entries).pack(side=tk.LEFT)

    def _load_entries(self):
        """Load all kingdoms."""
        entries = self.repository.list_all()

        self.tree.delete(*self.tree.get_children())
        for k in entries:
            self.tree.insert('', tk.END, iid=str(k['id']),
                           values=(k['id'], k['name'], k['leader']))

        self.on_status(f"Loaded {len(entries)} kingdoms")

    def _on_select(self, event):
        """Handle selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved", "Discard changes?"):
                if self.current_id:
                    self.tree.selection_set(str(self.current_id))
                return

        selection = self.tree.selection()
        if selection:
            self._load_entry(int(selection[0]))

    def _load_entry(self, id_val: int):
        """Load a kingdom into the editor."""
        k = self.repository.get_by_id(id_val)
        if not k:
            return

        self.current_id = id_val
        self.id_label.configure(text=str(id_val))
        self.name_var.set(k.get('name', ''))
        self.whoname_var.set(k.get('whoname', ''))
        self.leader_var.set(k.get('leader', ''))
        self.general_var.set(k.get('general', ''))
        self.kills_var.set(k.get('kills', 0))
        self.deaths_var.set(k.get('deaths', 0))
        self.qps_var.set(k.get('qps', 0))
        self.req_hit_var.set(k.get('req_hit', 0))
        self.req_move_var.set(k.get('req_move', 0))
        self.req_mana_var.set(k.get('req_mana', 0))
        self.req_qps_var.set(k.get('req_qps', 0))

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current kingdom."""
        if self.current_id is None:
            return

        data = {
            'name': self.name_var.get(),
            'whoname': self.whoname_var.get(),
            'leader': self.leader_var.get(),
            'general': self.general_var.get(),
            'kills': self.kills_var.get(),
            'deaths': self.deaths_var.get(),
            'qps': self.qps_var.get(),
            'req_hit': self.req_hit_var.get(),
            'req_move': self.req_move_var.get(),
            'req_mana': self.req_mana_var.get(),
            'req_qps': self.req_qps_var.get(),
        }

        self.repository.update(self.current_id, data)
        self.unsaved = False
        self._load_entries()
        self.tree.selection_set(str(self.current_id))
        self.on_status(f"Saved kingdom {self.current_id}")

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved", "Discard changes?")
        return True


class BansPanel(ttk.Frame):
    """Editor panel for bans."""

    def __init__(
        self,
        parent,
        repository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.current_id: Optional[int] = None
        self.unsaved = False

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: ban list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('id', 'name', 'reason'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('id', text='ID')
        self.tree.heading('name', text='Name/Site')
        self.tree.heading('reason', text='Reason')
        self.tree.column('id', width=40, stretch=False)
        self.tree.column('name', width=150)
        self.tree.column('reason', width=200)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 bans")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor
        right_frame = ttk.LabelFrame(paned, text="Edit Ban")
        paned.add(right_frame, weight=1)

        # Name
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Name/Site:", width=12).pack(side=tk.LEFT)
        self.name_var = tk.StringVar()
        self.name_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.name_var, width=30).pack(side=tk.LEFT)

        # Reason
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)
        ttk.Label(row, text="Reason:").pack(anchor=tk.NW)
        self.reason_text = tk.Text(row, wrap=tk.WORD, height=6, font=('Consolas', 10))
        self.reason_text.pack(fill=tk.BOTH, expand=True, pady=(4, 0))
        self.reason_text.bind('<KeyRelease>', lambda e: self._mark_unsaved())

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT)

    def _load_entries(self):
        """Load all bans."""
        entries = self.repository.list_all()

        self.tree.delete(*self.tree.get_children())
        for b in entries:
            reason_preview = (b.get('reason', '') or '')[:30]
            self.tree.insert('', tk.END, iid=str(b['id']),
                           values=(b['id'], b['name'], reason_preview))

        self.count_var.set(f"{len(entries)} bans")
        self.on_status(f"Loaded {len(entries)} bans")

    def _on_select(self, event):
        """Handle selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved", "Discard changes?"):
                if self.current_id:
                    self.tree.selection_set(str(self.current_id))
                return

        selection = self.tree.selection()
        if selection:
            self._load_entry(int(selection[0]))

    def _load_entry(self, id_val: int):
        """Load a ban into the editor."""
        b = self.repository.get_by_id(id_val)
        if not b:
            return

        self.current_id = id_val
        self.name_var.set(b.get('name', ''))
        self.reason_text.delete('1.0', tk.END)
        self.reason_text.insert('1.0', b.get('reason', ''))

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current ban."""
        if self.current_id is None:
            return

        data = {
            'name': self.name_var.get(),
            'reason': self.reason_text.get('1.0', tk.END).rstrip('\n'),
        }

        self.repository.update(self.current_id, data)
        self.unsaved = False
        self._load_entries()
        self.tree.selection_set(str(self.current_id))
        self.on_status(f"Saved ban {self.current_id}")

    def _new(self):
        """Create a new ban."""
        name = simpledialog.askstring("New Ban", "Enter name or site to ban:", parent=self)
        if not name:
            return

        self.repository.insert({'name': name, 'reason': ''})
        self._load_entries()
        self.on_status(f"Added ban for {name}")

    def _delete(self):
        """Delete current ban."""
        if self.current_id is None:
            return

        if not messagebox.askyesno("Confirm", "Delete this ban?"):
            return

        self.repository.delete(self.current_id)
        self.current_id = None
        self._clear_editor()
        self._load_entries()
        self.on_status("Ban deleted")

    def _clear_editor(self):
        self.current_id = None
        self.name_var.set("")
        self.reason_text.delete('1.0', tk.END)
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved", "Discard changes?")
        return True


class DisabledCommandsPanel(ttk.Frame):
    """Editor panel for disabled commands."""

    def __init__(
        self,
        parent,
        repository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.current_cmd: Optional[str] = None
        self.unsaved = False

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: command list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('command', 'level', 'disabled_by'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('command', text='Command')
        self.tree.heading('level', text='Level')
        self.tree.heading('disabled_by', text='Disabled By')
        self.tree.column('command', width=120)
        self.tree.column('level', width=60)
        self.tree.column('disabled_by', width=100)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 disabled")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor
        right_frame = ttk.LabelFrame(paned, text="Edit Disabled Command")
        paned.add(right_frame, weight=1)

        # Command
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Command:", width=12).pack(side=tk.LEFT)
        self.cmd_label = ttk.Label(row, text="-", font=('Consolas', 10, 'bold'))
        self.cmd_label.pack(side=tk.LEFT)

        # Level
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Level:", width=12).pack(side=tk.LEFT)
        self.level_var = tk.IntVar(value=0)
        ttk.Spinbox(row, from_=-1, to=12, width=6, textvariable=self.level_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT)
        ttk.Label(row, text="(-1=all, 0=mortals, 12=implementor)", foreground='gray').pack(side=tk.LEFT, padx=8)

        # Disabled by
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Disabled By:", width=12).pack(side=tk.LEFT)
        self.disabled_by_var = tk.StringVar()
        self.disabled_by_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.disabled_by_var, width=20).pack(side=tk.LEFT)

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete (Re-enable)", command=self._delete).pack(side=tk.LEFT)

    def _load_entries(self):
        """Load all disabled commands."""
        entries = self.repository.list_all()

        self.tree.delete(*self.tree.get_children())
        for c in entries:
            self.tree.insert('', tk.END, iid=c['command_name'],
                           values=(c['command_name'], c['level'], c['disabled_by']))

        self.count_var.set(f"{len(entries)} disabled")
        self.on_status(f"Loaded {len(entries)} disabled commands")

    def _on_select(self, event):
        """Handle selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved", "Discard changes?"):
                if self.current_cmd:
                    self.tree.selection_set(self.current_cmd)
                return

        selection = self.tree.selection()
        if selection:
            self._load_entry(selection[0])

    def _load_entry(self, cmd: str):
        """Load a command into the editor."""
        c = self.repository.get_by_id(cmd)
        if not c:
            return

        self.current_cmd = cmd
        self.cmd_label.configure(text=cmd)
        self.level_var.set(c.get('level', 0))
        self.disabled_by_var.set(c.get('disabled_by', ''))

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current entry."""
        if self.current_cmd is None:
            return

        data = {
            'level': self.level_var.get(),
            'disabled_by': self.disabled_by_var.get(),
        }

        self.repository.update(self.current_cmd, data)
        self.unsaved = False
        self._load_entries()
        self.tree.selection_set(self.current_cmd)
        self.on_status(f"Saved {self.current_cmd}")

    def _new(self):
        """Disable a new command."""
        cmd = simpledialog.askstring("Disable Command", "Enter command name to disable:", parent=self)
        if not cmd:
            return

        cmd = cmd.strip().lower()
        if self.repository.get_by_id(cmd):
            messagebox.showerror("Error", f"Command '{cmd}' is already disabled.")
            return

        admin = simpledialog.askstring("Disabled By", "Your admin name:", parent=self)
        if not admin:
            admin = "Unknown"

        self.repository.insert({
            'command_name': cmd,
            'level': 0,
            'disabled_by': admin
        })
        self._load_entries()
        self.tree.selection_set(cmd)
        self._load_entry(cmd)
        self.on_status(f"Disabled command: {cmd}")

    def _delete(self):
        """Re-enable (delete) command."""
        if self.current_cmd is None:
            return

        if not messagebox.askyesno("Confirm", f"Re-enable command '{self.current_cmd}'?"):
            return

        self.repository.delete(self.current_cmd)
        self.current_cmd = None
        self._clear_editor()
        self._load_entries()
        self.on_status("Command re-enabled")

    def _clear_editor(self):
        self.current_cmd = None
        self.cmd_label.configure(text="-")
        self.level_var.set(0)
        self.disabled_by_var.set("")
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved", "Discard changes?")
        return True


class LeaderboardPanel(ttk.Frame):
    """Editor panel for topboard and leaderboard."""

    def __init__(
        self,
        parent,
        topboard_repo,
        leaderboard_repo,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.topboard_repo = topboard_repo
        self.leaderboard_repo = leaderboard_repo
        self.on_status = on_status or (lambda msg: None)

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        notebook = ttk.Notebook(self)
        notebook.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Top Board tab
        top_frame = ttk.Frame(notebook)
        notebook.add(top_frame, text="Top PK Board")

        self.top_tree = ttk.Treeview(
            top_frame,
            columns=('rank', 'name', 'score'),
            show='headings',
            selectmode='browse'
        )
        self.top_tree.heading('rank', text='Rank')
        self.top_tree.heading('name', text='Player')
        self.top_tree.heading('score', text='PK Score')
        self.top_tree.column('rank', width=60)
        self.top_tree.column('name', width=150)
        self.top_tree.column('score', width=100)

        top_scroll = ttk.Scrollbar(top_frame, orient=tk.VERTICAL, command=self.top_tree.yview)
        self.top_tree.configure(yscrollcommand=top_scroll.set)
        self.top_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        top_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        # Leaderboard tab
        leader_frame = ttk.Frame(notebook)
        notebook.add(leader_frame, text="Leaderboards")

        self.leader_tree = ttk.Treeview(
            leader_frame,
            columns=('category', 'name', 'value'),
            show='headings',
            selectmode='browse'
        )
        self.leader_tree.heading('category', text='Category')
        self.leader_tree.heading('name', text='Leader')
        self.leader_tree.heading('value', text='Value')
        self.leader_tree.column('category', width=100)
        self.leader_tree.column('name', width=150)
        self.leader_tree.column('value', width=100)

        leader_scroll = ttk.Scrollbar(leader_frame, orient=tk.VERTICAL, command=self.leader_tree.yview)
        self.leader_tree.configure(yscrollcommand=leader_scroll.set)
        self.leader_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        leader_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        # Category labels
        categories = {
            'bestpk': 'Best PK Score',
            'pk': 'Most Player Kills',
            'pd': 'Most Player Deaths',
            'mk': 'Most Mob Kills',
            'md': 'Most Mob Deaths',
            'tt': 'Most Time Played',
            'qc': 'Most Quest Points',
        }
        self._category_labels = categories

        # Reload button
        btn_frame = ttk.Frame(self)
        btn_frame.pack(fill=tk.X, padx=8, pady=4)
        ttk.Button(btn_frame, text="Reload", command=self._load_entries).pack(side=tk.LEFT)
        ttk.Label(btn_frame, text="(Leaderboards are updated by the game server)",
                 foreground='gray').pack(side=tk.LEFT, padx=8)

    def _load_entries(self):
        """Load all leaderboard data."""
        # Top board
        self.top_tree.delete(*self.top_tree.get_children())
        for entry in self.topboard_repo.list_all():
            self.top_tree.insert('', tk.END,
                               values=(entry['rank'], entry['name'], entry['pkscore']))

        # Leaderboard
        self.leader_tree.delete(*self.leader_tree.get_children())
        for entry in self.leaderboard_repo.list_all():
            cat = entry['category']
            label = self._category_labels.get(cat, cat)
            self.leader_tree.insert('', tk.END,
                                   values=(label, entry['name'], entry['value']))

        self.on_status("Loaded leaderboard data")

    def check_unsaved(self) -> bool:
        return True


class NotesPanel(ttk.Frame):
    """Editor panel for notes/boards."""

    # Board names from the game
    BOARDS = {
        0: 'Changes',
        1: 'General',
        2: 'Admin',
        3: 'Story',
    }

    def __init__(
        self,
        parent,
        repository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.current_id: Optional[int] = None

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        # Top: board filter
        filter_frame = ttk.Frame(self)
        filter_frame.pack(fill=tk.X, padx=4, pady=4)
        ttk.Label(filter_frame, text="Board:").pack(side=tk.LEFT)
        self.board_var = tk.StringVar(value="All")
        board_combo = ttk.Combobox(
            filter_frame,
            textvariable=self.board_var,
            values=["All"] + list(self.BOARDS.values()),
            state='readonly',
            width=15
        )
        board_combo.pack(side=tk.LEFT, padx=4)
        board_combo.bind('<<ComboboxSelected>>', lambda e: self._load_entries())

        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: note list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('id', 'board', 'sender', 'subject', 'date'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('id', text='ID')
        self.tree.heading('board', text='Board')
        self.tree.heading('sender', text='From')
        self.tree.heading('subject', text='Subject')
        self.tree.heading('date', text='Date')
        self.tree.column('id', width=40, stretch=False)
        self.tree.column('board', width=80)
        self.tree.column('sender', width=80)
        self.tree.column('subject', width=150)
        self.tree.column('date', width=100)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 notes")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: viewer
        right_frame = ttk.LabelFrame(paned, text="Note")
        paned.add(right_frame, weight=2)

        # Header info
        header = ttk.Frame(right_frame)
        header.pack(fill=tk.X, padx=8, pady=4)

        self.header_label = ttk.Label(header, text="Select a note to view", foreground='gray')
        self.header_label.pack(anchor=tk.W)

        # Note text
        self.note_text = tk.Text(right_frame, wrap=tk.WORD, height=15, font=('Consolas', 10), state='disabled')
        self.note_text.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=4)
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT)

    def _load_entries(self):
        """Load notes based on filter."""
        board_filter = self.board_var.get()

        if board_filter == "All":
            entries = self.repository.list_all()
        else:
            # Find board index
            board_idx = None
            for idx, name in self.BOARDS.items():
                if name == board_filter:
                    board_idx = idx
                    break
            if board_idx is not None:
                entries = self.repository.get_by_board(board_idx)
            else:
                entries = []

        self.tree.delete(*self.tree.get_children())
        for n in entries:
            board_name = self.BOARDS.get(n['board_idx'], f"Board {n['board_idx']}")
            self.tree.insert('', tk.END, iid=str(n['id']),
                           values=(n['id'], board_name, n['sender'], n['subject'], n['date']))

        self.count_var.set(f"{len(entries)} notes")
        self.on_status(f"Loaded {len(entries)} notes")

    def _on_select(self, event):
        """Handle selection."""
        selection = self.tree.selection()
        if selection:
            self._load_entry(int(selection[0]))

    def _load_entry(self, id_val: int):
        """Load a note into the viewer."""
        n = self.repository.get_by_id(id_val)
        if not n:
            return

        self.current_id = id_val
        board_name = self.BOARDS.get(n['board_idx'], f"Board {n['board_idx']}")
        self.header_label.configure(
            text=f"From: {n['sender']}  |  To: {n['to_list']}  |  Board: {board_name}  |  Date: {n['date']}",
            foreground='black'
        )

        self.note_text.configure(state='normal')
        self.note_text.delete('1.0', tk.END)
        self.note_text.insert('1.0', f"Subject: {n['subject']}\n\n{n['text']}")
        self.note_text.configure(state='disabled')

    def _delete(self):
        """Delete current note."""
        if self.current_id is None:
            return

        if not messagebox.askyesno("Confirm", "Delete this note?"):
            return

        self.repository.delete(self.current_id)
        self.current_id = None
        self.header_label.configure(text="Select a note to view", foreground='gray')
        self.note_text.configure(state='normal')
        self.note_text.delete('1.0', tk.END)
        self.note_text.configure(state='disabled')
        self._load_entries()
        self.on_status("Note deleted")

    def check_unsaved(self) -> bool:
        return True


class BugsPanel(ttk.Frame):
    """Viewer panel for bug reports (read-only)."""

    def __init__(
        self,
        parent,
        repository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.current_id: Optional[int] = None

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: bug list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        # Search
        search_frame = ttk.Frame(left_frame)
        search_frame.pack(fill=tk.X, padx=2, pady=2)
        ttk.Label(search_frame, text="Filter:").pack(side=tk.LEFT)
        self.search_var = tk.StringVar()
        self.search_var.trace_add('write', lambda *_: self._filter_list())
        ttk.Entry(search_frame, textvariable=self.search_var).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(4, 0))

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('id', 'player', 'room', 'date'),
            show='headings',
            selectmode='extended'
        )
        self.tree.heading('id', text='ID')
        self.tree.heading('player', text='Player')
        self.tree.heading('room', text='Room')
        self.tree.heading('date', text='Date')
        self.tree.column('id', width=50, stretch=False)
        self.tree.column('player', width=100)
        self.tree.column('room', width=60)
        self.tree.column('date', width=100)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 bugs")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: viewer
        right_frame = ttk.LabelFrame(paned, text="Bug Report")
        paned.add(right_frame, weight=2)

        # Header
        self.header_label = ttk.Label(right_frame, text="Select a bug report to view", foreground='gray')
        self.header_label.pack(anchor=tk.W, padx=8, pady=4)

        # Message
        self.bug_text = tk.Text(right_frame, wrap=tk.WORD, height=15, font=('Consolas', 10), state='disabled')
        self.bug_text.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=4)
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT)
        ttk.Button(btn_frame, text="Refresh", command=self._load_entries).pack(side=tk.LEFT, padx=(4, 0))

        # Bulk selection buttons at bottom of left panel
        bulk_frame = ttk.Frame(left_frame)
        bulk_frame.pack(fill=tk.X, padx=2, pady=4)
        ttk.Button(bulk_frame, text="Select All", command=self._select_all).pack(side=tk.LEFT)
        ttk.Button(bulk_frame, text="Deselect All", command=self._deselect_all).pack(side=tk.LEFT, padx=(4, 0))
        ttk.Button(bulk_frame, text="Delete Selected", command=self._delete_selected).pack(side=tk.LEFT, padx=(4, 0))

        # Store all entries for filtering
        self._all_entries = []

    def _load_entries(self):
        """Load all bug reports."""
        self._all_entries = self.repository.list_all()
        self._populate_tree(self._all_entries)
        self.on_status(f"Loaded {len(self._all_entries)} bug reports")

    def _populate_tree(self, entries):
        """Populate tree with entries."""
        self.tree.delete(*self.tree.get_children())
        for b in entries:
            ts = b.get('timestamp', 0)
            date_str = datetime.fromtimestamp(ts).strftime('%Y-%m-%d') if ts else 'Unknown'
            self.tree.insert('', tk.END, iid=str(b['id']),
                           values=(b['id'], b.get('player', ''), b.get('room_vnum', 0), date_str))

        self.count_var.set(f"{len(entries)} bugs")

    def _filter_list(self):
        """Filter based on search term."""
        term = self.search_var.get().lower()
        if not term:
            self._populate_tree(self._all_entries)
        else:
            filtered = [
                e for e in self._all_entries
                if term in e.get('player', '').lower() or term in e.get('message', '').lower()
            ]
            self._populate_tree(filtered)

    def _on_select(self, event):
        """Handle selection."""
        selection = self.tree.selection()
        if selection:
            self._load_entry(int(selection[0]))

    def _load_entry(self, id_val: int):
        """Load a bug report into the viewer."""
        b = self.repository.get_by_id(id_val)
        if not b:
            return

        self.current_id = id_val
        ts = b.get('timestamp', 0)
        date_str = datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S') if ts else 'Unknown'
        self.header_label.configure(
            text=f"From: {b.get('player', 'Unknown')}  |  Room: {b.get('room_vnum', 0)}  |  Date: {date_str}",
            foreground='black'
        )

        self.bug_text.configure(state='normal')
        self.bug_text.delete('1.0', tk.END)
        self.bug_text.insert('1.0', b.get('message', ''))
        self.bug_text.configure(state='disabled')

    def _delete(self):
        """Delete current bug report."""
        if self.current_id is None:
            return

        if not messagebox.askyesno("Confirm", "Delete this bug report?"):
            return

        self.repository.delete(self.current_id)
        self.current_id = None
        self.header_label.configure(text="Select a bug report to view", foreground='gray')
        self.bug_text.configure(state='normal')
        self.bug_text.delete('1.0', tk.END)
        self.bug_text.configure(state='disabled')
        self._load_entries()
        self.on_status("Bug report deleted")

    def _select_all(self):
        """Select all visible bugs in the tree."""
        children = self.tree.get_children()
        if children:
            self.tree.selection_set(children)
            self.on_status(f"Selected {len(children)} bug reports")

    def _deselect_all(self):
        """Deselect all bugs."""
        self.tree.selection_remove(self.tree.selection())
        self.on_status("Selection cleared")

    def _delete_selected(self):
        """Delete all selected bug reports."""
        selection = self.tree.selection()
        if not selection:
            messagebox.showinfo("Info", "No bugs selected.")
            return

        count = len(selection)
        if not messagebox.askyesno("Confirm Bulk Delete",
                                   f"Delete {count} bug report(s)?\n\nThis cannot be undone."):
            return

        # Delete all selected bugs
        ids_to_delete = [int(iid) for iid in selection]
        deleted = self.repository.delete_many(ids_to_delete)

        self.current_id = None
        self.header_label.configure(text="Select a bug report to view", foreground='gray')
        self.bug_text.configure(state='normal')
        self.bug_text.delete('1.0', tk.END)
        self.bug_text.configure(state='disabled')
        self._load_entries()
        self.on_status(f"Deleted {deleted} bug reports")

    def check_unsaved(self) -> bool:
        return True


class SuperAdminsPanel(ttk.Frame):
    """Editor panel for super admins."""

    def __init__(
        self,
        parent,
        repository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        # Description
        desc_label = ttk.Label(
            self,
            text="Super Admins have elevated privileges and bypass certain restrictions.\n"
                 "Add player names here to grant super admin status.",
            foreground='gray'
        )
        desc_label.pack(anchor=tk.W, padx=8, pady=(8, 4))

        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: admin list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('name',),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('name', text='Player Name')
        self.tree.column('name', width=200)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.count_var = tk.StringVar(value="0 super admins")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Buttons
        btn_frame = ttk.Frame(left_frame)
        btn_frame.pack(fill=tk.X, padx=4, pady=8)
        ttk.Button(btn_frame, text="Add", command=self._add).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Remove", command=self._remove).pack(side=tk.LEFT)

    def _load_entries(self):
        """Load all super admins."""
        entries = self.repository.list_all()

        self.tree.delete(*self.tree.get_children())
        for a in entries:
            self.tree.insert('', tk.END, iid=a['name'], values=(a['name'],))

        self.count_var.set(f"{len(entries)} super admins")
        self.on_status(f"Loaded {len(entries)} super admins")

    def _add(self):
        """Add a new super admin."""
        name = simpledialog.askstring("Add Super Admin", "Enter player name:", parent=self)
        if not name:
            return

        name = name.strip()
        if not name:
            return

        if self.repository.exists(name):
            messagebox.showinfo("Info", f"'{name}' is already a super admin.")
            return

        self.repository.insert({'name': name})
        self._load_entries()
        self.on_status(f"Added super admin: {name}")

    def _remove(self):
        """Remove selected super admin."""
        selection = self.tree.selection()
        if not selection:
            messagebox.showinfo("Info", "Select a super admin to remove.")
            return

        name = selection[0]
        if not messagebox.askyesno("Confirm", f"Remove '{name}' from super admins?"):
            return

        self.repository.delete(name)
        self._load_entries()
        self.on_status(f"Removed super admin: {name}")

    def check_unsaved(self) -> bool:
        return True


class ImmortalPretitlesPanel(ttk.Frame):
    """Editor panel for immortal pretitles (displayed in who list)."""

    def __init__(
        self,
        parent,
        repository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.current_name: Optional[str] = None
        self.unsaved = False

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        # Description
        desc_label = ttk.Label(
            self,
            text="Immortal Pretitles are displayed before immortal names in the WHO list.\n"
                 "Example: '[Builder] Gandalf' or '[Head Admin] Merlin'",
            foreground='gray'
        )
        desc_label.pack(anchor=tk.W, padx=8, pady=(8, 4))

        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: pretitle list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('name', 'pretitle', 'set_by'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('name', text='Immortal')
        self.tree.heading('pretitle', text='Pretitle')
        self.tree.heading('set_by', text='Set By')
        self.tree.column('name', width=100)
        self.tree.column('pretitle', width=150)
        self.tree.column('set_by', width=100)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 pretitles")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor
        right_frame = ttk.LabelFrame(paned, text="Edit Pretitle")
        paned.add(right_frame, weight=1)

        # Immortal name
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Immortal:", width=12).pack(side=tk.LEFT)
        self.name_label = ttk.Label(row, text="-", font=('Consolas', 10, 'bold'))
        self.name_label.pack(side=tk.LEFT)

        # Pretitle
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Pretitle:", width=12).pack(side=tk.LEFT)
        self.pretitle_var = tk.StringVar()
        self.pretitle_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.pretitle_var, width=30).pack(side=tk.LEFT)

        # Set by (read-only info)
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Set By:", width=12).pack(side=tk.LEFT)
        self.setby_label = ttk.Label(row, text="-", foreground='gray')
        self.setby_label.pack(side=tk.LEFT)

        # Set date (read-only info)
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Set Date:", width=12).pack(side=tk.LEFT)
        self.setdate_label = ttk.Label(row, text="-", foreground='gray')
        self.setdate_label.pack(side=tk.LEFT)

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT)

    def _load_entries(self):
        """Load all immortal pretitles."""
        entries = self.repository.list_all()

        self.tree.delete(*self.tree.get_children())
        for p in entries:
            self.tree.insert('', tk.END, iid=p['immortal_name'],
                           values=(p['immortal_name'], p['pretitle'], p['set_by']))

        self.count_var.set(f"{len(entries)} pretitles")
        self.on_status(f"Loaded {len(entries)} immortal pretitles")

    def _on_select(self, event):
        """Handle selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved", "Discard changes?"):
                if self.current_name:
                    self.tree.selection_set(self.current_name)
                return

        selection = self.tree.selection()
        if selection:
            self._load_entry(selection[0])

    def _load_entry(self, name: str):
        """Load a pretitle into the editor."""
        p = self.repository.get_by_id(name)
        if not p:
            return

        self.current_name = name
        self.name_label.configure(text=name)
        self.pretitle_var.set(p.get('pretitle', ''))
        self.setby_label.configure(text=p.get('set_by', '-'))

        set_date = p.get('set_date', 0)
        if set_date:
            date_str = datetime.fromtimestamp(set_date).strftime('%Y-%m-%d %H:%M:%S')
        else:
            date_str = '-'
        self.setdate_label.configure(text=date_str)

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current pretitle."""
        if self.current_name is None:
            return

        pretitle = self.pretitle_var.get().strip()
        if not pretitle:
            messagebox.showerror("Error", "Pretitle cannot be empty.")
            return

        data = {
            'pretitle': pretitle,
            'set_by': 'Editor',
            'set_date': int(datetime.now().timestamp()),
        }

        self.repository.update(self.current_name, data)
        self.unsaved = False
        self._load_entries()
        self.tree.selection_set(self.current_name)
        self.on_status(f"Saved pretitle for {self.current_name}")

    def _new(self):
        """Create a new pretitle."""
        name = simpledialog.askstring("New Pretitle", "Enter immortal name:", parent=self)
        if not name:
            return

        name = name.strip()
        if not name:
            return

        if self.repository.get_by_id(name):
            messagebox.showinfo("Info", f"Pretitle for '{name}' already exists. Select it to edit.")
            return

        pretitle = simpledialog.askstring("New Pretitle", "Enter pretitle text:", parent=self)
        if not pretitle:
            return

        self.repository.insert({
            'immortal_name': name,
            'pretitle': pretitle.strip(),
            'set_by': 'Editor',
            'set_date': int(datetime.now().timestamp()),
        })
        self._load_entries()
        self.tree.selection_set(name)
        self._load_entry(name)
        self.on_status(f"Added pretitle for {name}")

    def _delete(self):
        """Delete current pretitle."""
        if self.current_name is None:
            return

        if not messagebox.askyesno("Confirm", f"Delete pretitle for '{self.current_name}'?"):
            return

        self.repository.delete(self.current_name)
        self.current_name = None
        self._clear_editor()
        self._load_entries()
        self.on_status("Pretitle deleted")

    def _clear_editor(self):
        self.current_name = None
        self.name_label.configure(text="-")
        self.pretitle_var.set("")
        self.setby_label.configure(text="-")
        self.setdate_label.configure(text="-")
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved", "Discard changes?")
        return True


class ForbiddenNamesPanel(ttk.Frame):
    """Editor panel for forbidden character names."""

    NAMETYPES = {0: 'Reserved', 1: 'Protected', 2: 'Blocked'}

    def __init__(
        self,
        parent,
        repository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.current_id: Optional[int] = None
        self.unsaved = False

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: name list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('id', 'name', 'type', 'added_by'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('id', text='ID')
        self.tree.heading('name', text='Name')
        self.tree.heading('type', text='Type')
        self.tree.heading('added_by', text='Added By')
        self.tree.column('id', width=40, stretch=False)
        self.tree.column('name', width=150)
        self.tree.column('type', width=80)
        self.tree.column('added_by', width=80)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 entries")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor
        right_frame = ttk.LabelFrame(paned, text="Edit Forbidden Name")
        paned.add(right_frame, weight=1)

        # Name
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Name:", width=12).pack(side=tk.LEFT)
        self.name_var = tk.StringVar()
        self.name_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.name_var, width=30).pack(side=tk.LEFT)

        # Type
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Type:", width=12).pack(side=tk.LEFT)
        self.type_var = tk.StringVar()
        self.type_combo = ttk.Combobox(
            row, textvariable=self.type_var, width=15, state='readonly',
            values=['Reserved', 'Protected', 'Blocked']
        )
        self.type_combo.pack(side=tk.LEFT)
        self.type_combo.bind('<<ComboboxSelected>>', lambda e: self._mark_unsaved())

        # Type descriptions
        desc_frame = ttk.Frame(right_frame)
        desc_frame.pack(fill=tk.X, padx=8, pady=(0, 4))
        ttk.Label(
            desc_frame,
            text="Reserved = exact match block\n"
                 "Protected = contains-block (exact allowed)\n"
                 "Blocked = exact name block",
            foreground='gray', font=('Consolas', 9), justify=tk.LEFT
        ).pack(anchor=tk.W, padx=12)

        # Added By
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Added By:", width=12).pack(side=tk.LEFT)
        self.added_by_var = tk.StringVar()
        self.added_by_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.added_by_var, width=20).pack(side=tk.LEFT)

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT)

    def _load_entries(self):
        """Load all forbidden names."""
        entries = self.repository.list_all()

        self.tree.delete(*self.tree.get_children())
        for e in entries:
            type_label = self.NAMETYPES.get(e.get('type', 0), '?')
            self.tree.insert('', tk.END, iid=str(e['id']),
                           values=(e['id'], e['name'], type_label, e.get('added_by', '')))

        self.count_var.set(f"{len(entries)} entries")
        self.on_status(f"Loaded {len(entries)} forbidden names")

    def _on_select(self, event):
        """Handle selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved", "Discard changes?"):
                if self.current_id:
                    self.tree.selection_set(str(self.current_id))
                return

        selection = self.tree.selection()
        if selection:
            self._load_entry(int(selection[0]))

    def _load_entry(self, id_val: int):
        """Load an entry into the editor."""
        e = self.repository.get_by_id(id_val)
        if not e:
            return

        self.current_id = id_val
        self.name_var.set(e.get('name', ''))
        type_label = self.NAMETYPES.get(e.get('type', 0), 'Reserved')
        self.type_var.set(type_label)
        self.added_by_var.set(e.get('added_by', ''))

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current entry."""
        if self.current_id is None:
            return

        # Reverse map type label to integer
        type_map = {v: k for k, v in self.NAMETYPES.items()}
        data = {
            'name': self.name_var.get(),
            'type': type_map.get(self.type_var.get(), 0),
            'added_by': self.added_by_var.get(),
        }

        self.repository.update(self.current_id, data)
        self.unsaved = False
        self._load_entries()
        self.tree.selection_set(str(self.current_id))
        self.on_status(f"Saved forbidden name {self.current_id}")

    def _new(self):
        """Create a new forbidden name."""
        name = simpledialog.askstring("New Forbidden Name", "Enter name to forbid:", parent=self)
        if not name:
            return

        self.repository.insert({'name': name, 'type': 0, 'added_by': 'editor'})
        self._load_entries()
        self.on_status(f"Added forbidden name: {name}")

    def _delete(self):
        """Delete current entry."""
        if self.current_id is None:
            return

        if not messagebox.askyesno("Confirm", "Delete this forbidden name?"):
            return

        self.repository.delete(self.current_id)
        self.current_id = None
        self._clear_editor()
        self._load_entries()
        self.on_status("Forbidden name deleted")

    def _clear_editor(self):
        self.current_id = None
        self.name_var.set("")
        self.type_var.set("")
        self.added_by_var.set("")
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved", "Discard changes?")
        return True


class ProfanityFiltersPanel(ttk.Frame):
    """Editor panel for profanity filter patterns."""

    def __init__(
        self,
        parent,
        repository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.current_id: Optional[int] = None
        self.unsaved = False

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: pattern list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('id', 'pattern', 'added_by'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('id', text='ID')
        self.tree.heading('pattern', text='Pattern')
        self.tree.heading('added_by', text='Added By')
        self.tree.column('id', width=40, stretch=False)
        self.tree.column('pattern', width=200)
        self.tree.column('added_by', width=100)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 patterns")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor
        right_frame = ttk.LabelFrame(paned, text="Edit Profanity Filter")
        paned.add(right_frame, weight=1)

        # Pattern
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Pattern:", width=12).pack(side=tk.LEFT)
        self.pattern_var = tk.StringVar()
        self.pattern_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.pattern_var, width=30).pack(side=tk.LEFT, fill=tk.X, expand=True)

        # Description
        desc_frame = ttk.Frame(right_frame)
        desc_frame.pack(fill=tk.X, padx=8, pady=(0, 4))
        ttk.Label(
            desc_frame,
            text="Substring patterns matched against player names and chat.\n"
                 "Confusable character normalization is applied before matching.",
            foreground='gray', font=('Consolas', 9), justify=tk.LEFT
        ).pack(anchor=tk.W, padx=12)

        # Added By
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Added By:", width=12).pack(side=tk.LEFT)
        self.added_by_var = tk.StringVar()
        self.added_by_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.added_by_var, width=20).pack(side=tk.LEFT)

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT)

    def _load_entries(self):
        """Load all profanity filters."""
        entries = self.repository.list_all()

        self.tree.delete(*self.tree.get_children())
        for e in entries:
            self.tree.insert('', tk.END, iid=str(e['id']),
                           values=(e['id'], e['pattern'], e.get('added_by', '')))

        self.count_var.set(f"{len(entries)} patterns")
        self.on_status(f"Loaded {len(entries)} profanity filters")

    def _on_select(self, event):
        """Handle selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved", "Discard changes?"):
                if self.current_id:
                    self.tree.selection_set(str(self.current_id))
                return

        selection = self.tree.selection()
        if selection:
            self._load_entry(int(selection[0]))

    def _load_entry(self, id_val: int):
        """Load an entry into the editor."""
        e = self.repository.get_by_id(id_val)
        if not e:
            return

        self.current_id = id_val
        self.pattern_var.set(e.get('pattern', ''))
        self.added_by_var.set(e.get('added_by', ''))

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current filter."""
        if self.current_id is None:
            return

        data = {
            'pattern': self.pattern_var.get(),
            'added_by': self.added_by_var.get(),
        }

        self.repository.update(self.current_id, data)
        self.unsaved = False
        self._load_entries()
        self.tree.selection_set(str(self.current_id))
        self.on_status(f"Saved profanity filter {self.current_id}")

    def _new(self):
        """Create a new profanity filter."""
        pattern = simpledialog.askstring("New Filter", "Enter profanity pattern:", parent=self)
        if not pattern:
            return

        self.repository.insert({'pattern': pattern, 'added_by': 'editor'})
        self._load_entries()
        self.on_status(f"Added profanity filter: {pattern}")

    def _delete(self):
        """Delete current filter."""
        if self.current_id is None:
            return

        if not messagebox.askyesno("Confirm", "Delete this profanity filter?"):
            return

        self.repository.delete(self.current_id)
        self.current_id = None
        self._clear_editor()
        self._load_entries()
        self.on_status("Profanity filter deleted")

    def _clear_editor(self):
        self.current_id = None
        self.pattern_var.set("")
        self.added_by_var.set("")
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved", "Discard changes?")
        return True


class ConfusableCharsPanel(ttk.Frame):
    """Editor panel for Unicode confusable character mappings."""

    def __init__(
        self,
        parent,
        repository,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.repository = repository
        self.on_status = on_status or (lambda msg: None)

        self.current_id: Optional[int] = None
        self.unsaved = False

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: character list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('id', 'codepoint', 'char', 'canonical'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('id', text='ID')
        self.tree.heading('codepoint', text='Codepoint')
        self.tree.heading('char', text='Char')
        self.tree.heading('canonical', text='Canonical')
        self.tree.column('id', width=40, stretch=False)
        self.tree.column('codepoint', width=80)
        self.tree.column('char', width=50, anchor=tk.CENTER)
        self.tree.column('canonical', width=60, anchor=tk.CENTER)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 entries")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor
        right_frame = ttk.LabelFrame(paned, text="Edit Confusable Character")
        paned.add(right_frame, weight=1)

        # Codepoint (hex)
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Codepoint:", width=12).pack(side=tk.LEFT)
        self.codepoint_var = tk.StringVar()
        self.codepoint_var.trace_add('write', lambda *_: self._on_codepoint_change())
        ttk.Entry(row, textvariable=self.codepoint_var, width=10).pack(side=tk.LEFT)
        ttk.Label(row, text="  (hex, e.g. 0441)").pack(side=tk.LEFT)

        # Character preview
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Preview:", width=12).pack(side=tk.LEFT)
        self.preview_label = ttk.Label(row, text="-", font=('Consolas', 14))
        self.preview_label.pack(side=tk.LEFT)

        # Canonical
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Canonical:", width=12).pack(side=tk.LEFT)
        self.canonical_var = tk.StringVar()
        self.canonical_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.canonical_var, width=5).pack(side=tk.LEFT)
        ttk.Label(row, text="  (ASCII equivalent)").pack(side=tk.LEFT)

        # Description
        desc_frame = ttk.Frame(right_frame)
        desc_frame.pack(fill=tk.X, padx=8, pady=(8, 4))
        ttk.Label(
            desc_frame,
            text="Maps Unicode lookalike characters to their ASCII equivalent\n"
                 "to prevent profanity filter bypass via homoglyph substitution.",
            foreground='gray', font=('Consolas', 9), justify=tk.LEFT
        ).pack(anchor=tk.W, padx=12)

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT)

    def _codepoint_to_char(self, cp: int) -> str:
        """Safely convert codepoint to character."""
        try:
            return chr(cp)
        except (ValueError, OverflowError):
            return '?'

    def _load_entries(self):
        """Load all confusable characters."""
        entries = self.repository.list_all()

        self.tree.delete(*self.tree.get_children())
        for e in entries:
            cp = e.get('codepoint', 0)
            cp_hex = f"U+{cp:04X}"
            char_preview = self._codepoint_to_char(cp)
            self.tree.insert('', tk.END, iid=str(e['id']),
                           values=(e['id'], cp_hex, char_preview, e.get('canonical', '')))

        self.count_var.set(f"{len(entries)} entries")
        self.on_status(f"Loaded {len(entries)} confusable characters")

    def _on_select(self, event):
        """Handle selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved", "Discard changes?"):
                if self.current_id:
                    self.tree.selection_set(str(self.current_id))
                return

        selection = self.tree.selection()
        if selection:
            self._load_entry(int(selection[0]))

    def _load_entry(self, id_val: int):
        """Load an entry into the editor."""
        e = self.repository.get_by_id(id_val)
        if not e:
            return

        self.current_id = id_val
        cp = e.get('codepoint', 0)
        self.codepoint_var.set(f"{cp:04X}")
        self.canonical_var.set(e.get('canonical', ''))
        self.preview_label.configure(text=self._codepoint_to_char(cp))

        self.unsaved = False

    def _on_codepoint_change(self):
        """Update preview when codepoint changes."""
        self._mark_unsaved()
        try:
            cp = int(self.codepoint_var.get(), 16)
            self.preview_label.configure(text=self._codepoint_to_char(cp))
        except ValueError:
            self.preview_label.configure(text="?")

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current entry."""
        if self.current_id is None:
            return

        try:
            cp = int(self.codepoint_var.get(), 16)
        except ValueError:
            messagebox.showerror("Error", "Invalid codepoint (must be hex)")
            return

        data = {
            'codepoint': cp,
            'canonical': self.canonical_var.get(),
        }

        self.repository.update(self.current_id, data)
        self.unsaved = False
        self._load_entries()
        self.tree.selection_set(str(self.current_id))
        self.on_status(f"Saved confusable character {self.current_id}")

    def _new(self):
        """Create a new confusable character."""
        cp_str = simpledialog.askstring(
            "New Confusable", "Enter Unicode codepoint (hex, e.g. 0441):", parent=self)
        if not cp_str:
            return

        try:
            cp = int(cp_str, 16)
        except ValueError:
            messagebox.showerror("Error", "Invalid hex codepoint")
            return

        canonical = simpledialog.askstring(
            "Canonical", f"Enter ASCII canonical for U+{cp:04X} ({self._codepoint_to_char(cp)}):",
            parent=self)
        if canonical is None:
            return

        self.repository.insert({'codepoint': cp, 'canonical': canonical})
        self._load_entries()
        self.on_status(f"Added confusable: U+{cp:04X} -> {canonical}")

    def _delete(self):
        """Delete current entry."""
        if self.current_id is None:
            return

        if not messagebox.askyesno("Confirm", "Delete this confusable character mapping?"):
            return

        self.repository.delete(self.current_id)
        self.current_id = None
        self._clear_editor()
        self._load_entries()
        self.on_status("Confusable character deleted")

    def _clear_editor(self):
        self.current_id = None
        self.codepoint_var.set("")
        self.canonical_var.set("")
        self.preview_label.configure(text="-")
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved", "Discard changes?")
        return True
