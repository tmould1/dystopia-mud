#!/usr/bin/env python3
"""
Connect as superadmin 'Claude', do copyover, and/or boost a target
character's stats for quest testing.

Automatically logs in the target character on a second connection so
get_char_world can find them (qadmin requires target to be online).

Usage:
    python copyover_and_boost.py --no-copyover --target Stancey --hp 25000 --mana 15000
    python copyover_and_boost.py --complete M05 --complete M04 --target Stancey
    python copyover_and_boost.py --no-copyover --target Stancey --complete M05 --hp 20000 --mana 15000
    python copyover_and_boost.py --no-copyover --target Stancey --cmd "selfclass vampire"
"""

import asyncio
import argparse
import logging
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))
from mudbot.connection.telnet_client import TelnetClient

logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)s] %(message)s',
                    datefmt='%H:%M:%S')
logger = logging.getLogger(__name__)

ADMIN_NAME = "Claude"
ADMIN_PASS = "test123"
HOST = "localhost"
PORT = 8888


async def connect_and_login(host, port, name, password):
    """Connect and login, handling both new and existing characters.

    Returns the telnet client on success, None on failure.
    """
    client = TelnetClient(host, port)
    await client.connect()
    await asyncio.sleep(1)
    await client.read(timeout=3)

    await client.send(f"{name}\n")
    await asyncio.sleep(1)
    resp = await client.read(timeout=3) or ""
    resp_lower = resp.lower()

    if "password" in resp_lower:
        await client.send(f"{password}\n")
    elif "tombstone" in resp_lower:
        # New character — walk through creation
        logger.info(f"Creating new character: {name}")
        await client.send("y\n")
        await asyncio.sleep(1)
        await client.read(timeout=3)
        await client.send(f"{password}\n")
        await asyncio.sleep(1)
        await client.read(timeout=3)
        await client.send(f"{password}\n")
        await asyncio.sleep(1)
        await client.read(timeout=3)
        await client.send("m\n")
        await asyncio.sleep(1)
        await client.read(timeout=3)
        await client.send("3\n")
        await asyncio.sleep(1)
        await client.read(timeout=3)
        await client.send("s\n")  # screenreader mode
        await asyncio.sleep(1)
        await client.read(timeout=3)
        await client.send("\n")  # MOTD
    else:
        logger.error(f"Unexpected: {resp[:200]}")
        return None

    await asyncio.sleep(2)
    resp = await client.read(timeout=5)
    # Handle MOTD press-enter if present
    if resp and ("enter" in resp.lower() or "[ENTER]" in resp):
        await client.send("\n")
        await asyncio.sleep(1)
        await client.read(timeout=3)

    logger.info(f"Logged in as {name}")
    return client


async def send_cmd(client, cmd, wait_time=1.5):
    """Send a command and return response."""
    await client.send(f"{cmd}\n")
    await asyncio.sleep(wait_time)
    resp = await client.read(timeout=3)
    return resp or ""


async def main():
    parser = argparse.ArgumentParser(description="Admin helper for quest testing")
    parser.add_argument("--target", default="Stancey", help="Character to boost")
    parser.add_argument("--target-pass", default="test12345", help="Target character password")
    parser.add_argument("--hp", type=int, default=0, help="HP to set (total, not delta)")
    parser.add_argument("--mana", type=int, default=0, help="Mana to set (total, not delta)")
    parser.add_argument("--move", type=int, default=0, help="Move to set (total, not delta)")
    parser.add_argument("--exp", type=int, default=0, help="Exp to add")
    parser.add_argument("--complete", action="append", default=[], help="Quest IDs to complete")
    parser.add_argument("--cmd", action="append", default=[], help="Commands to run AS the target character")
    parser.add_argument("--no-copyover", action="store_true", help="Skip copyover step")
    args = parser.parse_args()

    # Step 1: Connect and login as admin
    logger.info(f"Connecting as {ADMIN_NAME}...")
    admin = await connect_and_login(HOST, PORT, ADMIN_NAME, ADMIN_PASS)
    if not admin:
        return

    resp = await send_cmd(admin, "relevel")
    logger.info(f"Relevel: {resp.strip()[:60]}")

    # Step 2: Copyover if requested
    if not args.no_copyover:
        logger.info("Copyover...")
        await admin.send("copyover\n")
        await asyncio.sleep(5)
        admin.disconnect()
        await asyncio.sleep(3)
        logger.info("Reconnecting admin...")
        admin = await connect_and_login(HOST, PORT, ADMIN_NAME, ADMIN_PASS)
        if not admin:
            logger.error("Admin reconnect failed")
            return
        resp = await send_cmd(admin, "relevel")

    # Step 3: Login target character on second connection
    target = args.target
    needs_target = args.hp or args.mana or args.move or args.exp or args.complete or args.cmd
    target_client = None

    if needs_target:
        logger.info(f"Logging in target: {target}...")
        target_client = await connect_and_login(HOST, PORT, target, args.target_pass)
        if not target_client:
            logger.error(f"Failed to login target {target}")
            await send_cmd(admin, "quit")
            return
        await asyncio.sleep(1)

    # Step 4: Boost stats (values are totals — calculate delta from current)
    for stat, val in [("hp", args.hp), ("mana", args.mana), ("move", args.move)]:
        if val > 0:
            # Get current stats via qadmin info
            resp = await send_cmd(admin, f"score", wait_time=0.5)
            # Just boost by the requested amount as a delta
            resp = await send_cmd(admin, f"qadmin {target} boost {stat} {val}")
            logger.info(f"boost {stat} +{val}: {resp.strip()[:150]}")

    if args.exp > 0:
        resp = await send_cmd(admin, f"qadmin {target} boost exp {args.exp}")
        logger.info(f"boost exp +{args.exp}: {resp.strip()[:150]}")

    # Step 5: Complete quests
    for quest_id in args.complete:
        resp = await send_cmd(admin, f"qadmin {target} complete {quest_id}")
        logger.info(f"complete {quest_id}: {resp.strip()[:150]}")

    # Step 6: Run commands as the target character
    if target_client and args.cmd:
        for cmd in args.cmd:
            resp = await send_cmd(target_client, cmd)
            logger.info(f"target cmd '{cmd}': {resp.strip()[:200]}")

    # Step 7: Save the target character
    if target_client:
        await send_cmd(target_client, "save")
        logger.info("Target saved")
        await send_cmd(target_client, "quit")
        target_client = None

    await send_cmd(admin, "quit")
    logger.info("Done!")

if __name__ == "__main__":
    asyncio.run(main())
