"""
Server admin utility for bot-driven operations.

Usage:
    python -m mudbot.admin login     -- Login as claude, verify connection
    python -m mudbot.admin relevel   -- Login, relevel to superadmin
    python -m mudbot.admin copyover  -- Login, relevel, copyover (hot-reload exe)
    python -m mudbot.admin cmd <cmd> -- Login, relevel, run arbitrary command
"""

import asyncio
import sys
import logging

from .connection import TelnetClient
from .bot.state_machine import LoginStateMachine

logging.basicConfig(level=logging.WARNING, stream=sys.stdout)
logger = logging.getLogger(__name__)

ADMIN_NAME = "claude"
ADMIN_PASS = "test123"
HOST = "localhost"
PORT = 8888


async def admin_connect() -> tuple[TelnetClient, bool]:
    """Connect and login as the admin character. Returns (client, success)."""
    c = TelnetClient(HOST, PORT)
    if not await c.connect():
        print("ERROR: Cannot connect to server")
        return c, False

    sm = LoginStateMachine(ADMIN_NAME, ADMIN_PASS, "m", 3, "s")

    for _ in range(40):
        text = await c.read(timeout=5)
        if text is None:
            if not c.connected:
                print("ERROR: Server closed connection during login")
                return c, False
            continue
        response = sm.process(text)
        if response is not None:
            await asyncio.sleep(0.2)
            await c.send(response)
        if sm.logged_in:
            break

    if not sm.logged_in:
        print("ERROR: Login failed")
        return c, False

    # Drain post-login output by sending a sync marker command.
    # 'time' is a simple command that always produces output.
    await asyncio.sleep(2)
    # Drain any immediate output
    while True:
        text = await c.read(timeout=1)
        if not text:
            break

    # Send sync command and wait for its output
    await c.send("time")
    await asyncio.sleep(1)
    while True:
        text = await c.read(timeout=2)
        if not text:
            break
        # Once we see "time" output (contains day/month/hour info), buffer is synced
        if any(w in (text or "").lower() for w in ["day of", "hour", "o'clock", "time"]):
            break

    # Final drain
    while True:
        text = await c.read(timeout=0.5)
        if not text:
            break

    return c, True


async def send_cmd(c: TelnetClient, cmd: str, wait: float = 1.0) -> str:
    """Send a command and return the response text."""
    await c.send(cmd)
    await asyncio.sleep(wait)
    parts = []
    while True:
        text = await c.read(timeout=1)
        if not text:
            break
        parts.append(text)
    return "".join(parts)


async def do_login():
    """Just login and verify."""
    c, ok = await admin_connect()
    if not ok:
        await c.disconnect()
        return False

    text = await send_cmd(c, "who")
    safe = text.encode("ascii", "replace").decode("ascii").strip()
    print(f"Connected as {ADMIN_NAME}.")
    print(f"WHO: {safe[:300]}")

    await send_cmd(c, "save")
    await send_cmd(c, "quit")
    await c.disconnect()
    return True


async def do_relevel():
    """Login and relevel to superadmin."""
    c, ok = await admin_connect()
    if not ok:
        await c.disconnect()
        return False

    text = await send_cmd(c, "relevel")
    if "CHEF" in text:
        print("Releveled to superadmin!")
        await send_cmd(c, "save")
        await send_cmd(c, "quit")
        await c.disconnect()
        return True
    else:
        safe = text.encode("ascii", "replace").decode("ascii").strip()
        print(f"Relevel failed. Response: {safe[:200]}")
        # Show score for debug
        text2 = await send_cmd(c, "score")
        safe2 = text2.encode("ascii", "replace").decode("ascii").strip()
        print(f"Score: {safe2[:300]}")
        await c.disconnect()
        return False


async def do_copyover():
    """Login, relevel, and copyover to hot-reload the server exe."""
    c, ok = await admin_connect()
    if not ok:
        await c.disconnect()
        return False

    text = await send_cmd(c, "relevel")
    if "CHEF" not in text:
        safe = text.encode("ascii", "replace").decode("ascii").strip()
        print(f"Relevel failed: {safe[:200]}")
        await c.disconnect()
        return False

    print("Releveled! Sending copyover...")
    await c.send("copyover")
    await asyncio.sleep(5)

    # After copyover, server restarts. Try to read reconnect output.
    text = await c.read(timeout=10)
    if text:
        safe = text.encode("ascii", "replace").decode("ascii").strip()
        print(f"Post-copyover: {safe[:300]}")

    await c.disconnect()
    print("Copyover complete. Server should be running new exe.")
    return True


async def do_cmd(cmd: str):
    """Login, relevel, run a command."""
    c, ok = await admin_connect()
    if not ok:
        await c.disconnect()
        return False

    text = await send_cmd(c, "relevel")
    if "CHEF" not in text:
        # Maybe already max level, try anyway
        pass

    text = await send_cmd(c, cmd, wait=2.0)
    safe = text.encode("ascii", "replace").decode("ascii").strip()
    print(f"Response:\n{safe}")

    await send_cmd(c, "save")
    await send_cmd(c, "quit")
    await c.disconnect()
    return True


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1

    action = sys.argv[1]

    if action == "login":
        ok = asyncio.run(do_login())
    elif action == "relevel":
        ok = asyncio.run(do_relevel())
    elif action == "copyover":
        ok = asyncio.run(do_copyover())
    elif action == "cmd":
        if len(sys.argv) < 3:
            print("Usage: python -m mudbot.admin cmd <command>")
            return 1
        cmd = " ".join(sys.argv[2:])
        ok = asyncio.run(do_cmd(cmd))
    else:
        print(f"Unknown action: {action}")
        print(__doc__)
        return 1

    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
