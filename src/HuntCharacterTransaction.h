#ifndef HUNT_CHARACTER_TRANSACTION_H
#define HUNT_CHARACTER_TRANSACTION_H

#include "DatabaseEnv.h"
#include "Transaction.h"
#include <exception>

namespace hunts
{
// For startup/world-thread callers needing completion before a receipt query.
// Execute on a database worker: item/mail prepared statements are ASYNC-only.
// Do not call from a CharacterDatabase worker or register a second consumer of
// this future. get() waits for the transaction result; no polling is needed.
inline bool CommitCharacterTransactionAndWait(CharacterDatabaseTransaction const& transaction)
{
    try
    {
        auto callback = CharacterDatabase.AsyncCommitTransaction(transaction);
        if (!callback.m_future.valid())
            return false;

        return callback.m_future.get();
    }
    catch (std::exception const&)
    {
        // Completion is not confirmed. Leave durable receipts authoritative;
        // callers must neither verify as if committed nor publish cached mail.
        return false;
    }
}
}

#endif
