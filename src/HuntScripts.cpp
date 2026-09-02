#include "HuntManager.h"

#include "AllCreatureScript.h"
#include "Chat.h"
#include "Creature.h"
#include "CreatureScript.h"
#include "GameObject.h"
#include "GameObjectScript.h"
#include "Player.h"
#include "PlayerScript.h"
#include "ScriptedCreature.h"
#include "SpellAuras.h"
#include "SpellInfo.h"

#include <algorithm>
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
    ACTION_GUARD_HUNTMASTER = GOSSIP_ACTION_INFO_DEF + 700
};

class HuntmasterScript final : public CreatureScript
{
public:
    HuntmasterScript() : CreatureScript("mod_hunts_huntmaster") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!sHuntMgr.IsEnabled() || !sHuntMgr.IsHuntGiver(creature->GetEntry()))
            return false;

        hunts::HuntRuntime const* runtime = sHuntMgr.GetRuntime(player);
        if (!runtime)
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I seek dangerous prey.", GOSSIP_SENDER_MAIN, ACTION_REQUEST_HUNT);
            if (sHuntMgr.IsEliteUnlocked(player))
            {
                if (sHuntMgr.IsEliteAvailableToday(player))
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I seek an Elite Hunt.", GOSSIP_SENDER_MAIN, ACTION_REQUEST_ELITE_HUNT);
                else
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I have completed today's Elite Hunt.", GOSSIP_SENDER_MAIN, ACTION_HUNT_STATS);
            }
        }
        else if (runtime->State == hunts::HuntState::ReadyToTurnIn)
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I have slain my quarry.", GOSSIP_SENDER_MAIN, ACTION_TURN_IN_HUNT);
        }
        else
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Tell me about my current hunt.", GOSSIP_SENDER_MAIN, ACTION_HUNT_STATUS);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I wish to abandon this hunt.", GOSSIP_SENDER_MAIN, ACTION_ABANDON_HUNT);
        }

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Show me my hunting record.", GOSSIP_SENDER_MAIN, ACTION_HUNT_STATS);
        SendGossipMenuFor(player, 1, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);
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
    HuntPlayerScript() : PlayerScript("HuntPlayerScript", {
        PLAYERHOOK_ON_CREATURE_KILL,
        PLAYERHOOK_ON_CREATURE_KILLED_BY_PET
    }) { }

    void OnPlayerCreatureKill(Player* killer, Creature* killed) override
    {
        sHuntMgr.OnCreatureKill(killer, killed);
    }

    void OnPlayerCreatureKilledByPet(Player* owner, Creature* killed) override
    {
        sHuntMgr.OnCreatureKill(owner, killed);
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
