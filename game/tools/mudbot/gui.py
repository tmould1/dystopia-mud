"""
Bot Commander GUI - tkinter interface for managing multiple MUD bots.

Supports multiple servers, each with its own set of bots with individual passwords.

Run from game/tools directory with:
    python -m mudbot.gui

Or run directly (will adjust path):
    cd game/tools/mudbot
    python gui.py
"""

import asyncio
import json
import logging
import queue
import sys
import threading
import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Optional

# Settings file location (next to gui.py)
SETTINGS_FILE = Path(__file__).parent / "gui_settings.json"

# Handle imports for both module and direct execution
try:
    # When run as module (python -m mudbot.gui)
    from .config import BotConfig
    from .bot.avatar_bot import AvatarProgressionBot
    from .bot import ClassRegistry
except ImportError:
    # When run directly (python gui.py) - need to fix up imports
    # Add parent directory to allow 'mudbot' package imports
    _tools_dir = Path(__file__).parent.parent
    if str(_tools_dir) not in sys.path:
        sys.path.insert(0, str(_tools_dir))

    from mudbot.config import BotConfig
    from mudbot.bot.avatar_bot import AvatarProgressionBot
    from mudbot.bot import ClassRegistry

logger = logging.getLogger(__name__)


# =============================================================================
# Data Models
# =============================================================================

@dataclass
class BotEntry:
    """A bot configuration within a server."""
    name: str
    password: str
    bot_type: str = "avatar"  # "avatar" or class name like "demon"


@dataclass
class ServerConfig:
    """A server with its associated bots."""
    host: str
    port: int
    bots: List[BotEntry] = field(default_factory=list)

    @property
    def key(self) -> str:
        """Unique key for this server."""
        return f"{self.host}:{self.port}"

    def to_dict(self) -> dict:
        """Convert to dictionary for JSON serialization."""
        return {
            'host': self.host,
            'port': self.port,
            'bots': [{'name': b.name, 'password': b.password, 'bot_type': b.bot_type} for b in self.bots]
        }

    @classmethod
    def from_dict(cls, data: dict) -> 'ServerConfig':
        """Create from dictionary."""
        bots = [
            BotEntry(
                name=b['name'],
                password=b['password'],
                bot_type=b.get('bot_type', 'avatar')  # Default to avatar for backwards compat
            )
            for b in data.get('bots', [])
        ]
        return cls(host=data['host'], port=data['port'], bots=bots)


@dataclass
class ManagedBot:
    """Container for a managed bot instance."""
    name: str
    server_key: str
    bot_type: str = "avatar"
    bot: Optional[Any] = None  # Can be AvatarProgressionBot or any ClassProgressionBot
    task: Optional[asyncio.Task] = None
    error: Optional[str] = None

    @property
    def full_key(self) -> str:
        """Full unique key including server."""
        return f"{self.server_key}:{self.name}"


# =============================================================================
# Logging Handler
# =============================================================================

class QueueLogHandler(logging.Handler):
    """Logging handler that puts messages into a queue for GUI consumption."""

    def __init__(self, log_queue: queue.Queue):
        super().__init__()
        self.log_queue = log_queue

    def emit(self, record):
        try:
            msg = self.format(record)
            self.log_queue.put(msg)
        except Exception:
            self.handleError(record)


# =============================================================================
# Async Bot Runner
# =============================================================================

class AsyncBotRunner:
    """Manages asyncio event loop and bot tasks in a background thread."""

    def __init__(self, status_queue: queue.Queue, log_queue: queue.Queue):
        self.status_queue = status_queue
        self.log_queue = log_queue
        self.loop: Optional[asyncio.AbstractEventLoop] = None
        self.bots: Dict[str, ManagedBot] = {}  # key: "server_key:bot_name"
        self._thread: Optional[threading.Thread] = None
        self._stopping = False

    def start(self):
        """Start the background thread with asyncio event loop."""
        self._thread = threading.Thread(target=self._run_loop, daemon=True)
        self._thread.start()

    def _run_loop(self):
        """Thread target - runs the asyncio event loop."""
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        self.loop.run_forever()

    def stop(self):
        """Stop all bots and the event loop."""
        self._stopping = True
        if self.loop:
            self.loop.call_soon_threadsafe(self._stop_all_sync)

    def _stop_all_sync(self):
        """Stop all bots (called from event loop thread)."""
        for managed in self.bots.values():
            if managed.task and not managed.task.done():
                managed.task.cancel()
        # Stop the loop after tasks are cancelled
        self.loop.call_later(1.0, self.loop.stop)

    def spawn_bot(self, server: ServerConfig, bot_entry: BotEntry):
        """Create and start a bot (thread-safe, schedules on event loop)."""
        if self.loop:
            asyncio.run_coroutine_threadsafe(
                self._spawn_bot_async(server, bot_entry),
                self.loop
            )

    async def _spawn_bot_async(self, server: ServerConfig, bot_entry: BotEntry):
        """Async implementation of bot spawning."""
        full_key = f"{server.key}:{bot_entry.name}"

        # Create config
        config = BotConfig(
            name=bot_entry.name,
            password=bot_entry.password,
            host=server.host,
            port=server.port
        )

        # Create bot based on type
        bot_type = bot_entry.bot_type
        if bot_type == "avatar":
            # Use legacy avatar bot
            bot = AvatarProgressionBot(config)
        elif ClassRegistry.is_registered(bot_type):
            # Use class progression bot from registry
            bot = ClassRegistry.create(bot_type, config)
        else:
            logger.error(f"Unknown bot type: {bot_type}")
            self.status_queue.put(('error', full_key, f"Unknown bot type: {bot_type}"))
            return

        # Set up status callback
        def on_status_change(status: dict):
            self.status_queue.put(('status', full_key, status))

        bot.on_status_change = on_status_change

        # Create managed container
        managed = ManagedBot(name=bot_entry.name, server_key=server.key, bot_type=bot_type, bot=bot)
        self.bots[full_key] = managed

        # Post initial status
        self.status_queue.put(('status', full_key, bot.get_status()))

        # Start bot task
        managed.task = asyncio.create_task(self._run_bot(managed))

    async def _run_bot(self, managed: ManagedBot):
        """Run a single bot with error handling."""
        try:
            await managed.bot.run()
        except asyncio.CancelledError:
            logger.info(f"[{managed.full_key}] Bot cancelled")
        except Exception as e:
            logger.error(f"[{managed.full_key}] Bot error: {e}")
            managed.error = str(e)
            self.status_queue.put(('error', managed.full_key, str(e)))

    def pause_bot(self, full_key: str):
        """Pause a bot."""
        if self.loop and full_key in self.bots:
            self.loop.call_soon_threadsafe(self._pause_bot_sync, full_key)

    def _pause_bot_sync(self, full_key: str):
        managed = self.bots.get(full_key)
        if managed and managed.bot:
            managed.bot.pause()

    def resume_bot(self, full_key: str):
        """Resume a paused bot."""
        if self.loop and full_key in self.bots:
            self.loop.call_soon_threadsafe(self._resume_bot_sync, full_key)

    def _resume_bot_sync(self, full_key: str):
        managed = self.bots.get(full_key)
        if managed and managed.bot:
            managed.bot.resume()

    def stop_bot(self, full_key: str):
        """Stop a single bot."""
        if self.loop and full_key in self.bots:
            self.loop.call_soon_threadsafe(self._stop_bot_sync, full_key)

    def _stop_bot_sync(self, full_key: str):
        managed = self.bots.get(full_key)
        if managed:
            if managed.bot:
                managed.bot.stop()
            if managed.task and not managed.task.done():
                managed.task.cancel()

    def restart_bot(self, server: ServerConfig, bot_entry: BotEntry):
        """Stop and restart a bot."""
        full_key = f"{server.key}:{bot_entry.name}"
        if full_key in self.bots:
            self.stop_bot(full_key)
            # Wait a bit then spawn new
            if self.loop:
                self.loop.call_later(1.0, lambda: asyncio.run_coroutine_threadsafe(
                    self._spawn_bot_async(server, bot_entry),
                    self.loop
                ))
        else:
            self.spawn_bot(server, bot_entry)


# =============================================================================
# Main GUI
# =============================================================================

class BotManagerGUI(tk.Tk):
    """Main GUI application for managing MUD bots across multiple servers."""

    def __init__(self):
        super().__init__()

        self.title("Bot Commander")
        self.geometry("900x750")
        self.minsize(700, 550)

        # Queues for thread-safe communication
        self.status_queue = queue.Queue()
        self.log_queue = queue.Queue()

        # Set up logging to capture bot logs
        self._setup_logging()

        # Bot runner (manages asyncio loop in background)
        self.runner = AsyncBotRunner(self.status_queue, self.log_queue)
        self.runner.start()

        # Server configurations
        self.servers: List[ServerConfig] = []
        self.current_server: Optional[ServerConfig] = None

        # Status tracking by full key
        self.bot_statuses: Dict[str, dict] = {}

        # Create UI
        self._create_main_layout()

        # Load saved settings
        self._load_settings()

        # Start queue polling
        self._poll_queues()

        # Handle window close
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    def _setup_logging(self):
        """Set up logging to capture bot logs into the GUI."""
        handler = QueueLogHandler(self.log_queue)
        handler.setFormatter(logging.Formatter('%(asctime)s %(message)s', datefmt='%H:%M:%S'))

        # Only add to root logger - child loggers propagate up automatically
        root_logger = logging.getLogger()
        root_logger.addHandler(handler)
        root_logger.setLevel(logging.INFO)

    def _create_main_layout(self):
        """Create the main application layout."""
        # Main horizontal paned window
        self.main_paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        self.main_paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Left pane: Server list
        self._create_server_panel()

        # Right pane: Bot configuration (initially empty)
        self._create_bot_panel()

        # Log output at bottom
        self._create_log_frame()

    def _create_server_panel(self):
        """Create the server list panel (left side)."""
        left_frame = ttk.LabelFrame(self.main_paned, text="Servers", padding=10)
        self.main_paned.add(left_frame, weight=1)

        # Server listbox
        list_frame = ttk.Frame(left_frame)
        list_frame.pack(fill=tk.BOTH, expand=True)

        self.server_listbox = tk.Listbox(list_frame, height=10, selectmode=tk.SINGLE)
        self.server_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.server_listbox.bind('<<ListboxSelect>>', self._on_server_select)

        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.server_listbox.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.server_listbox.config(yscrollcommand=scrollbar.set)

        # Add server form
        form_frame = ttk.Frame(left_frame)
        form_frame.pack(fill=tk.X, pady=(10, 0))

        ttk.Label(form_frame, text="Host:").grid(row=0, column=0, sticky=tk.W)
        self.new_host_var = tk.StringVar(value="localhost")
        ttk.Entry(form_frame, textvariable=self.new_host_var, width=15).grid(row=0, column=1, padx=2)

        ttk.Label(form_frame, text="Port:").grid(row=1, column=0, sticky=tk.W)
        self.new_port_var = tk.StringVar(value="8888")
        ttk.Entry(form_frame, textvariable=self.new_port_var, width=8).grid(row=1, column=1, padx=2, sticky=tk.W)

        btn_frame = ttk.Frame(left_frame)
        btn_frame.pack(fill=tk.X, pady=(5, 0))
        ttk.Button(btn_frame, text="Add Server", command=self._add_server).pack(fill=tk.X, pady=2)
        ttk.Button(btn_frame, text="Remove Server", command=self._remove_server).pack(fill=tk.X, pady=2)

    def _create_bot_panel(self):
        """Create the bot configuration panel (right side)."""
        self.right_frame = ttk.LabelFrame(self.main_paned, text="Server: (none selected)", padding=10)
        self.main_paned.add(self.right_frame, weight=3)

        # Placeholder label when no server selected
        self.no_server_label = ttk.Label(
            self.right_frame,
            text="Select a server from the left panel\nor add a new server to get started.",
            justify=tk.CENTER
        )
        self.no_server_label.pack(expand=True)

        # Bot list frame (hidden initially)
        self.bot_list_frame = ttk.LabelFrame(self.right_frame, text="Bots", padding=5)

        # Bot treeview (name, type, password masked)
        columns = ('name', 'type', 'password')
        self.bot_tree = ttk.Treeview(self.bot_list_frame, columns=columns, show='headings', height=5)
        self.bot_tree.heading('name', text='Name')
        self.bot_tree.heading('type', text='Type')
        self.bot_tree.heading('password', text='Password')
        self.bot_tree.column('name', width=100)
        self.bot_tree.column('type', width=70)
        self.bot_tree.column('password', width=80)

        bot_scroll = ttk.Scrollbar(self.bot_list_frame, orient=tk.VERTICAL, command=self.bot_tree.yview)
        self.bot_tree.configure(yscrollcommand=bot_scroll.set)
        self.bot_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        bot_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        # Bot controls
        bot_btn_frame = ttk.Frame(self.bot_list_frame)
        bot_btn_frame.pack(side=tk.RIGHT, fill=tk.Y, padx=(10, 0))
        ttk.Button(bot_btn_frame, text="Remove", command=self._remove_bot).pack(fill=tk.X, pady=2)

        # Add bot form
        self.add_bot_frame = ttk.Frame(self.right_frame)
        ttk.Label(self.add_bot_frame, text="Name:").grid(row=0, column=0, sticky=tk.W)
        self.new_bot_name_var = tk.StringVar()
        ttk.Entry(self.add_bot_frame, textvariable=self.new_bot_name_var, width=12).grid(row=0, column=1, padx=2)

        ttk.Label(self.add_bot_frame, text="Password:").grid(row=0, column=2, sticky=tk.W, padx=(5, 0))
        self.new_bot_pass_var = tk.StringVar()
        ttk.Entry(self.add_bot_frame, textvariable=self.new_bot_pass_var, width=12, show="*").grid(row=0, column=3, padx=2)

        ttk.Label(self.add_bot_frame, text="Type:").grid(row=0, column=4, sticky=tk.W, padx=(5, 0))
        self.new_bot_type_var = tk.StringVar(value="avatar")
        # Build list of available types: avatar + registered classes
        bot_types = ["avatar"] + ClassRegistry.available_classes()
        self.bot_type_combo = ttk.Combobox(
            self.add_bot_frame,
            textvariable=self.new_bot_type_var,
            values=bot_types,
            width=8,
            state="readonly"
        )
        self.bot_type_combo.grid(row=0, column=5, padx=2)

        ttk.Button(self.add_bot_frame, text="Add Bot", command=self._add_bot).grid(row=0, column=6, padx=(10, 0))

        # Start/Stop all buttons
        self.control_frame = ttk.Frame(self.right_frame)
        ttk.Button(self.control_frame, text="Start All", command=self._start_all).pack(side=tk.LEFT, padx=2)
        ttk.Button(self.control_frame, text="Stop All", command=self._stop_all).pack(side=tk.LEFT, padx=2)

        # Status frame
        self.status_frame = ttk.LabelFrame(self.right_frame, text="Bot Status", padding=5)

        columns = ('name', 'type', 'state', 'progress', 'status')
        self.status_tree = ttk.Treeview(self.status_frame, columns=columns, show='headings', height=6)

        self.status_tree.heading('name', text='Name')
        self.status_tree.heading('type', text='Type')
        self.status_tree.heading('state', text='State')
        self.status_tree.heading('progress', text='Progress')
        self.status_tree.heading('status', text='Status')

        self.status_tree.column('name', width=80)
        self.status_tree.column('type', width=60)
        self.status_tree.column('state', width=80)
        self.status_tree.column('progress', width=140)
        self.status_tree.column('status', width=100)

        status_scroll = ttk.Scrollbar(self.status_frame, orient=tk.VERTICAL, command=self.status_tree.yview)
        self.status_tree.configure(yscrollcommand=status_scroll.set)
        self.status_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        status_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        # Status control buttons
        status_ctrl = ttk.Frame(self.status_frame)
        status_ctrl.pack(side=tk.RIGHT, fill=tk.Y, padx=(10, 0))
        ttk.Button(status_ctrl, text="▶ Start", command=self._start_selected).pack(fill=tk.X, pady=2)
        ttk.Button(status_ctrl, text="⏸ Pause", command=self._pause_selected).pack(fill=tk.X, pady=2)
        ttk.Button(status_ctrl, text="⏹ Stop", command=self._stop_selected).pack(fill=tk.X, pady=2)

        # Configure tag colors
        self.status_tree.tag_configure('complete', background='#90EE90')
        self.status_tree.tag_configure('error', background='#FFB6C1')
        self.status_tree.tag_configure('paused', background='#D3D3D3')
        self.status_tree.tag_configure('running', background='#FFFACD')

    def _create_log_frame(self):
        """Create log output panel."""
        frame = ttk.LabelFrame(self, text="Log Output", padding=10)
        frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        header = ttk.Frame(frame)
        header.pack(fill=tk.X)
        ttk.Button(header, text="Clear", command=self._clear_log).pack(side=tk.RIGHT)

        self.log_text = scrolledtext.ScrolledText(frame, height=8, state=tk.DISABLED)
        self.log_text.pack(fill=tk.BOTH, expand=True)

    # =========================================================================
    # Server Management
    # =========================================================================

    def _add_server(self):
        """Add a new server configuration."""
        host = self.new_host_var.get().strip()
        port_str = self.new_port_var.get().strip()

        if not host:
            messagebox.showwarning("Invalid Host", "Please enter a hostname.")
            return

        try:
            port = int(port_str)
            if port < 1 or port > 65535:
                raise ValueError()
        except ValueError:
            messagebox.showwarning("Invalid Port", "Please enter a valid port number (1-65535).")
            return

        # Check for duplicate
        key = f"{host}:{port}"
        for server in self.servers:
            if server.key == key:
                messagebox.showwarning("Duplicate Server", f"Server '{key}' already exists.")
                return

        # Create and add server
        server = ServerConfig(host=host, port=port)
        self.servers.append(server)
        self.server_listbox.insert(tk.END, server.key)

        # Select the new server
        self.server_listbox.selection_clear(0, tk.END)
        self.server_listbox.selection_set(tk.END)
        self._on_server_select(None)

        self._save_settings()

    def _remove_server(self):
        """Remove the selected server."""
        selection = self.server_listbox.curselection()
        if not selection:
            messagebox.showinfo("No Selection", "Please select a server to remove.")
            return

        idx = selection[0]
        server = self.servers[idx]

        # Confirm if server has bots
        if server.bots:
            if not messagebox.askyesno("Confirm Remove",
                f"Server '{server.key}' has {len(server.bots)} bot(s). Remove anyway?"):
                return

        # Stop all bots on this server
        for bot_entry in server.bots:
            full_key = f"{server.key}:{bot_entry.name}"
            self.runner.stop_bot(full_key)

        # Remove from list
        self.servers.pop(idx)
        self.server_listbox.delete(idx)

        # Clear selection
        if self.current_server == server:
            self.current_server = None
            self._update_bot_panel()

        self._save_settings()

    def _on_server_select(self, _):
        """Handle server selection change."""
        selection = self.server_listbox.curselection()
        if not selection:
            self.current_server = None
        else:
            self.current_server = self.servers[selection[0]]
        self._update_bot_panel()

    def _update_bot_panel(self):
        """Update the right panel based on selected server."""
        if self.current_server is None:
            # Show placeholder
            self.no_server_label.pack(expand=True)
            self.bot_list_frame.pack_forget()
            self.add_bot_frame.pack_forget()
            self.control_frame.pack_forget()
            self.status_frame.pack_forget()
            self.right_frame.configure(text="Server: (none selected)")
        else:
            # Hide placeholder, show bot config
            self.no_server_label.pack_forget()
            self.bot_list_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 10))
            self.add_bot_frame.pack(fill=tk.X, pady=5)
            self.control_frame.pack(fill=tk.X, pady=5)
            self.status_frame.pack(fill=tk.BOTH, expand=True)
            self.right_frame.configure(text=f"Server: {self.current_server.key}")

            # Refresh bot list
            self._refresh_bot_tree()
            self._refresh_status_tree()

    def _refresh_bot_tree(self):
        """Refresh the bot configuration tree for current server."""
        self.bot_tree.delete(*self.bot_tree.get_children())
        if self.current_server:
            for bot in self.current_server.bots:
                masked_pw = "*" * min(len(bot.password), 8)
                self.bot_tree.insert('', tk.END, iid=bot.name, values=(bot.name, bot.bot_type, masked_pw))

    def _refresh_status_tree(self):
        """Refresh the status tree for current server."""
        self.status_tree.delete(*self.status_tree.get_children())
        if self.current_server:
            for bot in self.current_server.bots:
                full_key = f"{self.current_server.key}:{bot.name}"
                status = self.bot_statuses.get(full_key, {})
                status['bot_type'] = bot.bot_type  # Include bot type
                self._update_status_row(bot.name, status, initialize=True)

    # =========================================================================
    # Bot Management
    # =========================================================================

    def _add_bot(self):
        """Add a bot to the current server."""
        if not self.current_server:
            return

        name = self.new_bot_name_var.get().strip()
        password = self.new_bot_pass_var.get()
        bot_type = self.new_bot_type_var.get()

        if not name:
            messagebox.showwarning("Invalid Name", "Please enter a bot name.")
            return

        # Capitalize first letter, lowercase rest
        name = name[0].upper() + name[1:].lower()

        # Validate: letters only
        if not name.isalpha():
            messagebox.showwarning("Invalid Name", "Bot name must contain only letters.")
            return

        if not password:
            messagebox.showwarning("Invalid Password", "Please enter a password.")
            return

        # Check for duplicate within this server
        for bot in self.current_server.bots:
            if bot.name == name:
                messagebox.showwarning("Duplicate", f"Bot '{name}' already exists on this server.")
                return

        # Add bot
        bot_entry = BotEntry(name=name, password=password, bot_type=bot_type)
        self.current_server.bots.append(bot_entry)

        # Update UI
        masked_pw = "*" * min(len(password), 8)
        self.bot_tree.insert('', tk.END, iid=name, values=(name, bot_type, masked_pw))
        self.status_tree.insert('', tk.END, iid=name, values=(name, 'IDLE', '-', '-', 'Not started'))

        # Clear form
        self.new_bot_name_var.set("")
        self.new_bot_pass_var.set("")

        self._save_settings()

    def _remove_bot(self):
        """Remove selected bot from current server."""
        if not self.current_server:
            return

        selection = self.bot_tree.selection()
        if not selection:
            messagebox.showinfo("No Selection", "Please select a bot to remove.")
            return

        name = selection[0]

        # Stop if running
        full_key = f"{self.current_server.key}:{name}"
        self.runner.stop_bot(full_key)

        # Remove from server
        self.current_server.bots = [b for b in self.current_server.bots if b.name != name]

        # Update UI
        self.bot_tree.delete(name)
        if self.status_tree.exists(name):
            self.status_tree.delete(name)

        self._save_settings()

    def _start_all(self):
        """Start all bots on current server."""
        if not self.current_server:
            return

        for i, bot_entry in enumerate(self.current_server.bots):
            # Stagger starts by 2 seconds
            self.after(i * 2000, lambda b=bot_entry: self._start_bot(b))

    def _stop_all(self):
        """Stop all bots on current server."""
        if not self.current_server:
            return

        for bot_entry in self.current_server.bots:
            full_key = f"{self.current_server.key}:{bot_entry.name}"
            self.runner.stop_bot(full_key)

    def _start_bot(self, bot_entry: BotEntry):
        """Start a single bot on current server."""
        if not self.current_server:
            return
        self.runner.restart_bot(self.current_server, bot_entry)
        self._update_status_row(bot_entry.name, {'progression_state': 'CONNECTING', 'bot_type': bot_entry.bot_type})

    def _start_selected(self):
        """Start the selected bot(s) in status tree."""
        if not self.current_server:
            return

        selection = self.status_tree.selection()
        if not selection:
            messagebox.showinfo("No Selection", "Please select a bot from the status table.")
            return

        for name in selection:
            bot_entry = next((b for b in self.current_server.bots if b.name == name), None)
            if bot_entry:
                self._start_bot(bot_entry)

    def _pause_selected(self):
        """Pause/resume the selected bot(s)."""
        if not self.current_server:
            return

        selection = self.status_tree.selection()
        if not selection:
            return

        for name in selection:
            full_key = f"{self.current_server.key}:{name}"
            status = self.bot_statuses.get(full_key, {})
            if status.get('is_paused'):
                self.runner.resume_bot(full_key)
            else:
                self.runner.pause_bot(full_key)

    def _stop_selected(self):
        """Stop the selected bot(s)."""
        if not self.current_server:
            return

        selection = self.status_tree.selection()
        if not selection:
            return

        for name in selection:
            full_key = f"{self.current_server.key}:{name}"
            self.runner.stop_bot(full_key)

    # =========================================================================
    # Status Updates
    # =========================================================================

    def _poll_queues(self):
        """Poll status and log queues for updates."""
        # Process status updates
        try:
            while True:
                item = self.status_queue.get_nowait()
                msg_type, full_key, data = item
                self.bot_statuses[full_key] = data

                # Only update UI if this bot belongs to current server
                if self.current_server:
                    server_key = self.current_server.key
                    if full_key.startswith(server_key + ":"):
                        bot_name = full_key[len(server_key) + 1:]
                        if msg_type == 'status':
                            self._update_status_row(bot_name, data)
                        elif msg_type == 'error':
                            self._update_status_row(bot_name, {'error': data})
        except queue.Empty:
            pass

        # Process log messages
        try:
            while True:
                msg = self.log_queue.get_nowait()
                self._append_log(msg)
        except queue.Empty:
            pass

        # Schedule next poll
        self.after(100, self._poll_queues)

    def _update_status_row(self, name: str, status: dict, initialize: bool = False):
        """Update a row in the status table."""
        if not self.status_tree.exists(name):
            if initialize:
                bot_type = status.get('bot_type', status.get('class', 'avatar'))
                self.status_tree.insert('', tk.END, iid=name, values=(name, bot_type, '-', '-', 'Not started'))
            else:
                return

        # Build display values
        bot_type = status.get('bot_type', status.get('class', 'avatar'))
        state = status.get('state', '-')
        is_paused = status.get('is_paused', False)
        error = status.get('error')

        # Build progress text based on bot type
        prog_state = status.get('progression_state', '-')
        demon_state = status.get('demon_state')
        avatar_state = status.get('avatar_state')

        # For class bots, show more detailed progress
        if demon_state:
            current_disc = status.get('current_discipline', '')
            disc_levels = status.get('discipline_levels', {})
            if demon_state in ('FARMING_DISCIPLINE_POINTS', 'STARTING_RESEARCH', 'TRAINING_DISCIPLINE'):
                progress_text = f"{demon_state}: {current_disc}"
            elif disc_levels:
                # Show attack level progress
                attack_lvl = disc_levels.get('attack', 0)
                progress_text = f"{demon_state} (Atk:{attack_lvl})"
            else:
                progress_text = demon_state
        elif avatar_state:
            kills = status.get('kills', 0)
            target = status.get('target_kills', 5)
            progress_text = f"{avatar_state} ({kills}/{target})"
        elif prog_state != '-':
            kills = status.get('kills', 0)
            target = status.get('target_kills', 5)
            progress_text = f"{prog_state} ({kills}/{target})"
        else:
            progress_text = '-'

        # Determine status text and color tag
        final_state = demon_state or prog_state
        if error:
            status_text = f"Error: {error[:15]}"
            tag = 'error'
        elif final_state == 'COMPLETE':
            status_text = "Complete"
            tag = 'complete'
        elif final_state == 'FAILED':
            status_text = "Failed"
            tag = 'error'
        elif is_paused:
            status_text = "Paused"
            tag = 'paused'
        elif final_state in ('IDLE', '-', None):
            status_text = "Not started"
            tag = ''
        else:
            status_text = "Running"
            tag = 'running'

        # Update values
        self.status_tree.item(name, values=(name, bot_type, state, progress_text, status_text), tags=(tag,))

    def _append_log(self, message: str):
        """Append a message to the log output."""
        self.log_text.config(state=tk.NORMAL)
        self.log_text.insert(tk.END, message + "\n")
        # Limit to 1000 lines
        lines = int(self.log_text.index('end-1c').split('.')[0])
        if lines > 1000:
            self.log_text.delete(1.0, f"{lines - 1000}.0")
        self.log_text.see(tk.END)
        self.log_text.config(state=tk.DISABLED)

    def _clear_log(self):
        """Clear the log output."""
        self.log_text.config(state=tk.NORMAL)
        self.log_text.delete(1.0, tk.END)
        self.log_text.config(state=tk.DISABLED)

    # =========================================================================
    # Settings Persistence
    # =========================================================================

    def _load_settings(self):
        """Load saved settings from file."""
        if not SETTINGS_FILE.exists():
            return

        try:
            with open(SETTINGS_FILE, 'r') as f:
                data = json.load(f)

            # Check for old format (has 'bot_names' key) and migrate
            if 'bot_names' in data:
                logger.info("Migrating old settings format...")
                self._migrate_old_settings(data)
            else:
                # New format - load servers
                for server_data in data.get('servers', []):
                    server = ServerConfig.from_dict(server_data)
                    self.servers.append(server)
                    self.server_listbox.insert(tk.END, server.key)

            logger.info(f"Loaded settings: {len(self.servers)} server(s)")

            # Select first server if any
            if self.servers:
                self.server_listbox.selection_set(0)
                self._on_server_select(None)

        except Exception as e:
            logger.warning(f"Failed to load settings: {e}")

    def _migrate_old_settings(self, old_data: dict):
        """Migrate old settings format to new multi-server format."""
        host = old_data.get('host', 'localhost')
        port = int(old_data.get('port', '8888'))
        password = old_data.get('password', '')
        bot_names = old_data.get('bot_names', [])

        # Create single server with all bots using the shared password
        bots = [BotEntry(name=name, password=password) for name in bot_names]
        server = ServerConfig(host=host, port=port, bots=bots)

        self.servers.append(server)
        self.server_listbox.insert(tk.END, server.key)

        # Save in new format
        self._save_settings()
        logger.info(f"Migrated {len(bots)} bot(s) to new format")

    def _save_settings(self):
        """Save current settings to file."""
        data = {
            'servers': [server.to_dict() for server in self.servers]
        }

        try:
            with open(SETTINGS_FILE, 'w') as f:
                json.dump(data, f, indent=2)
        except Exception as e:
            logger.warning(f"Failed to save settings: {e}")

    def _on_close(self):
        """Handle window close."""
        self._save_settings()
        self.runner.stop()
        self.destroy()


# =============================================================================
# Entry Point
# =============================================================================

def main():
    """Main entry point."""
    app = BotManagerGUI()
    app.mainloop()


if __name__ == "__main__":
    main()
