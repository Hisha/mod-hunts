#include "HuntManager.h"

#include "AllCreatureScript.h"
#include "Chat.h"
#include "Creature.h"
#include "CreatureScript.h"
#include "GameObject.h"
#include "GameObjectScript.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "SharedDefines.h"
#include "Player.h"
#include "PlayerScript.h"
#include "ScriptedCreature.h"
#include "SpellAuras.h"
#include "SpellInfo.h"

#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "ScriptedGossip.h"

namespace
{
enum HuntGossipAction : uint32
{
    ACTION_HUNT_STATUS = GOSSIP_ACTION_INFO_DEF + 1,
    ACTION_REQUEST_HUNT = GOSSIP_ACTION_INFO_DEF + 2,
    ACTION_TURN_IN_HUNT = GOSSIP_ACTION_INFO_DEF + 3,
    ACTION_ABANDON_HUNT = GOSSIP_ACTION_INFO_DEF + 4,
    ACTION_HUNT_STATS = GOSSIP_ACTION_INFO_DEF + 5,
    ACTION_REQUEST_ELITE_HUNT = GOSSIP_ACTION_INFO_DEF + 6,
    ACTION_SEAL_STORE = GOSSIP_ACTION_INFO_DEF + 20,
    ACTION_SEAL_SPEC_BASE = GOSSIP_ACTION_INFO_DEF + 100,
    ACTION_SEAL_TIER_BASE = GOSSIP_ACTION_INFO_DEF + 110,
    ACTION_SEAL_SLOT_BASE = GOSSIP_ACTION_INFO_DEF + 120,
    ACTION_SEAL_ITEM_BASE = GOSSIP_ACTION_INFO_DEF + 200,
    ACTION_SEAL_BUY_CONFIRM = GOSSIP_ACTION_INFO_DEF + 250,
    ACTION_SEAL_CANCEL = GOSSIP_ACTION_INFO_DEF + 251,
    ACTION_GUARD_HUNTMASTER = GOSSIP_ACTION_INFO_DEF + 700
};

struct SealSpecChoice
{
    uint32 Spec = 0;
    char const* Name = "";
};

struct SealStoreContext
{
    uint32 Spec = 0;
    uint8 Tier = 0;
    hunts::SealStoreSlot Slot = hunts::SealStoreSlot::Weapon;
    std::vector<uint32> ItemIds;
    uint32 PendingItemId = 0;
};

std::unordered_map<uint32, SealStoreContext> sealStoreContexts;
std::unordered_set<uint32> huntsAddonSessions;
std::unordered_map<uint32, ObjectGuid> huntsAddonStoreGivers;

std::vector<std::string> SplitAddonRequest(std::string const& value, char delimiter = '|')
{
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string part;
    while (std::getline(stream, part, delimiter))
        parts.push_back(part);
    return parts;
}

uint32 ParseAddonUInt(std::string const& value)
{
    try
    {
        return static_cast<uint32>(std::stoul(value));
    }
    catch (...)
    {
        return 0;
    }
}

bool HasHuntsAddon(Player const* player)
{
    return player && huntsAddonSessions.find(player->GetGUID().GetCounter()) != huntsAddonSessions.end();
}


std::vector<SealSpecChoice> GetSealSpecs(Player const* player)
{
    if (!player)
        return {};

    switch (player->getClass())
    {
        case CLASS_WARRIOR:
            return {{TALENT_TREE_WARRIOR_ARMS, "Arms"}, {TALENT_TREE_WARRIOR_FURY, "Fury"}, {TALENT_TREE_WARRIOR_PROTECTION, "Protection"}};
        case CLASS_PALADIN:
            return {{TALENT_TREE_PALADIN_HOLY, "Holy"}, {TALENT_TREE_PALADIN_PROTECTION, "Protection"}, {TALENT_TREE_PALADIN_RETRIBUTION, "Retribution"}};
        case CLASS_HUNTER:
            return {{TALENT_TREE_HUNTER_BEAST_MASTERY, "Beast Mastery"}, {TALENT_TREE_HUNTER_MARKSMANSHIP, "Marksmanship"}, {TALENT_TREE_HUNTER_SURVIVAL, "Survival"}};
        case CLASS_ROGUE:
            return {{TALENT_TREE_ROGUE_ASSASSINATION, "Assassination"}, {TALENT_TREE_ROGUE_COMBAT, "Combat"}, {TALENT_TREE_ROGUE_SUBTLETY, "Subtlety"}};
        case CLASS_PRIEST:
            return {{TALENT_TREE_PRIEST_DISCIPLINE, "Discipline"}, {TALENT_TREE_PRIEST_HOLY, "Holy"}, {TALENT_TREE_PRIEST_SHADOW, "Shadow"}};
        case CLASS_DEATH_KNIGHT:
            return {{TALENT_TREE_DEATH_KNIGHT_BLOOD, "Blood"}, {TALENT_TREE_DEATH_KNIGHT_FROST, "Frost"}, {TALENT_TREE_DEATH_KNIGHT_UNHOLY, "Unholy"}};
        case CLASS_SHAMAN:
            return {{TALENT_TREE_SHAMAN_ELEMENTAL, "Elemental"}, {TALENT_TREE_SHAMAN_ENHANCEMENT, "Enhancement"}, {TALENT_TREE_SHAMAN_RESTORATION, "Restoration"}};
        case CLASS_MAGE:
            return {{TALENT_TREE_MAGE_ARCANE, "Arcane"}, {TALENT_TREE_MAGE_FIRE, "Fire"}, {TALENT_TREE_MAGE_FROST, "Frost"}};
        case CLASS_WARLOCK:
            return {{TALENT_TREE_WARLOCK_AFFLICTION, "Affliction"}, {TALENT_TREE_WARLOCK_DEMONOLOGY, "Demonology"}, {TALENT_TREE_WARLOCK_DESTRUCTION, "Destruction"}};
        case CLASS_DRUID:
            return {{TALENT_TREE_DRUID_BALANCE, "Balance"}, {TALENT_TREE_DRUID_FERAL_COMBAT, "Feral"}, {TALENT_TREE_DRUID_RESTORATION, "Restoration"}};
        default:
            return {};
    }
}

char const* GetSealSlotName(hunts::SealStoreSlot slot)
{
    switch (slot)
    {
        case hunts::SealStoreSlot::Weapon: return "Weapons";
        case hunts::SealStoreSlot::Head: return "Head";
        case hunts::SealStoreSlot::Neck: return "Neck";
        case hunts::SealStoreSlot::Shoulder: return "Shoulders";
        case hunts::SealStoreSlot::Back: return "Back";
        case hunts::SealStoreSlot::Chest: return "Chest";
        case hunts::SealStoreSlot::Wrist: return "Wrists";
        case hunts::SealStoreSlot::Hands: return "Hands";
        case hunts::SealStoreSlot::Waist: return "Waist";
        case hunts::SealStoreSlot::Legs: return "Legs";
        case hunts::SealStoreSlot::Feet: return "Feet";
        case hunts::SealStoreSlot::Ring: return "Rings";
        case hunts::SealStoreSlot::Trinket: return "Trinkets";
        case hunts::SealStoreSlot::OffHand: return "Off-hand / Shields";
        case hunts::SealStoreSlot::Relic: return "Ranged / Relics";
        default: return "Equipment";
    }
}

class HuntmasterScript final : public CreatureScript
{
public:
    HuntmasterScript() : CreatureScript("mod_hunts_huntmaster") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!sHuntMgr.IsEnabled() || !sHuntMgr.IsHuntGiver(creature->GetEntry()))
            return false;

        sealStoreContexts.erase(player->GetGUID().GetCounter());
        ShowMainMenu(player, creature);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);
        uint32 const guid = player->GetGUID().GetCounter();

        if (action == ACTION_SEAL_STORE)
        {
            sealStoreContexts[guid] = {};

            // Always remember the Huntmaster before choosing the UI path.
            // HuntsUI's OPEN request is sent by the client immediately after the
            // gossip option is selected.  On a fresh login that request can race
            // the HELLO handshake, so gating this GUID behind HasHuntsAddon()
            // created a circular dependency: the addon could ask to open the
            // store only after the server had already refused to remember the
            // Huntmaster.
            huntsAddonStoreGivers[guid] = creature->GetGUID();

            if (HasHuntsAddon(player))
                CloseGossipMenuFor(player);
            else
                ShowSealSpecMenu(player, creature);

            return true;
        }

        if (action >= ACTION_SEAL_SPEC_BASE && action < ACTION_SEAL_SPEC_BASE + 3)
        {
            std::vector<SealSpecChoice> specs = GetSealSpecs(player);
            uint32 const index = action - ACTION_SEAL_SPEC_BASE;
            if (index < specs.size())
            {
                sealStoreContexts[guid].Spec = specs[index].Spec;
                ShowSealTierMenu(player, creature);
                return true;
            }
        }

        if (action >= ACTION_SEAL_TIER_BASE && action < ACTION_SEAL_TIER_BASE + 4)
        {
            SealStoreContext& context = sealStoreContexts[guid];
            context.Tier = static_cast<uint8>((action - ACTION_SEAL_TIER_BASE) + 1);
            ShowSealSlotMenu(player, creature);
            return true;
        }

        if (action >= ACTION_SEAL_SLOT_BASE && action <= ACTION_SEAL_SLOT_BASE + static_cast<uint32>(hunts::SealStoreSlot::Relic))
        {
            SealStoreContext& context = sealStoreContexts[guid];
            context.Slot = static_cast<hunts::SealStoreSlot>(action - ACTION_SEAL_SLOT_BASE);
            ShowSealItemMenu(player, creature);
            return true;
        }

        if (action >= ACTION_SEAL_ITEM_BASE && action < ACTION_SEAL_ITEM_BASE + 20)
        {
            auto contextIt = sealStoreContexts.find(guid);
            if (contextIt != sealStoreContexts.end())
            {
                uint32 const index = action - ACTION_SEAL_ITEM_BASE;
                if (index < contextIt->second.ItemIds.size())
                {
                    contextIt->second.PendingItemId = contextIt->second.ItemIds[index];
                    ShowSealConfirmation(player, creature);
                    return true;
                }
            }
        }

        if (action == ACTION_SEAL_BUY_CONFIRM)
        {
            auto contextIt = sealStoreContexts.find(guid);
            std::string message;
            if (contextIt != sealStoreContexts.end() && contextIt->second.PendingItemId)
                sHuntMgr.PurchaseSealStoreItem(player, contextIt->second.Spec, contextIt->second.Tier, contextIt->second.PendingItemId, message);
            else
                message = "That Huntmaster reward selection expired. Please choose it again.";

            ChatHandler(player->GetSession()).PSendSysMessage("|cff33ccff[Hunts]|r {}", message);
            sealStoreContexts.erase(guid);
            CloseGossipMenuFor(player);
            return true;
        }

        if (action == ACTION_SEAL_CANCEL)
        {
            sealStoreContexts.erase(guid);
            ShowMainMenu(player, creature);
            return true;
        }

        std::string message;
        switch (action)
        {
            case ACTION_REQUEST_HUNT:
                sHuntMgr.RequestHunt(player, creature, message);
                break;
            case ACTION_REQUEST_ELITE_HUNT:
                sHuntMgr.RequestEliteHunt(player, creature, message);
                break;
            case ACTION_TURN_IN_HUNT:
                sHuntMgr.TurnInHunt(player, creature, message);
                break;
            case ACTION_ABANDON_HUNT:
                sHuntMgr.AbandonHunt(player, message);
                break;
            case ACTION_HUNT_STATS:
                message = sHuntMgr.BuildStats(player);
                break;
            case ACTION_HUNT_STATUS:
            default:
                message = sHuntMgr.BuildStatus(player);
                break;
        }

        ChatHandler(player->GetSession()).PSendSysMessage("|cff33ccff[Hunts]|r {}", message);
        CloseGossipMenuFor(player);
        return true;
    }

private:
    void ShowMainMenu(Player* player, Creature* creature)
    {
        ClearGossipMenuFor(player);
        hunts::HuntRuntime const* runtime = sHuntMgr.GetRuntime(player);
        if (!runtime)
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I seek dangerous prey.", GOSSIP_SENDER_MAIN, ACTION_REQUEST_HUNT);
            if (sHuntMgr.IsEliteUnlocked(player))
            {
                if (sHuntMgr.IsEliteAvailableToday(player))
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I seek an Elite Hunt.", GOSSIP_SENDER_MAIN, ACTION_REQUEST_ELITE_HUNT);
                else
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "My Elite assignment is spent for today.", GOSSIP_SENDER_MAIN, ACTION_HUNT_STATS);
            }
        }
        else if (runtime->State == hunts::HuntState::ReadyToTurnIn)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I have slain my quarry.", GOSSIP_SENDER_MAIN, ACTION_TURN_IN_HUNT);
        else
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Tell me about my current hunt.", GOSSIP_SENDER_MAIN, ACTION_HUNT_STATUS);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I wish to abandon this hunt.", GOSSIP_SENDER_MAIN, ACTION_ABANDON_HUNT);
        }

        if (sHuntMgr.IsSealStoreAvailable(player))
        {
            std::ostringstream label;
            uint32 const seals = sHuntMgr.GetSealBalance(player);
            label << "Browse Huntmaster's Seal rewards. (" << seals << " Seal" << (seals == 1 ? "" : "s") << ")";
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, label.str(), GOSSIP_SENDER_MAIN, ACTION_SEAL_STORE);
        }

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Show me my hunting record.", GOSSIP_SENDER_MAIN, ACTION_HUNT_STATS);
        SendGossipMenuFor(player, 1, creature->GetGUID());
    }

    void ShowSealSpecMenu(Player* player, Creature* creature)
    {
        ClearGossipMenuFor(player);
        std::vector<SealSpecChoice> specs = GetSealSpecs(player);
        for (uint32 i = 0; i < specs.size() && i < 3; ++i)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, std::string("Shop for ") + specs[i].Name + " gear.", GOSSIP_SENDER_MAIN, ACTION_SEAL_SPEC_BASE + i);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Never mind.", GOSSIP_SENDER_MAIN, ACTION_SEAL_CANCEL);
        SendGossipMenuFor(player, 1, creature->GetGUID());
    }

    void ShowSealTierMenu(Player* player, Creature* creature)
    {
        ClearGossipMenuFor(player);
        for (uint8 tier = 1; tier <= 4; ++tier)
        {
            uint32 const cost = sHuntMgr.GetSealStoreTierCost(tier);
            if (!cost)
                continue;
            std::ostringstream label;
            label << "Tier " << static_cast<uint32>(tier) << " - ilvl " << sHuntMgr.GetSealStoreTierMinItemLevel(tier);
            if (sHuntMgr.GetSealStoreTierMaxItemLevel(tier) != sHuntMgr.GetSealStoreTierMinItemLevel(tier))
                label << "-" << sHuntMgr.GetSealStoreTierMaxItemLevel(tier);
            label << " - " << cost << " Seals";
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, label.str(), GOSSIP_SENDER_MAIN, ACTION_SEAL_TIER_BASE + (tier - 1));
        }
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Never mind.", GOSSIP_SENDER_MAIN, ACTION_SEAL_CANCEL);
        SendGossipMenuFor(player, 1, creature->GetGUID());
    }

    void ShowSealSlotMenu(Player* player, Creature* creature)
    {
        ClearGossipMenuFor(player);
        for (uint8 rawSlot = static_cast<uint8>(hunts::SealStoreSlot::Weapon);
             rawSlot <= static_cast<uint8>(hunts::SealStoreSlot::Relic); ++rawSlot)
        {
            hunts::SealStoreSlot const slot = static_cast<hunts::SealStoreSlot>(rawSlot);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, GetSealSlotName(slot), GOSSIP_SENDER_MAIN, ACTION_SEAL_SLOT_BASE + rawSlot);
        }
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Never mind.", GOSSIP_SENDER_MAIN, ACTION_SEAL_CANCEL);
        SendGossipMenuFor(player, 1, creature->GetGUID());
    }

    void ShowSealItemMenu(Player* player, Creature* creature)
    {
        ClearGossipMenuFor(player);
        SealStoreContext& context = sealStoreContexts[player->GetGUID().GetCounter()];
        context.ItemIds.clear();
        context.PendingItemId = 0;

        std::vector<hunts::SealStoreItem> items = sHuntMgr.BuildSealStoreItems(player, context.Spec, context.Tier, context.Slot);
        uint32 const cost = sHuntMgr.GetSealStoreTierCost(context.Tier);
        for (uint32 i = 0; i < items.size() && i < 20; ++i)
        {
            context.ItemIds.push_back(items[i].ItemId);
            std::ostringstream label;
            label << items[i].Name << " (ilvl " << items[i].ItemLevel << ") - " << cost << " Seals";
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, label.str(), GOSSIP_SENDER_MAIN, ACTION_SEAL_ITEM_BASE + i);
        }

        if (items.empty())
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "No suitable rewards are available in this category.", GOSSIP_SENDER_MAIN, ACTION_SEAL_CANCEL);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Never mind.", GOSSIP_SENDER_MAIN, ACTION_SEAL_CANCEL);
        SendGossipMenuFor(player, 1, creature->GetGUID());
    }

    void ShowSealConfirmation(Player* player, Creature* creature)
    {
        ClearGossipMenuFor(player);
        SealStoreContext const& context = sealStoreContexts[player->GetGUID().GetCounter()];
        ItemTemplate const* item = sObjectMgr->GetItemTemplate(context.PendingItemId);
        uint32 const cost = sHuntMgr.GetSealStoreTierCost(context.Tier);
        uint32 const balance = sHuntMgr.GetSealBalance(player);

        std::ostringstream buyLabel;
        buyLabel << "Purchase " << (item ? item->Name1 : "this item") << " for " << cost << " Seals. (Balance: " << balance << ")";
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, buyLabel.str(), GOSSIP_SENDER_MAIN, ACTION_SEAL_BUY_CONFIRM);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Cancel purchase.", GOSSIP_SENDER_MAIN, ACTION_SEAL_CANCEL);
        SendGossipMenuFor(player, 1, creature->GetGUID());
    }
};

// Extends the normal capital-city guard gossip without replacing the stock
// directions.  We prepare the guard's normal database menu, append one Living
// World option, and only consume our own action when it is selected.
class HuntGuardLocatorScript final : public AllCreatureScript
{
public:
    HuntGuardLocatorScript() : AllCreatureScript("HuntGuardLocatorScript") { }

    bool CanCreatureGossipHello(Player* player, Creature* creature) override
    {
        if (!sHuntMgr.IsEnabled() || !player || !creature || !sHuntMgr.IsGuardLocator(creature->GetEntry()))
            return false;

        player->PrepareGossipMenu(creature, creature->GetGossipMenuId(), true);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Where is the Huntmaster?", GOSSIP_SENDER_MAIN, ACTION_GUARD_HUNTMASTER);
        player->SendPreparedGossip(creature);
        return true;
    }

    bool CanCreatureGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        if (action != ACTION_GUARD_HUNTMASTER || !player || !creature || !sHuntMgr.IsGuardLocator(creature->GetEntry()))
            return false;

        std::string message;
        sHuntMgr.SendHuntmasterLocation(player, creature->GetEntry(), message);
        if (!message.empty())
            ChatHandler(player->GetSession()).PSendSysMessage("|cff33ccff[Hunts]|r {}", message);
        CloseGossipMenuFor(player);
        return true;
    }
};

// Elite Hunt prey are intended to fight like dangerous player-class opponents,
// not like dungeon bosses.  They therefore accept normal player crowd control,
// but hard CC uses PvP-style diminishing returns so an Elite cannot be
// stun-locked indefinitely.
//
// Stun DR is deliberately encounter-local:
//   1st stun = 100% duration
//   2nd stun =  50% duration
//   3rd stun =  25% duration
//   4th+     = immune until 15 seconds after the previous stun expires
//
// Other CC categories can be added to this same AI as Elite archetypes grow.
class HuntElitePreyAI final : public ScriptedAI
{
public:
    explicit HuntElitePreyAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        ClearStunDiminishing();
    }

    void SpellHit(Unit* caster, SpellInfo const* spellInfo) override
    {
        if (!caster || !spellInfo || !caster->GetCharmerOrOwnerPlayerOrPlayerItself())
            return;

        uint32 const mechanicMask = spellInfo->GetAllEffectsMechanicMask();
        if (!(mechanicMask & (1u << MECHANIC_STUN)))
            return;

        Aura* aura = me->GetAura(spellInfo->Id, caster->GetGUID());
        if (!aura)
            return;

        // If the reset window elapsed between AI updates and this hit, begin a
        // fresh DR chain before adjusting the newly applied aura.
        if (_stunResetMs == 0)
            _stunApplications = 0;

        ++_stunApplications;

        int32 duration = aura->GetDuration();
        if (_stunApplications == 2)
            duration = std::max<int32>(1, duration / 2);
        else if (_stunApplications >= 3)
            duration = std::max<int32>(1, duration / 4);

        aura->SetDuration(duration);

        // WotLK-style DR resets after the CC has ended, not when it was cast.
        _stunResetMs = static_cast<uint32>(std::max<int32>(0, duration)) + 15000u;

        if (_stunApplications >= 3 && !_stunImmune)
        {
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
            _stunImmune = true;
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (_stunResetMs)
        {
            if (_stunResetMs <= diff)
                ClearStunDiminishing();
            else
                _stunResetMs -= diff;
        }

        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }

private:
    void ClearStunDiminishing()
    {
        if (_stunImmune)
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, false);

        _stunApplications = 0;
        _stunResetMs = 0;
        _stunImmune = false;
    }

    uint8 _stunApplications = 0;
    uint32 _stunResetMs = 0;
    bool _stunImmune = false;
};

class HuntElitePreyScript final : public AllCreatureScript
{
public:
    HuntElitePreyScript() : AllCreatureScript("HuntElitePreyScript") { }

    CreatureAI* GetCreatureAI(Creature* creature) const override
    {
        if (!creature || !sHuntMgr.IsEnabled() || !sHuntMgr.IsElitePreyEntry(creature->GetEntry()))
            return nullptr;

        return new HuntElitePreyAI(creature);
    }
};

class HuntActivationScript final : public GameObjectScript
{
public:
    HuntActivationScript() : GameObjectScript("mod_hunts_activation") { }

    bool OnGossipHello(Player* player, GameObject* gameObject) override
    {
        std::string message;
        sHuntMgr.OnFinalActivatorUsed(player, gameObject, message);
        if (!message.empty())
            ChatHandler(player->GetSession()).PSendSysMessage("|cff33ccff[Hunts]|r {}", message);
        return true;
    }
};

class HuntPlayerScript final : public PlayerScript
{
public:
    HuntPlayerScript() : PlayerScript("HuntPlayerScript") { }

    void OnPlayerCreatureKill(Player* killer, Creature* killed) override
    {
        sHuntMgr.OnCreatureKill(killer, killed);
    }

    void OnPlayerCreatureKilledByPet(Player* owner, Creature* killed) override
    {
        sHuntMgr.OnCreatureKill(owner, killed);
    }

    void OnPlayerBeforeSendChatMessage(Player* player, uint32& type, uint32& lang, std::string& msg) override
    {
        if (!player || lang != LANG_ADDON || type != CHAT_MSG_WHISPER)
            return;

        static std::string const prefix = "HUNTS\t";
        if (msg.compare(0, prefix.size(), prefix) != 0)
            return;

        uint32 const guid = player->GetGUID().GetCounter();
        std::string const request = msg.substr(prefix.size());
        std::vector<std::string> parts = SplitAddonRequest(request);
        std::string response = "ERR|bad-request";

        if (!parts.empty() && parts[0] == "HELLO")
        {
            huntsAddonSessions.insert(guid);
            response = "HELLO|1";
        }
        else if (!parts.empty() && parts[0] == "OPEN")
        {
            huntsAddonSessions.insert(guid);
            auto giverIt = huntsAddonStoreGivers.find(guid);
            Creature* giver = giverIt != huntsAddonStoreGivers.end() ? ObjectAccessor::GetCreature(*player, giverIt->second) : nullptr;
            if (!giver || !sHuntMgr.IsHuntGiver(giver->GetEntry()) || !player->IsWithinDistInMap(giver, 12.0f))
                response = "ERR|Talk to a Huntmaster and choose Seal rewards first.";
            else if (!sHuntMgr.IsSealStoreAvailable(player))
                response = "ERR|The Huntmaster Seal store is not available to you.";
            else
            {
                // OPEN is also the authoritative late handshake.  If HELLO had
                // not completed before the player clicked Rewards, replace the
                // just-opened gossip fallback with HuntsUI now.
                huntsAddonSessions.insert(guid);
                CloseGossipMenuFor(player);

                std::vector<SealSpecChoice> specs = GetSealSpecs(player);
                std::ostringstream out;
                out << "OPEN|" << sHuntMgr.GetSealBalance(player) << "|";
                for (uint32 i = 0; i < specs.size(); ++i)
                {
                    if (i)
                        out << ",";
                    out << specs[i].Spec << ":" << specs[i].Name;
                }
                response = out.str();
            }
        }
        else if (parts.size() >= 5 && parts[0] == "LIST")
        {
            auto giverIt = huntsAddonStoreGivers.find(guid);
            Creature* giver = giverIt != huntsAddonStoreGivers.end() ? ObjectAccessor::GetCreature(*player, giverIt->second) : nullptr;
            if (!giver || !sHuntMgr.IsHuntGiver(giver->GetEntry()) || !player->IsWithinDistInMap(giver, 12.0f))
                response = "ERR|You are no longer speaking with a Huntmaster.";
            else
            {
                uint32 const spec = ParseAddonUInt(parts[1]);
                uint8 const tier = static_cast<uint8>(ParseAddonUInt(parts[2]));
                uint8 const rawSlot = static_cast<uint8>(ParseAddonUInt(parts[3]));
                uint32 const page = ParseAddonUInt(parts[4]);
                if (tier < 1 || tier > 4 || rawSlot > static_cast<uint8>(hunts::SealStoreSlot::Relic))
                    response = "ERR|Invalid reward category.";
                else
                {
                    std::vector<hunts::SealStoreItem> items =
                        sHuntMgr.BuildSealStoreItems(player, spec, tier, static_cast<hunts::SealStoreSlot>(rawSlot));
                    constexpr uint32 pageSize = 10;
                    uint32 const offset = page * pageSize;
                    std::ostringstream out;
                    out << "ITEMS|" << sHuntMgr.GetSealBalance(player) << "|" << sHuntMgr.GetSealStoreTierCost(tier)
                        << "|" << page << "|" << ((offset + pageSize < items.size()) ? 1 : 0) << "|";
                    for (uint32 i = offset; i < items.size() && i < offset + pageSize; ++i)
                    {
                        if (i != offset)
                            out << ",";
                        out << items[i].ItemId << ":" << items[i].ItemLevel;
                    }
                    response = out.str();
                }
            }
        }
        else if (parts.size() >= 4 && parts[0] == "BUY")
        {
            auto giverIt = huntsAddonStoreGivers.find(guid);
            Creature* giver = giverIt != huntsAddonStoreGivers.end() ? ObjectAccessor::GetCreature(*player, giverIt->second) : nullptr;
            if (!giver || !sHuntMgr.IsHuntGiver(giver->GetEntry()) || !player->IsWithinDistInMap(giver, 12.0f))
                response = "ERR|You are no longer speaking with a Huntmaster.";
            else
            {
                uint32 const spec = ParseAddonUInt(parts[1]);
                uint8 const tier = static_cast<uint8>(ParseAddonUInt(parts[2]));
                uint32 const itemId = ParseAddonUInt(parts[3]);
                std::string purchaseMessage;
                bool const success = sHuntMgr.PurchaseSealStoreItem(player, spec, tier, itemId, purchaseMessage);
                std::ostringstream out;
                out << "BUY|" << (success ? 1 : 0) << "|" << sHuntMgr.GetSealBalance(player) << "|" << itemId;
                response = out.str();
                ChatHandler(player->GetSession()).PSendSysMessage("|cff33ccff[Hunts]|r {}", purchaseMessage);
            }
        }

        msg = prefix + response;
        if (msg.size() > 250)
            msg = prefix + "ERR|response-too-large";
    }

    void OnPlayerBeforeLogout(Player* player) override
    {
        if (!player)
            return;
        uint32 const guid = player->GetGUID().GetCounter();
        huntsAddonSessions.erase(guid);
        huntsAddonStoreGivers.erase(guid);
        sealStoreContexts.erase(guid);
    }
};
}

void AddHuntGameplayScripts()
{
    new HuntmasterScript();
    new HuntGuardLocatorScript();
    new HuntElitePreyScript();
    new HuntActivationScript();
    new HuntPlayerScript();
}
