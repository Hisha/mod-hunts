# 4.2.0 extended-cost consumer tests

## Read-only EPF contract

From mod-hunts:

```sh
python3 tests/test_extended_cost_epf.py
```

For the exact upgrade comparison, keep the old EPF outside scanned directories and supply it:

```sh
python3 tests/test_extended_cost_epf.py --previous-epf /path/to/backup/mod-hunts-4.1.0.epf
```

This uses Python's standard library, writes nothing, and connects to no database. It proves version 4.2.0, one `seal-cost-5`, local symbolic `seal` count 5, no concrete item/cost IDs, preserved existing logical declarations and no extra archive payload. The optional comparison requires all other manifest content and payload bytes to match 4.1.0 exactly.

## Production Build integration, private MySQL only

`extended_cost_consumer_mysql.cpp` links against the **existing** Phase 5 mod-content-manager implementation. No Content Manager source modification is required. Use its `tests/mysql_adapter` and bundled StormLib. The fixture injects configuration/discovery only; parsing, package install/uninstall, allocation, registry, composition, Build, MPQ and parity are production code.

The adapter connects as root without a password to the fixed database `phase4_test` over the supplied socket. **Use only a newly initialized private MySQL instance with TCP networking disabled, never an existing server.** Both adapter pools use the same private database, so character tables are colocated only for this test. This does not test Eitrigg's live database, async worker pool or native client.

Prepare that empty fixture database with utf8mb4_unicode_ci and import mod-content-manager's base schema, plus the reference core's CREATE TABLE definitions (without stock data) for `item_template`, `currencytypes_dbc`, `itemextendedcost_dbc`. Add these test-only occupancy tables:

```sql
CREATE TABLE npc_vendor(item int, ExtendedCost int unsigned NOT NULL DEFAULT 0) ENGINE=InnoDB;
INSERT INTO npc_vendor(item) VALUES(-56808),(0);
CREATE TABLE game_event_npc_vendor(ExtendedCost int unsigned NOT NULL DEFAULT 0) ENGINE=InnoDB;
CREATE TABLE item_refund_instance(paidExtendedCost smallint unsigned NOT NULL DEFAULT 0) ENGINE=InnoDB;
CREATE TABLE playercreateinfo_item(itemid int unsigned);
CREATE TABLE creature_equip_template(ItemID1 int unsigned, ItemID2 int unsigned, ItemID3 int unsigned);
CREATE TABLE item_instance(itemEntry int unsigned);
```

Create a fresh private fixture root with disposable copies under `baseline/` of Item.dbc, CurrencyTypes.dbc, CurrencyCategory.dbc and ItemExtendedCost.dbc. This test seeds the observed Eitrigg retained leases as database fixture values (56807/4/5); use compatible Wrath baselines. It does not author those identities into either EPF. Test copies of the locally inspected files were used during development; ItemExtendedCost SHA matches the real baseline hash subsequently reported by the user.

Compile in C++17 with assertions enabled (no `-DNDEBUG`), including `CM/tests/mysql_adapter` before `CM/src`, mysqlclient headers and `CM/third_party/StormLib/src`. Compile this fixture plus these Content Manager `.cpp` units:

```text
ItemExtendedCostDbc ContentExtendedCostServer ContentBuildService ContentPackage
ContentPackageRegistry ContentResourceAllocator CurrencyDbcComposer ItemDbcComposer
DbcReader DbcDescriptor MpqBuilder ContentServerBundle ContentCurrencyServer
ContentServerOwnership ContentAllocationRegistry ContentBuildRegistry ContentBuildHash
ContentBuildPublisher ServerTableDescriptor CurrencyCategoryDbcComposer ContentBaselineRegistry
```

Also compile `CM/src/third_party/miniz/miniz.c` and link the bundled StormLib static library, mysqlclient and OpenSSL crypto. Do not link ContentManager.cpp: the fixture supplies its config/discovery methods. Run the executable with five arguments:

```text
CONSUMER_TEST PRIVATE_SOCKET PRIVATE_FIXTURE_ROOT OLD_4_1_0_EPF NEW_4_2_0_EPF AQ_SCARAB_GONG_EPF
```

The fixture performs a 4.1.0 reference build, then uninstalls/reinstalls mod-hunts as 4.2.0 **without an intervening build**, and creates two STAGED consumer builds. It never calls Apply, Activate or Publish. Separate INSERT/UPDATE/DELETE-denying triggers on both vendor tables make any attempted vendor mutation fail the test, not just mutations changing row counts.

Coverage:

| Requested checks | Evidence |
|---|---|
| 1–6 Version, three old symbols, one new symbol, local seal x5, no authored IDs | Python contract/comparison plus production parser in C++ fixture |
| 7–10 Three retained leases and one automatic new namespace lease | Actual production Build and SQL allocation registry; exactly four mod-hunts leases |
| 11–12 Existing item resolution and resulting cost row | Canonical server/parity verification plus all 16 composed row words checked |
| 13–14 Repeated ID/DBC stability | Second 4.2.0 build, persistent lease comparison and exact DBC-byte equality |
| 15–16 Existing DBCs and AQ coexistence | Actual MPQ enumeration; every prior 4.1.0 content entry compared byte-for-byte |
| 17 No vendor mutation | Database triggers forbid writes; no server Apply call |
| 18 Gameplay untouched | Runtime change limited to EPF manifest; source/config/SQL hashes unchanged |

Additional assertions: first-use baseline registration from UNREGISTERED without a pin; exact baseline-row/string preservation; one new record; canonical format-4 sidecars; all build/server states STAGED; no live item/currency/cost/ownership rows created. MPQ file inventory is printed, rather than inferred solely from a count. The fixture does not assert a particular allocated cost ID.

Results are captured in `../docs/PHASE5_CONSUMER_TEST_RESULTS.txt`. Real Eitrigg consumer acceptance remains pending and must stop at the staged artifacts as described in `../docs/PHASE5_CONSUMER_4_2_0.md`.
