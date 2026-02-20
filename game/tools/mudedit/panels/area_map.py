"""
3D Area Map Visualizer panel.

Displays rooms as colored cubes in 3D space with directional arrows for exits.
Supports rotation, zoom, pan, and click-to-inspect.
"""

import math
import tkinter as tk
from tkinter import ttk
from typing import Callable, Dict, List, Optional, Set, Tuple

from ..viz.layout import (
    DIR_NAMES, REVERSE_DIR, SECTOR_NAMES, SECTOR_COLORS, ROOM_FLAGS,
    assign_room_coordinates, detect_one_way_exits, detect_warped_exits,
    decode_flags,
)
from ..viz.renderer import (
    rotate_point, project_point, draw_cube, draw_exit_arrow,
)


class AreaMapPanel(ttk.Frame):
    """
    3D area map visualizer panel.

    Shows rooms as colored cubes with directional arrows for exits.
    Click a cube to see room details in the side panel.
    """

    CUBE_SIZE = 0.6
    CUBE_SPACING = 2.0
    DEFAULT_ZOOM = 50.0
    DEFAULT_ROT_X = math.radians(25)
    DEFAULT_ROT_Y = math.radians(45)
    RENDER_DEBOUNCE_MS = 33  # ~30fps during drag

    def __init__(
        self,
        parent,
        room_repo,
        exit_repo,
        on_open_room: Optional[Callable] = None,
        on_status: Optional[Callable[[str], None]] = None,
        **kwargs
    ):
        super().__init__(parent, **kwargs)

        self.room_repo = room_repo
        self.exit_repo = exit_repo
        self.on_open_room = on_open_room
        self.on_status = on_status or (lambda m: None)

        # Room data
        self._rooms: Dict[int, dict] = {}       # vnum -> room dict
        self._exits_by_room: Dict[int, List[dict]] = {}
        self._coords: Dict[int, Tuple[int, int, int]] = {}
        self._one_way: Set[Tuple[int, int]] = set()
        self._warped: Set[Tuple[int, int]] = set()
        self._room_name_cache: Dict[int, str] = {}
        self._z_levels: List[int] = []

        # Camera state
        self._rot_x = self.DEFAULT_ROT_X
        self._rot_y = self.DEFAULT_ROT_Y
        self._zoom = self.DEFAULT_ZOOM
        self._pan_x = 0.0
        self._pan_y = 0.0

        # Interaction state
        self._drag_start = None
        self._drag_button = None
        self._drag_moved = False
        self._selected_vnum: Optional[int] = None
        self._render_pending = False

        self._build_ui()
        self._load_data()

    def _build_ui(self):
        """Build the panel UI."""
        # Top toolbar
        toolbar = ttk.Frame(self)
        toolbar.pack(fill=tk.X, padx=4, pady=(4, 0))

        ttk.Label(toolbar, text='Z-Level:').pack(side=tk.LEFT, padx=(0, 4))
        self._z_var = tk.StringVar(value='All')
        self._z_combo = ttk.Combobox(
            toolbar, textvariable=self._z_var,
            values=['All'], state='readonly', width=8
        )
        self._z_combo.pack(side=tk.LEFT, padx=(0, 8))
        self._z_combo.bind('<<ComboboxSelected>>', lambda e: self._render())

        ttk.Button(toolbar, text='Reset View', command=self._reset_view).pack(
            side=tk.LEFT, padx=(0, 4))
        ttk.Button(toolbar, text='Fit', command=self._fit_view).pack(
            side=tk.LEFT, padx=(0, 8))
        ttk.Button(toolbar, text='Top Down', command=self._top_down_view).pack(
            side=tk.LEFT, padx=(0, 4))

        self._info_label = ttk.Label(toolbar, text='')
        self._info_label.pack(side=tk.RIGHT, padx=4)

        # Main horizontal split
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Left: 3D canvas
        canvas_frame = ttk.Frame(paned)
        paned.add(canvas_frame, weight=3)

        self._canvas = tk.Canvas(
            canvas_frame, bg='#1a1a2e', highlightthickness=0
        )
        self._canvas.pack(fill=tk.BOTH, expand=True)

        # Canvas bindings - use press/release for click detection (not Button-1
        # which conflicts with ButtonPress-1 in Tkinter)
        self._canvas.bind('<ButtonPress-1>', self._on_press)
        self._canvas.bind('<ButtonPress-3>', self._on_press)
        self._canvas.bind('<B1-Motion>', self._on_drag)
        self._canvas.bind('<B3-Motion>', self._on_pan_drag)
        self._canvas.bind('<ButtonRelease-1>', self._on_release)
        self._canvas.bind('<ButtonRelease-3>', self._on_release)
        self._canvas.bind('<Double-Button-1>', self._on_double_click)
        self._canvas.bind('<MouseWheel>', self._on_scroll)
        self._canvas.bind('<Configure>', self._on_resize)

        # Right: details panel
        details_frame = ttk.LabelFrame(paned, text='Room Details', width=300)
        paned.add(details_frame, weight=0)

        details_inner = ttk.Frame(details_frame)
        details_inner.pack(fill=tk.BOTH, expand=True, padx=6, pady=6)

        # Room name
        self._detail_name = ttk.Label(
            details_inner, text='Click a room to view details',
            font=('Consolas', 12, 'bold'), wraplength=280
        )
        self._detail_name.pack(anchor=tk.W, pady=(0, 4))

        # VNUM + Sector
        info_frame = ttk.Frame(details_inner)
        info_frame.pack(fill=tk.X, pady=(0, 4))

        ttk.Label(info_frame, text='VNUM:', font=('Consolas', 9, 'bold')).pack(
            side=tk.LEFT)
        self._detail_vnum = ttk.Label(info_frame, text='', font=('Consolas', 9))
        self._detail_vnum.pack(side=tk.LEFT, padx=(4, 12))

        ttk.Label(info_frame, text='Sector:', font=('Consolas', 9, 'bold')).pack(
            side=tk.LEFT)
        self._detail_sector = ttk.Label(info_frame, text='', font=('Consolas', 9))
        self._detail_sector.pack(side=tk.LEFT, padx=(4, 0))

        # Flags
        self._detail_flags = ttk.Label(
            details_inner, text='', font=('Consolas', 9),
            wraplength=280, foreground='#666666'
        )
        self._detail_flags.pack(anchor=tk.W, pady=(0, 8))

        # Separator
        ttk.Separator(details_inner, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=4)

        # Exits
        ttk.Label(details_inner, text='Exits:', font=('Consolas', 10, 'bold')).pack(
            anchor=tk.W, pady=(4, 2))

        self._exit_list_frame = ttk.Frame(details_inner)
        self._exit_list_frame.pack(fill=tk.X, pady=(0, 8))

        # Separator
        ttk.Separator(details_inner, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=4)

        # Description
        ttk.Label(details_inner, text='Description:', font=('Consolas', 10, 'bold')).pack(
            anchor=tk.W, pady=(4, 2))

        self._detail_desc = tk.Text(
            details_inner, height=10, wrap=tk.WORD,
            font=('Consolas', 9), state=tk.DISABLED,
            bg='#f0f0f0', relief=tk.FLAT, padx=4, pady=4
        )
        self._detail_desc.pack(fill=tk.BOTH, expand=True, pady=(0, 8))

        # Open in editor button
        self._open_btn = ttk.Button(
            details_inner, text='Open in Room Editor',
            command=self._open_selected_room, state=tk.DISABLED
        )
        self._open_btn.pack(fill=tk.X, pady=(4, 0))

        # Legend at bottom of details
        ttk.Separator(details_inner, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=8)
        legend_frame = ttk.LabelFrame(details_inner, text='Legend')
        legend_frame.pack(fill=tk.X)
        legends = [
            ('Solid line', 'Bidirectional exit'),
            ('Arrow >', 'One-way exit'),
            ('Dashed + ?', 'Non-Euclidean'),
            ('Thick line', 'Door'),
        ]
        for symbol, desc in legends:
            row = ttk.Frame(legend_frame)
            row.pack(fill=tk.X, padx=4, pady=1)
            ttk.Label(row, text=symbol, font=('Consolas', 8, 'bold'),
                     width=12, anchor=tk.W).pack(side=tk.LEFT)
            ttk.Label(row, text=desc, font=('Consolas', 8)).pack(
                side=tk.LEFT)

    def _load_data(self):
        """Load room and exit data from the database."""
        rooms = self.room_repo.list_all()
        self._rooms = {r['vnum']: r for r in rooms}
        self._room_name_cache = {r['vnum']: r['name'] for r in rooms}

        # Load exits for all rooms
        self._exits_by_room = {}
        for vnum in self._rooms:
            exits = self.exit_repo.get_for_room(vnum)
            if exits:
                self._exits_by_room[vnum] = exits

        room_vnums = set(self._rooms.keys())

        # Assign coordinates
        self._coords = assign_room_coordinates(rooms, self._exits_by_room)

        # Detect special exits
        self._one_way = detect_one_way_exits(self._exits_by_room, room_vnums)
        self._warped = detect_warped_exits(self._coords, self._exits_by_room, room_vnums)

        # Collect Z-levels
        z_values = sorted(set(c[2] for c in self._coords.values()))
        self._z_levels = z_values
        z_options = ['All'] + [f'Level {z}' for z in z_values]
        self._z_combo['values'] = z_options

        self._info_label.config(text=f'{len(self._rooms)} rooms')
        self.on_status(f'Loaded {len(self._rooms)} rooms')

        # Schedule initial render after canvas is visible
        self.after(100, self._fit_view)

    def _get_active_z(self) -> Optional[int]:
        """Get the currently selected Z-level filter, or None for all."""
        val = self._z_var.get()
        if val == 'All':
            return None
        try:
            return int(val.split()[-1])
        except (ValueError, IndexError):
            return None

    def _grid_to_world(self, gx: int, gy: int, gz: int) -> tuple:
        """Convert BFS grid coords to 3D world coords.

        Grid: X=East/West, Y=North/South, Z=Up/Down
        World: X=East/West, Y=Up/Down (visual vertical), Z=North/South (depth)

        This swap ensures MUD 'up' exits go visually upward on screen.
        """
        return (
            gx * self.CUBE_SPACING,
            gz * self.CUBE_SPACING,   # MUD up/down → visual Y (vertical)
            gy * self.CUBE_SPACING,   # MUD north/south → visual Z (depth)
        )

    def _get_visible_rooms(self) -> Dict[int, dict]:
        """Get rooms visible at the current Z-level filter."""
        z_filter = self._get_active_z()
        if z_filter is None:
            return self._rooms

        return {
            vnum: room for vnum, room in self._rooms.items()
            if vnum in self._coords and self._coords[vnum][2] == z_filter
        }

    def _render(self):
        """Render the 3D scene to the canvas."""
        self._render_pending = False
        self._canvas.delete('all')

        canvas_w = self._canvas.winfo_width()
        canvas_h = self._canvas.winfo_height()
        if canvas_w < 10 or canvas_h < 10:
            return

        visible_rooms = self._get_visible_rooms()
        z_filter = self._get_active_z()

        # Collect all drawable items with depth for painter's algorithm
        depth_items: List[Tuple[float, str]] = []  # (depth, type)

        # Pre-compute room screen positions for exit drawing
        room_screen_pos: Dict[int, Tuple[float, float]] = {}
        room_depths: Dict[int, float] = {}

        for vnum in visible_rooms:
            if vnum not in self._coords:
                continue
            wx, wy, wz = self._grid_to_world(*self._coords[vnum])

            rx, ry, rz = rotate_point(wx, wy, wz, self._rot_x, self._rot_y)
            sx, sy = project_point(
                rx, ry, rz, self._zoom,
                self._pan_x, self._pan_y, canvas_w, canvas_h
            )
            room_screen_pos[vnum] = (sx, sy)
            room_depths[vnum] = rz

        # Draw exits first (behind cubes)
        drawn_exits: Set[Tuple[int, int]] = set()

        for vnum in visible_rooms:
            for exit_data in self._exits_by_room.get(vnum, []):
                dest_vnum = exit_data['to_vnum']
                direction = exit_data['direction']

                if dest_vnum <= 0:
                    continue

                # Skip if already drawn in reverse direction
                pair = (min(vnum, dest_vnum), max(vnum, dest_vnum))
                is_one_way = (vnum, direction) in self._one_way
                is_warped = (vnum, direction) in self._warped
                is_door = bool(exit_data.get('exit_info', 0) & 1)

                if not is_one_way and not is_warped and pair in drawn_exits:
                    continue

                if vnum in room_screen_pos and dest_vnum in room_screen_pos:
                    sx1, sy1 = room_screen_pos[vnum]
                    sx2, sy2 = room_screen_pos[dest_vnum]

                    exit_color = '#886644' if is_door else '#667788'
                    if is_warped:
                        exit_color = '#ff6666'

                    draw_exit_arrow(
                        self._canvas, sx1, sy1, sx2, sy2,
                        one_way=is_one_way,
                        warped=is_warped,
                        is_door=is_door,
                        color=exit_color,
                        tag='exit'
                    )
                    drawn_exits.add(pair)

                elif vnum in room_screen_pos and dest_vnum not in visible_rooms:
                    # Exit to room outside view (different Z or different area)
                    sx1, sy1 = room_screen_pos[vnum]
                    # Draw a short stub arrow in the exit direction
                    from ..viz.layout import DIR_OFFSETS
                    dx, dy, dz = DIR_OFFSETS.get(direction, (0, 0, 0))
                    stub_len = 25
                    # Swap Y/Z to match world coords (MUD up→visual Y)
                    rx, ry, rz = rotate_point(
                        dx * 0.5, dz * 0.5, dy * 0.5,
                        self._rot_x, self._rot_y
                    )
                    stub_x = sx1 + rx * stub_len
                    stub_y = sy1 - ry * stub_len

                    dest_name = self._room_name_cache.get(dest_vnum, f'#{dest_vnum}')
                    draw_exit_arrow(
                        self._canvas, sx1, sy1, stub_x, stub_y,
                        one_way=True,
                        warped=is_warped,
                        color='#555577',
                        tag='exit'
                    )

        # Sort rooms by depth (draw furthest first - painter's algorithm)
        sorted_rooms = sorted(
            visible_rooms.keys(),
            key=lambda v: room_depths.get(v, 0),
            reverse=True  # Draw furthest rooms first
        )

        # Draw cubes
        for vnum in sorted_rooms:
            if vnum not in self._coords:
                continue

            room = self._rooms[vnum]
            wx, wy, wz = self._grid_to_world(*self._coords[vnum])

            sector = room.get('sector_type', 0)
            color = SECTOR_COLORS.get(sector, '#808080')

            is_selected = (vnum == self._selected_vnum)
            outline = '#ffff00' if is_selected else '#333333'
            outline_w = 2 if is_selected else 1

            tag = f'room_{vnum}'
            draw_cube(
                self._canvas,
                wx, wy, wz,
                self.CUBE_SIZE,
                color,
                self._rot_x, self._rot_y,
                self._zoom,
                self._pan_x, self._pan_y,
                canvas_w, canvas_h,
                tag=tag,
                outline=outline,
                outline_width=outline_w,
                label=str(vnum) if self._zoom > 60 else ''
            )

    def _schedule_render(self):
        """Schedule a debounced render."""
        if not self._render_pending:
            self._render_pending = True
            self.after(self.RENDER_DEBOUNCE_MS, self._render)

    def _reset_view(self):
        """Reset camera to default position."""
        self._rot_x = self.DEFAULT_ROT_X
        self._rot_y = self.DEFAULT_ROT_Y
        self._zoom = self.DEFAULT_ZOOM
        self._pan_x = 0.0
        self._pan_y = 0.0
        self._render()

    def _top_down_view(self):
        """Switch to a top-down view."""
        self._rot_x = math.radians(90)
        self._rot_y = 0.0
        self._render()

    def _fit_view(self):
        """Auto-fit zoom and pan to show all rooms."""
        if not self._coords:
            return

        canvas_w = self._canvas.winfo_width()
        canvas_h = self._canvas.winfo_height()
        if canvas_w < 10 or canvas_h < 10:
            self.after(100, self._fit_view)
            return

        visible = self._get_visible_rooms()
        if not visible:
            return

        # Find bounding box in screen space at current rotation
        min_sx = float('inf')
        max_sx = float('-inf')
        min_sy = float('inf')
        max_sy = float('-inf')

        test_zoom = 50.0
        for vnum in visible:
            if vnum not in self._coords:
                continue
            wx, wy, wz = self._grid_to_world(*self._coords[vnum])

            rx, ry, rz = rotate_point(wx, wy, wz, self._rot_x, self._rot_y)
            sx, sy = project_point(rx, ry, rz, test_zoom, 0, 0, 0, 0)

            min_sx = min(min_sx, sx)
            max_sx = max(max_sx, sx)
            min_sy = min(min_sy, sy)
            max_sy = max(max_sy, sy)

        if max_sx <= min_sx or max_sy <= min_sy:
            self._zoom = self.DEFAULT_ZOOM
            self._pan_x = 0.0
            self._pan_y = 0.0
            self._render()
            return

        # Calculate zoom to fit with padding
        extent_x = max_sx - min_sx
        extent_y = max_sy - min_sy
        padding = 80

        zoom_x = (canvas_w - padding * 2) / (extent_x / test_zoom) if extent_x > 0 else test_zoom
        zoom_y = (canvas_h - padding * 2) / (extent_y / test_zoom) if extent_y > 0 else test_zoom
        self._zoom = min(zoom_x, zoom_y, 120.0)
        self._zoom = max(self._zoom, 10.0)

        # Center the view
        center_sx = (min_sx + max_sx) / 2.0
        center_sy = (min_sy + max_sy) / 2.0
        # Recalculate with new zoom
        self._pan_x = -center_sx * (self._zoom / test_zoom)
        self._pan_y = -center_sy * (self._zoom / test_zoom)

        self._render()

    # ── Mouse Interaction ──────────────────────────────────────────────

    def _on_press(self, event):
        """Record drag start position."""
        self._drag_start = (event.x, event.y)
        self._drag_button = event.num
        self._drag_moved = False

    def _on_release(self, event):
        """Handle button release - detect click if didn't drag."""
        was_drag = getattr(self, '_drag_moved', False)
        self._drag_start = None
        self._drag_button = None

        # Left button release without drag = click to select room
        if event.num == 1 and not was_drag:
            self._handle_click(event)

    def _handle_click(self, event):
        """Handle left click - select room."""
        # Find closest canvas item at click position
        items = self._canvas.find_closest(event.x, event.y)
        if not items:
            self._select_room(None)
            return

        # Check tags for room identification
        tags = self._canvas.gettags(items[0])
        for tag in tags:
            if tag.startswith('room_'):
                try:
                    vnum = int(tag[5:])
                    self._select_room(vnum)
                    return
                except ValueError:
                    pass

        # Check if click is within tolerance of any room item
        x, y = event.x, event.y
        overlapping = self._canvas.find_overlapping(x - 5, y - 5, x + 5, y + 5)
        for item_id in overlapping:
            tags = self._canvas.gettags(item_id)
            for tag in tags:
                if tag.startswith('room_'):
                    try:
                        vnum = int(tag[5:])
                        self._select_room(vnum)
                        return
                    except ValueError:
                        pass

        self._select_room(None)

    def _on_double_click(self, event):
        """Handle double-click - open room in editor."""
        if self._selected_vnum is not None and self.on_open_room:
            self.on_open_room(self._selected_vnum)

    def _on_drag(self, event):
        """Handle left-button drag - rotate."""
        if self._drag_start is None:
            return

        dx = event.x - self._drag_start[0]
        dy = event.y - self._drag_start[1]

        if abs(dx) > 3 or abs(dy) > 3:
            self._drag_moved = True

        self._rot_y += dx * 0.01
        self._rot_x += dy * 0.01

        # Clamp pitch to avoid flipping
        self._rot_x = max(-math.pi / 2 + 0.01, min(math.pi / 2 - 0.01, self._rot_x))

        self._drag_start = (event.x, event.y)
        self._schedule_render()

    def _on_pan_drag(self, event):
        """Handle right-button drag - pan."""
        if self._drag_start is None:
            return

        dx = event.x - self._drag_start[0]
        dy = event.y - self._drag_start[1]

        self._pan_x += dx
        self._pan_y += dy

        self._drag_start = (event.x, event.y)
        self._schedule_render()

    def _on_scroll(self, event):
        """Handle mouse wheel - zoom."""
        factor = 1.1 if event.delta > 0 else 0.9
        self._zoom *= factor
        self._zoom = max(5.0, min(200.0, self._zoom))
        self._schedule_render()

    def _on_resize(self, event):
        """Handle canvas resize."""
        self._schedule_render()

    # ── Room Selection & Details ───────────────────────────────────────

    def _select_room(self, vnum: Optional[int]):
        """Select a room and update the details panel."""
        self._selected_vnum = vnum
        self._update_details()
        self._render()

    def _update_details(self):
        """Update the details panel with the selected room's info."""
        vnum = self._selected_vnum

        if vnum is None or vnum not in self._rooms:
            self._detail_name.config(text='Click a room to view details')
            self._detail_vnum.config(text='')
            self._detail_sector.config(text='')
            self._detail_flags.config(text='')
            self._clear_exit_list()
            self._detail_desc.config(state=tk.NORMAL)
            self._detail_desc.delete('1.0', tk.END)
            self._detail_desc.config(state=tk.DISABLED)
            self._open_btn.config(state=tk.DISABLED)
            return

        room = self._rooms[vnum]

        # Get full room data with description
        full_room = self.room_repo.get_by_id(vnum)
        if full_room:
            room = full_room

        self._detail_name.config(text=room.get('name', 'Unknown'))
        self._detail_vnum.config(text=str(vnum))

        sector = room.get('sector_type', 0)
        sector_name = SECTOR_NAMES.get(sector, f'unknown ({sector})')
        self._detail_sector.config(text=sector_name)

        flags = room.get('room_flags', 0)
        flag_names = decode_flags(flags, ROOM_FLAGS)
        self._detail_flags.config(
            text=', '.join(flag_names) if flag_names else 'none'
        )

        # Exits
        self._clear_exit_list()
        exits = self._exits_by_room.get(vnum, [])
        if exits:
            for exit_data in exits:
                direction = exit_data['direction']
                dest_vnum = exit_data['to_vnum']
                dir_name = DIR_NAMES[direction] if direction < len(DIR_NAMES) else '?'
                dest_name = self._room_name_cache.get(dest_vnum, f'#{dest_vnum}')

                # Build indicator string
                indicators = []
                if (vnum, direction) in self._one_way:
                    indicators.append('one-way')
                if (vnum, direction) in self._warped:
                    indicators.append('non-euclidean')
                if exit_data.get('exit_info', 0) & 1:
                    indicators.append('door')

                text = f'  {dir_name.capitalize():6s} -> {dest_vnum} ({dest_name})'
                if indicators:
                    text += f'  [{", ".join(indicators)}]'

                label = ttk.Label(
                    self._exit_list_frame, text=text,
                    font=('Consolas', 9)
                )
                label.pack(anchor=tk.W)
        else:
            ttk.Label(
                self._exit_list_frame, text='  No exits',
                font=('Consolas', 9), foreground='#999999'
            ).pack(anchor=tk.W)

        # Description
        desc = room.get('description', '')
        # Strip MUD color codes for display
        import re
        clean_desc = re.sub(r'#[0-9a-zA-Z]', '', desc)
        clean_desc = re.sub(r'\^[A-Za-z]', '', clean_desc)

        self._detail_desc.config(state=tk.NORMAL)
        self._detail_desc.delete('1.0', tk.END)
        self._detail_desc.insert('1.0', clean_desc.strip())
        self._detail_desc.config(state=tk.DISABLED)

        self._open_btn.config(state=tk.NORMAL)

        self.on_status(f'Selected room {vnum}: {room.get("name", "")}')

    def _clear_exit_list(self):
        """Clear the exit list frame."""
        for widget in self._exit_list_frame.winfo_children():
            widget.destroy()

    def _open_selected_room(self):
        """Open the selected room in the room editor."""
        if self._selected_vnum is not None and self.on_open_room:
            self.on_open_room(self._selected_vnum)

    def check_unsaved(self) -> bool:
        """No unsaved state in the visualizer."""
        return True
