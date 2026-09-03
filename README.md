# mod-hunts

Standalone Hunt gameplay module for AzerothCore WotLK 3.3.5a. No other custom server module is required and no client patch/MPQ is used.

## Features
- Repeatable normal Hunts from level 10 through 80.
- Elite Hunts unlock after configurable normal-Hunt progression and use a per-character daily assignment limit.
- Huntmasters in major capitals/hubs with stock guard-direction integration.
- Zone tracking, required ambush encounters, authored final locations, map POI guidance, activation crystal, and final prey encounter.
- Group-friendly normal Hunt progression with configurable nearby credit.
- Data-driven normal and Elite prey, including class-style Elite combat brains and authored abilities.
- Level-aware and spec-aware equipment rewards.
- Level-80 Huntmaster's Seal progression with configurable reward tiers.
- Full server-side Seal-store fallback for stock clients.
- Optional **HuntsUI** companion addon using Blizzard's real MerchantFrame, native item rows/tooltips/paging, spec switching, Seal prices, and Seal balance.
- Self-contained dynamic creature-template materialization for Hunt prey.
- In-game Hunt authoring, validation, final-location coverage reporting, and export tools.

## Requirements
- AzerothCore WotLK / WoW 3.3.5a.
- MySQL/MariaDB as required by AzerothCore.
- No dependency on `mod-living-world`.
- No client patch is required.
- HuntsUI is optional. HuntsUI 1.0.1 or newer is recommended for the graphical Huntmaster Seal store.

## Installation
1. Place `mod-hunts` in your AzerothCore `modules` directory.
2. Apply the SQL under `data/sql/db-world/base` to the world database.
3. Apply the SQL under `data/sql/db-characters/base` to the characters database.
4. Apply `data/sql/db-world/prebuilt` in numeric order.
5. Copy `conf/mod_hunts.conf.dist` through the normal AzerothCore module configuration process and adjust the `Hunts.*` settings as desired.
6. Re-run CMake/configuration if required by your AzerothCore build, compile, and restart the realm.

The canonical base/prebuilt SQL represents the complete current 1.0-era schema/content. Very early development installs that predate the standalone repository should be treated as development conversions: back up the databases and reconcile/reload the canonical `hunt_*` schema/content rather than relying on old `lw_hunt_*` tables.

## Huntmaster's Seal store
Huntmaster's Seals are virtual per-character progression currency stored by the server; they consume no bag space and require no DBC/client patch.

Without HuntsUI, selecting **Huntmaster's Seal rewards** opens the complete server-side gossip store with specialization, tier, slot, item, and confirmation steps.

With HuntsUI installed, the addon announces itself for the login session. The same gossip option opens Blizzard's native MerchantFrame. `mod-hunts` generates the catalog per character/spec, validates every purchase, deducts Seals, creates the item, and enforces soulbinding. HuntsUI is presentation/interaction only and cannot grant items or choose prices.

## Configuration
Copy `conf/mod_hunts.conf.dist` through the normal AzerothCore module configuration process. All settings use the `Hunts.*` namespace. `Hunts.Debug` defaults to `0`; enable it only when using the in-game authoring/debug commands.

Important groups include:
- `Hunts.MinimumLevel`, `Hunts.SearchScope`, and `Hunts.XPMultiplier`
- tracking/group-credit settings
- Elite unlock, daily limit, difficulty, rewards, and ranged-combat tuning
- Huntmaster Seal reward level/count
- four configurable Seal-store item-level/cost tiers

## Commands
Commands are rooted at `.hunt`:
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

Authoring commands require `Hunts.Debug = 1`.

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

## 1.0.13-dev - Elite assignment lock and no-upgrade Seal fallback

Elite Hunt daily limits are now consumed when the hunter **accepts** an Elite assignment, not only when the Elite is completed. `hunt_stats` tracks daily Elite assignments separately from completion statistics. Abandoning an Elite Hunt still removes the active runtime, but the day's assignment remains consumed, preventing repeated abandon/reaccept rerolls for an easier or preferred prey. Normal Hunts remain freely abandonable. Existing installations must run `data/sql/db-characters/updates/1.0.13_elite_assignment_lock_upgrade.sql`; fresh installs already have the canonical columns.

Level-cap Elite random gear now applies a stricter definition of an upgrade when `Hunts.Elite.RewardRequireUpgrade = 1`: a candidate must exceed both the item level and the spec-weighted power of the currently equipped matching slot. Rings, trinkets, and one-hand weapons compare against the weaker applicable slot. This prevents the fixed item-level-200 reward pool from repeatedly awarding a nominally scored piece after every relevant slot has already progressed beyond item level 200.

When no legitimate level-cap Elite equipment upgrade remains, the item reward is skipped and the hunter receives configurable bonus Huntmaster's Seals instead. `Hunts.Elite.NoUpgradeBonusSeals` defaults to `1` and is added to the normal `Hunts.Elite.SealsPerCompletion` award. Bag-full failures do not grant the fallback bonus because a valid upgrade existed; the player simply lacked inventory space.

## 1.0.12-dev - Terrain-safe ranged movement and Death and Decay Grip

- Corrected **The Dusk Confessor** presentation by moving the dynamic template to a normal-rank priest shell while retaining the explicit Elite rank override.
- Ranged Elite retreat/re-center movement now validates terrain height and MMAP reachability before committing to a destination. Straight-back retreats that would run into walls, cliffs, or broken navigation try alternate escape angles; if no safe point exists, the prey holds position instead of issuing an evade-prone movement order.
- **The Gravebound** now remembers the center and active window of its most recent Death and Decay. Death Grip becomes immediately eligible when the hunter crosses the edge of that active hazard, so attempting to escape the red circle can trigger the intended pull-back interaction even when the hunter is still relatively close. Ordinary range-based Death Grip remains available as a fallback.

## 1.0.11-dev - Shadow Priest and Death Knight Elite archetypes

Added two more class-style Elite Hunt prey to the canonical `prebuilt/911_elite_prey.sql` roster:

- **The Dusk Confessor** - Shadow Priest archetype. Opens in Shadowform, maintains Shadow Word: Pain and Vampiric Touch, channels Mind Flay, bursts with Mind Blast, uses Psychic Scream through the existing encounter-local fear diminishing returns, and uses a once-per-final low-health Dispersion defensive pause.
- **The Gravebound** - Death Knight archetype. Maintains disease pressure with Icy Touch and Plague Strike, uses Chains of Ice for control, Death Grip tactically at range, Death Strike for low-health sustain, Mind Freeze only while the hunter is casting, and a final low-health Raise Dead escalation.

Death and Decay is implemented as true ground-targeted area denial at the hunter's current position rather than as an invisible unit-targeted damage effect. The stock spell therefore provides its normal visible ground circle, giving the hunter a positional warning and an opportunity to move while Grip and Chains make that movement tactically meaningful.

Psychic Scream shares the same Elite fear DR framework proven by The Ashen Pact: full Hunt-controlled duration, then 50%, then 25%, then immunity until the DR reset window expires. This keeps class identity without recreating the original Warlock fear-lock problem.

The Elite prebuilt cleanup header now covers the complete current Elite roster/templates so reapplying `911_elite_prey.sql` remains idempotent as new archetypes are added.

## 1.0.14-dev - The Farstrider Hunter Elite

Adds Elite prey **The Farstrider**, a Marksmanship-style Hunter encounter built around the existing terrain-safe ranged combat brain. The Farstrider maintains approximately 24 yards with Hunter's Mark, Serpent Sting, Steady Shot, Aimed Shot and Concussive Shot, uses Disengage under pressure, adds Explosive Trap during the final encounter, and uses Deterrence as a low-health defensive window.

The Farstrider also introduces the first Hunt-owned combat companion. Because an NPC creature cannot truthfully use the player's stable/pet subsystem, the Hunt combat brain intercepts the authored Call Pet action and summons **Nightfang**, a temporary hostile wolf from its own dynamic Hunt creature template. Nightfang scales its level to the prey, receives encounter-relative health, attacks the hunter independently, does not count as ordinary tracking progress if killed, and is explicitly removed when the ambush/final prey ends or disappears. No client patch or schema change is required.

## 1.0.15-dev - Farstrider hostile-shell correction

Fixed the first live Farstrider test where Nightfang attacked correctly but The Farstrider inherited non-attackable/friendly runtime flags from the stock blood-elf ranger presentation shell. Hunt prey are now normalized after spawn to faction 14, aggressive react state, and attackable/selectable player-combat flags, so authored class-looking shells cannot silently turn the actual Hunt target into a friendly NPC.

Hunter companion death bookkeeping also now erases Nightfang's encounter GUID safely without touching the prey runtime or required-ambush gate. Killing Nightfang is optional encounter progress only; the ambush remains pending until The Farstrider himself reaches the configured escape-health threshold.

## 1.0.16-dev - Optional HuntsUI Seal vendor

Adds the optional **HuntsUI** companion addon for Huntmaster's Seal rewards while preserving the stock gossip store as the no-addon fallback. The Huntmaster's normal Blizzard gossip remains the front door: Hunt/Elite Hunt request, status/turn-in/abandon, rewards, and hunting statistics remain visible there. The Seal reward entry now uses the normal vendor/bag gossip icon.

When HuntsUI has announced itself for the current login session, selecting the existing Seal reward gossip option closes gossip and opens a token-vendor-style reward frame. The addon provides specialization, reward tier, and equipment-slot selectors, paged item rows with normal item tooltips, Seal prices, affordability dimming, and the current Huntmaster's Seal balance. Clients without HuntsUI continue through the existing specialization/tier/slot/item gossip menus unchanged.

The addon uses the `HUNTS` self-whisper addon-message protocol. The server remains authoritative: the graphical client requests catalog pages and purchases, while `mod-hunts` reuses `BuildSealStoreItems` and `PurchaseSealStoreItem` for eligibility, price, Seal deduction, inventory checks, and soulbinding. Addon store requests are only accepted while the player remains near the Huntmaster whose reward gossip option was selected, so installing HuntsUI does not create a remote Seal vendor. No client patch or database/schema change is required.


## Development history

### 1.0.24-dev - Release cleanup and stock-client fallback
- Restored the intended optional-addon behavior for Huntmaster Seal rewards. HuntsUI sessions use the native dynamic MerchantFrame catalog; clients without HuntsUI remain in the complete server-side gossip store.
- Removed the four static Wrath gear rows used only during the MerchantFrame proof-of-concept. Huntmasters retain the vendor NPC flag, while their graphical inventory is generated dynamically per character/spec.
- Removed the obsolete `PING/PONG` transport diagnostic and the superseded custom-window `LIST` / `BUY` addon protocol. The shipping graphical store uses `HELLO`, `OPEN`, `CATALOG`, and `BUYITEM`.
- Changed the distributed `Hunts.Debug` default from `1` to production-safe `0`.
- Refreshed the repository README around the current standalone module, installation flow, optional HuntsUI architecture, Seal-store behavior, and canonical SQL policy.
- HuntsUI 1.0.1 adds the login `HELLO` announcement used to distinguish graphical-store clients from stock/no-addon clients.

### 1.0.23-dev
- Added explicit Seal-store weapon/off-hand/ranged/relic filtering for all ten Wrath classes and every talent specialization.
- Warriors now distinguish Arms, Fury, and Protection weapon/off-hand rules; Hunters receive physical ranged weapons; Rogues receive appropriate one-hand plus physical ranged/thrown choices.
- Priest, Mage, and Warlock catalogs are restricted to caster weapons, wands, and caster off-hands.
- Death Knights receive DK melee weapon families and Sigils only; Shamans separate caster/shield and Enhancement dual-wield choices and receive Totems only.
- Druids separate Balance/Restoration caster choices from Feral physical two-handers and receive Idols only.
- Paladin weapon/Libram restrictions from 1.0.22 remain intact.
- Filtering remains entirely server-side; HuntsUI 1.0 requires no changes.

### 1.0.22-dev
- Tightened Paladin Seal-store equipment filtering.
- Holy and Protection now receive only valid one-handed Paladin weapon subclasses (axes, maces, and swords); daggers, wands, and unrelated weapon types are excluded.
- Retribution now explicitly limits weapon rewards to valid two-handed Paladin weapon types (axes, maces, swords, and polearms).
- Paladin `Ranged / Relics` rewards are now Librams only, so ranged weapons can no longer displace Librams from the dynamic native merchant catalog.
- Paladin held-in-off-hand frills and off-hand weapons are excluded; shields remain available where appropriate.

### 1.0.21-dev
- Replaced the four-item MerchantFrame proof catalog with a per-character, per-spec dynamic Seal reward catalog.
- The server reuses `BuildSealStoreItems()` and selects the highest-ranked eligible item for each slot/tier combination.
- Catalog order is slot-first, then 5/10/20/30-Seal tiers: Head, Shoulders, Chest, Hands, Legs, Wrists, Waist, Feet, Back, Neck, Rings, Trinkets, Weapons, Off-hand/Shields, and Ranged/Relics.
- Added `CATALOG` and `BUYITEM` addon protocol operations. The server remains authoritative for catalog membership, spec/tier eligibility, Seal balance, bag space, deduction, item creation, and soulbinding.
- The native merchant packet's price field carries the virtual Seal cost so HuntsUI can render correct configured prices without maintaining a second price table.

### 1.0.20-dev
- Historical MerchantFrame proof-of-concept: switched HuntsUI rewards to AzerothCore's real merchant pipeline and Blizzard's stock `MerchantFrame` using four temporary stock Wrath gear rows. Those rows were removed during 1.0.24 release cleanup after the dynamic catalog/purchase path was validated.


### 1.0.19-dev
- Added only the HuntsUI transport diagnostic: `PING` returns `PONG` through the existing `HUNTS` addon-message path.
- No Seal-store behavior changes in this step.



### 1.0.18-dev
- Reworked HuntsUI store opening so it no longer depends on the client completing a handshake before the Rewards gossip selection.
- Selecting Huntmaster's Seal rewards now always prepares the normal server-side gossip store as the stock-client fallback and simultaneously pushes an `OPEN` addon packet directly to the player.
- HuntsUI clients close the fallback gossip page when that `OPEN` packet arrives; clients without HuntsUI ignore the addon packet and remain in the fully functional gossip store.
- The existing self-whisper request/response transport remains in place for catalog paging and purchases.



### 1.0.17-dev
- Fixed the HuntsUI Seal-store open handshake. The Huntmaster is now remembered as soon as the Rewards gossip option is selected, even when the addon's login `HELLO` has not completed yet.
- An addon's `OPEN` request now acts as a valid late handshake and closes the temporary gossip fallback before returning the graphical Seal catalog.
- Players without HuntsUI still receive the existing server-side specialization/tier/slot gossip store.
