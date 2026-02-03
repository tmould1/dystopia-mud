#!/usr/bin/env python3
"""
GUI Help File Editor for Dystopia MUD.

Tkinter-based browser/editor for base_help.db with live color preview.

Usage:
    python helpedit_gui.py
"""

import sqlite3
import sys
import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from pathlib import Path

# Import shared code from helpedit.py
sys.path.insert(0, str(Path(__file__).resolve().parent))
from helpedit import DB_PATH, COLOR_CODES, strip_colors, get_db


# ---------------------------------------------------------------------------
# MUD color code -> hex color mapping for tkinter
# ---------------------------------------------------------------------------

TK_COLORS = {
    '0': '#808080',   # Bright Black (gray)
    '1': '#ff5555',   # Bright Red
    '2': '#55ff55',   # Bright Green
    '3': '#ffff55',   # Bright Yellow
    '4': '#5555ff',   # Bright Blue
    '5': '#ff55ff',   # Bright Purple
    '6': '#55ffff',   # Bright Cyan
    '7': '#c0c0c0',   # White
    '8': '#404040',   # Black
    '9': '#ffffff',   # Bright White
    'r': '#aa0000',   # Dark Red
    'g': '#00aa00',   # Dark Green
    'o': '#aa5500',   # Brown
    'l': '#0000aa',   # Dark Blue
    'p': '#aa00aa',   # Dark Purple
    'c': '#00aaaa',   # Dark Cyan
    'y': '#ffff55',   # Bright Yellow
    'R': '#ff5555',   # Bright Red
    'G': '#55ff55',   # Bright Green
    'L': '#5555ff',   # Bright Blue
    'P': '#ff55ff',   # Bright Purple
    'C': '#55ffff',   # Bright Cyan
    'n': '#c0c0c0',   # Reset (default text)
    'i': None,        # Inverse (handled specially)
    'u': None,        # Underline (handled specially)
}

DEFAULT_FG = '#c0c0c0'
PREVIEW_BG = '#1a1a1a'
EDITOR_BG  = '#2d2d2d'
EDITOR_FG  = '#d4d4d4'

# Approximate 256-color palette to hex
def xterm256_to_hex(n):
    """Convert xterm 256-color index to hex color."""
    if n < 16:
        # Standard colors
        basic = [
            '#000000','#aa0000','#00aa00','#aa5500',
            '#0000aa','#aa00aa','#00aaaa','#c0c0c0',
            '#808080','#ff5555','#55ff55','#ffff55',
            '#5555ff','#ff55ff','#55ffff','#ffffff',
        ]
        return basic[n]
    elif n < 232:
        # 6x6x6 color cube
        n -= 16
        b = n % 6
        g = (n // 6) % 6
        r = n // 36
        return f'#{r*51:02x}{g*51:02x}{b*51:02x}'
    else:
        # Grayscale
        v = 8 + (n - 232) * 10
        return f'#{v:02x}{v:02x}{v:02x}'


# ---------------------------------------------------------------------------
# Preview renderer: parse MUD text into (text, tag) segments for tk.Text
# ---------------------------------------------------------------------------

def parse_colored_segments(text):
    """Parse MUD color-coded text into list of (plain_text, color_tag) tuples."""
    segments = []
    current_tag = 'color_n'
    buf = []
    i = 0

    while i < len(text):
        if text[i] == '#' and i + 1 < len(text):
            nxt = text[i + 1]
            # Flush buffer
            if buf:
                segments.append((''.join(buf), current_tag))
                buf = []

            if nxt == '#':
                buf.append('#')
                i += 2
            elif nxt == '-':
                buf.append('~')
                i += 2
            elif nxt == '+':
                buf.append('%')
                i += 2
            elif nxt == 'x' and i + 4 < len(text) and text[i+2:i+5].isdigit():
                code = int(text[i+2:i+5])
                current_tag = f'color_x{code}'
                i += 5
            elif nxt in TK_COLORS:
                current_tag = f'color_{nxt}'
                i += 2
            else:
                buf.append(text[i])
                i += 1
        else:
            buf.append(text[i])
            i += 1

    if buf:
        segments.append((''.join(buf), current_tag))

    return segments


# ---------------------------------------------------------------------------
# Main application
# ---------------------------------------------------------------------------

class HelpEditorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Dystopia MUD Help Editor")
        self.root.geometry("1100x720")
        self.root.minsize(800, 500)

        self.entries = []       # List of (id, level, keyword, text)
        self.current_id = None  # Currently selected entry ID
        self.unsaved = False

        self._build_ui()
        self._setup_tags()
        self._load_entries()

    # ----- UI Construction -----

    def _build_ui(self):
        # Main horizontal pane
        paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left panel: search + entry list
        left = ttk.Frame(paned, width=280)
        paned.add(left, weight=1)

        # Search bar
        search_frame = ttk.Frame(left)
        search_frame.pack(fill=tk.X, padx=2, pady=(2, 4))
        ttk.Label(search_frame, text="Search:").pack(side=tk.LEFT)
        self.search_var = tk.StringVar()
        self.search_var.trace_add('write', lambda *_: self._filter_list())
        search_entry = ttk.Entry(search_frame, textvariable=self.search_var)
        search_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(4, 0))

        # Treeview
        tree_frame = ttk.Frame(left)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(
            tree_frame, columns=('id', 'keyword'), show='headings',
            selectmode='browse'
        )
        self.tree.heading('id', text='ID', anchor=tk.W)
        self.tree.heading('keyword', text='Keyword', anchor=tk.W)
        self.tree.column('id', width=40, minwidth=30, stretch=False)
        self.tree.column('keyword', width=200, minwidth=100)

        tree_scroll = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL,
                                     command=self.tree.yview)
        self.tree.configure(yscrollcommand=tree_scroll.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        tree_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree.bind('<<TreeviewSelect>>', self._on_select)

        # Right panel: editor
        right = ttk.Frame(paned)
        paned.add(right, weight=3)

        # Header: keyword, level, id
        header = ttk.Frame(right)
        header.pack(fill=tk.X, padx=2, pady=(2, 4))

        ttk.Label(header, text="Keyword:").grid(row=0, column=0, sticky=tk.W)
        self.keyword_var = tk.StringVar()
        self.keyword_entry = ttk.Entry(header, textvariable=self.keyword_var,
                                        width=40)
        self.keyword_entry.grid(row=0, column=1, sticky=tk.W, padx=(4, 12))

        ttk.Label(header, text="Level:").grid(row=0, column=2, sticky=tk.W)
        self.level_var = tk.IntVar(value=0)
        self.level_spin = ttk.Spinbox(header, from_=-1, to=100, width=5,
                                       textvariable=self.level_var)
        self.level_spin.grid(row=0, column=3, sticky=tk.W, padx=(4, 12))

        ttk.Label(header, text="ID:").grid(row=0, column=4, sticky=tk.W)
        self.id_label = ttk.Label(header, text="-")
        self.id_label.grid(row=0, column=5, sticky=tk.W, padx=(4, 0))

        # Preview pane (top half of right)
        preview_label = ttk.Label(right, text="Preview (colored)")
        preview_label.pack(anchor=tk.W, padx=2)

        preview_frame = ttk.Frame(right)
        preview_frame.pack(fill=tk.BOTH, expand=True, padx=2, pady=(0, 4))

        self.preview = tk.Text(
            preview_frame, wrap=tk.WORD, state=tk.DISABLED,
            bg=PREVIEW_BG, fg=DEFAULT_FG,
            font=('Consolas', 10), insertbackground=DEFAULT_FG,
            relief=tk.SUNKEN, borderwidth=2
        )
        preview_scroll = ttk.Scrollbar(preview_frame, orient=tk.VERTICAL,
                                        command=self.preview.yview)
        self.preview.configure(yscrollcommand=preview_scroll.set)
        self.preview.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        preview_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        # Raw editor pane (bottom half of right)
        editor_label = ttk.Label(right, text="Raw Editor (color codes)")
        editor_label.pack(anchor=tk.W, padx=2)

        editor_frame = ttk.Frame(right)
        editor_frame.pack(fill=tk.BOTH, expand=True, padx=2, pady=(0, 4))

        self.editor = tk.Text(
            editor_frame, wrap=tk.WORD,
            bg=EDITOR_BG, fg=EDITOR_FG,
            font=('Consolas', 10), insertbackground='#ffffff',
            relief=tk.SUNKEN, borderwidth=2, undo=True
        )
        editor_scroll = ttk.Scrollbar(editor_frame, orient=tk.VERTICAL,
                                       command=self.editor.yview)
        self.editor.configure(yscrollcommand=editor_scroll.set)
        self.editor.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        editor_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        # Button bar
        btn_frame = ttk.Frame(right)
        btn_frame.pack(fill=tk.X, padx=2, pady=(0, 2))

        ttk.Button(btn_frame, text="Save", command=self._save).pack(
            side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="New", command=self._new).pack(
            side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Delete", command=self._delete).pack(
            side=tk.LEFT, padx=(0, 4))
        ttk.Button(btn_frame, text="Refresh Preview",
                    command=self._refresh_preview).pack(side=tk.LEFT, padx=(0, 4))

        # Status bar
        self.status_var = tk.StringVar(value="Ready")
        status = ttk.Label(self.root, textvariable=self.status_var,
                           relief=tk.SUNKEN, anchor=tk.W)
        status.pack(fill=tk.X, side=tk.BOTTOM, padx=4, pady=(0, 4))

        # Keyboard shortcuts
        self.root.bind('<Control-s>', lambda e: self._save())
        self.root.bind('<Control-n>', lambda e: self._new())
        self.root.bind('<F5>', lambda e: self._refresh_preview())

    def _setup_tags(self):
        """Configure text tags for color rendering in preview."""
        for code, hex_color in TK_COLORS.items():
            tag = f'color_{code}'
            if code == 'i':
                # Inverse: swap fg/bg
                self.preview.tag_configure(tag, background=DEFAULT_FG,
                                           foreground=PREVIEW_BG)
            elif code == 'u':
                self.preview.tag_configure(tag, underline=True,
                                           foreground=DEFAULT_FG)
            elif hex_color:
                self.preview.tag_configure(tag, foreground=hex_color)

    # ----- Data Loading -----

    def _load_entries(self):
        """Load all entries from database."""
        conn = get_db()
        self.entries = conn.execute(
            "SELECT id, level, keyword, text FROM helps ORDER BY id"
        ).fetchall()
        conn.close()
        self._populate_tree()
        self.status_var.set(f"Loaded {len(self.entries)} entries from {DB_PATH.name}")

    def _populate_tree(self):
        """Fill the treeview with entries, applying search filter."""
        self.tree.delete(*self.tree.get_children())
        search = self.search_var.get().lower()

        for entry in self.entries:
            eid, level, keyword, text = entry
            if search:
                if (search not in keyword.lower() and
                    search not in strip_colors(text).lower()):
                    continue
            self.tree.insert('', tk.END, iid=str(eid),
                           values=(eid, keyword))

    def _filter_list(self):
        """Re-filter the tree based on search box."""
        self._populate_tree()

    # ----- Selection -----

    def _on_select(self, event):
        """Handle treeview selection."""
        sel = self.tree.selection()
        if not sel:
            return

        if self.unsaved:
            if not messagebox.askyesno("Unsaved Changes",
                    "You have unsaved changes. Discard them?"):
                return

        eid = int(sel[0])
        self._load_entry(eid)

    def _load_entry(self, eid):
        """Load a specific entry into the editor."""
        entry = None
        for e in self.entries:
            if e[0] == eid:
                entry = e
                break
        if not entry:
            return

        self.current_id = eid
        _, level, keyword, text = entry

        self.keyword_var.set(keyword)
        self.level_var.set(level)
        self.id_label.configure(text=str(eid))

        # Raw editor
        self.editor.delete('1.0', tk.END)
        self.editor.insert('1.0', text)
        self.editor.edit_reset()

        # Preview
        self._render_preview(text)

        self.unsaved = False
        self.status_var.set(f"Loaded [{eid}] {keyword}")

    # ----- Preview Rendering -----

    def _render_preview(self, text):
        """Render color-coded text into the preview widget."""
        self.preview.configure(state=tk.NORMAL)
        self.preview.delete('1.0', tk.END)

        segments = parse_colored_segments(text)
        for plain, tag in segments:
            # Ensure tag exists for dynamic #xNNN colors
            if tag.startswith('color_x') and tag not in self._get_existing_tags():
                try:
                    code = int(tag[7:])
                    hex_color = xterm256_to_hex(code)
                    self.preview.tag_configure(tag, foreground=hex_color)
                except (ValueError, IndexError):
                    pass
            self.preview.insert(tk.END, plain, tag)

        self.preview.configure(state=tk.DISABLED)

    def _get_existing_tags(self):
        """Return set of configured tag names."""
        return set(self.preview.tag_names())

    def _refresh_preview(self):
        """Refresh preview from current editor content."""
        text = self.editor.get('1.0', tk.END).rstrip('\n')
        self._render_preview(text)
        self.status_var.set("Preview refreshed")

    # ----- CRUD Operations -----

    def _save(self):
        """Save current entry to database."""
        if self.current_id is None:
            self.status_var.set("No entry selected")
            return

        keyword = self.keyword_var.get().strip()
        if not keyword:
            messagebox.showwarning("Warning", "Keyword cannot be empty.")
            return

        level = self.level_var.get()
        text = self.editor.get('1.0', tk.END).rstrip('\n')

        conn = get_db()
        conn.execute(
            "UPDATE helps SET keyword = ?, level = ?, text = ? WHERE id = ?",
            (keyword, level, text, self.current_id)
        )
        conn.commit()
        conn.close()

        # Refresh in-memory data
        self.entries = [
            (eid, level, keyword, text) if eid == self.current_id else e
            for eid, e in ((e[0], e) for e in self.entries)
        ]
        self._load_entries_keep_selection()
        self._render_preview(text)

        self.unsaved = False
        self.status_var.set(f"Saved [{self.current_id}] {keyword}")

    def _new(self):
        """Create a new help entry."""
        keyword = simpledialog.askstring("New Help Entry", "Keyword(s):",
                                          parent=self.root)
        if not keyword:
            return
        keyword = keyword.upper().strip()

        conn = get_db()
        conn.execute(
            "INSERT INTO helps (level, keyword, text) VALUES (?, ?, ?)",
            (0, keyword, '')
        )
        conn.commit()
        new_id = conn.execute("SELECT last_insert_rowid()").fetchone()[0]
        conn.close()

        self._load_entries()
        # Select the new entry
        self.tree.selection_set(str(new_id))
        self.tree.see(str(new_id))
        self._load_entry(new_id)
        self.status_var.set(f"Created [{new_id}] {keyword}")

    def _delete(self):
        """Delete the current entry."""
        if self.current_id is None:
            self.status_var.set("No entry selected")
            return

        keyword = self.keyword_var.get()
        if not messagebox.askyesno("Confirm Delete",
                f"Delete [{self.current_id}] {keyword}?"):
            return

        conn = get_db()
        conn.execute("DELETE FROM helps WHERE id = ?", (self.current_id,))
        conn.commit()
        conn.close()

        old_id = self.current_id
        self.current_id = None
        self.keyword_var.set('')
        self.level_var.set(0)
        self.id_label.configure(text='-')
        self.editor.delete('1.0', tk.END)
        self.preview.configure(state=tk.NORMAL)
        self.preview.delete('1.0', tk.END)
        self.preview.configure(state=tk.DISABLED)

        self._load_entries()
        self.unsaved = False
        self.status_var.set(f"Deleted [{old_id}] {keyword}")

    def _load_entries_keep_selection(self):
        """Reload entries but keep current selection."""
        conn = get_db()
        self.entries = conn.execute(
            "SELECT id, level, keyword, text FROM helps ORDER BY id"
        ).fetchall()
        conn.close()
        self._populate_tree()

        if self.current_id is not None:
            try:
                self.tree.selection_set(str(self.current_id))
                self.tree.see(str(self.current_id))
            except tk.TclError:
                pass


def main():
    if not DB_PATH.exists():
        print(f"Error: {DB_PATH} not found.")
        sys.exit(1)

    root = tk.Tk()
    app = HelpEditorApp(root)
    root.mainloop()


if __name__ == '__main__':
    main()
