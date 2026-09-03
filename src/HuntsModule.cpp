#include "HuntManager.h"
#include "HuntCreatureTemplateManager.h"

#include "Chat.h"
#include "CommandScript.h"
#include "ConfigValueCache.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <algorithm>
#include <cctype>
#include <string>

using namespace Acore::ChatCommands;

namespace
{
enum class HuntsConfig
{
    Enabled,
    Debug,
    MinimumLevel,
    XpMultiplier,
    SearchScope,
    EliteRequiredNormalCompletions,
    EliteDailyLimit,
    EliteHealthMultiplier,
    EliteDamageMultiplier,
    EliteArmorMultiplier,
    EliteXpMultiplier,
    EliteGoldMultiplier,
    EliteSealMinimumLevel,
    EliteSealsPerCompletion,
    EliteEndgameRewardLevel,
    EliteEndgameRewardMinItemLevel,
    EliteEndgameRewardMaxItemLevel,
    EliteRewardRequireUpgrade,
    EliteRewardUpgradePoolPct,
    EliteNoUpgradeBonusSeals,
    EliteRangedPanicRange,
    EliteRangedRetreatRangePct,
    EliteRangedBlinkCooldownMs,
    EliteRangedReactionMs,
    EliteRangedArenaRadius,
    SealStoreTier1Cost,
    SealStoreTier1MinItemLevel,
    SealStoreTier1MaxItemLevel,
    SealStoreTier2Cost,
    SealStoreTier2MinItemLevel,
    SealStoreTier2MaxItemLevel,
    SealStoreTier3Cost,
    SealStoreTier3MinItemLevel,
    SealStoreTier3MaxItemLevel,
    SealStoreTier4Cost,
    SealStoreTier4MinItemLevel,
    SealStoreTier4MaxItemLevel,
    TrackingProgressMin,
    TrackingProgressMax,
    GroupCreditRadius,
    SharedFinalCreditRadius,
    Count
};

class HuntsConfigData final : public ConfigValueCache<HuntsConfig>
{
public:
    HuntsConfigData() : ConfigValueCache(HuntsConfig::Count) { }
    void BuildConfigCache() override
    {
        SetConfigValue<bool>(HuntsConfig::Enabled, "Hunts.Enable", true);
        SetConfigValue<bool>(HuntsConfig::Debug, "Hunts.Debug", false);
        SetConfigValue<uint32>(HuntsConfig::MinimumLevel, "Hunts.MinimumLevel", 10);
        SetConfigValue<float>(HuntsConfig::XpMultiplier, "Hunts.XPMultiplier", 0.75f);
        SetConfigValue<uint32>(HuntsConfig::SearchScope, "Hunts.SearchScope", 0);
        SetConfigValue<uint32>(HuntsConfig::EliteRequiredNormalCompletions, "Hunts.Elite.RequiredNormalCompletions", 10);
        SetConfigValue<uint32>(HuntsConfig::EliteDailyLimit, "Hunts.Elite.DailyLimit", 1);
        SetConfigValue<float>(HuntsConfig::EliteHealthMultiplier, "Hunts.Elite.HealthMultiplier", 1.0f);
        SetConfigValue<float>(HuntsConfig::EliteDamageMultiplier, "Hunts.Elite.DamageMultiplier", 1.0f);
        SetConfigValue<float>(HuntsConfig::EliteArmorMultiplier, "Hunts.Elite.ArmorMultiplier", 1.0f);
        SetConfigValue<float>(HuntsConfig::EliteXpMultiplier, "Hunts.Elite.XPMultiplier", 1.0f);
        SetConfigValue<float>(HuntsConfig::EliteGoldMultiplier, "Hunts.Elite.GoldMultiplier", 1.0f);
        SetConfigValue<uint32>(HuntsConfig::EliteSealMinimumLevel, "Hunts.Elite.SealMinimumLevel", 80);
        SetConfigValue<uint32>(HuntsConfig::EliteSealsPerCompletion, "Hunts.Elite.SealsPerCompletion", 1);
        SetConfigValue<uint32>(HuntsConfig::EliteEndgameRewardLevel, "Hunts.Elite.EndgameRewardLevel", 80);
        SetConfigValue<uint32>(HuntsConfig::EliteEndgameRewardMinItemLevel, "Hunts.Elite.EndgameRewardMinItemLevel", 200);
        SetConfigValue<uint32>(HuntsConfig::EliteEndgameRewardMaxItemLevel, "Hunts.Elite.EndgameRewardMaxItemLevel", 200);
        SetConfigValue<bool>(HuntsConfig::EliteRewardRequireUpgrade, "Hunts.Elite.RewardRequireUpgrade", true);
        SetConfigValue<float>(HuntsConfig::EliteRewardUpgradePoolPct, "Hunts.Elite.RewardUpgradePoolPct", 0.70f);
        SetConfigValue<uint32>(HuntsConfig::EliteNoUpgradeBonusSeals, "Hunts.Elite.NoUpgradeBonusSeals", 1);
        SetConfigValue<float>(HuntsConfig::EliteRangedPanicRange, "Hunts.Elite.Ranged.PanicRange", 10.0f);
        SetConfigValue<float>(HuntsConfig::EliteRangedRetreatRangePct, "Hunts.Elite.Ranged.RetreatRangePct", 1.0f);
        SetConfigValue<uint32>(HuntsConfig::EliteRangedBlinkCooldownMs, "Hunts.Elite.Ranged.BlinkCooldownMs", 15000);
        SetConfigValue<uint32>(HuntsConfig::EliteRangedReactionMs, "Hunts.Elite.Ranged.ReactionMs", 500);
        SetConfigValue<float>(HuntsConfig::EliteRangedArenaRadius, "Hunts.Elite.Ranged.ArenaRadius", 35.0f);
        SetConfigValue<uint32>(HuntsConfig::SealStoreTier1Cost, "Hunts.SealStore.Tier1.Cost", 5);
        SetConfigValue<uint32>(HuntsConfig::SealStoreTier1MinItemLevel, "Hunts.SealStore.Tier1.MinItemLevel", 213);
        SetConfigValue<uint32>(HuntsConfig::SealStoreTier1MaxItemLevel, "Hunts.SealStore.Tier1.MaxItemLevel", 219);
        SetConfigValue<uint32>(HuntsConfig::SealStoreTier2Cost, "Hunts.SealStore.Tier2.Cost", 10);
        SetConfigValue<uint32>(HuntsConfig::SealStoreTier2MinItemLevel, "Hunts.SealStore.Tier2.MinItemLevel", 226);
        SetConfigValue<uint32>(HuntsConfig::SealStoreTier2MaxItemLevel, "Hunts.SealStore.Tier2.MaxItemLevel", 232);
        SetConfigValue<uint32>(HuntsConfig::SealStoreTier3Cost, "Hunts.SealStore.Tier3.Cost", 20);
        SetConfigValue<uint32>(HuntsConfig::SealStoreTier3MinItemLevel, "Hunts.SealStore.Tier3.MinItemLevel", 245);
        SetConfigValue<uint32>(HuntsConfig::SealStoreTier3MaxItemLevel, "Hunts.SealStore.Tier3.MaxItemLevel", 251);
        SetConfigValue<uint32>(HuntsConfig::SealStoreTier4Cost, "Hunts.SealStore.Tier4.Cost", 30);
        SetConfigValue<uint32>(HuntsConfig::SealStoreTier4MinItemLevel, "Hunts.SealStore.Tier4.MinItemLevel", 264);
        SetConfigValue<uint32>(HuntsConfig::SealStoreTier4MaxItemLevel, "Hunts.SealStore.Tier4.MaxItemLevel", 264);
        SetConfigValue<uint32>(HuntsConfig::TrackingProgressMin, "Hunts.Tracking.ProgressMin", 3);
        SetConfigValue<uint32>(HuntsConfig::TrackingProgressMax, "Hunts.Tracking.ProgressMax", 7);
        SetConfigValue<float>(HuntsConfig::GroupCreditRadius, "Hunts.GroupCreditRadius", 100.0f);
        SetConfigValue<float>(HuntsConfig::SharedFinalCreditRadius, "Hunts.SharedFinalCreditRadius", 200.0f);
    }
};

HuntsConfigData huntsConfig;

Player* GetCommandPlayer(ChatHandler* handler)
{
    return handler && handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
}

class HuntsWorldScript final : public WorldScript
{
public:
    HuntsWorldScript() : WorldScript("HuntsWorldScript", { WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_STARTUP, WORLDHOOK_ON_UPDATE }) { }

    void OnAfterConfigLoad(bool reload) override
    {
        huntsConfig.Initialize(reload);
        bool const enabled = huntsConfig.GetConfigValue<bool>(HuntsConfig::Enabled);
        if (!enabled)
        {
            sHuntMgr.Reset();
            LOG_INFO("server.loading", "Hunts module is disabled.");
            return;
        }

        // Initial config load occurs before ObjectMgr creature templates are loaded.
        // Materialize Hunts-owned derived prey templates so ObjectMgr sees them.
        if (!reload && !sHuntCreatureTemplateMgr.MaterializeStartupTemplates())
            LOG_ERROR("server.loading", "Hunts failed to materialize derived prey creature templates.");

        sHuntMgr.Configure(
            true,
            static_cast<uint8>(huntsConfig.GetConfigValue<uint32>(HuntsConfig::MinimumLevel)),
            huntsConfig.GetConfigValue<float>(HuntsConfig::XpMultiplier),
            static_cast<hunts::HuntSearchScope>(std::min<uint32>(2, huntsConfig.GetConfigValue<uint32>(HuntsConfig::SearchScope))),
            huntsConfig.GetConfigValue<bool>(HuntsConfig::Debug),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::EliteRequiredNormalCompletions),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::EliteDailyLimit),
            huntsConfig.GetConfigValue<float>(HuntsConfig::EliteHealthMultiplier),
            huntsConfig.GetConfigValue<float>(HuntsConfig::EliteDamageMultiplier),
            huntsConfig.GetConfigValue<float>(HuntsConfig::EliteArmorMultiplier),
            huntsConfig.GetConfigValue<float>(HuntsConfig::EliteXpMultiplier),
            huntsConfig.GetConfigValue<float>(HuntsConfig::EliteGoldMultiplier),
            static_cast<uint8>(std::min<uint32>(255, huntsConfig.GetConfigValue<uint32>(HuntsConfig::EliteSealMinimumLevel))),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::EliteSealsPerCompletion),
            static_cast<uint8>(std::min<uint32>(255, huntsConfig.GetConfigValue<uint32>(HuntsConfig::EliteEndgameRewardLevel))),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::EliteEndgameRewardMinItemLevel),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::EliteEndgameRewardMaxItemLevel),
            static_cast<uint8>(std::min<uint32>(100, huntsConfig.GetConfigValue<uint32>(HuntsConfig::TrackingProgressMin))),
            static_cast<uint8>(std::min<uint32>(100, huntsConfig.GetConfigValue<uint32>(HuntsConfig::TrackingProgressMax))),
            huntsConfig.GetConfigValue<float>(HuntsConfig::GroupCreditRadius),
            huntsConfig.GetConfigValue<float>(HuntsConfig::SharedFinalCreditRadius));
        sHuntMgr.ConfigureEliteRewardTargeting(
            huntsConfig.GetConfigValue<bool>(HuntsConfig::EliteRewardRequireUpgrade),
            huntsConfig.GetConfigValue<float>(HuntsConfig::EliteRewardUpgradePoolPct),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::EliteNoUpgradeBonusSeals));
        sHuntMgr.ConfigureEliteCombat(
            huntsConfig.GetConfigValue<float>(HuntsConfig::EliteRangedPanicRange),
            huntsConfig.GetConfigValue<float>(HuntsConfig::EliteRangedRetreatRangePct),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::EliteRangedBlinkCooldownMs),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::EliteRangedReactionMs),
            huntsConfig.GetConfigValue<float>(HuntsConfig::EliteRangedArenaRadius));
        sHuntMgr.ConfigureSealStoreTier(1,
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::SealStoreTier1Cost),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::SealStoreTier1MinItemLevel),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::SealStoreTier1MaxItemLevel));
        sHuntMgr.ConfigureSealStoreTier(2,
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::SealStoreTier2Cost),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::SealStoreTier2MinItemLevel),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::SealStoreTier2MaxItemLevel));
        sHuntMgr.ConfigureSealStoreTier(3,
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::SealStoreTier3Cost),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::SealStoreTier3MinItemLevel),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::SealStoreTier3MaxItemLevel));
        sHuntMgr.ConfigureSealStoreTier(4,
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::SealStoreTier4Cost),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::SealStoreTier4MinItemLevel),
            huntsConfig.GetConfigValue<uint32>(HuntsConfig::SealStoreTier4MaxItemLevel));
        sHuntMgr.LoadDefinitions();
        LOG_INFO("server.loading", "Hunts module configured.");
    }

    void OnStartup() override
    {
        if (huntsConfig.GetConfigValue<bool>(HuntsConfig::Enabled))
            sHuntMgr.Initialize();
    }

    void OnUpdate(uint32 diff) override
    {
        if (huntsConfig.GetConfigValue<bool>(HuntsConfig::Enabled))
            sHuntMgr.Update(diff);
    }
};

class HuntsCommandScript final : public CommandScript
{
public:
    HuntsCommandScript() : CommandScript("HuntsCommandScript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable finalTable = {
            { "point", HandleSetFinalPoint, rbac::RBAC_PERM_COMMAND_SERVER_INFO, Console::No },
            { "list", HandleSetFinalList, rbac::RBAC_PERM_COMMAND_SERVER_INFO, Console::No },
            { "needs", HandleSetFinalNeeds, rbac::RBAC_PERM_COMMAND_SERVER_INFO, Console::No },
            { "export", HandleSetFinalExport, rbac::RBAC_PERM_COMMAND_SERVER_INFO, Console::No },
            { "levels", HandleSetFinalLevels, rbac::RBAC_PERM_COMMAND_SERVER_INFO, Console::No },
            { "goto", HandleSetFinalGoto, rbac::RBAC_PERM_COMMAND_SERVER_INFO, Console::No },
            { "delete", HandleSetFinalDelete, rbac::RBAC_PERM_COMMAND_SERVER_INFO, Console::No }
        };
        static ChatCommandTable setTable = {
            { "final", finalTable },
            { "needs", HandleSetFinalNeeds, rbac::RBAC_PERM_COMMAND_SERVER_INFO, Console::No }
        };
        static ChatCommandTable huntTable = {
            { "set", setTable },
            { "status", HandleStatus, rbac::RBAC_PERM_COMMAND_SERVER_INFO, Console::No },
            { "progress", HandleProgress, rbac::RBAC_PERM_COMMAND_SERVER_INFO, Console::No },
            { "ambush", HandleAmbush, rbac::RBAC_PERM_COMMAND_SERVER_INFO, Console::No },
            { "final", HandleFinal, rbac::RBAC_PERM_COMMAND_SERVER_INFO, Console::No },
            { "abandon", HandleAbandon, rbac::RBAC_PERM_COMMAND_SERVER_INFO, Console::No }
        };
        static ChatCommandTable root = { { "hunt", huntTable } };
        return root;
    }

private:
    static bool CanAuthor(ChatHandler* handler, Player*& player)
    {
        player = GetCommandPlayer(handler);
        if (!player) return false;
        if (!player->IsGameMaster()) { handler->SendSysMessage("[Hunts] Authoring commands are restricted to Game Masters."); return false; }
        if (!sHuntMgr.IsDebugEnabled()) { handler->SendSysMessage("[Hunts] Authoring commands require Hunts.Debug = 1."); return false; }
        return true;
    }

    static bool HandleSetFinalPoint(ChatHandler* h) { Player* p=nullptr; if(!CanAuthor(h,p))return true; std::string m; sHuntMgr.AddFinalLocationAtPlayer(p,m); h->SendSysMessage(m); return true; }
    static bool HandleSetFinalList(ChatHandler* h) { Player* p=nullptr; if(!CanAuthor(h,p))return true; h->SendSysMessage(sHuntMgr.BuildFinalLocationList(p)); return true; }
    static bool HandleSetFinalNeeds(ChatHandler* h, Optional<std::string> f) { Player* p=nullptr; if(!CanAuthor(h,p))return true; h->SendSysMessage(sHuntMgr.BuildFinalLocationNeeds(f.value_or(""))); return true; }
    static bool HandleSetFinalExport(ChatHandler* h, Optional<std::string> f) { Player* p=nullptr; if(!CanAuthor(h,p))return true; h->SendSysMessage(sHuntMgr.BuildFinalLocationExport(f.value_or(""))); return true; }
    static bool HandleSetFinalLevels(ChatHandler* h, uint32 id, std::string minOrAuto, Optional<uint32> maxLevel)
    {
        Player* p=nullptr; if(!CanAuthor(h,p))return true; std::string m,mode=minOrAuto;
        std::transform(mode.begin(),mode.end(),mode.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
        if(mode=="auto") sHuntMgr.SetFinalLocationLevels(id,0,0,true,m);
        else { uint32 min=0; try { size_t n=0; min=static_cast<uint32>(std::stoul(minOrAuto,&n)); if(n!=minOrAuto.size())min=0; } catch(...) {min=0;}
            if(!maxLevel.has_value()||!min||min>80||maxLevel.value()>80) m="[Hunts] Usage: .hunt set final levels <id> <min 1-80> <max 1-80> | .hunt set final levels <id> auto";
            else sHuntMgr.SetFinalLocationLevels(id,static_cast<uint8>(min),static_cast<uint8>(maxLevel.value()),false,m); }
        h->SendSysMessage(m); return true;
    }
    static bool HandleSetFinalGoto(ChatHandler* h,uint32 id){Player*p=nullptr;if(!CanAuthor(h,p))return true;std::string m;sHuntMgr.TeleportToFinalLocation(p,id,m);h->SendSysMessage(m);return true;}
    static bool HandleSetFinalDelete(ChatHandler* h,uint32 id){Player*p=nullptr;if(!CanAuthor(h,p))return true;std::string m;sHuntMgr.DeleteFinalLocation(id,m);h->SendSysMessage(m);return true;}
    static bool HandleStatus(ChatHandler*h){h->SendSysMessage(sHuntMgr.BuildStatus(GetCommandPlayer(h)));return true;}
    static bool HandleProgress(ChatHandler*h,uint32 a){Player*p=GetCommandPlayer(h);if(!p)return false;std::string m;sHuntMgr.AddProgress(p,static_cast<uint8>(std::min<uint32>(a,100)),m);h->SendSysMessage(m);return true;}
    static bool HandleAmbush(ChatHandler*h){Player*p=GetCommandPlayer(h);if(!p)return false;std::string m;sHuntMgr.ForceAmbush(p,m);h->SendSysMessage(m);return true;}
    static bool HandleFinal(ChatHandler*h){Player*p=GetCommandPlayer(h);if(!p)return false;std::string m;sHuntMgr.ForceFinal(p,m);h->SendSysMessage(m);return true;}
    static bool HandleAbandon(ChatHandler*h){Player*p=GetCommandPlayer(h);if(!p)return false;std::string m;sHuntMgr.AbandonHunt(p,m);h->SendSysMessage(m);return true;}
};
}

void AddHuntsModuleScripts()
{
    new HuntsWorldScript();
    new HuntsCommandScript();
}
