"""
Multiplayer commander for coordinated quest/story test scenarios.

This module adds a scenario-oriented orchestrator that can:
1) connect and login multiple bots,
2) execute scripted command steps by role (leader/followers/all),
3) run built-in scenarios for follow/group, PK, and story smoke checks.
"""

from __future__ import annotations

import asyncio
import json
import logging
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from ..bot.actions import BotActions
from ..bot.base import MudBot
from ..bot.quest_actions import QuestActions
from ..config import CommanderConfig

logger = logging.getLogger(__name__)


@dataclass
class SessionBot:
    """Runtime container for one connected bot session."""

    bot_id: int
    name: str
    bot: MudBot
    actions: BotActions
    quest_actions: QuestActions


@dataclass
class ScenarioStepResult:
    """Result for a single executed scenario step."""

    step_name: str
    bot_name: str
    command: str
    ok: bool
    response_excerpt: str = ""


@dataclass
class MultiplayerRunResult:
    """Aggregated scenario execution result."""

    success: bool
    scenario: str
    total_steps: int
    failed_steps: int
    details: list[ScenarioStepResult] = field(default_factory=list)


class MultiplayerCommander:
    """Coordinate multiple MudBot sessions for multiplayer scenario testing."""

    BUILTIN_SCENARIOS = (
        "follow_group_tutorial",
        "pk_duel_smoke",
        "story_party_smoke",
    )

    def __init__(self, config: CommanderConfig):
        self.config = config
        self._sessions: list[SessionBot] = []

    @property
    def sessions(self) -> list[SessionBot]:
        return self._sessions

    async def start(self) -> bool:
        """Connect and login all configured bots."""
        logger.info(
            "[Multiplayer] Starting %d bots on %s:%d",
            self.config.num_bots,
            self.config.host,
            self.config.port,
        )

        for idx in range(1, self.config.num_bots + 1):
            bot_cfg = self.config.generate_bot_config(idx)
            bot = MudBot(bot_cfg)

            if not await bot.connect():
                logger.error("[Multiplayer] %s failed to connect", bot_cfg.name)
                await self.stop()
                return False

            if not await bot.login():
                logger.error("[Multiplayer] %s failed login", bot_cfg.name)
                await self.stop()
                return False

            # Allow prompt/output to settle after login.
            await asyncio.sleep(1.5)

            session = SessionBot(
                bot_id=idx,
                name=bot_cfg.name,
                bot=bot,
                actions=BotActions(bot),
                quest_actions=QuestActions(bot),
            )
            self._sessions.append(session)
            logger.info("[Multiplayer] %s ready", session.name)

            if idx < self.config.num_bots and self.config.stagger_delay > 0:
                await asyncio.sleep(self.config.stagger_delay)

        return True

    async def stop(self) -> None:
        """Gracefully save/quit/disconnect all active sessions."""
        for session in self._sessions:
            if session.bot.connected:
                await session.actions.save()
                await session.actions.quit()

            await session.bot.disconnect()

        self._sessions.clear()

    async def run_builtin(self, scenario: str, strict: bool = False) -> MultiplayerRunResult:
        """Run one of the built-in scenario flows."""
        if scenario not in self.BUILTIN_SCENARIOS:
            raise ValueError(f"Unknown multiplayer scenario: {scenario}")

        if scenario == "follow_group_tutorial":
            return await self._run_follow_group_tutorial(strict=strict)
        if scenario == "pk_duel_smoke":
            return await self._run_pk_duel_smoke(strict=strict)
        return await self._run_story_party_smoke(strict=strict)

    async def run_script(self, script_path: str, strict: bool = False) -> MultiplayerRunResult:
        """Run a JSON-defined multiplayer scenario script."""
        path = Path(script_path)
        with path.open("r", encoding="utf-8") as f:
            data = json.load(f)

        scenario_name = data.get("name") or path.stem
        steps = data.get("steps")
        if not isinstance(steps, list) or not steps:
            raise ValueError("Scenario script must contain a non-empty 'steps' array")

        results: list[ScenarioStepResult] = []
        for idx, step in enumerate(steps, start=1):
            step_name = str(step.get("name") or f"step_{idx}")
            step_results = await self._execute_step(step_name, step, strict=strict)
            results.extend(step_results)

        failed = sum(1 for r in results if not r.ok)
        return MultiplayerRunResult(
            success=(failed == 0),
            scenario=scenario_name,
            total_steps=len(results),
            failed_steps=failed,
            details=results,
        )

    # ------------------------------------------------------------------
    # Built-in scenario implementations
    # ------------------------------------------------------------------

    async def _run_follow_group_tutorial(self, strict: bool) -> MultiplayerRunResult:
        """Smoke test party actions commonly needed by tutorial/quest objectives."""
        if len(self._sessions) < 2:
            raise ValueError("follow_group_tutorial requires at least 2 bots")

        leader = self._sessions[0]
        wing = self._sessions[1]

        plan = [
            {"name": "leader_quest_list", "bot": "leader", "command": "quest list"},
            {"name": "wing_quest_list", "bot": "followers", "command": "quest list"},
            {
                "name": "wing_follow_leader",
                "bot": wing.name,
                "command": f"follow {leader.name}",
            },
            {
                "name": "leader_group_wing",
                "bot": "leader",
                "command": f"group {wing.name}",
            },
            {"name": "leader_group_report", "bot": "leader", "command": "group"},
            {"name": "party_group_chat", "bot": "all", "command": "gtell mudbot party check"},
            {"name": "party_tutorial_commands", "bot": "all", "command": "look"},
            {"name": "party_score", "bot": "all", "command": "score"},
            {"name": "party_quest_progress", "bot": "all", "command": "quest progress"},
        ]

        results: list[ScenarioStepResult] = []
        for step in plan:
            results.extend(await self._execute_step(step["name"], step, strict=strict))

        failed = sum(1 for r in results if not r.ok)
        return MultiplayerRunResult(
            success=(failed == 0),
            scenario="follow_group_tutorial",
            total_steps=len(results),
            failed_steps=failed,
            details=results,
        )

    async def _run_pk_duel_smoke(self, strict: bool) -> MultiplayerRunResult:
        """Smoke test player-vs-player objective hooks with two bots."""
        if len(self._sessions) < 2:
            raise ValueError("pk_duel_smoke requires at least 2 bots")

        attacker = self._sessions[0]
        defender = self._sessions[1]

        plan = [
            {"name": "all_recall", "bot": "all", "command": "recall"},
            {"name": "break_follow", "bot": defender.name, "command": "follow self"},
            {"name": "attacker_pk", "bot": attacker.name, "command": f"kill {defender.name}"},
            {"name": "defender_status", "bot": defender.name, "command": "score"},
            {"name": "attacker_status", "bot": attacker.name, "command": "score"},
        ]

        results: list[ScenarioStepResult] = []
        for step in plan:
            results.extend(await self._execute_step(step["name"], step, strict=strict))

        failed = sum(1 for r in results if not r.ok)
        return MultiplayerRunResult(
            success=(failed == 0),
            scenario="pk_duel_smoke",
            total_steps=len(results),
            failed_steps=failed,
            details=results,
        )

    async def _run_story_party_smoke(self, strict: bool) -> MultiplayerRunResult:
        """Smoke test story/quest observability while grouped."""
        if len(self._sessions) < 2:
            raise ValueError("story_party_smoke requires at least 2 bots")

        leader = self._sessions[0]
        wing = self._sessions[1]
        plan = [
            {"name": "all_story", "bot": "all", "command": "story"},
            {"name": "leader_follow_setup", "bot": wing.name, "command": f"follow {leader.name}"},
            {"name": "leader_group_setup", "bot": leader.name, "command": f"group {wing.name}"},
            {"name": "all_quest_progress", "bot": "all", "command": "quest progress"},
            {"name": "leader_newbie_chat", "bot": leader.name, "command": "newbie mudbot story smoke"},
            {"name": "all_story_again", "bot": "all", "command": "story"},
        ]

        results: list[ScenarioStepResult] = []
        for step in plan:
            results.extend(await self._execute_step(step["name"], step, strict=strict))

        failed = sum(1 for r in results if not r.ok)
        return MultiplayerRunResult(
            success=(failed == 0),
            scenario="story_party_smoke",
            total_steps=len(results),
            failed_steps=failed,
            details=results,
        )

    # ------------------------------------------------------------------
    # Step execution helpers
    # ------------------------------------------------------------------

    def _resolve_targets(self, target: str) -> list[SessionBot]:
        target_lower = target.lower()
        if target_lower == "all":
            return list(self._sessions)
        if target_lower == "leader":
            return [self._sessions[0]]
        if target_lower == "followers":
            return list(self._sessions[1:])

        # Name match.
        for session in self._sessions:
            if session.name.lower() == target_lower:
                return [session]

        raise ValueError(f"Unknown step target '{target}'")

    def _format_command(self, command: str) -> str:
        """Template helper for script steps with bot name placeholders."""
        tokens: dict[str, str] = {}
        if self._sessions:
            tokens["leader"] = self._sessions[0].name
        for idx, session in enumerate(self._sessions, start=1):
            tokens[f"bot{idx}"] = session.name
        return command.format(**tokens)

    async def _execute_step(
        self,
        step_name: str,
        step: dict[str, Any],
        strict: bool,
    ) -> list[ScenarioStepResult]:
        """Execute one step for one or more target bots."""
        target = str(step.get("bot") or "all")
        command = str(step.get("command") or "").strip()
        if not command:
            raise ValueError(f"Step '{step_name}' is missing a command")

        timeout = float(step.get("timeout", 6.0))
        expect = step.get("expect")
        delay = float(step.get("delay", 0.3))
        targets = self._resolve_targets(target)

        formatted = self._format_command(command)
        logger.info("[Multiplayer] %s -> %s: %s", step_name, target, formatted)

        results: list[ScenarioStepResult] = []
        for session in targets:
            response = await session.bot.send_and_read(formatted, timeout=timeout)
            excerpt = (response or "").replace("\n", " ").replace("\r", " ").strip()
            excerpt = excerpt[:180]

            ok = True
            if strict and expect:
                ok = str(expect).lower() in (response or "").lower()
            elif strict:
                ok = bool(response)

            results.append(
                ScenarioStepResult(
                    step_name=step_name,
                    bot_name=session.name,
                    command=formatted,
                    ok=ok,
                    response_excerpt=excerpt,
                )
            )

            if delay > 0:
                await asyncio.sleep(delay)

        return results
