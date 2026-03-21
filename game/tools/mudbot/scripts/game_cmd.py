#!/usr/bin/env python3
"""
Unified game command relay and bot launcher. Connects to the MUD and
executes commands from a JSON command file, then optionally launches
one or more quest bots (in parallel).

All game interaction goes through this single script.

Usage:
    # Run a command file:
    python game_cmd.py commands.json

    # Inline admin commands:
    python game_cmd.py --admin "relevel" --admin "qadmin Stancey boost hp 25000"

    # Copyover (rebuild hot-reload):
    python game_cmd.py --copyover

    # Full setup + parallel bot run from command file:
    python game_cmd.py class_test_batch1.json

Command file format (JSON):
{
    "copyover": false,
    "admin_cmds": ["relevel"],
    "targets": [
        {
            "name": "BotVamp",
            "password": "test12345",
            "qadmin_cmds": ["boost level 3", "boost hp 25000", "complete M05"],
            "target_cmds": ["selfclass vampire"],
            "repeat_cmd": {"cmd": "boost exp 1000000", "times": 30, "wait": 1.5}
        },
        {
            "name": "BotDemon",
            "password": "test12345",
            "qadmin_cmds": ["boost level 3", "boost hp 25000", "complete M05"],
            "target_cmds": ["selfclass demon"]
        }
    ],
    "bots": [
        {
            "name": "BotVamp", "password": "test12345",
            "explevel": 3, "selfclass": "vampire",
            "stop_after": "", "mode": "all"
        },
        {
            "name": "BotDemon", "password": "test12345",
            "explevel": 3, "selfclass": "demon",
            "stop_after": "", "mode": "all"
        }
    ]
}
"""

import asyncio
import argparse
import json
import logging
import sys
import os
import traceback

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
    """Connect and login, handling both new and existing characters."""
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


def sanitize(text, maxlen=200):
    """Sanitize text for logging (ASCII only, truncated)."""
    clean = text.encode('ascii', errors='replace').decode('ascii')
    clean = clean.strip()
    return clean[:maxlen] if len(clean) > maxlen else clean


async def setup_target(admin, target_cfg):
    """Set up a single target character via admin relay.

    Logs in the target, runs qadmin commands, runs target commands,
    then saves and quits the target.
    """
    name = target_cfg["name"]
    password = target_cfg.get("password", "test12345")

    logger.info(f"--- Setting up target: {name} ---")

    # Login target
    target_client = await connect_and_login(HOST, PORT, name, password)
    if not target_client:
        logger.error(f"Failed to login target {name}")
        return False
    await asyncio.sleep(1)

    # Run qadmin commands
    for subcmd in target_cfg.get("qadmin_cmds", []):
        resp = await send_cmd(admin, f"qadmin {name} {subcmd}")
        logger.info(f"qadmin {name} {subcmd}: {sanitize(resp)}")

    # Run repeat command
    rc = target_cfg.get("repeat_cmd")
    if rc:
        cmd_str = rc["cmd"]
        times = rc.get("times", 1)
        is_qadmin = cmd_str.startswith("boost ") or cmd_str.startswith("complete ")
        wait = rc.get("wait", 1.5)
        logger.info(f"Repeating '{cmd_str}' x{times} for {name}...")
        for i in range(times):
            try:
                if is_qadmin:
                    resp = await send_cmd(admin, f"qadmin {name} {cmd_str}", wait_time=wait)
                else:
                    resp = await send_cmd(target_client, cmd_str, wait_time=wait)
            except Exception as e:
                logger.error(f"  [{i+1}/{times}] Connection lost: {e}")
                break
            if (i + 1) % 10 == 0 or i == 0 or i == times - 1:
                logger.info(f"  [{i+1}/{times}] {sanitize(resp, 100)}")

    # Run target commands
    for cmd in target_cfg.get("target_cmds", []):
        resp = await send_cmd(target_client, cmd)
        logger.info(f"{name}> {cmd}: {sanitize(resp)}")

    # Save and quit target
    await send_cmd(target_client, "save")
    logger.info(f"{name} saved")
    await send_cmd(target_client, "quit")
    return True


async def run_relay(cfg):
    """Run the admin/target relay commands."""
    targets = cfg.get("targets", [])

    # Legacy single-target support
    if not targets and cfg.get("target"):
        targets = [{
            "name": cfg["target"],
            "password": cfg.get("target_pass", "test12345"),
            "qadmin_cmds": cfg.get("qadmin_cmds", []),
            "target_cmds": cfg.get("target_cmds", []),
            "repeat_cmd": cfg.get("repeat_cmd"),
        }]

    has_relay_work = cfg.get("copyover") or cfg.get("admin_cmds") or targets

    if not has_relay_work:
        return

    # Step 1: Connect admin
    logger.info(f"Connecting as {ADMIN_NAME}...")
    admin = await connect_and_login(HOST, PORT, ADMIN_NAME, ADMIN_PASS)
    if not admin:
        return

    resp = await send_cmd(admin, "relevel")
    logger.info(f"Relevel: {sanitize(resp, 60)}")

    # Step 2: Copyover
    if cfg.get("copyover"):
        logger.info("Copyover...")
        await admin.send("copyover\n")
        await asyncio.sleep(5)
        try:
            await admin.disconnect()
        except Exception:
            pass
        await asyncio.sleep(3)
        logger.info("Reconnecting admin...")
        admin = await connect_and_login(HOST, PORT, ADMIN_NAME, ADMIN_PASS)
        if not admin:
            logger.error("Admin reconnect failed")
            return
        await send_cmd(admin, "relevel")

    # Step 3: Run admin commands
    for cmd in cfg.get("admin_cmds", []):
        resp = await send_cmd(admin, cmd)
        logger.info(f"admin> {cmd}: {sanitize(resp)}")

    # Step 4: Set up each target sequentially (they share the admin connection)
    for target_cfg in (targets or []):
        await setup_target(admin, target_cfg)

    await send_cmd(admin, "quit")
    logger.info("Relay phase done.")


async def run_bot(bot_cfg):
    """Launch a single quest bot. Returns (name, success, results)."""
    from mudbot.config import BotConfig, QuestConfig, ProgressionConfig
    from mudbot.bot.quest_bot import QuestBot

    name = bot_cfg["name"]
    password = bot_cfg["password"]
    explevel = bot_cfg.get("explevel", 3)
    selfclass = bot_cfg.get("selfclass", "demon")
    stop_after = bot_cfg.get("stop_after", "")
    mode = bot_cfg.get("mode", "all")
    enable_story = bot_cfg.get("enable_story", False)
    enable_pvp = bot_cfg.get("enable_pvp", False)
    delay = bot_cfg.get("delay", 0.5)

    logger.info(f"Launching quest bot: {name} (class={selfclass}, "
                f"stop={stop_after}, story={enable_story})")

    config = BotConfig(
        name=name,
        password=password,
        host=HOST,
        port=PORT,
        sex="m",
        experience_level=explevel,
        command_delay=delay,
    )

    quest_config = QuestConfig(
        mode=mode,
        stop_after_quest=stop_after,
        selfclass=selfclass,
        enable_story=enable_story,
        enable_pvp=enable_pvp,
    )

    prog_config = ProgressionConfig()
    bot = QuestBot(config, quest_config, prog_config)

    try:
        await bot.run()
        status = bot.get_status()
        state = status.get('quest_state', 'UNKNOWN')
        completed = status.get('quests_completed', [])
        results = status.get('results', [])
        logger.info(f"[{name}] Bot finished: state={state}, "
                    f"completed={len(completed)} quests: {completed}")
        return name, state == 'COMPLETE', status
    except Exception as e:
        logger.error(f"[{name}] Bot error: {e}\n{traceback.format_exc()}")
        return name, False, {"error": str(e)}


async def run_bots_parallel(bot_cfgs):
    """Launch multiple quest bots in parallel. Returns list of (name, success, status)."""
    logger.info(f"=== Launching {len(bot_cfgs)} bots in parallel ===")
    tasks = [run_bot(cfg) for cfg in bot_cfgs]
    results = await asyncio.gather(*tasks, return_exceptions=True)

    # Print summary
    logger.info(f"\n=== Parallel Bot Results ===")
    all_success = True
    for r in results:
        if isinstance(r, Exception):
            logger.error(f"Bot crashed: {r}")
            all_success = False
        else:
            name, success, status = r
            mark = "PASS" if success else "FAIL"
            completed = status.get('quests_completed', [])
            logger.info(f"  [{mark}] {name}: {len(completed)} quests completed")
            if not success:
                all_success = False

    return all_success


async def main():
    parser = argparse.ArgumentParser(description="Unified game command relay and bot launcher")
    parser.add_argument("cmdfile", nargs="?", help="JSON command file")
    parser.add_argument("--target", default=None, help="Target character name")
    parser.add_argument("--target-pass", default="test12345", help="Target password")
    parser.add_argument("--admin", action="append", default=[], help="Admin commands")
    parser.add_argument("--qadmin", action="append", default=[], help="qadmin <target> subcommands")
    parser.add_argument("--tcmd", action="append", default=[], help="Commands as target character")
    parser.add_argument("--copyover", action="store_true", help="Do copyover first")
    args = parser.parse_args()

    # Merge command file with CLI args
    cfg = {
        "target": args.target,
        "target_pass": args.target_pass,
        "copyover": args.copyover,
        "admin_cmds": list(args.admin),
        "qadmin_cmds": list(args.qadmin),
        "target_cmds": list(args.tcmd),
        "repeat_cmd": None,
        "bot": None,
        "bots": None,
        "targets": None,
    }

    if args.cmdfile:
        with open(args.cmdfile) as f:
            file_cfg = json.load(f)
        cfg["target"] = file_cfg.get("target", cfg["target"])
        cfg["target_pass"] = file_cfg.get("target_pass", cfg["target_pass"])
        cfg["copyover"] = file_cfg.get("copyover", cfg["copyover"])
        cfg["admin_cmds"] = file_cfg.get("admin_cmds", []) + cfg["admin_cmds"]
        cfg["qadmin_cmds"] = file_cfg.get("qadmin_cmds", []) + cfg["qadmin_cmds"]
        cfg["target_cmds"] = file_cfg.get("target_cmds", []) + cfg["target_cmds"]
        cfg["repeat_cmd"] = file_cfg.get("repeat_cmd", None)
        cfg["bot"] = file_cfg.get("bot", None)
        cfg["bots"] = file_cfg.get("bots", None)
        cfg["targets"] = file_cfg.get("targets", None)

    # Phase 1: Relay commands (admin setup, boosts, quest completions)
    await run_relay(cfg)

    # Phase 2: Launch bot(s)
    bots = cfg.get("bots")
    if bots:
        # Parallel bot run
        success = await run_bots_parallel(bots)
        sys.exit(0 if success else 1)
    elif cfg.get("bot"):
        # Single bot run
        name, success, status = await run_bot(cfg["bot"])
        sys.exit(0 if success else 1)

    logger.info("Done!")


if __name__ == "__main__":
    asyncio.run(main())
