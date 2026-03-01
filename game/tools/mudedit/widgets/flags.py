"""
Flag editor widget for editing bitfield flags.

Displays a grid of checkboxes for each flag in a bitfield.
"""

import tkinter as tk
from tkinter import ttk
from typing import Callable, Dict, Optional


class FlagEditor(ttk.LabelFrame):
    """
    A checkbox grid for editing bitfield flags.

    Displays each flag as a checkbox and combines them into a single integer value.
    """

    def __init__(
        self,
        parent,
        flags: Dict[int, str],
        label: str = "Flags",
        columns: int = 3,
        on_change: Optional[Callable[[int], None]] = None,
        **kwargs
    ):
        """
        Initialize the flag editor.

        Args:
            parent: Parent Tkinter widget
            flags: Dictionary mapping bit values to flag names
            label: Label for the frame
            columns: Number of columns in the checkbox grid
            on_change: Callback invoked with new value when flags change
            **kwargs: Additional arguments passed to ttk.LabelFrame
        """
        super().__init__(parent, text=label, **kwargs)

        self.flags = flags
        self.columns = columns
        self.on_change = on_change

        self._checkboxes: Dict[int, tk.BooleanVar] = {}
        self._build_ui()

    def _build_ui(self):
        """Build the checkbox grid."""
        # Sort flags by bit value
        sorted_flags = sorted(self.flags.items(), key=lambda x: x[0])

        self._widgets = []
        row = 0
        col = 0
        for bit, name in sorted_flags:
            var = tk.BooleanVar(value=False)
            self._checkboxes[bit] = var

            cb = ttk.Checkbutton(
                self,
                text=name,
                variable=var,
                command=self._on_checkbox_change
            )
            cb.grid(row=row, column=col, sticky=tk.W, padx=2, pady=1)
            self._widgets.append(cb)

            col += 1
            if col >= self.columns:
                col = 0
                row += 1

        # Configure column weights for even distribution
        for c in range(self.columns):
            self.columnconfigure(c, weight=1)

    def _on_checkbox_change(self):
        """Handle checkbox state change."""
        if self.on_change:
            self.on_change(self.get_value())

    def get_value(self) -> int:
        """Get the combined bitfield value."""
        value = 0
        for bit, var in self._checkboxes.items():
            if var.get():
                value |= bit
        return value

    def set_value(self, value: int):
        """
        Set the bitfield value.

        Args:
            value: Integer bitfield value
        """
        for bit, var in self._checkboxes.items():
            var.set(bool(value & bit))

    def clear(self):
        """Clear all flags."""
        for var in self._checkboxes.values():
            var.set(False)

    def set_enabled(self, enabled: bool):
        """Enable or disable all checkboxes."""
        state = '!disabled' if enabled else 'disabled'
        for cb in self._widgets:
            cb.state([state])

    def get_flag_names(self) -> list:
        """Get a list of currently set flag names."""
        names = []
        for bit, var in self._checkboxes.items():
            if var.get():
                names.append(self.flags[bit])
        return names


class CompactFlagDisplay(ttk.Frame):
    """
    A compact, read-only display of flag values.

    Shows flags as comma-separated names with a numeric value.
    """

    def __init__(
        self,
        parent,
        flags: Dict[int, str],
        **kwargs
    ):
        """
        Initialize the compact flag display.

        Args:
            parent: Parent Tkinter widget
            flags: Dictionary mapping bit values to flag names
            **kwargs: Additional arguments passed to ttk.Frame
        """
        super().__init__(parent, **kwargs)

        self.flags = flags
        self._value = 0

        self._build_ui()

    def _build_ui(self):
        """Build the display UI."""
        self.label = ttk.Label(self, text="(none)")
        self.label.pack(side=tk.LEFT, fill=tk.X, expand=True)

        self.value_label = ttk.Label(self, text="[0]", foreground='gray')
        self.value_label.pack(side=tk.RIGHT)

    def set_value(self, value: int):
        """Set the displayed value."""
        self._value = value

        # Get flag names
        names = []
        for bit, name in sorted(self.flags.items()):
            if value & bit:
                names.append(name)

        if names:
            self.label.config(text=', '.join(names))
        else:
            self.label.config(text='(none)')

        self.value_label.config(text=f'[{value}]')

    def get_value(self) -> int:
        """Get the current value."""
        return self._value
