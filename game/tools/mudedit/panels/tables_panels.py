"""
Reference table editor panels.

Provides editors for tables.db data: socials, slays, liquids, wear locations, calendar.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Optional


class SocialsPanel(ttk.Frame):
    """Editor panel for social commands and their messages."""

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
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: social list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        # Search
        search_frame = ttk.Frame(left_frame)
        search_frame.pack(fill=tk.X, padx=2, pady=2)
        ttk.Label(search_frame, text="Filter:").pack(side=tk.LEFT)
        self.search_var = tk.StringVar()
        self.search_var.trace_add('write', lambda *_: self._filter_list())
        ttk.Entry(search_frame, textvariable=self.search_var).pack(
            side=tk.LEFT, fill=tk.X, expand=True, padx=(4, 0))

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('name',),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('name', text='Social Command')
        self.tree.column('name', width=160)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 socials")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor with scrollable form
        right_frame = ttk.LabelFrame(paned, text="Edit Social")
        paned.add(right_frame, weight=2)

        # Scrollable canvas for the form
        canvas = tk.Canvas(right_frame, highlightthickness=0)
        form_scroll = ttk.Scrollbar(right_frame, orient=tk.VERTICAL, command=canvas.yview)
        self.form_frame = ttk.Frame(canvas)
        self.form_frame.bind('<Configure>', lambda e: canvas.configure(scrollregion=canvas.bbox('all')))
        canvas.create_window((0, 0), window=self.form_frame, anchor='nw')
        canvas.configure(yscrollcommand=form_scroll.set)
        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        form_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        # Mousewheel scrolling
        def _on_mousewheel(event):
            canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")
        canvas.bind_all('<MouseWheel>', _on_mousewheel, add='+')

        form = self.form_frame

        # Name (read-only)
        row = ttk.Frame(form)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Name:", width=14).pack(side=tk.LEFT)
        self.name_label = ttk.Label(row, text="-", font=('Consolas', 10, 'bold'))
        self.name_label.pack(side=tk.LEFT)

        # Message fields - each is a labeled Text widget
        self._msg_fields = {}
        field_labels = [
            ('char_no_arg', 'You (no target)'),
            ('others_no_arg', 'Room (no target)'),
            ('char_found', 'You (target)'),
            ('others_found', 'Room (target)'),
            ('vict_found', 'Victim sees'),
            ('char_auto', 'You (self)'),
            ('others_auto', 'Room (self)'),
        ]

        for field_name, label_text in field_labels:
            lf = ttk.LabelFrame(form, text=label_text)
            lf.pack(fill=tk.X, padx=8, pady=3)
            text_widget = tk.Text(lf, wrap=tk.WORD, height=2, font=('Consolas', 10))
            text_widget.pack(fill=tk.X, padx=4, pady=2)
            text_widget.bind('<KeyRelease>', lambda e: self._mark_unsaved())
            self._msg_fields[field_name] = text_widget

        # Help text
        help_label = ttk.Label(form,
            text="Variables: $n=actor, $N=target, $e/$E=he/she, $m/$M=him/her, $s/$S=his/her",
            foreground='gray', font=('Consolas', 9))
        help_label.pack(anchor=tk.W, padx=8, pady=(4, 2))

        # Buttons
        btn_frame = ttk.Frame(form)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Reload", command=self._load_entries).pack(side=tk.LEFT)

        self._all_entries = []

    def _load_entries(self):
        """Load all socials."""
        self._all_entries = self.repository.list_all()
        self._populate_tree(self._all_entries)
        self.on_status(f"Loaded {len(self._all_entries)} socials")

    def _populate_tree(self, entries):
        """Populate tree with entries."""
        self.tree.delete(*self.tree.get_children())
        for s in entries:
            self.tree.insert('', tk.END, iid=s['name'], values=(s['name'],))
        self.count_var.set(f"{len(entries)} socials")

    def _filter_list(self):
        """Filter list by search term."""
        term = self.search_var.get().lower()
        if not term:
            self._populate_tree(self._all_entries)
        else:
            filtered = [e for e in self._all_entries if term in e['name'].lower()]
            self._populate_tree(filtered)

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
        """Load a social into the editor."""
        s = self.repository.get_by_id(name)
        if not s:
            return

        self.current_name = name
        self.name_label.configure(text=name)

        for field_name, text_widget in self._msg_fields.items():
            text_widget.delete('1.0', tk.END)
            val = s.get(field_name)
            if val:
                text_widget.insert('1.0', val)

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current social."""
        if self.current_name is None:
            return

        data = {}
        for field_name, text_widget in self._msg_fields.items():
            val = text_widget.get('1.0', tk.END).rstrip('\n')
            data[field_name] = val if val else None

        self.repository.update(self.current_name, data)
        self.unsaved = False
        self._load_entries()
        self.tree.selection_set(self.current_name)
        self.on_status(f"Saved social: {self.current_name}")

    def _new(self):
        """Create a new social."""
        name = simpledialog.askstring("New Social", "Enter social command name:", parent=self)
        if not name:
            return

        name = name.strip().lower()
        if not name:
            return

        if self.repository.get_by_id(name):
            messagebox.showinfo("Info", f"Social '{name}' already exists. Select it to edit.")
            return

        self.repository.insert({
            'name': name,
            'char_no_arg': None,
            'others_no_arg': None,
            'char_found': None,
            'others_found': None,
            'vict_found': None,
            'char_auto': None,
            'others_auto': None,
        })
        self._load_entries()
        self.tree.selection_set(name)
        self._load_entry(name)
        self.on_status(f"Created social: {name}")

    def _delete(self):
        """Delete current social."""
        if self.current_name is None:
            return

        if not messagebox.askyesno("Confirm", f"Delete social '{self.current_name}'?"):
            return

        self.repository.delete(self.current_name)
        self.current_name = None
        self._clear_editor()
        self._load_entries()
        self.on_status("Social deleted")

    def _clear_editor(self):
        self.current_name = None
        self.name_label.configure(text="-")
        for text_widget in self._msg_fields.values():
            text_widget.delete('1.0', tk.END)
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved", "Discard changes?")
        return True


class SlaysPanel(ttk.Frame):
    """Editor panel for immortal slay commands."""

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

        # Left: slay list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('id', 'owner', 'title'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('id', text='ID')
        self.tree.heading('owner', text='Owner')
        self.tree.heading('title', text='Title')
        self.tree.column('id', width=40, stretch=False)
        self.tree.column('owner', width=100)
        self.tree.column('title', width=150)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 slays")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor
        right_frame = ttk.LabelFrame(paned, text="Edit Slay")
        paned.add(right_frame, weight=2)

        # ID (read-only)
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="ID:", width=12).pack(side=tk.LEFT)
        self.id_label = ttk.Label(row, text="-", font=('Consolas', 10, 'bold'))
        self.id_label.pack(side=tk.LEFT)

        # Owner
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Owner:", width=12).pack(side=tk.LEFT)
        self.owner_var = tk.StringVar()
        self.owner_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.owner_var, width=20).pack(side=tk.LEFT)
        ttk.Label(row, text="(blank = available to all immortals)", foreground='gray').pack(
            side=tk.LEFT, padx=8)

        # Title
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Title:", width=12).pack(side=tk.LEFT)
        self.title_var = tk.StringVar()
        self.title_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.title_var, width=30).pack(side=tk.LEFT)

        # Message fields
        self._msg_texts = {}
        for field_name, label_text in [
            ('char_msg', 'You see (actor)'),
            ('vict_msg', 'Victim sees'),
            ('room_msg', 'Room sees'),
        ]:
            lf = ttk.LabelFrame(right_frame, text=label_text)
            lf.pack(fill=tk.X, padx=8, pady=3)
            text_widget = tk.Text(lf, wrap=tk.WORD, height=2, font=('Consolas', 10))
            text_widget.pack(fill=tk.X, padx=4, pady=2)
            text_widget.bind('<KeyRelease>', lambda e: self._mark_unsaved())
            self._msg_texts[field_name] = text_widget

        # Help
        ttk.Label(right_frame,
            text="Variables: $n=actor, $N=victim, $e/$E=he/she, $m/$M=him/her",
            foreground='gray', font=('Consolas', 9)).pack(anchor=tk.W, padx=8, pady=(4, 2))

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Reload", command=self._load_entries).pack(side=tk.LEFT)

    def _load_entries(self):
        """Load all slays."""
        entries = self.repository.list_all()

        self.tree.delete(*self.tree.get_children())
        for s in entries:
            self.tree.insert('', tk.END, iid=str(s['id']),
                           values=(s['id'], s['owner'], s['title']))

        self.count_var.set(f"{len(entries)} slays")
        self.on_status(f"Loaded {len(entries)} slay types")

    def _on_select(self, event):
        """Handle selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved", "Discard changes?"):
                if self.current_id is not None:
                    self.tree.selection_set(str(self.current_id))
                return

        selection = self.tree.selection()
        if selection:
            self._load_entry(int(selection[0]))

    def _load_entry(self, id_val: int):
        """Load a slay into the editor."""
        s = self.repository.get_by_id(id_val)
        if not s:
            return

        self.current_id = id_val
        self.id_label.configure(text=str(id_val))
        self.owner_var.set(s.get('owner', ''))
        self.title_var.set(s.get('title', ''))

        for field_name, text_widget in self._msg_texts.items():
            text_widget.delete('1.0', tk.END)
            val = s.get(field_name, '')
            if val:
                text_widget.insert('1.0', val)

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current slay."""
        if self.current_id is None:
            return

        data = {
            'owner': self.owner_var.get(),
            'title': self.title_var.get(),
        }
        for field_name, text_widget in self._msg_texts.items():
            data[field_name] = text_widget.get('1.0', tk.END).rstrip('\n')

        self.repository.update(self.current_id, data)
        self.unsaved = False
        self._load_entries()
        self.tree.selection_set(str(self.current_id))
        self.on_status(f"Saved slay {self.current_id}")

    def _new(self):
        """Create a new slay."""
        # Find next available ID
        entries = self.repository.list_all()
        next_id = max((e['id'] for e in entries), default=-1) + 1

        title = simpledialog.askstring("New Slay", "Enter slay title:", parent=self)
        if not title:
            return

        self.repository.insert({
            'id': next_id,
            'owner': '',
            'title': title.strip(),
            'char_msg': '',
            'vict_msg': '',
            'room_msg': '',
        })
        self._load_entries()
        self.tree.selection_set(str(next_id))
        self._load_entry(next_id)
        self.on_status(f"Created slay {next_id}: {title}")

    def _delete(self):
        """Delete current slay."""
        if self.current_id is None:
            return

        if not messagebox.askyesno("Confirm", f"Delete slay {self.current_id}?"):
            return

        self.repository.delete(self.current_id)
        self.current_id = None
        self._clear_editor()
        self._load_entries()
        self.on_status("Slay deleted")

    def _clear_editor(self):
        self.current_id = None
        self.id_label.configure(text="-")
        self.owner_var.set("")
        self.title_var.set("")
        for text_widget in self._msg_texts.values():
            text_widget.delete('1.0', tk.END)
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved", "Discard changes?")
        return True


class LiquidsPanel(ttk.Frame):
    """Editor panel for liquid types and their properties."""

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

        # Left: liquid list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('id', 'name', 'color'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('id', text='ID')
        self.tree.heading('name', text='Name')
        self.tree.heading('color', text='Color')
        self.tree.column('id', width=40, stretch=False)
        self.tree.column('name', width=120)
        self.tree.column('color', width=80)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 liquids")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor
        right_frame = ttk.LabelFrame(paned, text="Edit Liquid")
        paned.add(right_frame, weight=1)

        # ID
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="ID:", width=14).pack(side=tk.LEFT)
        self.id_label = ttk.Label(row, text="-", font=('Consolas', 10, 'bold'))
        self.id_label.pack(side=tk.LEFT)

        # Name
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Name:", width=14).pack(side=tk.LEFT)
        self.name_var = tk.StringVar()
        self.name_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.name_var, width=20).pack(side=tk.LEFT)

        # Color
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Color:", width=14).pack(side=tk.LEFT)
        self.color_var = tk.StringVar()
        self.color_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.color_var, width=20).pack(side=tk.LEFT)

        # Effects frame
        effects_frame = ttk.LabelFrame(right_frame, text="Effects per Sip")
        effects_frame.pack(fill=tk.X, padx=8, pady=8)

        # Proof (alcohol)
        row = ttk.Frame(effects_frame)
        row.pack(fill=tk.X, padx=4, pady=4)
        ttk.Label(row, text="Proof:", width=14).pack(side=tk.LEFT)
        self.proof_var = tk.IntVar()
        ttk.Spinbox(row, from_=-50, to=100, width=8, textvariable=self.proof_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT)
        ttk.Label(row, text="(alcohol content)", foreground='gray').pack(side=tk.LEFT, padx=8)

        # Full effect
        row = ttk.Frame(effects_frame)
        row.pack(fill=tk.X, padx=4, pady=4)
        ttk.Label(row, text="Full Effect:", width=14).pack(side=tk.LEFT)
        self.full_var = tk.IntVar()
        ttk.Spinbox(row, from_=-50, to=100, width=8, textvariable=self.full_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT)
        ttk.Label(row, text="(hunger satisfaction)", foreground='gray').pack(side=tk.LEFT, padx=8)

        # Thirst
        row = ttk.Frame(effects_frame)
        row.pack(fill=tk.X, padx=4, pady=4)
        ttk.Label(row, text="Thirst:", width=14).pack(side=tk.LEFT)
        self.thirst_var = tk.IntVar()
        ttk.Spinbox(row, from_=-50, to=100, width=8, textvariable=self.thirst_var,
                   command=self._mark_unsaved).pack(side=tk.LEFT)
        ttk.Label(row, text="(thirst quenching)", foreground='gray').pack(side=tk.LEFT, padx=8)

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Reload", command=self._load_entries).pack(side=tk.LEFT)

    def _load_entries(self):
        """Load all liquids."""
        entries = self.repository.list_all()

        self.tree.delete(*self.tree.get_children())
        for liq in entries:
            self.tree.insert('', tk.END, iid=str(liq['id']),
                           values=(liq['id'], liq['name'], liq['color']))

        self.count_var.set(f"{len(entries)} liquids")
        self.on_status(f"Loaded {len(entries)} liquids")

    def _on_select(self, event):
        """Handle selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved", "Discard changes?"):
                if self.current_id is not None:
                    self.tree.selection_set(str(self.current_id))
                return

        selection = self.tree.selection()
        if selection:
            self._load_entry(int(selection[0]))

    def _load_entry(self, id_val: int):
        """Load a liquid into the editor."""
        liq = self.repository.get_by_id(id_val)
        if not liq:
            return

        self.current_id = id_val
        self.id_label.configure(text=str(id_val))
        self.name_var.set(liq.get('name', ''))
        self.color_var.set(liq.get('color', ''))
        self.proof_var.set(liq.get('proof', 0))
        self.full_var.set(liq.get('full_effect', 0))
        self.thirst_var.set(liq.get('thirst', 0))

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current liquid."""
        if self.current_id is None:
            return

        data = {
            'name': self.name_var.get(),
            'color': self.color_var.get(),
            'proof': self.proof_var.get(),
            'full_effect': self.full_var.get(),
            'thirst': self.thirst_var.get(),
        }

        self.repository.update(self.current_id, data)
        self.unsaved = False
        self._load_entries()
        self.tree.selection_set(str(self.current_id))
        self.on_status(f"Saved liquid {self.current_id}")

    def _new(self):
        """Create a new liquid."""
        entries = self.repository.list_all()
        next_id = max((e['id'] for e in entries), default=-1) + 1

        name = simpledialog.askstring("New Liquid", "Enter liquid name:", parent=self)
        if not name:
            return

        self.repository.insert({
            'id': next_id,
            'name': name.strip(),
            'color': 'clear',
            'proof': 0,
            'full_effect': 0,
            'thirst': 0,
        })
        self._load_entries()
        self.tree.selection_set(str(next_id))
        self._load_entry(next_id)
        self.on_status(f"Created liquid {next_id}: {name}")

    def _delete(self):
        """Delete current liquid."""
        if self.current_id is None:
            return

        if not messagebox.askyesno("Confirm", f"Delete liquid {self.current_id}?"):
            return

        self.repository.delete(self.current_id)
        self.current_id = None
        self._clear_editor()
        self._load_entries()
        self.on_status("Liquid deleted")

    def _clear_editor(self):
        self.current_id = None
        self.id_label.configure(text="-")
        self.name_var.set("")
        self.color_var.set("")
        self.proof_var.set(0)
        self.full_var.set(0)
        self.thirst_var.set(0)
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved", "Discard changes?")
        return True


class WearLocationsPanel(ttk.Frame):
    """Editor panel for equipment wear slot display strings."""

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

        # Left: slot list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('slot_id', 'slot_name'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('slot_id', text='Slot')
        self.tree.heading('slot_name', text='Name')
        self.tree.column('slot_id', width=50, stretch=False)
        self.tree.column('slot_name', width=100)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 slots")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor
        right_frame = ttk.LabelFrame(paned, text="Edit Wear Location")
        paned.add(right_frame, weight=2)

        # Slot ID
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Slot ID:", width=14).pack(side=tk.LEFT)
        self.id_label = ttk.Label(row, text="-", font=('Consolas', 10, 'bold'))
        self.id_label.pack(side=tk.LEFT)

        # Slot name
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Slot Name:", width=14).pack(side=tk.LEFT)
        self.slot_name_label = ttk.Label(row, text="-", font=('Consolas', 10))
        self.slot_name_label.pack(side=tk.LEFT)

        # Display text (with color codes)
        lf = ttk.LabelFrame(right_frame, text="Display Text (with color codes)")
        lf.pack(fill=tk.X, padx=8, pady=4)
        self.display_var = tk.StringVar()
        self.display_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(lf, textvariable=self.display_var, width=40, font=('Consolas', 10)).pack(
            fill=tk.X, padx=4, pady=4)

        # Screen reader text
        lf = ttk.LabelFrame(right_frame, text="Screen Reader Text")
        lf.pack(fill=tk.X, padx=8, pady=4)
        self.sr_var = tk.StringVar()
        self.sr_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(lf, textvariable=self.sr_var, width=40, font=('Consolas', 10)).pack(
            fill=tk.X, padx=4, pady=4)

        # Help text
        ttk.Label(right_frame,
            text="Color codes: #R=red, #G=green, #B=blue, #C=cyan, #Y=yellow, #W=white, #n=reset",
            foreground='gray', font=('Consolas', 9)).pack(anchor=tk.W, padx=8, pady=(4, 2))

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Reload", command=self._load_entries).pack(side=tk.LEFT)

    def _load_entries(self):
        """Load all wear locations."""
        entries = self.repository.list_all()

        self.tree.delete(*self.tree.get_children())
        for loc in entries:
            self.tree.insert('', tk.END, iid=str(loc['slot_id']),
                           values=(loc['slot_id'], loc['slot_name']))

        self.count_var.set(f"{len(entries)} slots")
        self.on_status(f"Loaded {len(entries)} wear locations")

    def _on_select(self, event):
        """Handle selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved", "Discard changes?"):
                if self.current_id is not None:
                    self.tree.selection_set(str(self.current_id))
                return

        selection = self.tree.selection()
        if selection:
            self._load_entry(int(selection[0]))

    def _load_entry(self, id_val: int):
        """Load a wear location into the editor."""
        loc = self.repository.get_by_id(id_val)
        if not loc:
            return

        self.current_id = id_val
        self.id_label.configure(text=str(id_val))
        self.slot_name_label.configure(text=loc.get('slot_name', ''))
        self.display_var.set(loc.get('display_text', ''))
        self.sr_var.set(loc.get('sr_text', ''))

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current wear location."""
        if self.current_id is None:
            return

        data = {
            'display_text': self.display_var.get(),
            'sr_text': self.sr_var.get(),
        }

        self.repository.update(self.current_id, data)
        self.unsaved = False
        self._load_entries()
        self.tree.selection_set(str(self.current_id))
        self.on_status(f"Saved wear location {self.current_id}")

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved", "Discard changes?")
        return True


class ScriptLibraryPanel(ttk.Frame):
    """Editor panel for shared Lua script templates in the script library."""

    TRIGGER_NAMES = {1: 'GREET', 2: 'SPEECH', 4: 'TICK', 8: 'KILL', 16: 'DEATH'}

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

    @classmethod
    def trigger_display(cls, trigger: int) -> str:
        names = []
        for bit, name in sorted(cls.TRIGGER_NAMES.items()):
            if trigger & bit:
                names.append(name)
        return '+'.join(names) if names else '(none)'

    def _build_ui(self):
        """Build the panel UI."""
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: library entry list
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        # Search
        search_frame = ttk.Frame(left_frame)
        search_frame.pack(fill=tk.X, padx=2, pady=2)
        ttk.Label(search_frame, text="Filter:").pack(side=tk.LEFT)
        self.search_var = tk.StringVar()
        self.search_var.trace_add('write', lambda *_: self._filter_list())
        ttk.Entry(search_frame, textvariable=self.search_var).pack(
            side=tk.LEFT, fill=tk.X, expand=True, padx=(4, 0))

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('name', 'trigger'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('name', text='Name')
        self.tree.heading('trigger', text='Trigger')
        self.tree.column('name', width=180)
        self.tree.column('trigger', width=80)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 entries")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor
        right_frame = ttk.LabelFrame(paned, text="Edit Library Script")
        paned.add(right_frame, weight=2)

        # Name
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Name:", width=12).pack(side=tk.LEFT)
        self.name_label = ttk.Label(row, text="-", font=('Consolas', 10, 'bold'))
        self.name_label.pack(side=tk.LEFT)

        # Trigger
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Trigger:", width=12).pack(side=tk.LEFT)
        self.trigger_vars = {}
        for bit, name in sorted(self.TRIGGER_NAMES.items()):
            var = tk.BooleanVar()
            cb = ttk.Checkbutton(row, text=name, variable=var,
                                 command=self._mark_unsaved)
            cb.pack(side=tk.LEFT, padx=(0, 8))
            self.trigger_vars[bit] = var

        # Pattern
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Pattern:", width=12).pack(side=tk.LEFT)
        self.pattern_var = tk.StringVar()
        self.pattern_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.pattern_var, width=30).pack(
            side=tk.LEFT, padx=4, fill=tk.X, expand=True)
        ttk.Label(row, text="(SPEECH only)", foreground='gray').pack(side=tk.LEFT, padx=4)

        # Chance
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Chance:", width=12).pack(side=tk.LEFT)
        self.chance_var = tk.IntVar(value=0)
        ttk.Spinbox(row, from_=0, to=100, width=5, textvariable=self.chance_var,
                     command=self._mark_unsaved).pack(side=tk.LEFT, padx=(2, 4))
        ttk.Label(row, text="(0 = always)", foreground='gray').pack(side=tk.LEFT)

        # Code editor
        code_frame = ttk.LabelFrame(right_frame, text="Lua Code")
        code_frame.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)

        code_inner = ttk.Frame(code_frame)
        code_inner.pack(fill=tk.BOTH, expand=True)

        self.code_text = tk.Text(
            code_inner, font=('Consolas', 10), wrap=tk.NONE,
            height=20, undo=True)
        code_xscroll = ttk.Scrollbar(code_inner, orient=tk.HORIZONTAL, command=self.code_text.xview)
        code_yscroll = ttk.Scrollbar(code_inner, orient=tk.VERTICAL, command=self.code_text.yview)
        self.code_text.configure(xscrollcommand=code_xscroll.set, yscrollcommand=code_yscroll.set)

        self.code_text.grid(row=0, column=0, sticky='nsew')
        code_yscroll.grid(row=0, column=1, sticky='ns')
        code_xscroll.grid(row=1, column=0, sticky='ew')
        code_inner.grid_rowconfigure(0, weight=1)
        code_inner.grid_columnconfigure(0, weight=1)

        self.code_text.bind('<<Modified>>', self._on_code_modified)

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Reload", command=self._load_entries).pack(side=tk.LEFT)

        self._all_entries = []

    def _on_code_modified(self, event):
        if self.code_text.edit_modified():
            self._mark_unsaved()
            self.code_text.edit_modified(False)

    def _get_trigger_value(self) -> int:
        value = 0
        for bit, var in self.trigger_vars.items():
            if var.get():
                value |= bit
        return value

    def _set_trigger_value(self, value: int):
        for bit, var in self.trigger_vars.items():
            var.set(bool(value & bit))

    def _load_entries(self):
        """Load all library entries."""
        self._all_entries = self.repository.list_all()
        self._populate_tree(self._all_entries)
        self.on_status(f"Loaded {len(self._all_entries)} library scripts")

    def _populate_tree(self, entries):
        self.tree.delete(*self.tree.get_children())
        for e in entries:
            self.tree.insert('', tk.END, iid=e['name'],
                             values=(e['name'], self.trigger_display(e['trigger'])))
        self.count_var.set(f"{len(entries)} entries")

    def _filter_list(self):
        term = self.search_var.get().lower()
        if not term:
            self._populate_tree(self._all_entries)
        else:
            filtered = [e for e in self._all_entries if term in e['name'].lower()]
            self._populate_tree(filtered)

    def _on_select(self, event):
        if self.unsaved:
            if not messagebox.askyesno("Unsaved", "Discard changes?"):
                if self.current_name:
                    self.tree.selection_set(self.current_name)
                return

        selection = self.tree.selection()
        if selection:
            self._load_entry(selection[0])

    def _load_entry(self, name: str):
        e = self.repository.get_by_id(name)
        if not e:
            return

        self.current_name = name
        self.name_label.configure(text=name)
        self._set_trigger_value(e.get('trigger', 0))
        self.pattern_var.set(e.get('pattern', '') or '')
        self.chance_var.set(e.get('chance', 0))

        self.code_text.delete('1.0', tk.END)
        self.code_text.insert('1.0', e.get('code', ''))
        self.code_text.edit_modified(False)

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        if self.current_name is None:
            return

        code = self.code_text.get('1.0', tk.END).rstrip('\n')
        if not code.strip():
            messagebox.showwarning("Warning", "Script code cannot be empty.")
            return

        trigger = self._get_trigger_value()
        if trigger == 0:
            messagebox.showwarning("Warning", "Select at least one trigger type.")
            return

        pattern = self.pattern_var.get().strip() or None

        data = {
            'trigger': trigger,
            'code': code,
            'pattern': pattern,
            'chance': self.chance_var.get(),
        }

        self.repository.update(self.current_name, data)
        self.unsaved = False
        self._load_entries()
        self.tree.selection_set(self.current_name)
        self.on_status(f"Saved library script: {self.current_name}")

    def _new(self):
        from tkinter import simpledialog
        name = simpledialog.askstring("New Library Script",
                                       "Enter script name:", parent=self)
        if not name:
            return

        name = name.strip().lower().replace(' ', '_')
        if not name:
            return

        if self.repository.get_by_id(name):
            messagebox.showinfo("Info", f"Script '{name}' already exists.")
            return

        self.repository.insert({
            'name': name,
            'trigger': 4,  # TRIG_TICK
            'code': '-- New library script\nfunction on_tick(mob)\n    return false\nend\n',
            'pattern': None,
            'chance': 0,
        })
        self._load_entries()
        self.tree.selection_set(name)
        self._load_entry(name)
        self.on_status(f"Created library script: {name}")

    def _delete(self):
        if self.current_name is None:
            return

        if not messagebox.askyesno("Confirm",
                f"Delete library script '{self.current_name}'?\n\n"
                "Area scripts referencing it will fail to load."):
            return

        self.repository.delete(self.current_name)
        self.current_name = None
        self._clear_editor()
        self._load_entries()
        self.on_status("Library script deleted")

    def _clear_editor(self):
        self.current_name = None
        self.name_label.configure(text="-")
        self._set_trigger_value(0)
        self.pattern_var.set('')
        self.chance_var.set(0)
        self.code_text.delete('1.0', tk.END)
        self.code_text.edit_modified(False)
        self.unsaved = False

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved", "Discard changes?")
        return True


class CalendarPanel(ttk.Frame):
    """Editor panel for in-game day and month names."""

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

        self.current_type: Optional[str] = None
        self.current_idx: Optional[int] = None
        self.unsaved = False

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: combined list of days and months
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame,
            columns=('type', 'idx', 'name'),
            show='headings',
            selectmode='browse'
        )
        self.tree.heading('type', text='Type')
        self.tree.heading('idx', text='#')
        self.tree.heading('name', text='Name')
        self.tree.column('type', width=60, stretch=False)
        self.tree.column('idx', width=40, stretch=False)
        self.tree.column('name', width=180)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        self.count_var = tk.StringVar(value="0 entries")
        ttk.Label(left_frame, textvariable=self.count_var).pack(anchor=tk.W, padx=4)

        # Right: editor
        right_frame = ttk.LabelFrame(paned, text="Edit Calendar Entry")
        paned.add(right_frame, weight=1)

        # Type (read-only)
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Type:", width=12).pack(side=tk.LEFT)
        self.type_label = ttk.Label(row, text="-", font=('Consolas', 10, 'bold'))
        self.type_label.pack(side=tk.LEFT)

        # Index (read-only)
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Index:", width=12).pack(side=tk.LEFT)
        self.idx_label = ttk.Label(row, text="-", font=('Consolas', 10, 'bold'))
        self.idx_label.pack(side=tk.LEFT)

        # Name
        row = ttk.Frame(right_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Name:", width=12).pack(side=tk.LEFT)
        self.name_var = tk.StringVar()
        self.name_var.trace_add('write', lambda *_: self._mark_unsaved())
        ttk.Entry(row, textvariable=self.name_var, width=30).pack(side=tk.LEFT)

        # Context info
        ttk.Label(right_frame,
            text="Days are used for 'Day of <name>' display.\n"
                 "Months are displayed as 'the Month of <name>'.",
            foreground='gray', font=('Consolas', 9)).pack(anchor=tk.W, padx=8, pady=(8, 2))

        # Buttons
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Reload", command=self._load_entries).pack(side=tk.LEFT)

    def _load_entries(self):
        """Load all calendar entries."""
        entries = self.repository.list_all()

        self.tree.delete(*self.tree.get_children())
        for c in entries:
            iid = f"{c['type']}:{c['idx']}"
            type_label = "Day" if c['type'] == 'day' else "Month"
            self.tree.insert('', tk.END, iid=iid,
                           values=(type_label, c['idx'], c['name']))

        self.count_var.set(f"{len(entries)} entries")
        self.on_status(f"Loaded {len(entries)} calendar entries")

    def _on_select(self, event):
        """Handle selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved", "Discard changes?"):
                if self.current_type and self.current_idx is not None:
                    self.tree.selection_set(f"{self.current_type}:{self.current_idx}")
                return

        selection = self.tree.selection()
        if selection:
            parts = selection[0].split(':')
            self._load_entry(parts[0], int(parts[1]))

    def _load_entry(self, entry_type: str, idx: int):
        """Load a calendar entry into the editor."""
        # Fetch by composite key
        rows = self.repository.conn.execute(
            "SELECT * FROM calendar WHERE type = ? AND idx = ?",
            (entry_type, idx)
        ).fetchall()
        if not rows:
            return

        c = dict(rows[0])

        self.current_type = entry_type
        self.current_idx = idx
        type_label = "Day" if entry_type == 'day' else "Month"
        self.type_label.configure(text=type_label)
        self.idx_label.configure(text=str(idx))
        self.name_var.set(c.get('name', ''))

        self.unsaved = False

    def _mark_unsaved(self):
        self.unsaved = True

    def _save(self):
        """Save current calendar entry."""
        if self.current_type is None or self.current_idx is None:
            return

        self.repository.conn.execute(
            "UPDATE calendar SET name = ? WHERE type = ? AND idx = ?",
            (self.name_var.get(), self.current_type, self.current_idx)
        )
        self.repository.conn.commit()

        self.unsaved = False
        self._load_entries()
        self.tree.selection_set(f"{self.current_type}:{self.current_idx}")
        self.on_status(f"Saved {self.current_type} {self.current_idx}")

    def check_unsaved(self) -> bool:
        if self.unsaved:
            return messagebox.askyesno("Unsaved", "Discard changes?")
        return True
