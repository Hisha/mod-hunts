# mod-hunts 4.2.0 — one staged extended-cost consumer

## Scope and exact change

The runtime change is confined to `content/mod-hunts.epf`: Schema 2 package `mod-hunts` changes from **4.1.0 to 4.2.0** and adds exactly this declaration:

```json
"extendedCosts": [
  {
    "symbol": "seal-cost-5",
    "requirements": [
      {
        "item": { "symbol": "seal" },
        "count": 5
      }
    ]
  }
]
```

These are the property names accepted by the installed Content Manager parser. The local requirement resolves to `mod-hunts/seal`; the EPF authors no ItemID, ItemExtendedCost ID, known bit or currency-category ID. The number 5 is a quantity. All preexisting manifest content is unchanged, including the Item and item_template declarations, `seal-currency`, `hunts`, name localization and empty raw-content array. The archive contains only `manifest.json`.

The declaration is intentionally unused by vendors. No gameplay source, Huntmaster gossip/store logic, rewards, hunt_stats balances, SQL, configuration, HuntsUI, Portalkeeper or Content Manager file is changed. No server rebuild is required for this content-only change when the live-validated Phase 5 infrastructure is already installed.

## Changed files

The changed-files ZIP is rooted at `modules/mod-hunts/`, suitable for extraction at the AzerothCore root:

- `content/mod-hunts.epf` — the only runtime change.
- `tests/test_extended_cost_epf.py` — read-only EPF contract and optional 4.1.0 comparison.
- `tests/extended_cost_consumer_mysql.cpp` — private-database test of the unchanged Content Manager parser/build pipeline.
- `tests/PHASE5_CONSUMER_TESTS.md` — test prerequisites and reproduction instructions.
- `docs/PHASE5_CONSUMER_4_2_0.md` — this handoff.
- `docs/PHASE5_CONSUMER_TEST_RESULTS.txt` — captured local test results.

No baseline DBC, generated MPQ, server artifact, executable or old EPF is shipped. Keep a backup of your existing EPF outside all scanned content directories.

## Expected persistent allocations and composition

The three previously verified Eitrigg leases must remain:

| Logical identity | Expected retained value |
|---|---:|
| mod-hunts / seal / item.id | 56807 |
| mod-hunts / seal-currency / currency.known-bit | 4 |
| mod-hunts / hunts / currency-category.id | 5 |

These numbers describe existing registry state; they are not authored in the EPF. One new lease is requested:

```text
mod-hunts / seal-cost-5 / item-extended-cost.id = <automatically allocated>
```

Do not select or expect a particular cost ID. Content Manager considers Eitrigg's baseline, retained leases, SQL definitions and vendor/event/refund references. The namespace is independent from the other three.

The generated cost row must have the allocated ID, `ItemID_1` equal to the resolved retained `mod-hunts/seal/item.id`, and `ItemCount_1=5`. HonorPoints, ArenaPoints, ArenaBracket, RequiredArenaRating, ItemPurchaseGroup and all other item/count slots are zero under the existing composer defaults. Existing baseline rows and string bytes remain intact. With the verified 972-row baseline and just this one declaration, ItemExtendedCost.dbc has 973 records, 16 fields, 64-byte records and the preserved one-byte string block.

For the known two installed packages, the expected cumulative content is:

```text
DBFilesClient/Item.dbc
DBFilesClient/CurrencyTypes.dbc
DBFilesClient/CurrencyCategory.dbc
DBFilesClient/ItemExtendedCost.dbc
World/KALIMDOR/SILITHUS/PASSIVEDOODADS/GONG/SilithidGong.m2
```

That is five content files (MPQ internal metadata entries are not content files). Other legitimate installed packages may add files; inspect and record the actual inventory rather than deleting content to force a count of five. The existing three composed DBCs and AQ asset should be unchanged by this declaration.

The user reports that Eitrigg's real inspection passed descriptor v1/build 12340, 972/16/64/1 dimensions, SHA-256 `606622955569b472e9c9de13b352242f5f0ec8d8bda6c0c3667b9d07feffac8a`, registry UNREGISTERED. The first validated consuming build may automatically register it. **No new SHA configuration or manual pin is needed.** If the file differs from the reviewed live inspection, stop and investigate; this consumer task does not approve a replacement baseline.

## Eitrigg procedure — stop at STAGED artifacts

1. Preserve the current 4.1.0 EPF outside scanned package directories, and record the currently active build. Apply the changed-files ZIP at the AzerothCore root, replacing `modules/mod-hunts/content/mod-hunts.epf`. Leave all other module EPFs and configurations unchanged. No SQL migration accompanies this consumer change.
2. Run:

   ```text
   .content scan
   ```

   Confirm the discovered package is `mod-hunts` **4.2.0** and valid. The registry may still show installed version 4.1.0 at this point. Resolve any duplicate/missing/invalid EPF issue before proceeding.
3. Update the selected package version with these consecutive commands:

   ```text
   .content uninstall mod-hunts
   .content install mod-hunts
   .content scan
   ```

   **Do not build between uninstall and install.** Uninstall changes package selection, not leases or already active artifacts. Confirm installed/discovered version 4.2.0 and the existing AQ package is still selected.
4. Run:

   ```text
   .content build
   .content allocations
   ```

   Record the actual assigned build number and the exact output MPQ/server/parity filenames printed by the build. Do not assume it will be 000011. Build must finish **STAGED**. The allocations output must contain the unchanged three identities above plus `mod-hunts/seal-cost-5/item-extended-cost.id` with an automatically selected value. There may be unrelated retained leases belonging to other packages.
5. **Stop before any server apply or client activation/publication.** In this implementation the server mutation command is `.content server apply <build-number>`; do not invoke it (nor `.content apply`, if an alias exists). Do not invoke `.content activate <build-number>`.
6. Review the staged files at the paths printed by Build:

   - `<actual-name>.mpq.server.json`: format 4; exactly one mod-hunts extended cost named `seal-cost-5`, version 4.2.0, with one requirement for package `mod-hunts`, symbol `seal`, resolved `itemId` equal to its retained lease and `count=5`.
   - `<actual-name>.mpq.parity.json`: the four mod-hunts resources; the matching resolved cost; valid `extendedCostDbcSha256`; an accepted ItemExtendedCost baseline snapshot for build 12340/descriptor 1 with the live-reviewed baseline hash; and the unchanged Item/CurrencyTypes/Category relationships. Sidecar/build hashes must agree with the build report.
   - The staged MPQ: enumerate its actual content files with the existing read-only MPQ inspection tooling. If extracting for inspection, use an isolated temporary directory, never the baseline or active client directory. Check all four DBCs and the AQ asset. Verify the new cost row's ID, retained item relationship and count 5, and preserve stock records.

   Optional read-only status commands:

   ```text
   .content server status <actual-build-number>
   .content dbc inspect ItemExtendedCost
   ```

   Server status must remain STAGED. Baseline inspection should now report the accepted registry identity matching the unchanged reviewed baseline. Inspection reads the baseline, so it still reports **972** stock records; **973** is expected only in the composed staged DBC for this one consumer.
7. After the first artifacts have been captured for review, a second `.content build` is permitted solely to test repeatability, followed by `.content allocations`. It must also remain STAGED. Compare the automatically allocated cost value and extracted ItemExtendedCost.dbc bytes between the two staged builds; both must match. Compare the other three DBCs and AQ bytes too. Build numbers/parity build metadata differ by design, so do not require whole parity files to be byte-identical.

The acceptance checkpoint is four retained mod-hunts allocations plus staged parity proving `seal-cost-5 -> mod-hunts/seal -> retained item.id`, count 5. The active/published client patch and live server definitions must remain unchanged. Native vendor behavior and purchases are not tested or changed in this task.

If validation fails, stop without applying/activating. Keep the staged/failure artifacts and diagnostic output. To restore package selection to 4.1.0 before any deployment, restore its backed-up EPF and repeat uninstall/install/scan without a build between them. Do not delete the newly retained cost lease or baseline registry/history, and do not recycle its ID.

## Local validation and limits

The read-only contract test verifies that the only manifest differences from 4.1.0 are version and the single exact declaration. The C++ fixture uses the current, unchanged Content Manager production parser, package registry, allocator, provenance registry, Build service, composers, MPQ builder and parity verifier against private MySQL and disposable baseline copies. It compares a 4.1.0 staged build to 4.2.0, then repeats the 4.2.0 build. Vendor-write-denying triggers ensure even attempted insert/update/delete operations fail the test. No Apply, Activate or Publish entry point is called.

These are local fixture tests, not a claim that a live Eitrigg 4.2.0 build has occurred. The user-provided live baseline inspection is accepted as the prerequisite; the new consumer's staged allocations/artifacts still require the Eitrigg procedure above. Existing gameplay is preserved by file scope: no gameplay or SQL file is in the ZIP.

## Recorded local results

PASS: EPF contract and exact 4.1.0 comparison; production parser/build integration; uninstall/install without an intervening build; unchanged three leases plus one automatic cost lease; local seal resolution with count 5; deterministic repeated DBC bytes; all previous DBC/AQ bytes unchanged; automatic baseline registration; STAGED-only state; vendor-write-denying triggers; original source/configuration/SQL hash comparison. Actual local MPQ inventory was exactly the five files listed above. Complete output is in `PHASE5_CONSUMER_TEST_RESULTS.txt`.
