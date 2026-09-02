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
            huntsConfig.GetConfigValue<bool>(HuntsConfig::Debug));
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
