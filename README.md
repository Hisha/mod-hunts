# mod-hunts

Standalone AzerothCore WotLK Hunt system. No other custom module is required.

## Features
- Repeatable normal Hunts from level 10 through 80.
- Elite Hunts unlock after 10 completed Hunts and may be completed once per character per day.
- Levels 10-79 remain group-friendly; max-level Elite final completion is solo.
- Huntmasters in capital cities and guard-direction integration.
- Zone tracking, ambushes, authored final locations, map POI refresh, activation crystal and final prey.
- Level-aware final-location selection and in-game authoring/report/export tools.
- Spec-aware existing-item rewards.
- Data-driven prey abilities including level gates, health triggers, melee requirements, once-per-encounter and aura checks.
- Self-contained dynamic creature-template materialization for Hunt prey.

## Configuration
Copy `conf/mod_hunts.conf.dist` through the normal AzerothCore module configuration process. Settings use only the `Hunts.*` namespace.

## Database
This first standalone development release intentionally uses a clean canonical schema, not the old `lw_hunt_*` names. The module owns:
- world: `hunt_*` and `hunt_creature_template*` tables
- characters: `hunt_runtime`, `hunt_stats`

For a clean development conversion from the Living World Hunt subsystem, remove the old Hunt-only tables after confirming the new module is working. Do not remove `lw_creature_template*` if Living World still uses them for invasions/travelers.

Load world base SQL first, character base SQL second, then the Hunt prebuilt SQL files in numeric order.

## Commands
Standalone commands are rooted at `.hunt` instead of `.lw hunt`, for example:
- `.hunt status`
- `.hunt progress <amount>`
- `.hunt ambush`
- `.hunt final`
- `.hunt abandon`
- `.hunt set final point`
- `.hunt set final list`
- `.hunt set final needs [zone]`
- `.hunt set final export [zone]`
- `.hunt set final levels <id> <min> <max>`
- `.hunt set final levels <id> auto`
- `.hunt set final goto <id>`
- `.hunt set final delete <id>`

## 1.0.0-dev fork point
Forked from the working Living World 0.7.0-dev Hunt implementation after the first successful Elite Hunt test. Normal Hunt tuning/content and Elite Hunt behavior are preserved while removing all runtime/module dependency on mod-living-world.
