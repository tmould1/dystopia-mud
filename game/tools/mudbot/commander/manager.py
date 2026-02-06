"""
Bot Commander - manages multiple bot instances.
"""

import asyncio
import logging
from typing import Dict, List, Optional
from dataclasses import dataclass
from enum import Enum, auto

from ..config import BotConfig, CommanderConfig, ProgressionConfig
from ..bot.avatar_bot import AvatarProgressionBot

logger = logging.getLogger(__name__)


class BotStatus(Enum):
    """Status of a managed bot."""
    PENDING = auto()
    CONNECTING = auto()
    RUNNING = auto()
    COMPLETE = auto()
    FAILED = auto()


@dataclass
class ManagedBot:
    """Container for a managed bot instance."""
    bot_id: int
    config: BotConfig
    bot: Optional[AvatarProgressionBot] = None
    task: Optional[asyncio.Task] = None
    status: BotStatus = BotStatus.PENDING


class BotCommander:
    """
    Manages multiple bot instances for load testing.

    Handles spawning, monitoring, and cleanup of bots.
    """

    def __init__(self, config: CommanderConfig):
        self.config = config
        self._bots: Dict[int, ManagedBot] = {}
        self._running = False

    def get_status(self) -> Dict[str, any]:
        """Get status of all managed bots."""
        return {
            'total': len(self._bots),
            'running': self._running,
            'bots': {
                bot_id: {
                    'name': mb.config.name,
                    'status': mb.status.name,
                    'progression': mb.bot.get_status() if mb.bot else None,
                }
                for bot_id, mb in self._bots.items()
            }
        }

    def print_status(self) -> None:
        """Print status summary to console."""
        status = self.get_status()
        print(f"\n=== Bot Commander Status ===")
        print(f"Total bots: {status['total']}")

        counts = {s: 0 for s in BotStatus}
        for bot_id, bot_info in status['bots'].items():
            status_enum = BotStatus[bot_info['status']]
            counts[status_enum] += 1

        print(f"Pending: {counts[BotStatus.PENDING]}, "
              f"Running: {counts[BotStatus.RUNNING]}, "
              f"Complete: {counts[BotStatus.COMPLETE]}, "
              f"Failed: {counts[BotStatus.FAILED]}")

        print("\nBot Details:")
        for bot_id, bot_info in status['bots'].items():
            prog = bot_info.get('progression') or {}
            kills = prog.get('kills', 0)
            target = prog.get('target_kills', 5)
            prog_state = prog.get('progression_state', 'N/A')
            print(f"  [{bot_id}] {bot_info['name']}: {bot_info['status']} "
                  f"(kills: {kills}/{target}, state: {prog_state})")

    async def spawn_bot(self, bot_id: int) -> ManagedBot:
        """
        Spawn a single bot instance.

        Args:
            bot_id: Unique ID for this bot.

        Returns:
            ManagedBot instance.
        """
        bot_config = self.config.generate_bot_config(bot_id)
        managed = ManagedBot(
            bot_id=bot_id,
            config=bot_config,
            status=BotStatus.PENDING,
        )
        self._bots[bot_id] = managed

        # Create the avatar progression bot
        prog_config = ProgressionConfig()
        managed.bot = AvatarProgressionBot(bot_config, prog_config)

        return managed

    async def _run_single_bot(self, managed: ManagedBot) -> None:
        """Run a single bot to completion."""
        try:
            managed.status = BotStatus.CONNECTING
            logger.info(f"[Commander] Starting bot {managed.config.name}")

            await managed.bot.run()

            # Check final state
            if managed.bot.progression_state.name == 'COMPLETE':
                managed.status = BotStatus.COMPLETE
                logger.info(f"[Commander] Bot {managed.config.name} completed successfully")
            else:
                managed.status = BotStatus.FAILED
                logger.warning(f"[Commander] Bot {managed.config.name} did not complete")

        except asyncio.CancelledError:
            logger.info(f"[Commander] Bot {managed.config.name} cancelled")
            managed.status = BotStatus.FAILED
        except Exception as e:
            logger.error(f"[Commander] Bot {managed.config.name} error: {e}")
            managed.status = BotStatus.FAILED

    async def spawn_all(self) -> List[ManagedBot]:
        """Spawn all configured bots."""
        logger.info(f"[Commander] Spawning {self.config.num_bots} bots...")
        bots = []
        for i in range(1, self.config.num_bots + 1):
            managed = await self.spawn_bot(i)
            bots.append(managed)
        return bots

    async def run_all(self) -> Dict[int, BotStatus]:
        """
        Run all spawned bots with staggered starts.

        Returns:
            Dict mapping bot_id to final status.
        """
        self._running = True
        logger.info(f"[Commander] Starting all bots with {self.config.stagger_delay}s stagger")

        # Create tasks for each bot with staggered starts
        tasks = []
        for bot_id, managed in self._bots.items():
            # Schedule this bot's start
            delay = (bot_id - 1) * self.config.stagger_delay

            async def run_with_delay(m: ManagedBot, d: float):
                if d > 0:
                    logger.debug(f"[Commander] Bot {m.config.name} starting in {d}s")
                    await asyncio.sleep(d)
                m.status = BotStatus.RUNNING
                await self._run_single_bot(m)

            task = asyncio.create_task(run_with_delay(managed, delay))
            managed.task = task
            tasks.append(task)

        # Wait for all bots to complete
        try:
            await asyncio.gather(*tasks, return_exceptions=True)
        except asyncio.CancelledError:
            logger.info("[Commander] Run cancelled, stopping bots...")
            for task in tasks:
                task.cancel()
            await asyncio.gather(*tasks, return_exceptions=True)

        self._running = False

        # Return final status
        return {bot_id: mb.status for bot_id, mb in self._bots.items()}

    async def stop_all(self) -> None:
        """Stop all running bots."""
        logger.info("[Commander] Stopping all bots...")
        for bot_id, managed in self._bots.items():
            if managed.task and not managed.task.done():
                managed.task.cancel()
            if managed.bot:
                managed.bot.stop()
                await managed.bot.disconnect()

        self._running = False

    def summary(self) -> str:
        """Get a summary string of results."""
        complete = sum(1 for mb in self._bots.values() if mb.status == BotStatus.COMPLETE)
        failed = sum(1 for mb in self._bots.values() if mb.status == BotStatus.FAILED)
        total = len(self._bots)
        return f"Complete: {complete}/{total}, Failed: {failed}/{total}"
