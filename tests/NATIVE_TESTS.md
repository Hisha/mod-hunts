# Native realm cutover tests

These tests are development fixtures. Database harnesses use a disposable MySQL instance with networking disabled, empty root password, and the fixed test database `phase4_test`. **They must not be connected to Eitrigg or another real database.** Tests are serial because they share that fixture name.

## Executed coverage

- `realm_migration_mysql.cpp` executes production `HuntCurrencyMigration.cpp` against actual InnoDB transactions. Only the physical delivery callback is substituted. It tests zero/fresh snapshots, offline balances, unchanged virtual values, chunked delivery, restart idempotence, partial preparation failure, SQL rollback, missing recipients and recovery. Receipts and simulated delivery rows commit or roll back together.
- `currency_service_mysql.cpp` links production `HuntCurrencyService.cpp` and `HuntCurrencyMigration.cpp` with **no Content Manager objects**. `currency_adapter` supplies narrow core player/item/mail/registry doubles, while all migration/native completion SQL and prepared-mail transactions execute in MySQL. Tests cover missing provider, inactive/invalid activation, dynamic non-Eitrigg IDs, legacy balance/spend/refund and virtual award selection, native physical balance/award, preserved virtual balance, online mail publication after commit, rollback without cached mail, restart, and paused operations after removing the provider from a migrated installation.
- `mod-content-manager/tests/vendor_mysql_tests.cpp` executes the real package parser, registry, allocator, composers, MPQ builder, parity, server apply and activation against MySQL. It upgrades the 4.2.0 EPF to 4.3.0; checks retained values and baseline identities, byte-identical DBC/AQ payloads, deterministic server artifacts, no build-time vendor mutation, retained ACTIVE baseline before explicit activation, symbolic costs, unowned collision, preflight/transaction drift, rollback after insertion, repeat apply, and owned-row drift.
- Eight existing CM standalone suites and the production Phase 5 MySQL regression passed, including typed allocation, retained/retired occupancy, descriptor validation, baseline acceptance/history, collisions, cross-package costs, raw DBC rejection, canonical parity and rollback.
- `test_extended_cost_epf.py --previous-epf <saved-4.2.0.epf>` now validates 4.3.0, preserves the existing four declarations/cost, and accepts only the new logical vendor relationship plus version/description changes.
- Real-core C++17 compile checks passed for the changed CM and Hunts translation units against reference core commit `06234df3d5ab26c93f4f1f06f3edb828b73ecd3c`. Hunts compilation used its own vendored public header, not a CM include path. This was not a full linked Eitrigg worldserver build.

The mail doubles test atomic persistence/cache ordering; they do not replace a real-client mailbox test. The core BeforeBuy/list hooks and core ExtendedCost debit were inspected and compiled, but actual insufficient/sufficient-funds packets and Currency tab behavior are live acceptance items, not claimed local passes.

## Reproduction outline

Standalone EPF and CM suites require Python 3 and a C++17 compiler:

```sh
python3 modules/mod-hunts/tests/test_extended_cost_epf.py --previous-epf /path/to/saved-4.2.0.epf
python3 modules/mod-content-manager/tests/run_phase4.py
```

Optional CM arguments `--currency-dbc` and `--extended-cost-dbc` read baselines without writing them. Do not pass the 4.3.0 EPF to historical tests that explicitly assert the old 4.1.0 fixture; use the saved fixture appropriate to those tests.

For the two Hunt SQL tests, compile the named production .cpp files with `tests/currency_adapter` before `src` in the include path and link the MySQL client library, for example:

```sh
g++ -std=c++17 $(mysql_config --cflags) \
  -Imodules/mod-hunts/tests/currency_adapter -Imodules/mod-hunts/src \
  modules/mod-hunts/tests/currency_service_mysql.cpp \
  modules/mod-hunts/src/HuntCurrencyService.cpp \
  modules/mod-hunts/src/HuntCurrencyMigration.cpp \
  $(mysql_config --libs) -o /tmp/hunt-currency-service-test
```

Create a fresh disposable `phase4_test`, import `004_hunt_currency.sql`, then create these test-only tables:

```sql
CREATE TABLE hunt_stats(guid INT UNSIGNED PRIMARY KEY,huntmaster_seals INT UNSIGNED NOT NULL) ENGINE=InnoDB;
CREATE TABLE characters(guid INT UNSIGNED PRIMARY KEY) ENGINE=InnoDB;
CREATE TABLE hunt_runtime(guid INT UNSIGNED PRIMARY KEY) ENGINE=InnoDB;
CREATE TABLE item_instance(guid INT PRIMARY KEY,owner_guid INT,itemEntry INT,count INT) ENGINE=InnoDB;
CREATE TABLE mail_items(mail_id INT,item_guid INT PRIMARY KEY,receiver INT) ENGINE=InnoDB;
CREATE TABLE mail(id INT PRIMARY KEY,messageType INT,stationery INT,mailTemplateId INT,sender INT,receiver INT,
 subject TEXT,body TEXT,has_items INT,expire_time BIGINT UNSIGNED,deliver_time BIGINT UNSIGNED,money INT,cod INT,checked INT) ENGINE=InnoDB;
```

Run `/tmp/hunt-currency-service-test /absolute/private/mysql.sock`. For the migration test, use a separately reset fixture, compile `realm_migration_mysql.cpp` plus `HuntCurrencyMigration.cpp`, and add `delivery_items(id INT AUTO_INCREMENT PRIMARY KEY,guid INT,itemEntry INT,amount INT) ENGINE=InnoDB`. It needs the realm/delivery schema plus the simple hunt_stats/characters tables above.

The CM vendor harness follows the existing `PHASE5_TESTS.md` linking/setup procedure, adding production `ContentVendorRow.cpp` and `ContentVendorServer.cpp`. Use the complete reference CREATE TABLE definitions for item_template, currencytypes_dbc, itemextendedcost_dbc, npc_vendor, game_event_npc_vendor, creature_template and creature. Seed stock item 40717, existing creature 14999989 with npcflag=1/ScriptName=mod_hunts_huntmaster, and the occupancy fixture `npc_vendor(entry,item)=(1,-56808)`. Add the existing test-only reference/refund tables described in PHASE5_TESTS.md. Arguments are private socket, disposable fixture root containing baseline copies, saved 4.2.0 EPF, new 4.3.0 EPF, and the Schema-1 AQ EPF. Tests apply/activate ONLY inside this fixture.

See `../docs/NATIVE_TEST_RESULTS.txt` for captured successful output. Intentional transaction-rejection lines are fault-injection results.
