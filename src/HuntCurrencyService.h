#ifndef HUNT_CURRENCY_SERVICE_H
#define HUNT_CURRENCY_SERVICE_H
#include "api/ContentCapabilityApiV1.h"
#include <cstdint>
#include <string>
class Player;
// One installation-wide authority, selected once before logins. PAUSED is a
// safety stop, never a switch back to the preserved virtual balance.
class HuntCurrencyService
{
public:
    enum class Mode { Legacy, Native, Paused };
    static HuntCurrencyService& Instance();
    void Initialize();
    Mode GetMode() const { return _mode; }
    bool IsLegacy() const { return _mode==Mode::Legacy; }
    bool IsNative() const { return _mode==Mode::Native; }
    bool Available() const { return _mode!=Mode::Paused; }
    std::uint32_t GetBalance(Player const* player) const;
    std::uint32_t VirtualAward(std::uint32_t amount) const { return IsLegacy()?amount:0; }
    bool SpendLegacy(Player*,std::uint32_t amount);
    void RefundLegacy(Player*,std::uint32_t amount);
    // Native award, stats, and removal of the outstanding hunt commit together.
    bool CompleteNativeHunt(Player*,std::uint32_t amount,std::string const& statsSql,std::string& error);
    ContentCapabilitiesV1::Vendor const& Vendor() const { return _vendor; }
    std::uint32_t SealItem() const { return _seal; }
    std::string const& Reason() const { return _reason; }
private:
    Mode _mode=Mode::Paused;
    std::uint32_t _seal=0;
    ContentCapabilitiesV1::Vendor _vendor;
    std::string _reason="Currency service has not initialized";
    bool Migrate();
    void Pause(std::string const& reason);
};
#define sHuntCurrency HuntCurrencyService::Instance()
#endif
