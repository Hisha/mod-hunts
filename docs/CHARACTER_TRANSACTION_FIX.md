# mod-hunts CharacterDatabase worker transaction fix

Apply this changed-files-only ZIP at the AzerothCore root. It contains only `modules/mod-hunts/...` paths. Original files under `/home/smithkt/git/mod-hunts` were read and copied, not edited. No core, SQL schema, statement registration, Content Manager integration, Seal quantity or migration state-machine change is included.

## Runtime files and exact sites

| File | Supplied-source site | Updated site and change |
|---|---|---|
| `src/HuntCharacterTransaction.h` | New | Line 14: shared inline `hunts::CommitCharacterTransactionAndWait` helper. |
| `src/HuntCurrencyMigration.cpp` | Local helper at line 14; snapshot call at 39 | Remove the local helper; include shared header. Snapshot uses shared helper at line 30 and checks its result before the state query. |
| `src/HuntCurrencyMigration.cpp` | Final NATIVE transition call at 65 | Shared helper at line 56; failure returns before completion verification. |
| `src/HuntCurrencyMigration.cpp` | Physical delivery call at 83 | Shared helper at line 74; failure returns before delivered_amount verification and committed(). |
| `src/HuntCurrencyService.cpp` | DirectCommitTransaction at line 169 | Shared helper at line 170; failure pauses currency operations, sets the error and returns before the receipt query (176) or Delivery::Publish. |

The supplied migration source already contained an AsyncCommitTransaction/get helper and checked its result at all three migration sites. Those sites were consolidated, not incorrectly treated as still direct-committed. The native completion at service line 169 was the remaining runtime direct commit.

## Helper

The helper submits the unchanged whole transaction to `CharacterDatabase.AsyncCommitTransaction`, checks `callback.m_future.valid()`, and uses `callback.m_future.get()` to wait and obtain the bool success result. It returns false for an invalid future or an exception while submitting/obtaining completion. Callers fail closed on false; they do not interpret an unconfirmed completion as permission to publish mail or advance through a verification query.

This uses the database worker connection on which the item/mail prepared statements are registered. It does not change CONNECTION_ASYNC/CONNECTION_SYNCH registrations. There are no sleeps, polling, callback-loop dependencies, statement-by-statement commits or separate item/mail transactions. The helper is for startup/world-thread callers, not database worker threads.

## Atomicity and durability

All existing transaction contents, guards and post-commit receipt comparisons remain unchanged. The initial snapshot is still one transaction. Each batch still includes item_instance, mail_items, mail and delivered_amount in one transaction. The final NATIVE transition remains one guarded transaction. The native reward transaction likewise retains its original complete statement set.

Each caller requires a successful worker result before issuing its immediate receipt verification query. Publication requires both successful commit and successful receipt verification. Failed or unconfirmed completion leaves durable state/receipts authoritative; migration can resume its existing pending-work logic on restart without blindly reissuing a committed batch. This patch does not introduce retries or change migration semantics.

## Test files

- `tests/currency_adapter/DatabaseEnv.h`: update the existing test-only SQL adapter to expose AsyncCommitTransaction and its future/bool result. Transaction failures now yield false instead of being hidden by a void direct-commit shim. This remains a synchronous SQL *test double*, not a model of core worker connection registrations.
- `tests/currency_adapter/Transaction.h`: compatibility include for that test adapter.
- `tests/transaction_wait_tests.cpp` and `tests/transaction_wait_adapter/{DatabaseEnv,Field,QueryResult,Transaction}.h`: database-free future and production-migration control-flow tests.

No DirectCommitTransaction uses remain anywhere in the updated mod-hunts tree, including tests. The supplied tree's other occurrence was the old test adapter method; it has also been replaced. Unrelated DirectExecute calls are not transaction commits and are unchanged.

## Validation and build result

Passed:

- Helper tests: an actual std::async worker completes before return; the exact transaction object is submitted; true, false, invalid future, worker exception and submission exception outcomes.
- Production HuntCurrencyMigration::Run with a memory-only database double: failed snapshot, failed delivery and failed final transition do not issue post-commit verification queries or publish; an incorrect receipt does not publish; successful delivery verifies then publishes; final transition verifies. The item/mail/attachment markers and receipt update are checked in the same submitted transaction.
- Both changed production .cpp files compile with the real available reference AzerothCore headers/API (commit `06234df3d5ab26c93f4f1f06f3edb828b73ecd3c`).
- Both production files and the existing migration/service SQL test sources pass compile checks with the updated currency test adapter. Those SQL tests were **not run**, so no databases were altered.

Requested PTR build command was attempted:

```sh
cd ~/azerothcore-wotlk-ptr/build && make -j$(nproc)
```

Result: the shell exited with status 1 because `/home/smithkt/azerothcore-wotlk-ptr/build` does not exist on this machine. `make` therefore did not run. The reference compile checks are not a substitute for a successful PTR tree build. After applying the ZIP on the PTR host, run that exact build command. Do not start worldserver as part of this source/build-only fix.

No worldserver or database server was started, and no database operations were executed during this task.

Reproduce the database-free test from the AzerothCore root:

```sh
g++ -std=c++17 -pthread \
  -Imodules/mod-hunts/tests/transaction_wait_adapter \
  -Imodules/mod-hunts/src \
  modules/mod-hunts/tests/transaction_wait_tests.cpp \
  modules/mod-hunts/src/HuntCurrencyMigration.cpp \
  -o /tmp/hunts-transaction-wait-tests
/tmp/hunts-transaction-wait-tests
```
