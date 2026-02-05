"""
Class Vnum Ranges editor panel.

Provides interface for editing class equipment vnum ranges to track
which vnums are used by which class and prevent conflicts.
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from typing import Callable, Optional

from ..db.repository import get_class_name, CLASS_NAMES


class ClassVnumRangesPanel(ttk.Frame):
    """
    Editor panel for class vnum ranges.

    Shows a list of classes with their armor vnum ranges and mastery vnums,
    allows editing, and provides conflict detection.
    """

    def __init__(
        self,
        parent,
        vnum_ranges_repo,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.vnum_ranges_repo = vnum_ranges_repo
        self.on_status = on_status or (lambda msg: None)

        self.current_class_id: Optional[int] = None
        self.unsaved = False

        self._build_ui()
        self._load_entries()

    def _build_ui(self):
        """Build the panel UI."""
        main_paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: vnum ranges list
        left_frame = ttk.LabelFrame(main_paned, text="Equipment Vnum Ranges")
        main_paned.add(left_frame, weight=1)

        tree_frame = ttk.Frame(left_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.range_tree = ttk.Treeview(
            tree_frame,
            columns=('class_id', 'class_name', 'start', 'end', 'mastery', 'description'),
            show='headings',
            selectmode='browse'
        )
        self.range_tree.heading('class_id', text='ID')
        self.range_tree.heading('class_name', text='Class')
        self.range_tree.heading('start', text='Start')
        self.range_tree.heading('end', text='End')
        self.range_tree.heading('mastery', text='Mastery')
        self.range_tree.heading('description', text='Description')
        self.range_tree.column('class_id', width=50, stretch=False)
        self.range_tree.column('class_name', width=100)
        self.range_tree.column('start', width=60, stretch=False)
        self.range_tree.column('end', width=60, stretch=False)
        self.range_tree.column('mastery', width=60, stretch=False)
        self.range_tree.column('description', width=150)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.range_tree.yview)
        self.range_tree.configure(yscrollcommand=scrollbar.set)
        self.range_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.range_tree.bind('<<TreeviewSelect>>', self._on_range_select)

        # Button row for list
        btn_frame = ttk.Frame(left_frame)
        btn_frame.pack(fill=tk.X, padx=4, pady=4)

        ttk.Button(btn_frame, text="New Range", command=self._new_range).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Delete", command=self._delete_range).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Next Available", command=self._show_next_available).pack(side=tk.LEFT, padx=2)

        # Right: editor
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=2)

        # Edit section
        edit_frame = ttk.LabelFrame(right_frame, text="Edit Vnum Range")
        edit_frame.pack(fill=tk.X, padx=4, pady=4)

        # Row 1: Class name
        row1 = ttk.Frame(edit_frame)
        row1.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row1, text="Class:").pack(side=tk.LEFT)
        self.class_label = ttk.Label(row1, text="-", font=('Consolas', 10, 'bold'), width=15)
        self.class_label.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row1, text="ID:").pack(side=tk.LEFT)
        self.id_label = ttk.Label(row1, text="-", font=('Consolas', 10), width=8)
        self.id_label.pack(side=tk.LEFT, padx=(4, 16))

        # Row 2: Armor vnum range
        row2 = ttk.Frame(edit_frame)
        row2.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row2, text="Armor Vnums:").pack(side=tk.LEFT)
        self.start_var = tk.StringVar()
        self.start_var.trace_add('write', lambda *_: self._on_change())
        self.start_entry = ttk.Entry(row2, textvariable=self.start_var, width=8, font=('Consolas', 10))
        self.start_entry.pack(side=tk.LEFT, padx=(4, 4))

        ttk.Label(row2, text="to").pack(side=tk.LEFT)
        self.end_var = tk.StringVar()
        self.end_var.trace_add('write', lambda *_: self._on_change())
        self.end_entry = ttk.Entry(row2, textvariable=self.end_var, width=8, font=('Consolas', 10))
        self.end_entry.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row2, text="(13 pieces typical: 33320-33332)", foreground='gray').pack(side=tk.LEFT)

        # Row 3: Mastery vnum
        row3 = ttk.Frame(edit_frame)
        row3.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row3, text="Mastery Vnum:").pack(side=tk.LEFT)
        self.mastery_var = tk.StringVar()
        self.mastery_var.trace_add('write', lambda *_: self._on_change())
        self.mastery_entry = ttk.Entry(row3, textvariable=self.mastery_var, width=8, font=('Consolas', 10))
        self.mastery_entry.pack(side=tk.LEFT, padx=(4, 16))

        ttk.Label(row3, text="(optional, 0 for none)", foreground='gray').pack(side=tk.LEFT)

        # Row 4: Description
        row4 = ttk.Frame(edit_frame)
        row4.pack(fill=tk.X, padx=8, pady=4)

        ttk.Label(row4, text="Description:").pack(side=tk.LEFT)
        self.desc_var = tk.StringVar()
        self.desc_var.trace_add('write', lambda *_: self._on_change())
        self.desc_entry = ttk.Entry(row4, textvariable=self.desc_var, width=40, font=('Consolas', 10))
        self.desc_entry.pack(side=tk.LEFT, padx=(4, 8), fill=tk.X, expand=True)

        # Row 5: Save button
        row5 = ttk.Frame(edit_frame)
        row5.pack(fill=tk.X, padx=8, pady=8)

        ttk.Button(row5, text="Save", command=self._save_entry).pack(side=tk.LEFT)
        ttk.Button(row5, text="Check Overlap", command=self._check_overlap).pack(side=tk.LEFT, padx=8)

        # Status/info section
        info_frame = ttk.LabelFrame(right_frame, text="Information")
        info_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        info_text = (
            "Track equipment vnum ranges to prevent conflicts when adding new classes.\n\n"
            "Each class with armor equipment needs a contiguous vnum range in classeq.db.\n"
            "Typical range is 13 vnums (one per armor slot + weapon).\n\n"
            "Use 'Next Available' to find the next free vnum range.\n"
            "Use 'Check Overlap' to verify your range doesn't conflict.\n\n"
            "Changes require server restart to take effect.\n"
            "Also update handler.c equipment restrictions when adding new ranges."
        )
        info_label = ttk.Label(info_frame, text=info_text, wraplength=400, justify=tk.LEFT)
        info_label.pack(anchor=tk.W, padx=8, pady=8)

        # Next available display
        self.next_avail_label = ttk.Label(info_frame, text="", font=('Consolas', 10, 'bold'))
        self.next_avail_label.pack(anchor=tk.W, padx=8, pady=4)

    def _load_entries(self):
        """Load entries into the tree."""
        self.range_tree.delete(*self.range_tree.get_children())

        try:
            entries = self.vnum_ranges_repo.list_all()
            for entry in entries:
                class_name = get_class_name(entry['class_id'])
                mastery = entry['mastery_vnum'] if entry['mastery_vnum'] else '-'
                desc = entry['description'] or ''
                self.range_tree.insert('', tk.END, iid=str(entry['class_id']),
                                       values=(entry['class_id'], class_name,
                                               entry['armor_vnum_start'], entry['armor_vnum_end'],
                                               mastery, desc))

            # Update next available
            next_vnum = self.vnum_ranges_repo.get_next_available_vnum()
            self.next_avail_label.config(text=f"Next available vnum range starts at: {next_vnum}")

            self.on_status(f"Loaded {len(entries)} vnum ranges")
        except Exception as e:
            self.on_status(f"Error loading vnum ranges: {e}")
            self.next_avail_label.config(text="Table may not exist yet - restart server to create")

    def _on_range_select(self, event):
        """Handle range selection."""
        if self.unsaved:
            if not messagebox.askyesno("Unsaved Changes", "Discard unsaved changes?"):
                return

        selection = self.range_tree.selection()
        if not selection:
            return

        class_id = int(selection[0])
        self.current_class_id = class_id

        entry = self.vnum_ranges_repo.get_by_id(class_id)
        if entry:
            class_name = get_class_name(class_id)
            self.class_label.config(text=class_name)
            self.id_label.config(text=str(class_id))
            self.start_var.set(str(entry['armor_vnum_start']))
            self.end_var.set(str(entry['armor_vnum_end']))
            self.mastery_var.set(str(entry['mastery_vnum']) if entry['mastery_vnum'] else '0')
            self.desc_var.set(entry['description'] or '')
            self.unsaved = False

        self.on_status(f"Editing {class_name}")

    def _on_change(self):
        """Called when any field changes."""
        self.unsaved = True

    def _save_entry(self):
        """Save current entry."""
        if self.current_class_id is None:
            messagebox.showwarning("No Selection", "Please select a range first.")
            return

        try:
            start = int(self.start_var.get().strip())
        except ValueError:
            messagebox.showwarning("Invalid Start", "Start vnum must be a number.")
            return

        try:
            end = int(self.end_var.get().strip())
        except ValueError:
            messagebox.showwarning("Invalid End", "End vnum must be a number.")
            return

        try:
            mastery = int(self.mastery_var.get().strip())
        except ValueError:
            mastery = 0

        if start > end:
            messagebox.showwarning("Invalid Range", "Start vnum must be <= end vnum.")
            return

        # Check for overlaps
        overlaps = self.vnum_ranges_repo.check_overlap(start, end, self.current_class_id)
        if overlaps:
            overlap_names = [get_class_name(o['class_id']) for o in overlaps]
            if not messagebox.askyesno("Overlap Warning",
                                        f"Range overlaps with: {', '.join(overlap_names)}\n\n"
                                        "Save anyway?"):
                return

        desc = self.desc_var.get().strip() or None

        self.vnum_ranges_repo.update(self.current_class_id, {
            'armor_vnum_start': start,
            'armor_vnum_end': end,
            'mastery_vnum': mastery if mastery > 0 else None,
            'description': desc
        })

        self.unsaved = False
        self._load_entries()  # Refresh list

        # Re-select current item
        self.range_tree.selection_set(str(self.current_class_id))

        self.on_status("Vnum range saved. Restart server to apply.")

    def _new_range(self):
        """Create a new vnum range entry."""
        # Show dialog to select class
        existing_ids = set(int(iid) for iid in self.range_tree.get_children())
        available = [(cid, name) for cid, name in CLASS_NAMES.items() if cid not in existing_ids]

        if not available:
            messagebox.showinfo("All Classes Defined",
                               "All known classes already have vnum ranges defined.")
            return

        # Simple dialog - select class by ID
        class_id = simpledialog.askinteger(
            "New Vnum Range",
            f"Enter class ID for new range:\n\nAvailable:\n" +
            "\n".join(f"  {cid}: {name}" for cid, name in sorted(available)[:10]) +
            (f"\n  ... and {len(available) - 10} more" if len(available) > 10 else ""),
            minvalue=1
        )

        if class_id is None:
            return

        if class_id in existing_ids:
            messagebox.showwarning("Already Exists", f"Class {class_id} already has a vnum range.")
            return

        # Get suggested start vnum
        next_start = self.vnum_ranges_repo.get_next_available_vnum()

        # Insert new entry
        self.vnum_ranges_repo.insert({
            'class_id': class_id,
            'armor_vnum_start': next_start,
            'armor_vnum_end': next_start + 12,  # 13 pieces
            'mastery_vnum': None,
            'description': f'{get_class_name(class_id)} armor'
        })

        self._load_entries()

        # Select the new entry
        self.range_tree.selection_set(str(class_id))
        self._on_range_select(None)

        self.on_status(f"Created vnum range for {get_class_name(class_id)}")

    def _delete_range(self):
        """Delete current vnum range."""
        if self.current_class_id is None:
            messagebox.showwarning("No Selection", "Please select a range first.")
            return

        class_name = get_class_name(self.current_class_id)
        if not messagebox.askyesno("Confirm Delete",
                                    f"Delete vnum range for {class_name}?"):
            return

        self.vnum_ranges_repo.delete(self.current_class_id)
        self.current_class_id = None

        self._clear_editor()
        self._load_entries()

        self.on_status(f"Deleted vnum range for {class_name}")

    def _clear_editor(self):
        """Clear the editor fields."""
        self.class_label.config(text="-")
        self.id_label.config(text="-")
        self.start_var.set("")
        self.end_var.set("")
        self.mastery_var.set("")
        self.desc_var.set("")
        self.unsaved = False

    def _check_overlap(self):
        """Check for overlapping ranges."""
        try:
            start = int(self.start_var.get().strip())
            end = int(self.end_var.get().strip())
        except ValueError:
            messagebox.showwarning("Invalid Range", "Enter valid start and end vnums first.")
            return

        overlaps = self.vnum_ranges_repo.check_overlap(start, end, self.current_class_id)
        if overlaps:
            overlap_info = "\n".join(
                f"  {get_class_name(o['class_id'])}: {o['armor_vnum_start']}-{o['armor_vnum_end']}"
                for o in overlaps
            )
            messagebox.showwarning("Overlap Found",
                                   f"Range {start}-{end} overlaps with:\n\n{overlap_info}")
        else:
            messagebox.showinfo("No Overlap", f"Range {start}-{end} does not overlap with any existing ranges.")

    def _show_next_available(self):
        """Show next available vnum range."""
        next_vnum = self.vnum_ranges_repo.get_next_available_vnum()
        messagebox.showinfo("Next Available",
                           f"Next available vnum range starts at: {next_vnum}\n\n"
                           f"Suggested range for 13 armor pieces: {next_vnum}-{next_vnum + 12}")

    def check_unsaved(self) -> bool:
        """Check for unsaved changes before closing."""
        if self.unsaved:
            return messagebox.askyesno("Unsaved Changes",
                                        "You have unsaved changes. Discard them?")
        return True
