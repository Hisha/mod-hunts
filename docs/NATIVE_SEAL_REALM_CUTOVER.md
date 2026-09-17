# Native Seal realm cutover and one-item proof — EPF 4.3.0

Build 000011 remains Eitrigg's current ACTIVE baseline. **Do not roll it back.** This package changes source/module content; it does not contain a generated realm MPQ, change an Eitrigg database, apply a build, or activate content.

## Installation lifecycle

There is one currency authority for the realm. A realm that has never activated the new native Hunt feature uses the existing virtual balance. The presence of Content Manager, installed packages, retained allocations, a STAGED build, or Build 000011's orphaned cost does not switch the economy.

The new EPF adds the logical `mod-hunts/seal-proof-vendor` relationship. Its successful server apply and explicit activation/publication establish the native activation signal. The required worldserver restart loads the resources, validates this signal once, and performs the realm cutover **before player logins**. No live hot-switch is attempted: use a maintenance window for apply, activation, client deployment and restart.

On this startup, Hunts snapshots every existing `hunt_stats` balance, including offline characters, and delivers physical Seals through persisted mail. After the complete snapshot has been delivered, the durable realm state becomes `NATIVE`; physical inventory is authoritative. An installation with no legacy balances creates no migration items. New characters use the same realm economy, without any login migration or character currency-mode flag.

There is no client attestation, per-login approval, capability polling, shadow balance, or automatic native/legacy switching. Compatible client patch deployment is an administrative activation prerequisite. The server flag cannot establish which files an individual client loaded; mixed/unpatched clients are unsupported during this proof. Portalkeeper and HuntsUI are unchanged.

If required resources are unavailable on a later startup after migration has begun, Hunts reports `PAUSED`. The saved virtual values never become spendable again. An intentional downgrade is a separate administrative migration, outside this release.

## Optional public integration

Content Manager owns `src/api/ContentCapabilityApiV1.h`; Hunts vendors the identical header. Its WorldScript also implements the public `ContentCapabilitiesV1::Provider` interface. Hunts discovers that interface through the core's existing WorldScript registry and RTTI once during startup. It makes no calls to Content Manager implementation symbols and does not query Content Manager SQL, hashes, build numbers or filesystem paths.

`Resolve(package, vendorSymbol, resources, vendor, reason)` returns Inactive, Ready or Invalid. The provider checks the ACTIVE build, artifact hashes, canonical server/parity data, allocations, accepted baseline history, APPLIED ownership/current rows and loaded core item/currency/cost/vendor data. A retained lease alone is insufficient. The first API supports `item.id` and `item-extended-cost.id`; it exposes only their resolved values and the resolved vendor relationship.

Hunts requests `seal/item.id`, `seal-cost-5/item-extended-cost.id`, and `seal-proof-vendor`. Currency registration is additionally checked internally because the Seal must actually be loaded as the native token. The exact five-Seal cost, zero additional costs, and proof merchandise's one-item buy count, zero gold price and BOP binding are checked. A valid installation logs `currency mode = NATIVE` only after realm migration succeeds. Never-activated standalone installations log `LEGACY`; unsafe/unreadable native state logs `PAUSED` with a reason.

The existing leases remain untouched: observed Eitrigg values 56807, 4, 5 and 3 are acceptance expectations only. Gameplay and EPF cost relationships do not author those allocations. No new allocation namespace or baseline fingerprint is required by this extension.

## Migration and delivery durability

Install the new character SQL `004_hunt_currency.sql`. It adds:

- `hunt_currency_realm`: singleton realm migration version 1, MIGRATING/NATIVE state, resolved Seal identity, immutable snapshot count/total, completion time and native completion receipt fields.
- `hunt_currency_delivery`: one accounting row per snapshotted character, with original amount and amount durably issued. These are delivery receipts, **not individual currency modes**.

The character database must be dedicated to this realm, as with the existing Hunt data. Required tables are checked for InnoDB. The first snapshot and realm latch commit in one transaction; original virtual balances are neither zeroed nor deleted. Zero balances require no item delivery. A missing character with a positive snapshot balance stops migration for administrative investigation rather than dropping its balance.

Each migration batch creates at most 120 stacks, split into normal 12-attachment mail messages. Core `Item::CreateItem` and `Item::SaveToDB`, the core prepared mail statements, and the exact delivered-amount receipt participate in the **same character transaction**. SQL guards abort the entire transaction if the expected receipt/state changed. A failed preparation or rolled-back transaction leaves that batch pending. A crash after commit cannot reissue the batch: restart reads its committed receipt. Already-issued chunks are retained while the remainder resumes. There is a 10,000-batch startup limit; if reached, the realm stays PAUSED and the next maintenance restart continues the remainder.

Mail is deliberately used for the full migration, avoiding online inventory state and bag-capacity failures. It has no COD and expires at the maximum unsigned database timestamp (2106), rather than the normal 30-day deadline. Preserve these receipts permanently; deleting them or the realm latch is not a supported reset. Character deletion and player disposal of delivered items remain normal core behavior.

Existing physical Seals are preserved. If a character already owns P physical Seals and has a virtual snapshot N, migration issues N additional physical Seals; it does not assume previous GM test grants represented the virtual balance. Audit that distinction before production cutover.

## Runtime currency and rewards

`HuntCurrencyService` centralizes balance authority, virtual award selection, legacy spending/refund, native completion and the startup cutover. Legacy award quantities and the old gossip/HuntsUI purchase path are preserved. Native stats updates add **zero** virtual Seals; physical carried token inventory supplies the balance. Mail and bank holdings are not counted as currently spendable inventory.

Native Hunt Seal rewards also arrive as physical mail attachments. Collect them to update the usable balance/Currency tab. This gives full delivery despite bag capacity and permits atomic persistence without publishing online mailbox objects before commit. Native Seal delivery, completion stats, persistent hunt removal and a completion receipt commit together. Online mail/item caches are updated only after the commit verifies. Failure pauses Seal operations. The core vendor alone debits physical Seals; Hunts never manually debits them for a native purchase.

Existing XP, gold and equipment reward calculations, quantities and timing are unchanged. They are **not made transactional by this currency change**. If a native turn-in reports a database/delivery failure after those rewards, keep the realm paused and reconcile that outstanding hunt before retrying; restarting is not a promise of whole-hunt rollback. Migration failure/restart is separately safe and requires no such reward reconciliation. Local tests cover the Seal transaction, not crash-atomic XP/gold/equipment.

## One-item vendor relationship

EPF 4.3.0 adds exactly:

```json
"vendorRows": [{
  "symbol": "seal-proof-vendor",
  "creatureEntry": 14999989,
  "itemEntry": 40717,
  "extendedCost": {"symbol": "seal-cost-5"}
}]
```

14999989 is the existing Dalaran Huntmaster Varyn. 40717 is existing **Ring of Invincibility**, an epic item-level-213, level-80 BOP ring in the normal tier-1 item-level range. Its stock buy count is one and gold price is zero. The tier-1 price must remain five for this proof to be eligible; no economy values are changed to force the test. Use a character whose class has a specialization eligible under the existing merchandise scoring rules (a level-80 physical-DPS character is a useful candidate). The same eligibility is checked when listing and buying. A class may use any specialization it could select in the existing store; no new spec picker is introduced.

Content Manager's bounded general extension resolves a locally declared cost symbol into one owned `npc_vendor` relationship per existing creature. It emits format-5 server/parity sidecars; formats 1–4 remain supported. The four DBC outputs are unchanged from the equivalent 4.2.0 build. Server apply inserts the resolved vendor row and ORs the vendor flag into the original NPC flags, retaining their original value in `content_manager_vendor_owner`. No hardcoded ExtendedCost SQL is shipped.

The extension refuses an existing unowned vendor row, other merchandise on the same creature, event-vendor relationships, missing templates, ownership drift or changed NPC flags. It does not adopt/delete/recreate a row. An already-owned unchanged relationship is verified and its provenance advanced on explicit apply. Price/relationship changes to an applied vendor are intentionally unsupported in this first bounded extension.

Only Varyn's one item is exposed in native mode. The native gossip text is distinct from the legacy text that HuntsUI intercepts. Legacy addon OPEN/CATALOG/BUYITEM requests are rejected in native/paused mode. Core list/buy hooks block invalid/direct packets to Huntmasters, recheck eligibility and permit one proof unit per purchase. The normal core ExtendedCost logic handles the insufficient-funds check and exact five-item debit. Other Hunt content and Portalkeeper are unchanged.

## Eitrigg deployment and acceptance

### 1. Preserve Build 000011 and verify LEGACY first

Before changes, retain the installed 4.2.0 EPF/source files, worldserver binary, world and character DB backups, Build 000011 MPQ/sidecars, baseline directory and published artifact. Use normal maintenance/backup procedures. Do not uninstall the active build, clear baseline history, retire/reassign leases, or regenerate hashes manually.

Extract the changed-files ZIP at `/home/smithkt/azerothcore-wotlk` (archive roots are `modules/mod-content-manager` and `modules/mod-hunts`). Install only these SQL inputs into their corresponding databases:

```text
modules/mod-content-manager/data/sql/db-world/base/content_manager_schema.sql
modules/mod-hunts/data/sql/db-characters/base/004_hunt_currency.sql
```

The CM schema is idempotent CREATE IF NOT EXISTS plus its existing lock-row INSERT IGNORE. The Hunts file creates the two new tables only. **Do not replay the destructive prebuilt Huntmaster/content SQL.** For example, using your existing MySQL credentials and actual database names:

```sh
cd /home/smithkt/azerothcore-wotlk
mysql <your-connection-options> <world-database> < modules/mod-content-manager/data/sql/db-world/base/content_manager_schema.sql
mysql <your-connection-options> <character-database> < modules/mod-hunts/data/sql/db-characters/base/004_hunt_currency.sql
```

Rebuild/install worldserver through Eitrigg's existing CMake build configuration (reconfigure so the new .cpp files are discovered), then restart with Build 000011 still ACTIVE. Do not activate the new EPF yet. The first Content Manager acceptance command after installation is:

```text
.content dbc inspect CurrencyCategory
.content dbc inspect ItemExtendedCost
.content build list
.content allocations
```

Inspection requires no configured SHA pin. Existing registered baseline identities must still match. Verify Build 000011 remains ACTIVE and the four observed leases remain 56807 / 4 / 5 / 3. Startup should report LEGACY because Build 000011 lacks the new proof-vendor activation declaration. Confirm a controlled character's legacy balance, unchanged reward amount and ordinary legacy store purchase. A separate standalone build with Content Manager excluded should also use LEGACY with the Hunts SQL installed.

### 2. Build the next content artifact and STOP at STAGED

Update the installed package version without an intervening build:

```text
.content uninstall mod-hunts
.content install mod-hunts
.content build
.content build list
.content allocations
.content server status <new-build-number>
```

Use the number actually returned, likely 000012 if no other build was made. Build 000011 must still be ACTIVE. The new client build and server status must be STAGED. Inspect the generated `.server.json` and `.parity.json` in the configured OutputDirectory:

- exactly one `vendorRows` entry: Varyn / Ring of Invincibility / logical `seal-cost-5`;
- its resolved `extendedCostId` matches the retained cost lease (currently 3), requiring five of the retained Seal item (currently 56807);
- the other three leases are unchanged, and no new resource identity was allocated;
- server and parity artifacts agree; all baseline provenance remains accepted;
- four DBCs and the Schema-1 AQ payload remain in the cumulative MPQ;
- no vendor/world flag change or character migration occurred during build.

Review the artifacts before proceeding. Do not issue apply/activate as part of a build script.

### 3. Rehearse the realm cutover on an isolated clone

Because migration is deliberately realm-wide, a controlled migration rehearsal requires an isolated clone of the character/world data; it is **not** a per-player production switch. Keep the clone's publication/client directory separate from live Eitrigg.

Prepare test characters with known virtual balances (zero, 20, and more than one stack); include an offline character and one with full bags. Record any existing physical holdings separately. Apply and activate the reviewed build on the clone, deploy its client patch, and restart before logins. Verify the SQL receipts and mail behavior below. If testing failed delivery, do fault injection only in that isolated test environment. The supplied integration harnesses exercise rollback without a live realm.

### 4. Explicit production apply, activation and migration

After reviewing the staged artifacts and rehearsal, take a current consistent backup, keep players out of the realm, and execute:

```text
.content server apply <new-build-number>
.content server status <new-build-number>
.content activate <new-build-number>
.content build list
```

Require APPLIED before activation. Deploy the published compatible client artifact through the normal Portalkeeper procedure. Restart worldserver to load the owned item/currency/cost/vendor data and perform the realm migration. Keep maintenance in effect until startup reports **NATIVE**, never PAUSED. This is the cutover; Build 000011 is superseded normally by the explicit activation, never rolled back.

Character DB checks:

```sql
SELECT * FROM hunt_currency_realm WHERE id=1;
SELECT COUNT(*) AS pending, COALESCE(SUM(legacy_amount-delivered_amount),0) AS remaining
FROM hunt_currency_delivery WHERE delivered_amount<>legacy_amount;
SELECT COUNT(*) AS receipts, COALESCE(SUM(legacy_amount),0) AS snapshotted,
       COALESCE(SUM(delivered_amount),0) AS issued FROM hunt_currency_delivery;
SELECT guid,legacy_amount,delivered_amount FROM hunt_currency_delivery ORDER BY guid;
```

Require migration_version=1, state=NATIVE, pending=0, remaining=0, matching snapshot totals/counts, and the dynamically resolved Seal identity. Verify the preserved `hunt_stats` values against the backup. Zero-balance/fresh realms have no item delivery; they still get a durable NATIVE realm latch.

### 5. Controlled live reward and one purchase

1. Log in the controlled character. Confirm migration mail contains exactly its old virtual balance, in addition to any prior physical holdings. Full bags must not have lost items. Collect attachments and check the native Hunts Currency category and physical count.
2. Relog and restart once; migration mail/items must not be issued again. Log in the previously offline test character and verify its delivery.
3. Complete a qualifying Elite Hunt. Verify the unchanged base/bonus amount arrives as physical mail, and the saved virtual balance does not increase. Collect the Seals; check the carried physical balance/Currency tab.
4. At Dalaran Huntmaster Varyn, use **Trade physical Seals for the proof reward**. Exactly Ring of Invincibility should appear, at five physical Seals and no gold. If hidden, inspect the existing class/spec eligibility, configured tier-1 cost/range, and required level; do not bypass them.
5. With fewer than five carried physical Seals, the normal purchase must fail without changing currency or merchandise. With exactly five, one purchase must give one BOP ring and consume exactly five Seals once. Verify Currency tab, bag contents and unchanged virtual balance. A second purchase with zero Seals must fail.
6. Confirm the full legacy catalog and its addon purchase requests cannot spend saved virtual balances after cutover. Confirm HuntsUI enabled and disabled both leave the proof on the normal core purchase path. Do not expand the catalog yet.

## Failure and fallback

Before native activation, standalone/never-activated realms retain their virtual economy. After the realm latch exists, missing CM/resources or migration errors pause Seal operations. Restore the correct modules/resources/SQL and resume startup migration from the existing receipts; never delete the latch or mark undelivered rows complete. An ambiguous commit is resolved by the database receipt on restart, not by manually regranting the balance.

A native runtime Hunt-completion failure needs review of the outstanding hunt and its already-issued non-Seal rewards before a retry, as described above. No automatic restoration of virtual balances is implemented. After cutover, do not run an older Hunts binary against the preserved virtual balances: it would ignore the latch. Full rollback/downgrade requires a separate deliberate economic migration. Build 000011 is not a rollback target for this procedure.

## Validation limits and test results

Local validation used the available reference AzerothCore checkout, commit `06234df3d5ab26c93f4f1f06f3edb828b73ecd3c`. Eitrigg's actual checkout/runtime is unavailable here.

Passed: all eight existing standalone CM suites; retained allocation/DBC/parity/baseline/rollback production MySQL regression; production vendor build/apply/activation integration; production realm-migration SQL tests; actual HuntCurrencyService linked without CM using narrow core delivery/player doubles and real MySQL; EPF 4.2-to-4.3 contract comparison; compilation checks against real core headers for all changed runtime units. Fault-injection rejection messages in logs are expected assertions, not deployment failures.

The local fixtures are not Eitrigg acceptance and their baseline fingerprints are not new Eitrigg pins. A full Eitrigg worldserver link, mailbox display, Currency tab refresh and real core vendor purchase still require the live steps above. No such live tests or deployment were performed here. See `NATIVE_TEST_RESULTS.txt` for captured results and `../tests/NATIVE_TESTS.md` for test coverage/reproduction notes.
