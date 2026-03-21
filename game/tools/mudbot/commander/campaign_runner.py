"""
Campaign runner for full quest + story coverage verification.

Runs multiple quest bot profiles, optionally multiplayer scenarios, and emits
machine-readable + markdown reports for release verification gates.
"""

from __future__ import annotations

import asyncio
import json
import logging
import sqlite3
from dataclasses import asdict, dataclass, field
from datetime import datetime
from pathlib import Path

from ..bot.quest_bot import QuestBot
from ..config import BotConfig, CommanderConfig, ProgressionConfig, QuestConfig, _alpha_suffix
from .multiplayer_manager import MultiplayerCommander, MultiplayerRunResult

logger = logging.getLogger(__name__)


DEFAULT_CLASS_PROFILES = [
    "demon",
    "vampire",
    "werewolf",
    "mage",
    "ninja",
    "monk",
    "drow",
    "angel",
    "samurai",
    "lich",
    "shapeshifter",
    "tanarri",
    "undead_knight",
    "spiderdroid",
    "dirgesinger",
    "psion",
    "dragonkin",
    "artificer",
    "mechanist",
    "cultist",
    "chronomancer",
    "paradox",
    "shaman",
    "spiritlord",
    "siren",
    "mindflayer",
    "wyrm",
]


@dataclass
class CampaignProfile:
    """One quest-bot profile in the campaign matrix."""

    selfclass: str
    explevel: int


@dataclass
class QuestRunSummary:
    """Result of one quest bot campaign run."""

    bot_name: str
    selfclass: str
    explevel: int
    quest_state: str
    completed_ids: list[str] = field(default_factory=list)
    story_nodes: list[int] = field(default_factory=list)
    result_rows: list[dict] = field(default_factory=list)


@dataclass
class CampaignReport:
    """Aggregated campaign result payload."""

    generated_at: str
    host: str
    port: int
    quest_target_total: int
    quest_covered_total: int
    story_target_total: int
    story_covered_total: int
    success: bool
    missing_quests: list[str] = field(default_factory=list)
    missing_story_nodes: list[int] = field(default_factory=list)
    quest_runs: list[QuestRunSummary] = field(default_factory=list)
    multiplayer_runs: list[dict] = field(default_factory=list)


class QuestStoryCampaignRunner:
    """Execute campaign matrix and write coverage reports."""

    def __init__(
        self,
        host: str,
        port: int,
        password: str,
        prefix: str,
        delay: float,
        strict: bool,
        output_dir: Path,
        db_path: Path,
        max_quest_cycles: int,
        force_story: bool,
        enable_pvp: bool,
    ):
        self.host = host
        self.port = port
        self.password = password
        self.prefix = prefix
        self.delay = delay
        self.strict = strict
        self.max_quest_cycles = max_quest_cycles
        self.force_story = force_story
        self.enable_pvp = enable_pvp
        self.output_dir = self._resolve_path(output_dir)
        self.db_path = self._resolve_path(db_path)

    @staticmethod
    def _repo_root() -> Path:
        """Return repository root inferred from this module location."""
        return Path(__file__).resolve().parents[4]

    def _resolve_path(self, path: Path) -> Path:
        """Resolve paths from cwd first, then repository root fallback."""
        if path.is_absolute():
            return path

        cwd_candidate = Path.cwd() / path
        if cwd_candidate.exists():
            return cwd_candidate

        root_candidate = self._repo_root() / path
        return root_candidate

    def _load_target_quests(self) -> list[str]:
        """Load canonical quest IDs from quest.db for full coverage checks."""
        if not self.db_path.exists():
            raise FileNotFoundError(f"Quest DB not found: {self.db_path}")
        with sqlite3.connect(self.db_path) as conn:
            cur = conn.cursor()
            rows = cur.execute("SELECT id FROM quest_defs ORDER BY sort_order").fetchall()
        return [r[0] for r in rows]

    def _make_name(self, run_index: int) -> str:
        """Generate deterministic, valid bot names under the 12-char limit."""
        suffix = _alpha_suffix(run_index)
        max_prefix = max(1, 12 - len(suffix))
        return f"{self.prefix[:max_prefix]}{suffix}"

    async def _run_single_profile(self, profile: CampaignProfile, run_index: int) -> QuestRunSummary:
        """Run one quest bot profile and collect detailed telemetry."""
        name = self._make_name(run_index)
        config = BotConfig(
            name=name,
            password=self.password,
            host=self.host,
            port=self.port,
            sex="m",
            experience_level=profile.explevel,
            command_delay=self.delay,
        )
        quest_config = QuestConfig(
            mode="all",
            stop_after_quest="",
            max_quest_cycles=self.max_quest_cycles,
            selfclass=profile.selfclass,
            enable_story=True,
            max_story_node=16,
            force_story_progress=(self.force_story and profile.explevel >= 3),
            enable_pvp=self.enable_pvp,
        )
        prog_config = ProgressionConfig()

        bot = QuestBot(config, quest_config, prog_config)
        await bot.run()

        status = bot.get_status()
        report_data = bot.get_report_data()
        return QuestRunSummary(
            bot_name=name,
            selfclass=profile.selfclass,
            explevel=profile.explevel,
            quest_state=status.get("quest_state", "UNKNOWN"),
            completed_ids=report_data.get("completed_ids", []),
            story_nodes=report_data.get("story_nodes", []),
            result_rows=report_data.get("results", []),
        )

    async def _run_multiplayer(self, count: int, scenarios: list[str]) -> list[dict]:
        """Execute multiplayer smoke scenarios and return normalized records."""
        if count < 2 or not scenarios:
            return []

        config = CommanderConfig(
            host=self.host,
            port=self.port,
            bot_prefix=self.prefix[:9],
            bot_password=self.password,
            num_bots=count,
            stagger_delay=1.0,
            command_delay=self.delay,
            experience_level=1,
        )
        manager = MultiplayerCommander(config)

        records: list[dict] = []
        try:
            if not await manager.start():
                return [{"scenario": "startup", "success": False, "failed_steps": 1, "total_steps": 1}]

            for scenario in scenarios:
                result: MultiplayerRunResult = await manager.run_builtin(scenario, strict=self.strict)
                records.append(
                    {
                        "scenario": result.scenario,
                        "success": result.success,
                        "failed_steps": result.failed_steps,
                        "total_steps": result.total_steps,
                        "details": [asdict(d) for d in result.details],
                    }
                )
        finally:
            await manager.stop()

        return records

    def _build_markdown(self, report: CampaignReport) -> str:
        """Render a concise markdown report."""
        lines: list[str] = []
        lines.append("# Quest + Story Verification Report")
        lines.append("")
        lines.append(f"- Generated: {report.generated_at}")
        lines.append(f"- Target: {report.host}:{report.port}")
        lines.append(f"- Campaign success: {'YES' if report.success else 'NO'}")
        lines.append("")
        lines.append("## Coverage Summary")
        lines.append("")
        lines.append(f"- Quests covered: {report.quest_covered_total}/{report.quest_target_total}")
        lines.append(f"- Story nodes covered: {report.story_covered_total}/{report.story_target_total}")
        lines.append("")

        lines.append("## Quest Runs")
        lines.append("")
        for run in report.quest_runs:
            lines.append(
                f"- {run.bot_name} | class={run.selfclass} | explevel={run.explevel} | "
                f"state={run.quest_state} | completed={len(run.completed_ids)} | "
                f"story_nodes={','.join(str(n) for n in run.story_nodes) if run.story_nodes else '-'}"
            )

        lines.append("")
        lines.append("## Missing Coverage")
        lines.append("")
        if report.missing_quests:
            lines.append(f"- Missing quests ({len(report.missing_quests)}): {', '.join(report.missing_quests)}")
        else:
            lines.append("- Missing quests: none")

        if report.missing_story_nodes:
            lines.append(
                f"- Missing story nodes ({len(report.missing_story_nodes)}): "
                f"{', '.join(str(n) for n in report.missing_story_nodes)}"
            )
        else:
            lines.append("- Missing story nodes: none")

        lines.append("")
        lines.append("## Multiplayer Smoke")
        lines.append("")
        if report.multiplayer_runs:
            for rec in report.multiplayer_runs:
                lines.append(
                    f"- {rec['scenario']}: "
                    f"{'PASS' if rec['success'] else 'FAIL'} "
                    f"({rec['total_steps'] - rec['failed_steps']}/{rec['total_steps']} steps)"
                )
        else:
            lines.append("- Not executed")

        return "\n".join(lines) + "\n"

    async def run(
        self,
        profiles: list[CampaignProfile],
        multiplayer_count: int,
        multiplayer_scenarios: list[str],
    ) -> tuple[CampaignReport, Path, Path]:
        """Run full campaign and write JSON + markdown report files."""
        target_quests = self._load_target_quests()
        target_story_nodes = list(range(1, 17))

        quest_runs: list[QuestRunSummary] = []
        for idx, profile in enumerate(profiles, start=1):
            logger.info(
                "[Campaign] Quest run %d/%d: class=%s explevel=%d",
                idx,
                len(profiles),
                profile.selfclass,
                profile.explevel,
            )
            try:
                run_summary = await self._run_single_profile(profile, idx)
            except Exception:
                logger.exception(
                    "[Campaign] Quest run failed hard for class=%s explevel=%d",
                    profile.selfclass,
                    profile.explevel,
                )
                run_summary = QuestRunSummary(
                    bot_name=self._make_name(idx),
                    selfclass=profile.selfclass,
                    explevel=profile.explevel,
                    quest_state="FAILED",
                )
            quest_runs.append(run_summary)

        multiplayer_runs = await self._run_multiplayer(multiplayer_count, multiplayer_scenarios)

        covered_quests: set[str] = set()
        covered_story_nodes: set[int] = set()
        for run in quest_runs:
            covered_quests.update(run.completed_ids)
            covered_story_nodes.update(run.story_nodes)

        missing_quests = [qid for qid in target_quests if qid not in covered_quests]
        missing_story_nodes = [node for node in target_story_nodes if node not in covered_story_nodes]
        multiplayer_ok = all(rec.get("success", False) for rec in multiplayer_runs)
        runs_ok = all(run.quest_state == "COMPLETE" for run in quest_runs)

        report = CampaignReport(
            generated_at=datetime.utcnow().isoformat(timespec="seconds") + "Z",
            host=self.host,
            port=self.port,
            quest_target_total=len(target_quests),
            quest_covered_total=len(covered_quests),
            story_target_total=len(target_story_nodes),
            story_covered_total=len(covered_story_nodes),
            success=(not missing_quests and not missing_story_nodes and multiplayer_ok and runs_ok),
            missing_quests=missing_quests,
            missing_story_nodes=missing_story_nodes,
            quest_runs=quest_runs,
            multiplayer_runs=multiplayer_runs,
        )

        self.output_dir.mkdir(parents=True, exist_ok=True)
        stamp = datetime.utcnow().strftime("%Y%m%d_%H%M%S")
        json_path = self.output_dir / f"quest_story_campaign_{stamp}.json"
        md_path = self.output_dir / f"quest_story_campaign_{stamp}.md"

        json_payload = {
            "generated_at": report.generated_at,
            "host": report.host,
            "port": report.port,
            "quest_target_total": report.quest_target_total,
            "quest_covered_total": report.quest_covered_total,
            "story_target_total": report.story_target_total,
            "story_covered_total": report.story_covered_total,
            "success": report.success,
            "missing_quests": report.missing_quests,
            "missing_story_nodes": report.missing_story_nodes,
            "quest_runs": [asdict(r) for r in report.quest_runs],
            "multiplayer_runs": report.multiplayer_runs,
        }

        with json_path.open("w", encoding="utf-8") as f:
            json.dump(json_payload, f, indent=2)

        with md_path.open("w", encoding="utf-8") as f:
            f.write(self._build_markdown(report))

        return report, json_path, md_path
