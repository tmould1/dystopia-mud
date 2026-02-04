"""
Class Registry editor panel.

Provides interface for editing class metadata (names, keywords, messages)
used by selfclass command and mudstat display.
"""

import tkinter as tk
from tkinter import ttk, messagebox
from typing import Callable, Optional


class ClassRegistryPanel(ttk.Frame):
    """
    Editor panel for class registry configuration.

    Shows a list of classes with their metadata and allows editing
    keywords, display names, and selfclass messages.
    """

    def __init__(
        self,
        parent,
        registry_repo,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.registry_repo = registry_repo
        self.on_status = on_status or (lambda msg: None)

        self.current_class_id: Optional[int] = None
        self.unsaved = False

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        main_paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: class list
        left_frame = ttk.LabelFrame(main_paned, text="Classes")
        main_paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.class_tree = ttk.Treeview(
            tree_frame,
            columns=('class_id', 'class_name', 'keyword', 'type'),
            show='headings',
            selectmode='browse'
        )
        self.class_tree.heading('class_id', text='ID')
        self.class_tree.heading('class_name', text='Name')
        self.class_tree.heading('keyword', text='Keyword')
        self.class_tree.heading('type', text='Type')
        self.class_tree.column('class_id', width=50, stretch=False)
        self.class_tree.column('class_name', width=100)
        self.class_tree.column('keyword', width=80)
        self.class_tree.column('type', width=60, stretch=False)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.class_tree.yview)
        self.class_tree.configure(yscrollcommand=scrollbar.set)
        self.class_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.class_tree.bind('<<TreeviewSelect>>', self._on_class_select)

        # Right: editor
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=2)

        # Basic Info section
        basic_frame = ttk.LabelFrame(right_frame, text="Basic Information")
        basic_frame.pack(fill=tk.X, padx=4, pady=4)

        # Class Name
        row = ttk.Frame(basic_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Class Name:", width=15).pack(side=tk.LEFT)
        self.name_var = tk.StringVar()
        self.name_var.trace_add('write', lambda *_: self._on_change())
        ttk.Entry(row, textvariable=self.name_var, width=20, font=('Consolas', 10)).pack(side=tk.LEFT)

        # Keyword
        row = ttk.Frame(basic_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Keyword:", width=15).pack(side=tk.LEFT)
        self.keyword_var = tk.StringVar()
        self.keyword_var.trace_add('write', lambda *_: self._on_change())
        ttk.Entry(row, textvariable=self.keyword_var, width=15, font=('Consolas', 10)).pack(side=tk.LEFT)
        ttk.Label(row, text="(for selfclass command)", foreground='gray').pack(side=tk.LEFT, padx=8)

        # Alternate Keyword
        row = ttk.Frame(basic_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Alt Keyword:", width=15).pack(side=tk.LEFT)
        self.keyword_alt_var = tk.StringVar()
        self.keyword_alt_var.trace_add('write', lambda *_: self._on_change())
        ttk.Entry(row, textvariable=self.keyword_alt_var, width=15, font=('Consolas', 10)).pack(side=tk.LEFT)
        ttk.Label(row, text="(optional, e.g. 'battlemage')", foreground='gray').pack(side=tk.LEFT, padx=8)

        # Mudstat section
        mudstat_frame = ttk.LabelFrame(right_frame, text="Mudstat Display")
        mudstat_frame.pack(fill=tk.X, padx=4, pady=4)

        row = ttk.Frame(mudstat_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Label (plural):", width=15).pack(side=tk.LEFT)
        self.label_var = tk.StringVar()
        self.label_var.trace_add('write', lambda *_: self._on_change())
        ttk.Entry(row, textvariable=self.label_var, width=15, font=('Consolas', 10)).pack(side=tk.LEFT)
        ttk.Label(row, text="(e.g. 'Demons', 'Vampires')", foreground='gray').pack(side=tk.LEFT, padx=8)

        row = ttk.Frame(mudstat_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Display Order:", width=15).pack(side=tk.LEFT)
        self.order_var = tk.StringVar()
        self.order_var.trace_add('write', lambda *_: self._on_change())
        ttk.Entry(row, textvariable=self.order_var, width=6, font=('Consolas', 10)).pack(side=tk.LEFT)

        # Class Type section
        type_frame = ttk.LabelFrame(right_frame, text="Class Type")
        type_frame.pack(fill=tk.X, padx=4, pady=4)

        row = ttk.Frame(type_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Upgrade From:", width=15).pack(side=tk.LEFT)
        self.upgrade_var = tk.StringVar()
        self.upgrade_combo = ttk.Combobox(row, textvariable=self.upgrade_var, width=25, state='readonly')
        self.upgrade_combo.pack(side=tk.LEFT)
        self.upgrade_combo.bind('<<ComboboxSelected>>', lambda e: self._on_change())

        row = ttk.Frame(type_frame)
        row.pack(fill=tk.X, padx=8, pady=4)
        ttk.Label(row, text="Requirements:", width=15).pack(side=tk.LEFT)
        self.requirements_var = tk.StringVar()
        self.requirements_var.trace_add('write', lambda *_: self._on_change())
        ttk.Entry(row, textvariable=self.requirements_var, width=40, font=('Consolas', 10)).pack(side=tk.LEFT, fill=tk.X, expand=True)

        # Selfclass section
        selfclass_frame = ttk.LabelFrame(right_frame, text="Selfclass Message")
        selfclass_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        ttk.Label(selfclass_frame, text="Confirmation message shown when player selfclasses (with color codes):").pack(anchor=tk.W, padx=8, pady=(4, 0))
        self.message_text = tk.Text(selfclass_frame, height=3, width=50, font=('Consolas', 10))
        self.message_text.pack(fill=tk.X, padx=8, pady=4)
        self.message_text.bind('<KeyRelease>', lambda e: self._on_change())

        # Save button
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, padx=4, pady=8)
        ttk.Button(btn_frame, text="Save", command=self._save_entry).pack(side=tk.LEFT)

    def _load_entries(self):
        """Load entries into the tree."""
        self.class_tree.delete(*self.class_tree.get_children())

        entries = self.registry_repo.list_all()

        # Build upgrade class options
        self.upgrade_options = {'(None - Base Class)': None}
        for entry in entries:
            self.upgrade_options[f"{entry['class_name']} ({entry['class_id']})"] = entry['class_id']
        self.upgrade_combo['values'] = list(self.upgrade_options.keys())

        for entry in entries:
            class_type = "Base" if entry['upgrade_class'] is None else "Upgrade"
            self.class_tree.insert('', tk.END, iid=str(entry['class_id']),
                                   values=(entry['class_id'], entry['class_name'],
                                           entry['keyword'], class_type))

        self.on_status(f"Loaded {len(entries)} class registry entries")

    def _on_class_select(self, event):
        """Handle class selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved Changes", "Discard unsaved changes?"):
                return

        selection = self.class_tree.selection()
        if not selection:
            return

        class_id = int(selection[0])
        self.current_class_id = class_id

        entry = self.registry_repo.get_by_id(class_id)
        if entry:
            self.name_var.set(entry['class_name'])
            self.keyword_var.set(entry['keyword'])
            self.keyword_alt_var.set(entry['keyword_alt'] or '')
            self.label_var.set(entry['mudstat_label'])
            self.order_var.set(str(entry['display_order']))

            # Set upgrade class combobox
            if entry['upgrade_class'] is None:
                self.upgrade_var.set('(None - Base Class)')
            else:
                for name, cid in self.upgrade_options.items():
                    if cid == entry['upgrade_class']:
                        self.upgrade_var.set(name)
                        break

            self.requirements_var.set(entry['requirements'] or '')

            self.message_text.delete('1.0', tk.END)
            self.message_text.insert('1.0', entry['selfclass_message'])

            self.unsaved = False

        self.on_status(f"Editing {entry['class_name']}")

    def _on_change(self):
        """Called when any field changes."""
        self.unsaved = True

    def _save_entry(self):
        """Save current entry."""
        if self.current_class_id is None:
            messagebox.showwarning("No Selection", "Please select a class first.")
            return

        try:
            display_order = int(self.order_var.get().strip())
        except ValueError:
            messagebox.showwarning("Invalid Order", "Display order must be a number.")
            return

        # Get upgrade class value
        upgrade_class = self.upgrade_options.get(self.upgrade_var.get())

        # Get values
        class_name = self.name_var.get().strip()
        keyword = self.keyword_var.get().strip().lower()
        keyword_alt = self.keyword_alt_var.get().strip().lower() or None
        mudstat_label = self.label_var.get().strip()
        selfclass_message = self.message_text.get('1.0', tk.END).strip()
        requirements = self.requirements_var.get().strip() or None

        if not class_name or not keyword or not mudstat_label or not selfclass_message:
            messagebox.showwarning("Missing Fields", "Class name, keyword, mudstat label, and message are required.")
            return

        self.registry_repo.update(self.current_class_id, {
            'class_name': class_name,
            'keyword': keyword,
            'keyword_alt': keyword_alt,
            'mudstat_label': mudstat_label,
            'selfclass_message': selfclass_message,
            'display_order': display_order,
            'upgrade_class': upgrade_class,
            'requirements': requirements
        })

        self.unsaved = False
        self._load_entries()  # Refresh list

        # Re-select current item
        self.class_tree.selection_set(str(self.current_class_id))

        self.on_status("Registry entry saved. Restart server to apply.")

    def check_unsaved(self) -> bool:
        """Check for unsaved changes before closing."""
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes",
                                        "You have unsaved changes. Discard them?")
        return True
