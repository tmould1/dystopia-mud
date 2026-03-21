"""
CLI interface for the bot commander.
"""

import argparse
import asyncio
import logging
import signal
import sys
from pathlib import Path

from ..config import BotConfig, CommanderConfig, ProgressionConfig, QuestConfig
from ..bot.avatar_bot import AvatarProgressionBot
from ..bot.quest_bot import QuestBot
from ..utils.logging import setup_logging
from .manager import BotCommander
from .multiplayer_manager import MultiplayerCommander
from .campaign_runner import QuestStoryCampaignRunner, CampaignProfile, DEFAULT_CLASS_PROFILES


def create_parser() -> argparse.ArgumentParser:
    """Create the argument parser."""
    parser = argparse.ArgumentParser(
        prog='mudbot',
        description='Dystopia MUD Bot Commander - Automated gameplay testing',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Run single bot (local server)
  python -m mudbot run --name TestBot --password secret123

  # Run single bot (remote server)
  python -m mudbot run --host mud.example.com --port 4000 --name Bot1 --password pass

  # Load test with 5 bots
  python -m mudbot load --count 5 --prefix LoadBot --password testpass

  # Quest testing (default: explevel 1, stop at M02)
  python -m mudbot quest --name QBot --password secret123

  # Quest testing (veteran, skip tutorials)
  python -m mudbot quest --name QBot --password secret123 --explevel 3

  # Quest testing (tutorial only)
  python -m mudbot quest --name QBot --password secret123 --mode tutorial

  # Verbose output
  python -m mudbot run --name TestBot --password secret123 --verbose

    # Multiplayer follow/group tutorial smoke test
    python -m mudbot multiplayer --count 2 --prefix Team --password testpass --scenario follow_group_tutorial

    # Multiplayer scripted campaign (JSON)
    python -m mudbot multiplayer --count 3 --prefix Squad --password testpass --script game/tools/mudbot/config/multiplayer_story.json
"""
    )

    # Global options
    parser.add_argument(
        '--host',
        default='localhost',
        help='MUD server hostname (default: localhost)'
    )
    parser.add_argument(
        '--port',
        type=int,
        default=8888,
        help='MUD server port (default: 8888)'
    )
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Enable verbose/debug logging'
    )

    subparsers = parser.add_subparsers(dest='command', required=True)

    # 'run' command - single bot
    run_parser = subparsers.add_parser(
        'run',
        help='Run a single bot'
    )
    run_parser.add_argument(
        '--name',
        required=True,
        help='Character name (3-12 alphanumeric characters)'
    )
    run_parser.add_argument(
        '--password',
        required=True,
        help='Character password (minimum 5 characters)'
    )
    run_parser.add_argument(
        '--sex',
        choices=['m', 'f'],
        default='m',
        help='Character sex (default: m)'
    )
    run_parser.add_argument(
        '--delay',
        type=float,
        default=0.5,
        help='Delay between commands in seconds (default: 0.5)'
    )
    run_parser.add_argument(
        '--kills',
        type=int,
        default=5,
        help='Number of monsters to kill (default: 5)'
    )
    run_parser.add_argument(
        '--hp-target',
        type=int,
        default=2000,
        help='HP target for avatar training (default: 2000)'
    )

    # 'load' command - multiple bots
    load_parser = subparsers.add_parser(
        'load',
        help='Run multiple bots for load testing'
    )
    load_parser.add_argument(
        '--count',
        type=int,
        default=5,
        help='Number of bots to spawn (default: 5, max: 100)'
    )
    load_parser.add_argument(
        '--prefix',
        default='LoadBot',
        help='Bot name prefix (default: LoadBot, generates LoadBot001, etc.)'
    )
    load_parser.add_argument(
        '--password',
        required=True,
        help='Password for all bots'
    )
    load_parser.add_argument(
        '--stagger',
        type=float,
        default=2.0,
        help='Delay between bot starts in seconds (default: 2.0)'
    )
    load_parser.add_argument(
        '--delay',
        type=float,
        default=0.5,
        help='Delay between commands per bot (default: 0.5)'
    )

    # 'quest' command - quest system testing
    quest_parser = subparsers.add_parser(
        'quest',
        help='Test the quest system end-to-end'
    )
    quest_parser.add_argument(
        '--name',
        required=True,
        help='Character name (3-12 alphanumeric characters)'
    )
    quest_parser.add_argument(
        '--password',
        required=True,
        help='Character password (minimum 5 characters)'
    )
    quest_parser.add_argument(
        '--sex',
        choices=['m', 'f'],
        default='m',
        help='Character sex (default: m)'
    )
    quest_parser.add_argument(
        '--explevel',
        type=int,
        choices=[1, 2, 3],
        default=1,
        help='Experience level for character creation (1=newbie, 2=MUD exp, 3=veteran; default: 1)'
    )
    quest_parser.add_argument(
        '--mode',
        choices=['tutorial', 'main', 'all'],
        default='all',
        help='Quest mode: tutorial (T quests only), main (T+M), all (default: all)'
    )
    quest_parser.add_argument(
        '--stop-after',
        default='M02',
        help='Stop after completing this quest ID (default: M02, empty=run all)'
    )
    quest_parser.add_argument(
        '--selfclass',
        default='demon',
        help='Class to select for M01 selfclass objective (default: demon)'
    )
    quest_parser.add_argument(
        '--delay',
        type=float,
        default=0.5,
        help='Delay between commands in seconds (default: 0.5)'
    )
    quest_parser.add_argument(
        '--enable-story',
        action='store_true',
        default=True,
        dest='enable_story',
        help='Use story system for area navigation and combat training (default: enabled)'
    )
    quest_parser.add_argument(
        '--no-story',
        action='store_false',
        dest='enable_story',
        help='Disable story system'
    )
    quest_parser.add_argument(
        '--enable-pvp',
        action='store_true',
        default=False,
        dest='enable_pvp',
        help='Enable PvP handlers (arena win, kill player)'
    )

    # 'multiplayer' command - coordinated multi-bot scenarios
    multi_parser = subparsers.add_parser(
        'multiplayer',
        help='Run multiplayer quest/story scenario tests'
    )
    multi_parser.add_argument(
        '--count',
        type=int,
        default=2,
        help='Number of bots to spawn (default: 2)'
    )
    multi_parser.add_argument(
        '--prefix',
        default='Team',
        help='Bot name prefix (default: Team)'
    )
    multi_parser.add_argument(
        '--password',
        required=True,
        help='Password for all bots'
    )
    multi_parser.add_argument(
        '--stagger',
        type=float,
        default=1.0,
        help='Delay between bot starts in seconds (default: 1.0)'
    )
    multi_parser.add_argument(
        '--delay',
        type=float,
        default=0.5,
        help='Delay between commands per bot (default: 0.5)'
    )
    multi_parser.add_argument(
        '--scenario',
        choices=['follow_group_tutorial', 'pk_duel_smoke', 'story_party_smoke'],
        default='follow_group_tutorial',
        help='Built-in multiplayer scenario (default: follow_group_tutorial)'
    )
    multi_parser.add_argument(
        '--script',
        help='Path to JSON scenario script; if set, overrides --scenario'
    )
    multi_parser.add_argument(
        '--strict',
        action='store_true',
        help='Treat missing/empty step responses as failures'
    )

    # 'campaign' command - full quest/story verification campaign
    campaign_parser = subparsers.add_parser(
        'campaign',
        help='Run full quest + story verification campaign and write reports'
    )
    campaign_parser.add_argument(
        '--password',
        required=True,
        help='Password for generated campaign bot characters'
    )
    campaign_parser.add_argument(
        '--prefix',
        default='CampBot',
        help='Bot name prefix (default: CampBot)'
    )
    campaign_parser.add_argument(
        '--classes',
        default='all',
        help="Comma list of classes, or 'all' for full class matrix"
    )
    campaign_parser.add_argument(
        '--explevels',
        default='1,3',
        help='Comma list of explevels to run per class (default: 1,3)'
    )
    campaign_parser.add_argument(
        '--max-profiles',
        type=int,
        default=0,
        help='Optional cap for profile count (0 = no cap)'
    )
    campaign_parser.add_argument(
        '--multiplayer-count',
        type=int,
        default=2,
        help='Multiplayer bot count for built-in smoke scenarios (default: 2)'
    )
    campaign_parser.add_argument(
        '--no-multiplayer',
        action='store_true',
        help='Skip multiplayer smoke scenarios'
    )
    campaign_parser.add_argument(
        '--strict',
        action='store_true',
        help='Enable strict checking for multiplayer script responses'
    )
    campaign_parser.add_argument(
        '--output-dir',
        default='game/tools/mudbot/reports',
        help='Directory for campaign report artifacts'
    )
    campaign_parser.add_argument(
        '--db-path',
        default='gamedata/db/game/quest.db',
        help='Canonical quest DB path (default: gamedata/db/game/quest.db)'
    )
    campaign_parser.add_argument(
        '--delay',
        type=float,
        default=0.5,
        help='Delay between commands in seconds (default: 0.5)'
    )
    campaign_parser.add_argument(
        '--max-quest-cycles',
        type=int,
        default=5000,
        help='Quest bot state-machine cycle budget per profile (default: 5000)'
    )
    campaign_parser.add_argument(
        '--no-force-story',
        action='store_true',
        help='Do not proactively advance story during avatar-capable profiles'
    )
    campaign_parser.add_argument(
        '--no-pvp',
        action='store_true',
        help='Disable PvP handlers for campaign quest profiles'
    )

    return parser


async def run_single_bot(args) -> int:
    """Run a single bot."""
    logger = logging.getLogger('mudbot')

    try:
        config = BotConfig(
            name=args.name,
            password=args.password,
            host=args.host,
            port=args.port,
            sex=args.sex,
            command_delay=args.delay,
        )
    except ValueError as e:
        logger.error("Configuration error: %s", e)
        return 1

    prog_config = ProgressionConfig(
        kills_needed=args.kills,
        hp_target=args.hp_target,
    )

    bot = AvatarProgressionBot(config, prog_config)

    # Handle Ctrl+C gracefully
    def signal_handler():
        logger.info("Interrupt received, stopping...")
        bot.stop()

    try:
        loop = asyncio.get_event_loop()
        if sys.platform != 'win32':
            loop.add_signal_handler(signal.SIGINT, signal_handler)
            loop.add_signal_handler(signal.SIGTERM, signal_handler)
    except NotImplementedError:
        # Windows doesn't support add_signal_handler in asyncio
        pass

    try:
        await bot.run()
        status = bot.get_status()
        if status['progression_state'] == 'COMPLETE':
            logger.info("Bot completed progression successfully!")
            return 0
        else:
            logger.warning("Bot ended in state: %s", status['progression_state'])
            return 1
    except KeyboardInterrupt:
        logger.info("Interrupted by user")
        await bot.disconnect()
        return 130


async def run_quest_bot(args) -> int:
    """Run the quest testing bot."""
    logger = logging.getLogger('mudbot')

    try:
        config = BotConfig(
            name=args.name,
            password=args.password,
            host=args.host,
            port=args.port,
            sex=args.sex,
            experience_level=args.explevel,
            command_delay=args.delay,
        )
    except ValueError as e:
        logger.error("Configuration error: %s", e)
        return 1

    quest_config = QuestConfig(
        mode=args.mode,
        stop_after_quest=args.stop_after,
        selfclass=args.selfclass,
        enable_story=args.enable_story,
        enable_pvp=args.enable_pvp,
    )

    prog_config = ProgressionConfig()

    bot = QuestBot(config, quest_config, prog_config)

    try:
        await bot.run()
        status = bot.get_status()
        if status['quest_state'] == 'COMPLETE':
            logger.info("Quest bot completed successfully!")
            return 0
        else:
            logger.warning("Quest bot ended in state: %s", status['quest_state'])
            return 1
    except KeyboardInterrupt:
        logger.info("Interrupted by user")
        await bot.disconnect()
        return 130


async def run_load_test(args) -> int:
    """Run multiple bots for load testing."""
    logger = logging.getLogger('mudbot')

    try:
        config = CommanderConfig(
            host=args.host,
            port=args.port,
            bot_prefix=args.prefix,
            bot_password=args.password,
            num_bots=args.count,
            stagger_delay=args.stagger,
            command_delay=args.delay,
        )
    except ValueError as e:
        logger.error("Configuration error: %s", e)
        return 1

    commander = BotCommander(config)

    # Handle Ctrl+C gracefully
    async def signal_handler():
        logger.info("Interrupt received, stopping all bots...")
        await commander.stop_all()

    try:
        loop = asyncio.get_event_loop()
        if sys.platform != 'win32':
            loop.add_signal_handler(signal.SIGINT, lambda: asyncio.create_task(signal_handler()))
            loop.add_signal_handler(signal.SIGTERM, lambda: asyncio.create_task(signal_handler()))
    except NotImplementedError:
        pass

    try:
        # Spawn all bots
        await commander.spawn_all()

        # Run them
        logger.info("Running %d bots against %s:%d", args.count, args.host, args.port)
        results = await commander.run_all()

        # Print summary
        commander.print_status()
        summary = commander.summary()
        logger.info("Load test complete: %s", summary)

        # Return success if all bots completed
        all_complete = all(s.name == 'COMPLETE' for s in results.values())
        return 0 if all_complete else 1

    except KeyboardInterrupt:
        logger.info("Interrupted by user")
        await commander.stop_all()
        return 130


async def run_multiplayer(args) -> int:
    """Run coordinated multiplayer scenarios for quest/story coverage."""
    logger = logging.getLogger('mudbot')

    if args.script:
        script_path = Path(args.script)
        if not script_path.exists():
            logger.error("Scenario script not found: %s", script_path)
            return 1

    try:
        config = CommanderConfig(
            host=args.host,
            port=args.port,
            bot_prefix=args.prefix,
            bot_password=args.password,
            num_bots=args.count,
            stagger_delay=args.stagger,
            command_delay=args.delay,
            experience_level=1,
        )
    except ValueError as e:
        logger.error("Configuration error: %s", e)
        return 1

    commander = MultiplayerCommander(config)

    try:
        started = await commander.start()
        if not started:
            return 1

        if args.script:
            result = await commander.run_script(args.script, strict=args.strict)
        else:
            result = await commander.run_builtin(args.scenario, strict=args.strict)

        logger.info(
            "Multiplayer scenario '%s' complete: success=%s, failed_steps=%d/%d",
            result.scenario,
            result.success,
            result.failed_steps,
            result.total_steps,
        )
        for entry in result.details:
            status = "PASS" if entry.ok else "FAIL"
            logger.info(
                "[%s] %s | %s | %s",
                status,
                entry.step_name,
                entry.bot_name,
                entry.response_excerpt,
            )

        return 0 if result.success else 1
    except KeyboardInterrupt:
        logger.info("Interrupted by user")
        return 130
    finally:
        await commander.stop()


def _parse_campaign_classes(raw: str) -> list[str]:
    """Parse --classes value into a class list."""
    if raw.strip().lower() == 'all':
        return list(DEFAULT_CLASS_PROFILES)
    return [c.strip().lower() for c in raw.split(',') if c.strip()]


def _parse_campaign_explevels(raw: str) -> list[int]:
    """Parse --explevels into sorted unique ints in [1, 2, 3]."""
    levels: set[int] = set()
    for part in raw.split(','):
        token = part.strip()
        if not token:
            continue
        try:
            lvl = int(token)
        except ValueError:
            continue
        if lvl in (1, 2, 3):
            levels.add(lvl)
    return sorted(levels)


async def run_campaign(args) -> int:
    """Run the full quest + story campaign and persist coverage reports."""
    logger = logging.getLogger('mudbot')

    classes = _parse_campaign_classes(args.classes)
    if not classes:
        logger.error("No valid classes resolved from --classes=%s", args.classes)
        return 1

    explevels = _parse_campaign_explevels(args.explevels)
    if not explevels:
        logger.error("No valid explevels resolved from --explevels=%s", args.explevels)
        return 1

    profiles = [CampaignProfile(selfclass=c, explevel=lvl) for c in classes for lvl in explevels]
    if args.max_profiles and args.max_profiles > 0:
        profiles = profiles[:args.max_profiles]

    if not profiles:
        logger.error("Campaign profile matrix is empty")
        return 1

    multiplayer_scenarios = [] if args.no_multiplayer else [
        'follow_group_tutorial',
        'pk_duel_smoke',
        'story_party_smoke',
    ]

    runner = QuestStoryCampaignRunner(
        host=args.host,
        port=args.port,
        password=args.password,
        prefix=args.prefix,
        delay=args.delay,
        strict=args.strict,
        output_dir=Path(args.output_dir),
        db_path=Path(args.db_path),
        max_quest_cycles=args.max_quest_cycles,
        force_story=(not args.no_force_story),
        enable_pvp=(not args.no_pvp),
    )

    logger.info("Campaign profiles: %d", len(profiles))
    logger.info("Campaign classes: %s", ', '.join(sorted(set(p.selfclass for p in profiles))))
    logger.info("Campaign explevels: %s", ', '.join(str(l) for l in sorted(set(p.explevel for p in profiles))))

    report, json_path, md_path = await runner.run(
        profiles=profiles,
        multiplayer_count=args.multiplayer_count,
        multiplayer_scenarios=multiplayer_scenarios,
    )

    logger.info("Campaign report (json): %s", json_path)
    logger.info("Campaign report (md): %s", md_path)
    logger.info(
        "Coverage: quests=%d/%d story=%d/%d",
        report.quest_covered_total,
        report.quest_target_total,
        report.story_covered_total,
        report.story_target_total,
    )

    if report.missing_quests:
        logger.warning("Missing quest IDs: %s", ', '.join(report.missing_quests))
    if report.missing_story_nodes:
        logger.warning(
            "Missing story nodes: %s",
            ', '.join(str(n) for n in report.missing_story_nodes),
        )

    return 0 if report.success else 1


def main() -> int:
    """Main entry point."""
    parser = create_parser()
    args = parser.parse_args()

    # Setup logging
    setup_logging(verbose=args.verbose)
    logger = logging.getLogger('mudbot')

    logger.info("Dystopia MUD Bot Commander")
    logger.info("Target: %s:%d", args.host, args.port)

    # Run the appropriate command
    if args.command == 'run':
        return asyncio.run(run_single_bot(args))
    elif args.command == 'quest':
        return asyncio.run(run_quest_bot(args))
    elif args.command == 'load':
        return asyncio.run(run_load_test(args))
    elif args.command == 'multiplayer':
        return asyncio.run(run_multiplayer(args))
    elif args.command == 'campaign':
        return asyncio.run(run_campaign(args))
    else:
        parser.print_help()
        return 1


if __name__ == '__main__':
    sys.exit(main())
