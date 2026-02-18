"""
Color text editor widget with live MUD color code preview.

Provides a split view with raw text editor (top) and colored preview (bottom).
"""

import tkinter as tk
from tkinter import ttk
from typing import Callable, Optional, Set

from .colors import (
    TK_COLORS, DEFAULT_FG, PREVIEW_BG, EDITOR_BG, EDITOR_FG,
    xterm256_to_hex, parse_colored_segments
)


class ColorTextEditor(ttk.Frame):
    """
    Text editor with live MUD color code preview.

    Displays a raw text editor on top and a read-only colored preview below.
    Supports standard MUD color codes (#X) and xterm 256-color codes (#xNNN).
    """

    def __init__(
        self,
        parent,
        show_preview: bool = True,
        on_change: Optional[Callable[[], None]] = None,
        **kwargs
    ):
        """
        Initialize the color text editor.

        Args:
            parent: Parent Tkinter widget
            show_preview: Whether to show the preview pane
            on_change: Callback invoked when text changes
            **kwargs: Additional arguments passed to ttk.Frame
        """
        super().__init__(parent, **kwargs)

        self.show_preview = show_preview
        self.on_change = on_change
        self._existing_tags: Set[str] = set()

        self._build_ui()
        self._setup_color_tags()

    def _build_ui(self):
        """Build the editor UI."""
        if self.show_preview:
            # Paned window for resizable split
            paned = ttk.PanedWindow(self, orient=tk.VERTICAL)
            paned.pack(fill=tk.BOTH, expand=True)

            # Preview pane (top)
            preview_frame = ttk.LabelFrame(paned, text="Preview (colored)")
            paned.add(preview_frame, weight=1)

            self.preview = tk.Text(
                preview_frame,
                wrap=tk.WORD,
                state=tk.DISABLED,
                bg=PREVIEW_BG,
                fg=DEFAULT_FG,
                font=('Consolas', 10),
                insertbackground=DEFAULT_FG,
                relief=tk.SUNKEN,
                borderwidth=2,
                height=8
            )
            preview_scroll = ttk.Scrollbar(
                preview_frame,
                orient=tk.VERTICAL,
                command=self.preview.yview
            )
            self.preview.configure(yscrollcommand=preview_scroll.set)
            self.preview.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
            preview_scroll.pack(side=tk.RIGHT, fill=tk.Y)

            # Editor pane (bottom)
            editor_frame = ttk.LabelFrame(paned, text="Raw Editor (color codes)")
            paned.add(editor_frame, weight=1)

            self.editor = tk.Text(
                editor_frame,
                wrap=tk.WORD,
                bg=EDITOR_BG,
                fg=EDITOR_FG,
                font=('Consolas', 10),
                insertbackground='#ffffff',
                relief=tk.SUNKEN,
                borderwidth=2,
                undo=True,
                height=8
            )
            editor_scroll = ttk.Scrollbar(
                editor_frame,
                orient=tk.VERTICAL,
                command=self.editor.yview
            )
            self.editor.configure(yscrollcommand=editor_scroll.set)
            self.editor.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
            editor_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        else:
            # No preview, just the editor
            self.preview = None

            self.editor = tk.Text(
                self,
                wrap=tk.WORD,
                bg=EDITOR_BG,
                fg=EDITOR_FG,
                font=('Consolas', 10),
                insertbackground='#ffffff',
                relief=tk.SUNKEN,
                borderwidth=2,
                undo=True
            )
            editor_scroll = ttk.Scrollbar(
                self,
                orient=tk.VERTICAL,
                command=self.editor.yview
            )
            self.editor.configure(yscrollcommand=editor_scroll.set)
            self.editor.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
            editor_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        # Bind change detection
        self.editor.bind('<KeyRelease>', self._on_key_release)

    def _setup_color_tags(self):
        """Configure text tags for color rendering in preview."""
        if not self.preview:
            return

        for code, hex_color in TK_COLORS.items():
            tag = f'color_{code}'
            if code == 'i':
                # Inverse: swap fg/bg
                self.preview.tag_configure(
                    tag,
                    background=DEFAULT_FG,
                    foreground=PREVIEW_BG
                )
            elif code == 'u':
                # Underline
                self.preview.tag_configure(
                    tag,
                    underline=True,
                    foreground=DEFAULT_FG
                )
            elif hex_color:
                self.preview.tag_configure(tag, foreground=hex_color)

            self._existing_tags.add(tag)

    def _on_key_release(self, event=None):
        """Handle key release - refresh preview and notify change callback."""
        if self.show_preview:
            self.refresh_preview()

        if self.on_change:
            self.on_change()

    def refresh_preview(self):
        """Refresh the preview from current editor content."""
        if not self.preview:
            return

        text = self.get_text()
        self._render_preview(text)

    def _render_preview(self, text: str):
        """Render color-coded text into the preview widget."""
        self.preview.configure(state=tk.NORMAL)
        self.preview.delete('1.0', tk.END)

        segments = parse_colored_segments(text)
        for plain, tag in segments:
            # Ensure tag exists for dynamic colors
            if tag not in self._existing_tags:
                if tag.startswith('color_x'):
                    try:
                        code = int(tag[7:])
                        hex_color = xterm256_to_hex(code)
                        self.preview.tag_configure(tag, foreground=hex_color)
                        self._existing_tags.add(tag)
                    except (ValueError, IndexError):
                        pass
                elif tag.startswith('color_t'):
                    hex_color = f'#{tag[7:]}'
                    self.preview.tag_configure(tag, foreground=hex_color)
                    self._existing_tags.add(tag)
                elif tag.startswith('color_T'):
                    hex_color = f'#{tag[7:]}'
                    self.preview.tag_configure(tag, background=hex_color)
                    self._existing_tags.add(tag)

            self.preview.insert(tk.END, plain, tag)

        self.preview.configure(state=tk.DISABLED)

    def get_text(self) -> str:
        """Get the current text content (without trailing newline)."""
        return self.editor.get('1.0', tk.END).rstrip('\n')

    def set_text(self, text: str):
        """Set the text content and refresh preview."""
        self.editor.delete('1.0', tk.END)
        self.editor.insert('1.0', text)
        self.editor.edit_reset()  # Clear undo history

        if self.show_preview:
            self._render_preview(text)

    def clear(self):
        """Clear all text content."""
        self.editor.delete('1.0', tk.END)
        self.editor.edit_reset()

        if self.preview:
            self.preview.configure(state=tk.NORMAL)
            self.preview.delete('1.0', tk.END)
            self.preview.configure(state=tk.DISABLED)

    def focus_set(self):
        """Set focus to the editor."""
        self.editor.focus_set()

    def configure_editor(self, **kwargs):
        """Configure the editor widget."""
        self.editor.configure(**kwargs)

    def configure_preview(self, **kwargs):
        """Configure the preview widget (if visible)."""
        if self.preview:
            self.preview.configure(**kwargs)
