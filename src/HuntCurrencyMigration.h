#ifndef HUNT_CURRENCY_MIGRATION_H
#define HUNT_CURRENCY_MIGRATION_H
#include "DatabaseEnv.h"
#include <functional>
#include <string>
#include <cstdint>
class HuntCurrencyMigration
{
public:
    using Prepare=std::function<bool(CharacterDatabaseTransaction const&,std::uint32_t,std::uint32_t,std::string&)>;
    // Caller must run before logins. Prepare appends delivery to this SAME tx.
    static bool Run(std::uint32_t seal,std::uint32_t stack,Prepare const& prepare,
        std::function<void()> const& committed,std::string& error);
};
#endif
