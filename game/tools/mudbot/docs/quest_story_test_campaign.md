> **Superseded**: Campaign strategy has moved to [coverage_matrix.md](coverage_matrix.md) and [roadmap.md](roadmap.md). This file is retained for git history.

# Quest + Story Full-Coverage Campaign Plan

## Goal

Use mudbot to validate that the quest and story systems guide both new and veteran players through the original Dystopia feature set, including multiplayer-only interactions.

## Success Criteria

- New-player path (experience level 1) reaches and completes tutorial/main onboarding checkpoints without manual intervention.
- Veteran path (experience level 3) can still access and complete critical quest/story progression and class systems.
- Multiplayer gating paths are executable and observable: follow, group, group chat, and PK-triggered quest hooks.
- Story progression is validated across hubs with tracked evidence from command outputs.
- Coverage report maps each tested step to a core game system.

## Coverage Matrix

| Feature/System | Validation Method | Bot Mode |
|---|---|---|
| Login + character creation | MudBot login state machine | single + multiplayer |
| Tutorial command literacy | quest objectives + scripted command replay | single + multiplayer |
| Quest accept/progress/complete | quest list/progress/complete/history parsing | single + multiplayer |
| Story state visibility | story + storyadmin command parsing | single + multiplayer |
| Movement and room navigation | look/exits/pathing/recall | single |
| Group mechanics | follow/group/gtell sequence | multiplayer |
| PvP mechanics | kill <player> duel smoke | multiplayer |
| Combat and progression | kill loops + train + stance checks | single |
| Class progression hooks | selfclass + class train/research handlers | single |
| Area exposure | story hubs + visit-area quest objectives | single |

## Execution Lanes

1. Single-bot progression lane:
   - Continue using `python -m mudbot quest` for deep objective handling.
   - Run two profiles per release: newbie (`--explevel 1`) and veteran (`--explevel 3`).

2. Multiplayer scenario lane:
   - Use new `python -m mudbot multiplayer` command for coordinated flows.
   - Start with built-in scenarios:
     - `follow_group_tutorial`
     - `pk_duel_smoke`
     - `story_party_smoke`
   - Add campaign scripts via JSON (`--script`) for release-specific regression packs.

3. Campaign evidence lane:
   - Persist logs per run with scenario, bot roster, and step result summary.
   - Attach coverage checklist to each release cycle.

## Recommended Release Gate

1. Run single-bot quest progression for newbie profile to target stop quest.
2. Run single-bot quest progression for veteran profile with story enabled.
3. Run multiplayer follow/group tutorial scenario.
4. Run multiplayer PK smoke scenario (on isolated test world).
5. Run custom script scenario for the current release quest/story deltas.
6. Fail the gate if any required lane has regressions.

## Immediate Backlog (Next Iterations)

- Add explicit expectation matchers to scenario scripts (`expect_any`, `expect_all`, regex support).
- Add timeline-style run report artifact (JSON + markdown summary).
- Add deterministic synchronization primitives (wait-for-prompt / barrier steps) for brittle multiplayer timing.
- Add story node checkpoint assertions (`min_node`, `max_node`) per bot role.
- Add quest coverage percent output from `quest history` against target ID set.

## Usage Examples

Built-in follow/group smoke:

```bash
python -m mudbot multiplayer --host localhost --port 8888 --count 2 --prefix Team --password testpass --scenario follow_group_tutorial
```

Built-in PK smoke:

```bash
python -m mudbot multiplayer --host localhost --port 8888 --count 2 --prefix Duel --password testpass --scenario pk_duel_smoke
```

Scripted campaign:

```bash
python -m mudbot multiplayer --host localhost --port 8888 --count 3 --prefix Squad --password testpass --script game/tools/mudbot/config/multiplayer_story.json
```
