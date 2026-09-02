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

### Elite Hunt crowd control

Elite Hunt prey are class-style opponents rather than dungeon bosses. Elite-derived creature templates therefore do not inherit blanket `CreatureImmunitiesId` crowd-control immunity from their visual/base creature.

Elite prey currently use encounter-local stun diminishing returns: the first stun has full duration, the second is 50%, the third is 25%, and further stuns are immune until 15 seconds after the previous stun expires. This is implemented as Elite Hunt combat behavior and is intended to expand to additional crowd-control categories as more Elite archetypes are added.


## Elite Hunt progression (1.0.3-dev)

**Huntmaster's Seals are level-80 progression only.** Elite Hunts completed below level 80 award XP, gold, and strong level-appropriate rare equipment, but never raid-tier epic gear and never a Huntmaster's Seal. Reaching level 80 does not retroactively award Seals for earlier Elite Hunts.

At level 80, each successful Elite Hunt awards one virtual **Huntmaster's Seal**. Seals are stored per character in `hunt_stats`, consume no bag space, and require no client/DBC patch. The Huntmaster hunting record displays the Seal balance only for level-80 characters.

At level 80, the immediate Elite Hunt equipment reward is capped at item level 200 epic weapon/armor appropriate for the active spec (entry-level Wrath 10-player raid power). Higher-tier, player-selected main-spec or off-spec equipment is intended to be purchased with accumulated Huntmaster's Seals rather than awarded by unrestricted random rolls.


## 1.0.3-dev - Admin tuning layer

Balance-sensitive server policy is now exposed through `mod_hunts.conf`: Elite unlock/daily limit, global Elite health/damage/armor, Elite XP/gold, Seal level/count, endgame reward level/item-level band, tracking gain, and group/shared-final credit radii. Per-prey identity and ability tuning remains data-driven in SQL; global config multipliers stack on top.
