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

## 1.0.4-dev - Huntmaster's Seal store and upgrade-aware Elite rewards

Level-80 Elite Hunts now feed a two-layer progression loop. The immediate Elite reward remains capped by the configured endgame item-level band, while each qualifying completion awards virtual Huntmaster's Seals. Huntmasters provide a fully server-authoritative Seal store with spec selection, tiered costs, confirmation before purchase, Seal deduction, and item delivery. The store deliberately does not depend on a custom client currency or DBC patch; an optional custom UI can be added later without changing server authority.

Elite random rewards are upgrade-aware. The reward selector evaluates the character's currently equipped gear and strongly favors slots where an eligible Hunt reward is a meaningful upgrade. Slots already carrying gear stronger than the configured Elite reward pool are excluded instead of repeatedly producing useless items (for example, a heroic ICC weapon should prevent an item-level-200 weapon reward from being selected).

## 1.0.5-dev - Elite archetypes: The Winterborn and The Headsman

Added two new class-style Elite Hunt prey to validate opposite combat styles:

- **The Winterborn** - Frost Mage archetype. Uses a ranged/kiting combat style with Frostbolt, Frost Nova, Ice Barrier, Cone of Cold, and a low-health Ice Block in the final encounter.
- **The Headsman** - Arms Warrior archetype. Uses melee pressure with Charge, Hamstring, Mortal Strike, Intimidating Shout, and Execute when the hunter is low on health.

The prey ability system now supports a hunter/victim-health threshold in addition to the existing prey-health threshold, allowing abilities such as Execute to react to the player's health. Prey definitions also support a data-driven combat style and preferred range so ranged Elite archetypes can maintain distance without prey-specific C++ branches.

### SQL maintenance convention

Canonical base SQL contains the complete current schema. Prebuilt content SQL contains only content/data and must not alter table structure. Schema changes for an already-running development install belong in clearly named files under `data/sql/db-world/updates/`. Elite prey content is kept together in `prebuilt/911_elite_prey.sql`; do not create one numbered prebuilt file per Elite archetype unless the content truly belongs to a separate subsystem.


## 1.0.6-dev - Elite combat brains: kiting and melee pressure

The Winterborn now has an actual ranged combat brain rather than only a Frost Mage spell list. When Frost Nova or a movement slow controls the hunter, the ranged archetype uses that control window to retreat toward its authored preferred range. At panic distance it can use Blink as a deliberate escape tool; Blink faces away from the hunter first and defaults to the Wrath 15-second cooldown. After defensive pauses such as Ice Block, movement is reevaluated and ranged spacing resumes instead of standing in melee. Global panic range, retreat percentage, Blink cooldown, and reaction timing are configurable under `Hunts.Elite.Ranged.*`.

The Headsman now treats distance as a tactical problem. Charge is reserved for legal Charge range instead of being wasted at point-blank or excessive distance, while normal chase remains the fallback. Warrior techniques are executed through the Hunt AI rather than depending on creature-shell rage/stance state, making Mortal Strike, Hamstring, Rend, Whirlwind, Intimidating Shout, and Execute reliable. Hamstring and Rend are aura-aware so they are maintained instead of blindly spammed.

## 1.0.7-dev - Ranged encounter leash

Ranged Elite prey now treat their summon/home position as an encounter arena rather than endlessly kiting away from it. Retreat destinations are clamped to a configurable soft radius (`Hunts.Elite.Ranged.ArenaRadius`, default 35 yards), and a ranged prey approaching the boundary prioritizes re-centering while remaining in combat. This prevents repeated Frost Nova/Cone of Cold retreats from pushing The Winterborn far enough from its spawn point to trigger AzerothCore's normal evade/reset heal.

Blink uses the same arena awareness: it still prefers to face away from the hunter, but if the projected Blink would cross the encounter boundary it faces back toward the arena instead. This keeps the Frost Mage escape behavior without letting the prey evade-bug itself.


## 1.0.8-dev - Required ambush completion, bound Elite progression, and two new archetypes

Ambushes are now persistent required Hunt stages rather than spawn-time milestones. Crossing an ambush threshold writes an `ambush_pending` flag to the character runtime and pauses further tracking progress. The ambush is only counted after the prey is actually driven to its configured escape-health threshold. If the prey evades/despawns, the player logs out, or the server restarts mid-ambush, the pending encounter remains and respawns when the hunter is back in the assigned hunt zone. A restart therefore cannot turn a 48% tracking state into a free skipped ambush. Existing installations must run `data/sql/db-characters/updates/1.0.8_ambush_completion_gate_upgrade.sql`; fresh installs already have the column in the canonical character base schema.

Binding policy is now explicit. Normal Hunt item rewards preserve the original Blizzard item template's binding behavior. Level-cap Elite immediate equipment rewards are forcibly soulbound when awarded, and all Huntmaster's Seal-store purchases are forcibly soulbound, preventing the personal Elite progression loop from becoming a source of tradable raid-level gear.

Two additional class-style Elite prey join the roster in the existing canonical `prebuilt/911_elite_prey.sql` file:

- **The Veiled Knife** - Rogue archetype using fast melee pressure, Rupture, Gouge, Evasion, Kidney Shot, and low-health Eviscerate burst. Rogue resource/combo-driven techniques are triggered by the Hunt AI so a creature shell does not depend on player-only energy/combo-point state.
- **The Ashen Pact** - Warlock archetype maintaining ranged pressure with Shadow Bolt, Corruption, Curse of Agony, Fear, Immolate, Drain Life, and a low-health Death Coil. It shares the reusable ranged-spacing brain but has no Frost-Mage Blink/root behavior.


## 1.0.9-dev - Rogue signature combat and Fear control tuning

**The Veiled Knife** now has a Rogue-specific combat brain rather than only a melee ability list. It applies Deadly and Crippling Poison pressure and gains a true Vanish/reposition/reopen cycle. Vanish remains inside the active Hunt encounter, moves the prey behind the hunter, and reopens with Ambush (or Cheap Shot when range prevents Ambush) without clearing the Hunt or triggering an evade reset.

**The Ashen Pact** keeps Fear as a signature Warlock tool but no longer chains full-duration fears. Hunt-owned Fear uses encounter-local diminishing returns of 6 seconds, 3 seconds, 1.5 seconds, then temporary immunity, resetting 15 seconds after the last Fear expires. Fear's authored cooldown is also widened to 24-30 seconds. A successful Fear pushes Death Coil back until at least five seconds after Fear ends so the Warlock cannot turn the two abilities into one long loss-of-control chain.

## 1.0.10-dev - Wildclaw and Stormcaller Elite archetypes

Two more class-style Elite prey join the canonical `prebuilt/911_elite_prey.sql` roster:

- **The Wildclaw** - Feral Druid archetype with a Hunt-owned two-phase combat brain. It opens in Cat Form with Pounce, Rake, and Mangle pressure, then deterministically shifts to Bear Form at 45% prey health for Bash, Bear Mangle, and a once-per-final low-health Frenzied Regeneration. The form transition is state-driven rather than left to random ability timing.
- **The Stormcaller** - Enhancement Shaman archetype using Lightning Shield, Stormstrike, Flame Shock, Earth Shock, Earthbind Totem, final-only Magma Totem, and a once-per-final Feral Spirit below 40%. The authored totem package is intentionally limited to Earthbind plus Magma rather than dropping a full four-totem field.

Druid and Shaman player-resource techniques are triggered by Hunt AI just like the existing Warrior/Rogue techniques, avoiding reliance on player-only rage, energy, mana, stance, or combo-point behavior in creature shells. No schema change is required for 1.0.10.

## 1.0.11-dev - Shadow Priest and Death Knight Elite archetypes

Added two more class-style Elite Hunt prey to the canonical `prebuilt/911_elite_prey.sql` roster:

- **The Dusk Confessor** - Shadow Priest archetype. Opens in Shadowform, maintains Shadow Word: Pain and Vampiric Touch, channels Mind Flay, bursts with Mind Blast, uses Psychic Scream through the existing encounter-local fear diminishing returns, and uses a once-per-final low-health Dispersion defensive pause.
- **The Gravebound** - Death Knight archetype. Maintains disease pressure with Icy Touch and Plague Strike, uses Chains of Ice for control, Death Grip tactically at range, Death Strike for low-health sustain, Mind Freeze only while the hunter is casting, and a final low-health Raise Dead escalation.

Death and Decay is implemented as true ground-targeted area denial at the hunter's current position rather than as an invisible unit-targeted damage effect. The stock spell therefore provides its normal visible ground circle, giving the hunter a positional warning and an opportunity to move while Grip and Chains make that movement tactically meaningful.

Psychic Scream shares the same Elite fear DR framework proven by The Ashen Pact: full Hunt-controlled duration, then 50%, then 25%, then immunity until the DR reset window expires. This keeps class identity without recreating the original Warlock fear-lock problem.

The Elite prebuilt cleanup header now covers the complete current Elite roster/templates so reapplying `911_elite_prey.sql` remains idempotent as new archetypes are added.