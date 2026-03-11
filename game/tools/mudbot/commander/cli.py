"""
CLI interface for the bot commander.
"""

import argparse
import asyncio
import logging
import signal
import sys

from ..config import BotConfig, CommanderConfig, ProgressionConfig, QuestConfig
from ..bot.avatar_bot import AvatarProgressionBot
from ..bot.quest_bot import QuestBot
from ..utils.logging import setup_logging
from .manager import BotCommander


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
        logger.error(f"Configuration error: {e}")
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
            logger.warning(f"Bot ended in state: {status['progression_state']}")
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
        logger.error(f"Configuration error: {e}")
        return 1

    quest_config = QuestConfig(
        mode=args.mode,
        stop_after_quest=args.stop_after,
        selfclass=args.selfclass,
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
            logger.warning(f"Quest bot ended in state: {status['quest_state']}")
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
        logger.error(f"Configuration error: {e}")
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
        logger.info(f"Running {args.count} bots against {args.host}:{args.port}")
        results = await commander.run_all()

        # Print summary
        commander.print_status()
        summary = commander.summary()
        logger.info(f"Load test complete: {summary}")

        # Return success if all bots completed
        all_complete = all(s.name == 'COMPLETE' for s in results.values())
        return 0 if all_complete else 1

    except KeyboardInterrupt:
        logger.info("Interrupted by user")
        await commander.stop_all()
        return 130


def main() -> int:
    """Main entry point."""
    parser = create_parser()
    args = parser.parse_args()

    # Setup logging
    setup_logging(verbose=args.verbose)
    logger = logging.getLogger('mudbot')

    logger.info(f"Dystopia MUD Bot Commander")
    logger.info(f"Target: {args.host}:{args.port}")

    # Run the appropriate command
    if args.command == 'run':
        return asyncio.run(run_single_bot(args))
    elif args.command == 'quest':
        return asyncio.run(run_quest_bot(args))
    elif args.command == 'load':
        return asyncio.run(run_load_test(args))
    else:
        parser.print_help()
        return 1


if __name__ == '__main__':
    sys.exit(main())
