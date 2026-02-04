"""
Dice notation editor widget.

For editing MUD dice expressions like "3d6+10" (NdS+P format).
"""

import tkinter as tk
from tkinter import ttk
from typing import Callable, Optional, Tuple


class DiceEditor(ttk.Frame):
    """
    Editor for dice notation (NdS+P format).

    Displays three spinboxes: [N] d [S] + [P]
    Where N = number of dice, S = sides per die, P = bonus/penalty
    """

    def __init__(
        self,
        parent,
        label: str = "",
        on_change: Optional[Callable[[Tuple[int, int, int]], None]] = None,
        **kwargs
    ):
        """
        Initialize the dice editor.

        Args:
            parent: Parent Tkinter widget
            label: Optional label to display before the dice notation
            on_change: Callback invoked with (n, s, p) tuple when values change
            **kwargs: Additional arguments passed to ttk.Frame
        """
        super().__init__(parent, **kwargs)

        self.on_change = on_change
        self._build_ui(label)

    def _build_ui(self, label: str):
        """Build the editor UI."""
        if label:
            ttk.Label(self, text=label).pack(side=tk.LEFT, padx=(0, 4))

        # Number of dice
        self.num_var = tk.IntVar(value=1)
        self.num_spin = ttk.Spinbox(
            self,
            from_=1,
            to=9999,
            width=5,
            textvariable=self.num_var,
            command=self._on_value_change
        )
        self.num_spin.pack(side=tk.LEFT)
        self.num_spin.bind('<KeyRelease>', lambda e: self._on_value_change())

        ttk.Label(self, text="d").pack(side=tk.LEFT, padx=2)

        # Dice size
        self.size_var = tk.IntVar(value=6)
        self.size_spin = ttk.Spinbox(
            self,
            from_=1,
            to=99999,
            width=6,
            textvariable=self.size_var,
            command=self._on_value_change
        )
        self.size_spin.pack(side=tk.LEFT)
        self.size_spin.bind('<KeyRelease>', lambda e: self._on_value_change())

        ttk.Label(self, text="+").pack(side=tk.LEFT, padx=2)

        # Bonus
        self.bonus_var = tk.IntVar(value=0)
        self.bonus_spin = ttk.Spinbox(
            self,
            from_=-999999,
            to=999999,
            width=8,
            textvariable=self.bonus_var,
            command=self._on_value_change
        )
        self.bonus_spin.pack(side=tk.LEFT)
        self.bonus_spin.bind('<KeyRelease>', lambda e: self._on_value_change())

        # Average display
        self.avg_var = tk.StringVar(value="")
        ttk.Label(
            self,
            textvariable=self.avg_var,
            foreground='gray'
        ).pack(side=tk.LEFT, padx=(8, 0))

        self._update_average()

    def _on_value_change(self):
        """Handle value change."""
        self._update_average()
        if self.on_change:
            self.on_change(self.get_value())

    def _update_average(self):
        """Update the average display."""
        try:
            n, s, p = self.get_value()
            avg = n * (s + 1) / 2 + p
            self.avg_var.set(f"avg: {avg:.1f}")
        except (ValueError, tk.TclError):
            self.avg_var.set("")

    def get_value(self) -> Tuple[int, int, int]:
        """Get the current dice values as (num, size, bonus) tuple."""
        try:
            n = self.num_var.get()
            s = self.size_var.get()
            p = self.bonus_var.get()
            return (n, s, p)
        except tk.TclError:
            return (1, 1, 0)

    def set_value(self, value: Tuple[int, int, int]):
        """
        Set the dice values.

        Args:
            value: Tuple of (num_dice, dice_size, bonus)
        """
        n, s, p = value
        self.num_var.set(n)
        self.size_var.set(s)
        self.bonus_var.set(p)
        self._update_average()

    def clear(self):
        """Reset to default values."""
        self.set_value((1, 1, 0))

    def format_string(self) -> str:
        """Get the dice notation as a formatted string."""
        n, s, p = self.get_value()
        if p > 0:
            return f"{n}d{s}+{p}"
        elif p < 0:
            return f"{n}d{s}{p}"
        else:
            return f"{n}d{s}"


class DiceDisplay(ttk.Label):
    """
    A read-only display of dice notation.

    Shows the notation and calculated average.
    """

    def __init__(self, parent, **kwargs):
        """
        Initialize the dice display.

        Args:
            parent: Parent Tkinter widget
            **kwargs: Additional arguments passed to ttk.Label
        """
        super().__init__(parent, **kwargs)
        self._value = (1, 1, 0)

    def set_value(self, value: Tuple[int, int, int]):
        """Set the displayed dice value."""
        self._value = value
        n, s, p = value

        # Format the string
        if p > 0:
            dice_str = f"{n}d{s}+{p}"
        elif p < 0:
            dice_str = f"{n}d{s}{p}"
        else:
            dice_str = f"{n}d{s}"

        # Calculate average
        avg = n * (s + 1) / 2 + p

        self.config(text=f"{dice_str} (avg: {avg:.0f})")

    def get_value(self) -> Tuple[int, int, int]:
        """Get the current dice value."""
        return self._value
