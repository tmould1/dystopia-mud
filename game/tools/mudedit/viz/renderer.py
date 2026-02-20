"""
3D rendering utilities for the area map visualizer.

Provides rotation, projection, cube geometry, and drawing functions
for rendering rooms and exits on a Tkinter Canvas.
"""

import math
from typing import List, Optional, Tuple


def rotate_point(
    x: float, y: float, z: float,
    rot_x: float, rot_y: float
) -> Tuple[float, float, float]:
    """
    Apply turntable rotation (Y-axis then X-axis).

    Args:
        x, y, z: World coordinates
        rot_x: Rotation around X axis in radians (pitch)
        rot_y: Rotation around Y axis in radians (yaw)

    Returns:
        Rotated (x, y, z)
    """
    # Rotate around Y axis (yaw)
    cos_y = math.cos(rot_y)
    sin_y = math.sin(rot_y)
    x1 = x * cos_y + z * sin_y
    y1 = y
    z1 = -x * sin_y + z * cos_y

    # Rotate around X axis (pitch)
    cos_x = math.cos(rot_x)
    sin_x = math.sin(rot_x)
    x2 = x1
    y2 = y1 * cos_x - z1 * sin_x
    z2 = y1 * sin_x + z1 * cos_x

    return x2, y2, z2


def project_point(
    x: float, y: float, z: float,
    zoom: float,
    pan_x: float, pan_y: float,
    canvas_w: float, canvas_h: float
) -> Tuple[float, float]:
    """
    Project a rotated 3D point to 2D screen coordinates.

    Uses a simple perspective projection with configurable zoom.

    Args:
        x, y, z: Rotated 3D coordinates
        zoom: Zoom factor (pixels per world unit)
        pan_x, pan_y: Pan offset in screen pixels
        canvas_w, canvas_h: Canvas dimensions

    Returns:
        (screen_x, screen_y)
    """
    # Orthographic projection - no perspective distortion.
    # This keeps cubes stable during rotation instead of warping.
    screen_x = x * zoom + canvas_w / 2 + pan_x
    screen_y = -y * zoom + canvas_h / 2 + pan_y  # Flip Y for screen coords

    return screen_x, screen_y


def get_depth(x: float, y: float, z: float) -> float:
    """Get depth value for sorting (larger = further from camera)."""
    return z


def get_cube_faces(
    cx: float, cy: float, cz: float, size: float
) -> List[Tuple[List[Tuple[float, float, float]], Tuple[float, float, float]]]:
    """
    Return the 6 faces of a cube centered at (cx, cy, cz).

    Each face is a tuple of (corners, normal_vector).
    Corners are in order for polygon rendering.

    Returns:
        List of (corners, normal) where corners is 4 (x,y,z) tuples
        and normal is a (nx, ny, nz) direction vector.
    """
    h = size / 2.0

    # Define faces with outward-pointing normals
    faces = [
        # Front face (facing camera, +Z)
        ([(cx - h, cy - h, cz + h), (cx + h, cy - h, cz + h),
          (cx + h, cy + h, cz + h), (cx - h, cy + h, cz + h)],
         (0, 0, 1)),
        # Back face (-Z)
        ([(cx + h, cy - h, cz - h), (cx - h, cy - h, cz - h),
          (cx - h, cy + h, cz - h), (cx + h, cy + h, cz - h)],
         (0, 0, -1)),
        # Top face (+Y)
        ([(cx - h, cy + h, cz + h), (cx + h, cy + h, cz + h),
          (cx + h, cy + h, cz - h), (cx - h, cy + h, cz - h)],
         (0, 1, 0)),
        # Bottom face (-Y)
        ([(cx - h, cy - h, cz - h), (cx + h, cy - h, cz - h),
          (cx + h, cy - h, cz + h), (cx - h, cy - h, cz + h)],
         (0, -1, 0)),
        # Right face (+X)
        ([(cx + h, cy - h, cz + h), (cx + h, cy - h, cz - h),
          (cx + h, cy + h, cz - h), (cx + h, cy + h, cz + h)],
         (1, 0, 0)),
        # Left face (-X)
        ([(cx - h, cy - h, cz - h), (cx - h, cy - h, cz + h),
          (cx - h, cy + h, cz + h), (cx - h, cy + h, cz - h)],
         (-1, 0, 0)),
    ]
    return faces


def is_front_facing(
    normal: Tuple[float, float, float],
    rot_x: float, rot_y: float
) -> bool:
    """
    Check if a face with given normal is front-facing after rotation.

    A face is front-facing if its rotated normal points toward the camera (positive Z).
    """
    nx, ny, nz = rotate_point(normal[0], normal[1], normal[2], rot_x, rot_y)
    return nz > 0


def shade_color(hex_color: str, factor: float) -> str:
    """
    Darken or lighten a hex color.

    Args:
        hex_color: Color in '#RRGGBB' format
        factor: 0.0 = black, 1.0 = original, >1.0 = lighter
    """
    r = int(hex_color[1:3], 16)
    g = int(hex_color[3:5], 16)
    b = int(hex_color[5:7], 16)

    r = min(255, max(0, int(r * factor)))
    g = min(255, max(0, int(g * factor)))
    b = min(255, max(0, int(b * factor)))

    return f'#{r:02x}{g:02x}{b:02x}'


def face_shade_factor(
    normal: Tuple[float, float, float],
    rot_x: float, rot_y: float
) -> float:
    """
    Calculate a fixed shading factor based on face direction in world space.

    Uses the unrotated normal so shading stays constant as the camera orbits.
    Top faces are brightest, side faces medium, bottom faces darkest.
    This prevents the jarring color-shifting that makes cubes appear to spin.
    """
    nx, ny, nz = normal
    if ny > 0.5:
        return 1.0     # Top face - brightest
    elif ny < -0.5:
        return 0.6     # Bottom face - darkest
    elif nx > 0.5 or nz > 0.5:
        return 0.85    # Right / front sides
    else:
        return 0.75    # Left / back sides


def draw_cube(
    canvas,
    cx: float, cy: float, cz: float,
    size: float,
    color: str,
    rot_x: float, rot_y: float,
    zoom: float,
    pan_x: float, pan_y: float,
    canvas_w: float, canvas_h: float,
    tag: str = '',
    outline: str = '#333333',
    outline_width: int = 1,
    label: str = ''
) -> List[Tuple[float, int]]:
    """
    Draw a cube on the canvas and return depth-sorted face info.

    Returns list of (depth, canvas_item_id) for painter's algorithm sorting.
    """
    faces = get_cube_faces(cx, cy, cz, size)
    items = []

    for corners, normal in faces:
        if not is_front_facing(normal, rot_x, rot_y):
            continue

        # Transform corners to screen space
        screen_points = []
        total_depth = 0.0
        for px, py, pz in corners:
            rx, ry, rz = rotate_point(px, py, pz, rot_x, rot_y)
            sx, sy = project_point(rx, ry, rz, zoom, pan_x, pan_y, canvas_w, canvas_h)
            screen_points.extend([sx, sy])
            total_depth += rz

        avg_depth = total_depth / len(corners)

        # Shade the face
        shade = face_shade_factor(normal, rot_x, rot_y)
        face_color = shade_color(color, shade)

        tags = (tag,) if tag else ()
        item_id = canvas.create_polygon(
            screen_points,
            fill=face_color,
            outline=outline,
            width=outline_width,
            tags=tags
        )
        items.append((avg_depth, item_id))

    # Draw label above cube if provided
    if label:
        rx, ry, rz = rotate_point(cx, cy + size * 0.8, cz, rot_x, rot_y)
        sx, sy = project_point(rx, ry, rz, zoom, pan_x, pan_y, canvas_w, canvas_h)
        tags = (tag,) if tag else ()
        label_id = canvas.create_text(
            sx, sy, text=label, fill='#ffffff',
            font=('Consolas', 8), tags=tags, anchor='s'
        )
        items.append((rz, label_id))

    return items


def draw_exit_arrow(
    canvas,
    x1: float, y1: float,
    x2: float, y2: float,
    one_way: bool = False,
    warped: bool = False,
    is_door: bool = False,
    color: str = '#aaaaaa',
    tag: str = ''
) -> Optional[int]:
    """
    Draw an exit arrow/line between two screen points.

    Args:
        canvas: Tkinter Canvas
        x1, y1: Source screen coordinates
        x2, y2: Destination screen coordinates
        one_way: If True, draw arrowhead at destination
        warped: If True, draw dashed and add "?" label
        is_door: If True, draw thicker line
        color: Line color
        tag: Canvas tag for the line
    """
    dx = x2 - x1
    dy = y2 - y1
    length = math.sqrt(dx * dx + dy * dy)
    if length < 1:
        return None

    # Shorten line to not overlap cubes
    shorten = min(15, length * 0.2)
    nx = dx / length
    ny = dy / length
    sx1 = x1 + nx * shorten
    sy1 = y1 + ny * shorten
    sx2 = x2 - nx * shorten
    sy2 = y2 - ny * shorten

    tags = (tag,) if tag else ()
    width = 2.5 if is_door else 1.5
    dash = (6, 4) if warped else ()

    kwargs = dict(fill=color, width=width, tags=tags)
    if dash:
        kwargs['dash'] = dash
    if one_way:
        kwargs['arrow'] = 'last'
        kwargs['arrowshape'] = (10, 12, 5)

    item_id = canvas.create_line(sx1, sy1, sx2, sy2, **kwargs)

    # Add "?" label for warped exits
    if warped:
        mid_x = (sx1 + sx2) / 2
        mid_y = (sy1 + sy2) / 2
        canvas.create_text(
            mid_x, mid_y - 8, text='?',
            fill='#ff6666', font=('Consolas', 10, 'bold'),
            tags=tags
        )

    return item_id
