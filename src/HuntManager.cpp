#include "HuntManager.h"

#include "Chat.h"
#include "Creature.h"
#include "GameObject.h"
#include "Formulas.h"
#include "HuntCreatureTemplateManager.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Log.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "Opcodes.h"
#include "Player.h"
#include "Random.h"
#include "TemporarySummon.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "CreatureAI.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <unordered_map>


namespace
{
enum class RewardRole
{
    Generic,
    StrengthMelee,
    AgilityMelee,
    HunterRanged,
    SpellDamage,
    Healer,
    Tank
};

RewardRole GetRewardRole(Player* player, uint32 spec)
{
    switch (spec)
    {
        case TALENT_TREE_WARRIOR_PROTECTION:
        case TALENT_TREE_PALADIN_PROTECTION:
        case TALENT_TREE_DEATH_KNIGHT_BLOOD:
            return RewardRole::Tank;

        case TALENT_TREE_WARRIOR_ARMS:
        case TALENT_TREE_WARRIOR_FURY:
        case TALENT_TREE_PALADIN_RETRIBUTION:
        case TALENT_TREE_DEATH_KNIGHT_FROST:
        case TALENT_TREE_DEATH_KNIGHT_UNHOLY:
            return RewardRole::StrengthMelee;

        case TALENT_TREE_ROGUE_ASSASSINATION:
        case TALENT_TREE_ROGUE_COMBAT:
        case TALENT_TREE_ROGUE_SUBTLETY:
        case TALENT_TREE_SHAMAN_ENHANCEMENT:
        case TALENT_TREE_DRUID_FERAL_COMBAT:
            return RewardRole::AgilityMelee;

        case TALENT_TREE_HUNTER_BEAST_MASTERY:
        case TALENT_TREE_HUNTER_MARKSMANSHIP:
        case TALENT_TREE_HUNTER_SURVIVAL:
            return RewardRole::HunterRanged;

        case TALENT_TREE_PALADIN_HOLY:
        case TALENT_TREE_PRIEST_DISCIPLINE:
        case TALENT_TREE_PRIEST_HOLY:
        case TALENT_TREE_SHAMAN_RESTORATION:
        case TALENT_TREE_DRUID_RESTORATION:
            return RewardRole::Healer;

        case TALENT_TREE_PRIEST_SHADOW:
        case TALENT_TREE_SHAMAN_ELEMENTAL:
        case TALENT_TREE_MAGE_ARCANE:
        case TALENT_TREE_MAGE_FIRE:
        case TALENT_TREE_MAGE_FROST:
        case TALENT_TREE_WARLOCK_AFFLICTION:
        case TALENT_TREE_WARLOCK_DEMONOLOGY:
        case TALENT_TREE_WARLOCK_DESTRUCTION:
        case TALENT_TREE_DRUID_BALANCE:
            return RewardRole::SpellDamage;
        default:
            break;
    }

    // Characters with too few talent points to establish a tree still get a
    // class-appropriate baseline instead of fully random equipment.
    switch (player->getClass())
    {
        case CLASS_ROGUE: return RewardRole::AgilityMelee;
        case CLASS_HUNTER: return RewardRole::HunterRanged;
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_PRIEST: return RewardRole::SpellDamage;
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT: return RewardRole::StrengthMelee;
        case CLASS_SHAMAN:
        case CLASS_DRUID: return RewardRole::Generic;
        default: return RewardRole::Generic;
    }
}

float GetRewardStatWeight(RewardRole role, uint32 stat)
{
    switch (role)
    {
        case RewardRole::StrengthMelee:
            switch (stat)
            {
                case ITEM_MOD_STRENGTH: return 1.00f;
                case ITEM_MOD_ATTACK_POWER: return 0.50f;
                case ITEM_MOD_CRIT_RATING:
                case ITEM_MOD_CRIT_MELEE_RATING: return 0.70f;
                case ITEM_MOD_HIT_RATING:
                case ITEM_MOD_HIT_MELEE_RATING: return 0.75f;
                case ITEM_MOD_EXPERTISE_RATING: return 0.75f;
                case ITEM_MOD_HASTE_RATING:
                case ITEM_MOD_HASTE_MELEE_RATING: return 0.55f;
                case ITEM_MOD_ARMOR_PENETRATION_RATING: return 0.55f;
                case ITEM_MOD_STAMINA: return 0.20f;
                default: return 0.0f;
            }
        case RewardRole::AgilityMelee:
            switch (stat)
            {
                case ITEM_MOD_AGILITY: return 1.00f;
                case ITEM_MOD_ATTACK_POWER: return 0.50f;
                case ITEM_MOD_CRIT_RATING:
                case ITEM_MOD_CRIT_MELEE_RATING: return 0.75f;
                case ITEM_MOD_HIT_RATING:
                case ITEM_MOD_HIT_MELEE_RATING: return 0.75f;
                case ITEM_MOD_EXPERTISE_RATING: return 0.70f;
                case ITEM_MOD_HASTE_RATING:
                case ITEM_MOD_HASTE_MELEE_RATING: return 0.55f;
                case ITEM_MOD_ARMOR_PENETRATION_RATING: return 0.50f;
                case ITEM_MOD_STAMINA: return 0.20f;
                default: return 0.0f;
            }
        case RewardRole::HunterRanged:
            switch (stat)
            {
                case ITEM_MOD_AGILITY: return 1.00f;
                case ITEM_MOD_ATTACK_POWER:
                case ITEM_MOD_RANGED_ATTACK_POWER: return 0.50f;
                case ITEM_MOD_CRIT_RATING:
                case ITEM_MOD_CRIT_RANGED_RATING: return 0.75f;
                case ITEM_MOD_HIT_RATING:
                case ITEM_MOD_HIT_RANGED_RATING: return 0.75f;
                case ITEM_MOD_HASTE_RATING:
                case ITEM_MOD_HASTE_RANGED_RATING: return 0.55f;
                case ITEM_MOD_INTELLECT: return 0.25f;
                case ITEM_MOD_STAMINA: return 0.15f;
                default: return 0.0f;
            }
        case RewardRole::SpellDamage:
            switch (stat)
            {
                case ITEM_MOD_SPELL_POWER: return 1.00f;
                case ITEM_MOD_INTELLECT: return 0.75f;
                case ITEM_MOD_HIT_RATING:
                case ITEM_MOD_HIT_SPELL_RATING: return 0.70f;
                case ITEM_MOD_CRIT_RATING:
                case ITEM_MOD_CRIT_SPELL_RATING: return 0.65f;
                case ITEM_MOD_HASTE_RATING:
                case ITEM_MOD_HASTE_SPELL_RATING: return 0.70f;
                case ITEM_MOD_SPIRIT: return 0.30f;
                case ITEM_MOD_MANA_REGENERATION: return 0.25f;
                case ITEM_MOD_STAMINA: return 0.10f;
                default: return 0.0f;
            }
        case RewardRole::Healer:
            switch (stat)
            {
                case ITEM_MOD_SPELL_POWER: return 1.00f;
                case ITEM_MOD_INTELLECT: return 0.85f;
                case ITEM_MOD_MANA_REGENERATION: return 0.80f;
                case ITEM_MOD_HASTE_RATING:
                case ITEM_MOD_HASTE_SPELL_RATING: return 0.70f;
                case ITEM_MOD_CRIT_RATING:
                case ITEM_MOD_CRIT_SPELL_RATING: return 0.50f;
                case ITEM_MOD_SPIRIT: return 0.45f;
                case ITEM_MOD_STAMINA: return 0.10f;
                default: return 0.0f;
            }
        case RewardRole::Tank:
            switch (stat)
            {
                case ITEM_MOD_STAMINA: return 1.00f;
                case ITEM_MOD_DEFENSE_SKILL_RATING: return 0.90f;
                case ITEM_MOD_DODGE_RATING:
                case ITEM_MOD_PARRY_RATING:
                case ITEM_MOD_BLOCK_RATING: return 0.75f;
                case ITEM_MOD_BLOCK_VALUE: return 0.65f;
                case ITEM_MOD_STRENGTH: return 0.55f;
                case ITEM_MOD_HIT_RATING:
                case ITEM_MOD_HIT_MELEE_RATING:
                case ITEM_MOD_EXPERTISE_RATING: return 0.35f;
                default: return 0.0f;
            }
        default:
            return stat == ITEM_MOD_STAMINA ? 0.15f : 0.0f;
    }
}

float GetArmorPreference(Player* player, ItemTemplate const& item)
{
    if (item.Class != ITEM_CLASS_ARMOR)
        return 0.0f;

    // Cloaks, rings, trinkets, necklaces and relics are not governed by armor
    // material preference. CanUseItem() still validates their real restrictions.
    if (item.InventoryType == INVTYPE_CLOAK || item.InventoryType == INVTYPE_NECK ||
        item.InventoryType == INVTYPE_FINGER || item.InventoryType == INVTYPE_TRINKET ||
        item.InventoryType == INVTYPE_RELIC || item.InventoryType == INVTYPE_SHIELD ||
        item.InventoryType == INVTYPE_HOLDABLE)
        return 12.0f;

    uint32 wantedSubclass = ITEM_SUBCLASS_ARMOR_CLOTH;
    switch (player->getClass())
    {
        case CLASS_ROGUE:
        case CLASS_DRUID:
            wantedSubclass = ITEM_SUBCLASS_ARMOR_LEATHER;
            break;
        case CLASS_HUNTER:
        case CLASS_SHAMAN:
            wantedSubclass = player->GetLevel() >= 40 ? ITEM_SUBCLASS_ARMOR_MAIL : ITEM_SUBCLASS_ARMOR_LEATHER;
            break;
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
            wantedSubclass = player->GetLevel() >= 40 ? ITEM_SUBCLASS_ARMOR_PLATE : ITEM_SUBCLASS_ARMOR_MAIL;
            break;
        case CLASS_DEATH_KNIGHT:
            wantedSubclass = ITEM_SUBCLASS_ARMOR_PLATE;
            break;
        default:
            wantedSubclass = ITEM_SUBCLASS_ARMOR_CLOTH;
            break;
    }

    if (item.SubClass == wantedSubclass)
        return 30.0f;

    // It may be technically equipable (for example, cloth on a rogue), but it
    // should almost never beat gear of the class's intended armor type.
    return -30.0f;
}

float GetWeaponPreference(Player* player, uint32 spec, ItemTemplate const& item)
{
    if (item.Class != ITEM_CLASS_WEAPON)
        return 0.0f;

    float score = 0.0f;
    switch (spec)
    {
        case TALENT_TREE_ROGUE_ASSASSINATION:
            score += item.SubClass == ITEM_SUBCLASS_WEAPON_DAGGER ? 45.0f : -20.0f;
            break;
        case TALENT_TREE_ROGUE_SUBTLETY:
            score += item.SubClass == ITEM_SUBCLASS_WEAPON_DAGGER ? 35.0f : 0.0f;
            break;
        case TALENT_TREE_ROGUE_COMBAT:
            if (item.SubClass == ITEM_SUBCLASS_WEAPON_SWORD || item.SubClass == ITEM_SUBCLASS_WEAPON_AXE ||
                item.SubClass == ITEM_SUBCLASS_WEAPON_MACE || item.SubClass == ITEM_SUBCLASS_WEAPON_FIST ||
                item.SubClass == ITEM_SUBCLASS_WEAPON_DAGGER)
                score += 30.0f;
            break;
        case TALENT_TREE_SHAMAN_ENHANCEMENT:
            if (item.SubClass == ITEM_SUBCLASS_WEAPON_AXE || item.SubClass == ITEM_SUBCLASS_WEAPON_MACE ||
                item.SubClass == ITEM_SUBCLASS_WEAPON_FIST)
                score += 35.0f;
            else
                score -= 15.0f;
            break;
        case TALENT_TREE_WARRIOR_ARMS:
        case TALENT_TREE_PALADIN_RETRIBUTION:
        case TALENT_TREE_DEATH_KNIGHT_UNHOLY:
            score += item.InventoryType == INVTYPE_2HWEAPON ? 35.0f : 0.0f;
            break;
        case TALENT_TREE_WARRIOR_PROTECTION:
        case TALENT_TREE_PALADIN_PROTECTION:
            score += (item.InventoryType == INVTYPE_WEAPON || item.InventoryType == INVTYPE_WEAPONMAINHAND) ? 30.0f : -15.0f;
            break;
        case TALENT_TREE_HUNTER_BEAST_MASTERY:
        case TALENT_TREE_HUNTER_MARKSMANSHIP:
        case TALENT_TREE_HUNTER_SURVIVAL:
            if (item.SubClass == ITEM_SUBCLASS_WEAPON_BOW || item.SubClass == ITEM_SUBCLASS_WEAPON_GUN ||
                item.SubClass == ITEM_SUBCLASS_WEAPON_CROSSBOW)
                score += 45.0f;
            break;
        case TALENT_TREE_PRIEST_DISCIPLINE:
        case TALENT_TREE_PRIEST_HOLY:
        case TALENT_TREE_PRIEST_SHADOW:
        case TALENT_TREE_SHAMAN_ELEMENTAL:
        case TALENT_TREE_SHAMAN_RESTORATION:
        case TALENT_TREE_MAGE_ARCANE:
        case TALENT_TREE_MAGE_FIRE:
        case TALENT_TREE_MAGE_FROST:
        case TALENT_TREE_WARLOCK_AFFLICTION:
        case TALENT_TREE_WARLOCK_DEMONOLOGY:
        case TALENT_TREE_WARLOCK_DESTRUCTION:
        case TALENT_TREE_DRUID_BALANCE:
        case TALENT_TREE_DRUID_RESTORATION:
            if (item.SubClass == ITEM_SUBCLASS_WEAPON_STAFF || item.SubClass == ITEM_SUBCLASS_WEAPON_DAGGER ||
                item.SubClass == ITEM_SUBCLASS_WEAPON_MACE || item.SubClass == ITEM_SUBCLASS_WEAPON_SWORD ||
                item.SubClass == ITEM_SUBCLASS_WEAPON_WAND)
                score += 25.0f;
            break;
        default:
            break;
    }

    return score;
}

bool IsRewardIdentityStat(RewardRole role, uint32 stat)
{
    switch (role)
    {
        case RewardRole::StrengthMelee:
            return stat == ITEM_MOD_STRENGTH || stat == ITEM_MOD_ATTACK_POWER;
        case RewardRole::AgilityMelee:
            return stat == ITEM_MOD_AGILITY || stat == ITEM_MOD_ATTACK_POWER;
        case RewardRole::HunterRanged:
            return stat == ITEM_MOD_AGILITY || stat == ITEM_MOD_ATTACK_POWER || stat == ITEM_MOD_RANGED_ATTACK_POWER;
        case RewardRole::SpellDamage:
            return stat == ITEM_MOD_SPELL_POWER || stat == ITEM_MOD_INTELLECT;
        case RewardRole::Healer:
            return stat == ITEM_MOD_SPELL_POWER || stat == ITEM_MOD_INTELLECT ||
                stat == ITEM_MOD_MANA_REGENERATION || stat == ITEM_MOD_SPIRIT;
        case RewardRole::Tank:
            return stat == ITEM_MOD_STAMINA || stat == ITEM_MOD_DEFENSE_SKILL_RATING ||
                stat == ITEM_MOD_DODGE_RATING || stat == ITEM_MOD_PARRY_RATING ||
                stat == ITEM_MOD_BLOCK_RATING || stat == ITEM_MOD_BLOCK_VALUE;
        default:
            return true;
    }
}

float GetRewardWrongDirectionPenalty(RewardRole role, uint32 stat, int32 value)
{
    if (value <= 0)
        return 0.0f;

    float amount = static_cast<float>(value);
    switch (role)
    {
        case RewardRole::StrengthMelee:
            if (stat == ITEM_MOD_INTELLECT || stat == ITEM_MOD_SPIRIT || stat == ITEM_MOD_SPELL_POWER ||
                stat == ITEM_MOD_MANA_REGENERATION)
                return -std::min(20.0f, amount * 0.75f);
            break;
        case RewardRole::AgilityMelee:
        case RewardRole::HunterRanged:
            if (stat == ITEM_MOD_STRENGTH || stat == ITEM_MOD_SPIRIT || stat == ITEM_MOD_SPELL_POWER ||
                stat == ITEM_MOD_MANA_REGENERATION)
                return -std::min(20.0f, amount * 0.75f);
            break;
        case RewardRole::SpellDamage:
        case RewardRole::Healer:
            if (stat == ITEM_MOD_STRENGTH || stat == ITEM_MOD_AGILITY || stat == ITEM_MOD_ATTACK_POWER ||
                stat == ITEM_MOD_RANGED_ATTACK_POWER)
                return -std::min(25.0f, amount * 0.85f);
            break;
        case RewardRole::Tank:
            if (stat == ITEM_MOD_SPIRIT || stat == ITEM_MOD_SPELL_POWER || stat == ITEM_MOD_MANA_REGENERATION)
                return -std::min(15.0f, amount * 0.60f);
            break;
        default:
            break;
    }

    return 0.0f;
}

float ScoreRewardItem(Player* player, uint32 spec, RewardRole role, ItemTemplate const& item)
{
    float score = 0.0f;

    // Being near the hunter's level matters, but it is intentionally weaker
    // than spec/stat suitability. A useful level-12 item beats a nonsense
    // level-15 item for a level-15 hunter.
    int32 levelGap = static_cast<int32>(player->GetLevel()) - static_cast<int32>(item.RequiredLevel);
    score += std::max(0.0f, 15.0f - static_cast<float>(std::max(0, levelGap)) * 2.0f);
    score += GetArmorPreference(player, item);
    score += GetWeaponPreference(player, spec, item);

    bool hasIdentityStat = role == RewardRole::Generic;
    bool hasAnyPositiveStat = false;
    for (uint32 i = 0; i < item.StatsCount && i < MAX_ITEM_PROTO_STATS; ++i)
    {
        int32 value = item.ItemStat[i].ItemStatValue;
        if (value <= 0)
            continue;

        hasAnyPositiveStat = true;
        uint32 stat = item.ItemStat[i].ItemStatType;
        if (IsRewardIdentityStat(role, stat))
            hasIdentityStat = true;

        score += GetRewardStatWeight(role, stat) * static_cast<float>(value);
        score += GetRewardWrongDirectionPenalty(role, stat, value);
    }

    // 0.6.2: secondary stats may improve a good item, but cannot define the
    // item's role by themselves. This prevents crit-only/stamina-only pieces
    // from outranking true caster, melee, healer, or tank gear simply because
    // one supporting stat happens to be desirable. Weapons with no explicit
    // stats still rely on their strong spec-specific weapon preference.
    if (role != RewardRole::Generic)
    {
        if (hasIdentityStat)
            score += 24.0f;
        else if (hasAnyPositiveStat)
            score -= 28.0f;
        else if (item.Class != ITEM_CLASS_WEAPON)
            score -= 18.0f;
    }

    return score;
}
}

namespace hunts
{

bool HuntManager::IsElitePreyEntry(uint32 creatureEntry) const
{
    if (!creatureEntry)
        return false;

    for (auto const& [preyId, hunt] : _hunts)
    {
        (void)preyId;
        if (hunt.Enabled && hunt.Tier >= 2 && ResolvePreyEntry(hunt) == creatureEntry)
            return true;
    }

    return false;
}

HuntManager& HuntManager::Instance()
{
    static HuntManager instance;
    return instance;
}

void HuntManager::Configure(bool enabled, uint8 minimumLevel, float xpMultiplier, HuntSearchScope searchScope, bool debug)
{
    _enabled = enabled;
    _minimumLevel = std::max<uint8>(1, minimumLevel);
    _xpMultiplier = std::max(0.0f, xpMultiplier);
    _searchScope = searchScope;
    _debug = debug;
}

void HuntManager::Reset()
{
    _hunts.clear();
    _preyAbilities.clear();
    _abilityTimers.clear();
    _abilityUsed.clear();
    _zones.clear();
    _finalLocations.clear();
    _giverEntries.clear();
    _givers.clear();
    _guardLocators.clear();
    _giverLocalZones.clear();
    _runtimes.clear();
    _updateTimerMs = 0;
    _finalPoiRefreshTimerMs = 0;
}

void HuntManager::LoadDefinitions()
{
    _hunts.clear();
    _preyAbilities.clear();
    _abilityTimers.clear();
    _abilityUsed.clear();
    _zones.clear();
    _finalLocations.clear();
    _giverEntries.clear();
    _givers.clear();
    _guardLocators.clear();
    _giverLocalZones.clear();

    if (!_enabled)
        return;

    if (QueryResult result = WorldDatabase.Query(
        "SELECT `id`,`name`,`min_level`,`max_level`,`prey_creature_entry`,`prey_template_id`,`activation_gameobject_entry`,`ambush_health_multiplier`,`final_health_multiplier`,`reward_multiplier`,`tier`,`escape_health_pct`,`ambush_count`,`enabled` FROM `hunt_prey` WHERE `enabled`=1"))
    {
        do
        {
            Field* f = result->Fetch();
            HuntDefinition d;
            d.Id=f[0].Get<uint32>(); d.Name=f[1].Get<std::string>(); d.MinLevel=f[2].Get<uint8>(); d.MaxLevel=f[3].Get<uint8>();
            d.PreyCreatureEntry=f[4].Get<uint32>(); d.PreyTemplateId=f[5].Get<uint32>(); d.ActivationGameObjectEntry=f[6].Get<uint32>();
            d.AmbushHealthMultiplier=f[7].Get<float>(); d.FinalHealthMultiplier=f[8].Get<float>(); d.RewardMultiplier=std::max(0.0f, f[9].Get<float>()); d.Tier=f[10].Get<uint8>();
            d.EscapeHealthPct=f[11].Get<uint8>(); d.AmbushCount=f[12].Get<uint8>(); d.Enabled=f[13].Get<uint8>()!=0;
            _hunts[d.Id]=d;
        } while (result->NextRow());
    }

    if (QueryResult result = WorldDatabase.Query(
        "SELECT `id`,`prey_id`,`spell_id`,`target`,`initial_min_ms`,`initial_max_ms`,`cooldown_min_ms`,`cooldown_max_ms`,`chance_pct`,`encounter_mask`,"
        "`min_hunter_level`,`max_hunter_level`,`health_below_pct`,`require_melee`,`once_per_encounter`,`require_aura_missing`,`enabled` "
        "FROM `hunt_prey_ability` WHERE `enabled`=1 ORDER BY `prey_id`,`id`"))
    {
        do
        {
            Field* f = result->Fetch();
            HuntPreyAbilityDefinition d;
            d.Id = f[0].Get<uint32>(); d.PreyId = f[1].Get<uint32>(); d.SpellId = f[2].Get<uint32>(); d.Target = f[3].Get<uint8>();
            d.InitialMinMs = f[4].Get<uint32>(); d.InitialMaxMs = f[5].Get<uint32>();
            d.CooldownMinMs = f[6].Get<uint32>(); d.CooldownMaxMs = f[7].Get<uint32>();
            d.ChancePct = f[8].Get<uint8>(); d.EncounterMask = f[9].Get<uint8>();
            d.MinHunterLevel=f[10].Get<uint8>(); d.MaxHunterLevel=f[11].Get<uint8>(); d.HealthBelowPct=f[12].Get<uint8>();
            d.RequireMelee=f[13].Get<uint8>()!=0; d.OncePerEncounter=f[14].Get<uint8>()!=0;
            d.RequireAuraMissing=f[15].Get<uint8>()!=0; d.Enabled = f[16].Get<uint8>() != 0;
            _preyAbilities[d.PreyId].push_back(d);
        } while (result->NextRow());
    }

    if (QueryResult result = WorldDatabase.Query(
        "SELECT `id`,`zone_id`,`map_id`,`continent_id`,`name`,`min_level`,`max_level`,`weight`,`enabled` FROM `hunt_zone` WHERE `enabled`=1"))
    {
        do
        {
            Field* f=result->Fetch(); HuntZoneDefinition d;
            d.Id=f[0].Get<uint32>(); d.ZoneId=f[1].Get<uint32>(); d.MapId=f[2].Get<uint16>(); d.ContinentId=f[3].Get<uint8>(); d.Name=f[4].Get<std::string>();
            d.MinLevel=f[5].Get<uint8>(); d.MaxLevel=f[6].Get<uint8>(); d.Weight=f[7].Get<uint32>(); d.Enabled=f[8].Get<uint8>()!=0;
            _zones.push_back(d);
        } while(result->NextRow());
    }

    if (QueryResult result = WorldDatabase.Query(
        "SELECT `id`,`zone_id`,`map_id`,`x`,`y`,`z`,`orientation`,`location_name`,`min_level`,`max_level`,`weight`,`enabled` FROM `hunt_final_location` WHERE `enabled`=1"))
    {
        do
        {
            Field* f=result->Fetch(); HuntFinalLocationDefinition d;
            d.Id=f[0].Get<uint32>(); d.ZoneId=f[1].Get<uint32>(); d.MapId=f[2].Get<uint16>(); d.X=f[3].Get<float>(); d.Y=f[4].Get<float>(); d.Z=f[5].Get<float>(); d.Orientation=f[6].Get<float>(); d.LocationName=f[7].Get<std::string>(); d.MinLevel=f[8].Get<uint8>(); d.MaxLevel=f[9].Get<uint8>(); d.Weight=f[10].Get<uint32>(); d.Enabled=f[11].Get<uint8>()!=0;
            _finalLocations.push_back(d);
        } while(result->NextRow());
    }

    // 0.6.4.8: startup analyzes every auto-derived final site from one bulk
    // creature-spawn read instead of issuing one spatial SQL query per site.
    // The in-game authoring command still analyzes only its newly added point.
    struct NearbyMobLevelSample
    {
        uint16 MapId = 0;
        float X = 0.0f;
        float Y = 0.0f;
        uint8 MinLevel = 1;
        uint8 MaxLevel = 1;
    };

    std::unordered_map<uint16, std::vector<NearbyMobLevelSample>> mobSamplesByMap;
    uint32 loadedMobSamples = 0;

    if (QueryResult result = WorldDatabase.Query(
        "SELECT c.`map`,c.`position_x`,c.`position_y`,ct.`minlevel`,ct.`maxlevel` "
        "FROM `creature` c JOIN `creature_template` ct ON ct.`entry`=c.`id` "
        "WHERE ct.`npcflag`=0 AND ct.`rank`=0 AND ct.`type`<>8 AND ct.`maxlevel`>=5"))
    {
        do
        {
            Field* f = result->Fetch();
            NearbyMobLevelSample sample;
            sample.MapId = f[0].Get<uint16>();
            sample.X = f[1].Get<float>();
            sample.Y = f[2].Get<float>();
            sample.MinLevel = f[3].Get<uint8>();
            sample.MaxLevel = f[4].Get<uint8>();
            mobSamplesByMap[sample.MapId].push_back(sample);
            ++loadedMobSamples;
        } while (result->NextRow());
    }

    LOG_INFO("server.loading", "[Hunts] Bulk-loaded {} ordinary creature spawn sample(s) for final-site auto-level analysis.",
        loadedMobSamples);

    constexpr uint32 MinAutoLevelSamples = 8;
    uint32 autoGood = 0;
    uint32 autoSparse = 0;
    uint32 autoEmpty = 0;
    uint32 autoOutside = 0;

    for (HuntFinalLocationDefinition& location : _finalLocations)
    {
        if (location.MinLevel && location.MaxLevel && !location.AutoDerivedLevels)
            continue;

        uint32 samples = 0;
        double minLevelSum = 0.0;
        double maxLevelSum = 0.0;

        auto const mapItr = mobSamplesByMap.find(location.MapId);
        if (mapItr != mobSamplesByMap.end())
        {
            for (NearbyMobLevelSample const& mob : mapItr->second)
            {
                float const dx = mob.X - location.X;
                float const dy = mob.Y - location.Y;
                if ((dx * dx + dy * dy) > 40000.0f)
                    continue;

                ++samples;
                minLevelSum += mob.MinLevel;
                maxLevelSum += mob.MaxLevel;
            }
        }

        double const avgMin = samples ? (minLevelSum / samples) : 0.0;
        double const avgMax = samples ? (maxLevelSum / samples) : 0.0;
        ApplyFinalLocationLevelAnalysis(location, samples, avgMin, avgMax);

        if (!location.NearbyMobSamples)
            ++autoEmpty;
        else if (location.NearbyMobSamples < MinAutoLevelSamples)
            ++autoSparse;
        else if (!location.LevelSelectionEligible)
            ++autoOutside;
        else
            ++autoGood;
    }

    LOG_INFO("server.loading", "[Hunts] Auto-level final sites: {} good, {} sparse, {} no-mob, {} outside-zone.",
        autoGood, autoSparse, autoEmpty, autoOutside);

    if (QueryResult result = WorldDatabase.Query(
        "SELECT `id`,`creature_entry`,`city_name`,`map_id`,`continent_id`,`x`,`y`,`z`,`enabled` FROM `hunt_giver` WHERE `enabled`=1"))
    {
        do
        {
            Field* f=result->Fetch(); HuntGiverDefinition d;
            d.Id=f[0].Get<uint32>(); d.CreatureEntry=f[1].Get<uint32>(); d.CityName=f[2].Get<std::string>(); d.MapId=f[3].Get<uint16>(); d.ContinentId=f[4].Get<uint8>();
            d.X=f[5].Get<float>(); d.Y=f[6].Get<float>(); d.Z=f[7].Get<float>(); d.Enabled=f[8].Get<uint8>()!=0;
            _giverEntries[d.CreatureEntry]=d.Id; _givers[d.Id]=d;
        } while(result->NextRow());
    }

    if (QueryResult result = WorldDatabase.Query("SELECT `guard_creature_entry`,`hunt_giver_id` FROM `hunt_guard_locator` WHERE `enabled`=1"))
    {
        do { Field* f=result->Fetch(); _guardLocators[f[0].Get<uint32>()]=f[1].Get<uint32>(); } while(result->NextRow());
    }

    // 0.6.4.10: a city can have several different information-giver creature
    // entries that expose the same stock directions menu.  Treat every
    // creature_template using an explicitly registered locator's gossip menu
    // as an alias for the same Huntmaster.  This keeps the table as a small set
    // of authoritative seeds instead of requiring us to enumerate every city
    // information NPC by hand.
    uint32 discoveredGuardAliases = 0;
    if (QueryResult result = WorldDatabase.Query(
        "SELECT DISTINCT sibling.`entry`, gl.`hunt_giver_id` "
        "FROM `hunt_guard_locator` gl "
        "JOIN `creature_template` seed ON seed.`entry`=gl.`guard_creature_entry` "
        "JOIN `creature_template` sibling ON sibling.`gossip_menu_id`=seed.`gossip_menu_id` "
        "WHERE gl.`enabled`=1 AND seed.`gossip_menu_id`<>0 AND sibling.`gossip_menu_id`<>0"))
    {
        do
        {
            Field* f = result->Fetch();
            uint32 const entry = f[0].Get<uint32>();
            uint32 const giverId = f[1].Get<uint32>();
            if (_guardLocators.emplace(entry, giverId).second)
                ++discoveredGuardAliases;
        } while (result->NextRow());
    }

    LOG_INFO("module", "[Hunts] Discovered {} additional city-direction NPC alias(es) from registered guard gossip menus.",
        discoveredGuardAliases);

    if (QueryResult result = WorldDatabase.Query("SELECT `hunt_giver_id`,`zone_id` FROM `hunt_local_region_zone` WHERE `enabled`=1"))
    {
        do { Field* f=result->Fetch(); _giverLocalZones[f[0].Get<uint32>()].push_back(f[1].Get<uint32>()); } while(result->NextRow());
    }

    size_t abilityCount = 0;
    for (auto const& [preyId, abilities] : _preyAbilities)
        abilityCount += abilities.size();

    LOG_INFO("server.loading", "[Hunts] Loaded {} prey definition(s), {} prey ability row(s), {} zone(s), {} final location(s), {} Huntmaster(s), and {} guard locator entry(s).",
        _hunts.size(), abilityCount, _zones.size(), _finalLocations.size(), _giverEntries.size(), _guardLocators.size());
}

void HuntManager::Initialize()
{
    if (_enabled)
        LoadRuntimes();
}

void HuntManager::LoadRuntimes()
{
    _runtimes.clear();
    if (QueryResult result = CharacterDatabase.Query("SELECT `guid`,`prey_id`,`giver_entry`,`giver_spawn_id`,`zone_id`,`final_location_id`,`tracking_progress`,`ambushes_completed`,`state` FROM `hunt_runtime`"))
    {
        do
        {
            Field* f=result->Fetch(); HuntRuntime r;
            r.CharacterGuid=f[0].Get<uint32>(); r.PreyId=f[1].Get<uint32>(); r.GiverEntry=f[2].Get<uint32>(); r.GiverSpawnId=f[3].Get<uint32>(); r.ZoneId=f[4].Get<uint32>(); r.FinalLocationId=f[5].Get<uint32>(); r.TrackingProgress=f[6].Get<uint8>(); r.AmbushesCompleted=f[7].Get<uint8>(); r.State=static_cast<HuntState>(f[8].Get<uint8>());
            _runtimes[r.CharacterGuid]=r;
        } while(result->NextRow());
    }
    LOG_INFO("server.loading", "[Hunts] Restored {} active hunt runtime(s).", _runtimes.size());
}

void HuntManager::SaveRuntime(HuntRuntime const& r)
{
    // Hunt state transitions are small but gameplay-critical. Persist them
    // synchronously so a restart/logout cannot leave the database one state
    // behind the in-memory runtime (for example tracking=100/state=1 while
    // memory has already advanced to FinalLocated).
    std::ostringstream sql;
    sql << "REPLACE INTO `hunt_runtime` "
        << "(`guid`,`prey_id`,`giver_entry`,`giver_spawn_id`,`zone_id`,`final_location_id`,`tracking_progress`,`ambushes_completed`,`state`) VALUES ("
        << r.CharacterGuid << ',' << r.PreyId << ',' << r.GiverEntry << ',' << r.GiverSpawnId << ','
        << r.ZoneId << ',' << r.FinalLocationId << ',' << uint32(r.TrackingProgress) << ','
        << uint32(r.AmbushesCompleted) << ',' << static_cast<uint32>(r.State) << ')';
    CharacterDatabase.DirectExecute(sql.str().c_str());
}

void HuntManager::DeleteRuntime(uint32 guid)
{
    CharacterDatabase.Execute("DELETE FROM `hunt_runtime` WHERE `guid`={}", guid);
    _abilityTimers.erase(guid);
    _runtimes.erase(guid);
}

bool HuntManager::HasActiveHunt(Player const* player) const { return GetRuntime(player)!=nullptr; }
HuntRuntime const* HuntManager::GetRuntime(Player const* player) const
{
    if(!player) return nullptr; auto it=_runtimes.find(player->GetGUID().GetCounter()); return it==_runtimes.end()?nullptr:&it->second;
}
HuntDefinition const* HuntManager::GetDefinition(uint32 id) const { auto it=_hunts.find(id); return it==_hunts.end()?nullptr:&it->second; }
bool HuntManager::IsHuntGiver(uint32 entry) const { return _giverEntries.find(entry)!=_giverEntries.end(); }
bool HuntManager::IsGuardLocator(uint32 entry) const { return _guardLocators.find(entry)!=_guardLocators.end(); }

HuntZoneDefinition const* HuntManager::GetZone(uint32 zoneId) const
{
    for (auto const& zone : _zones)
        if (zone.ZoneId == zoneId && zone.Enabled)
            return &zone;
    return nullptr;
}

HuntZoneDefinition const* HuntManager::SelectZone(uint8 level, HuntGiverDefinition const& giver) const
{
    std::vector<HuntZoneDefinition const*> eligible;
    uint64 totalWeight = 0;
    for (auto const& zone : _zones)
    {
        if (!zone.Enabled || level < zone.MinLevel || level > zone.MaxLevel)
            continue;

        if (_searchScope == HuntSearchScope::Continent && zone.ContinentId != giver.ContinentId)
            continue;

        if (_searchScope == HuntSearchScope::LocalRegion)
        {
            auto local = _giverLocalZones.find(giver.Id);
            if (local == _giverLocalZones.end() || std::find(local->second.begin(), local->second.end(), zone.ZoneId) == local->second.end())
                continue;
        }

        bool hasFinalSite = false;
        for (auto const& location : _finalLocations)
            if (location.Enabled && location.ZoneId == zone.ZoneId)
            {
                hasFinalSite = true;
                break;
            }

        if (!hasFinalSite)
            continue;

        eligible.push_back(&zone);
        totalWeight += std::max<uint32>(1, zone.Weight);
    }

    if (eligible.empty())
        return nullptr;

    uint64 roll = urand(1, static_cast<uint32>(std::min<uint64>(totalWeight, std::numeric_limits<uint32>::max())));
    for (HuntZoneDefinition const* zone : eligible)
    {
        uint32 weight = std::max<uint32>(1, zone->Weight);
        if (roll <= weight)
            return zone;
        roll -= weight;
    }
    return eligible.back();
}

HuntFinalLocationDefinition const* HuntManager::SelectFinalLocation(HuntRuntime const& runtime, uint8 hunterLevel) const
{
    std::vector<HuntFinalLocationDefinition const*> eligible;
    uint32 totalWeight = 0;
    for(auto const& l:_finalLocations)
        if(l.ZoneId==runtime.ZoneId && l.Enabled && l.LevelSelectionEligible && hunterLevel >= l.MinLevel && hunterLevel <= l.MaxLevel)
        {
            eligible.push_back(&l);
            totalWeight += std::max<uint32>(1, l.Weight);
        }

    // Never strand an existing hunt because authored/derived coverage has a gap.
    // Prefer the closest level band in the assigned zone as a graceful fallback.
    if (eligible.empty())
    {
        uint32 bestDistance = std::numeric_limits<uint32>::max();
        for (auto const& l : _finalLocations)
        {
            if (l.ZoneId != runtime.ZoneId || !l.Enabled || !l.LevelSelectionEligible)
                continue;
            uint32 distance = hunterLevel < l.MinLevel ? l.MinLevel - hunterLevel :
                (hunterLevel > l.MaxLevel ? hunterLevel - l.MaxLevel : 0);
            if (distance < bestDistance)
            {
                eligible.clear(); totalWeight = 0; bestDistance = distance;
            }
            if (distance == bestDistance)
            {
                eligible.push_back(&l);
                totalWeight += std::max<uint32>(1, l.Weight);
            }
        }
    }

    // If every site in the zone was rejected as suspicious, preserve the older
    // never-strand behavior and choose from all enabled sites as a last resort.
    if (eligible.empty())
    {
        for (auto const& l : _finalLocations)
        {
            if (l.ZoneId != runtime.ZoneId || !l.Enabled)
                continue;
            eligible.push_back(&l);
            totalWeight += std::max<uint32>(1, l.Weight);
        }
    }

    if(eligible.empty()) return nullptr;
    uint32 roll=urand(1,totalWeight);
    for(auto const* l:eligible)
    {
        uint32 weight=std::max<uint32>(1,l->Weight);
        if(roll<=weight) return l;
        roll-=weight;
    }
    return eligible.back();
}

bool HuntManager::SendHuntmasterLocation(Player* player, uint32 guardEntry, std::string& message) const
{
    if (!player) { message = "Player required."; return false; }
    auto locator = _guardLocators.find(guardEntry);
    if (locator == _guardLocators.end()) { message = "That guard does not know a Huntmaster location."; return false; }
    auto giver = _givers.find(locator->second);
    if (giver == _givers.end() || !giver->second.Enabled) { message = "The Huntmaster location is unavailable."; return false; }

    HuntGiverDefinition const& g = giver->second;
    if (player->GetMapId() != g.MapId) { message = "The Huntmaster is not on this map."; return false; }

    WorldPacket poi(SMSG_GOSSIP_POI, 64);
    poi << uint32(6) << float(g.X) << float(g.Y) << uint32(7) << uint32(0) << std::string("Huntmaster - ") + g.CityName;
    player->GetSession()->SendPacket(&poi);
    message = "The Huntmaster has been marked on your map.";
    return true;
}

bool HuntManager::RequestHunt(Player* player, Creature* giver, std::string& message)
{
    if(!_enabled){message="The Hunt system is disabled.";return false;}
    if(!player||!giver||!IsHuntGiver(giver->GetEntry())){message="That creature is not a Living World Huntmaster.";return false;}
    if(player->GetLevel()<_minimumLevel){message="You must be at least level "+std::to_string(_minimumLevel)+" to take a hunt.";return false;}
    if(HasActiveHunt(player)){message="You already have an active hunt.";return false;}

    std::vector<HuntDefinition const*> eligible;
    for(auto const& [id,h]:_hunts)
        if(h.Enabled && h.Tier == 1 && player->GetLevel()>=h.MinLevel && player->GetLevel()<=h.MaxLevel)
            eligible.push_back(&h);
    if(eligible.empty()){message="I have no suitable prey for you right now.";return false;}

    auto giverIdIt = _giverEntries.find(giver->GetEntry());
    auto giverDefIt = giverIdIt == _giverEntries.end() ? _givers.end() : _givers.find(giverIdIt->second);
    if (giverDefIt == _givers.end()) { message="This Huntmaster is not configured correctly."; return false; }
    HuntZoneDefinition const* zone=SelectZone(player->GetLevel(), giverDefIt->second);
    if(!zone){message="I have no suitable prey within the configured hunting range for your level.";return false;}

    HuntDefinition const& hunt=*eligible[urand(0,static_cast<uint32>(eligible.size()-1))];
    HuntRuntime r; r.CharacterGuid=player->GetGUID().GetCounter(); r.PreyId=hunt.Id; r.GiverEntry=giver->GetEntry(); r.GiverSpawnId=giver->GetSpawnId(); r.ZoneId=zone->ZoneId; r.State=HuntState::Tracking;
    _runtimes[r.CharacterGuid]=r; SaveRuntime(r);
    message="Your quarry is "+hunt.Name+". Travel to "+zone->Name+" and hunt normally; signs of your prey will reveal themselves.";
    return true;
}

bool HuntManager::IsEliteUnlocked(Player const* player) const
{
    if (!player)
        return false;
    if (QueryResult q = CharacterDatabase.Query(
        "SELECT `total_completed` FROM `hunt_stats` WHERE `guid`={}", player->GetGUID().GetCounter()))
        return q->Fetch()[0].Get<uint32>() >= 10;
    return false;
}

bool HuntManager::IsEliteAvailableToday(Player const* player) const
{
    if (!player)
        return false;
    if (QueryResult q = CharacterDatabase.Query(
        "SELECT IF(`elite_daily_reset_date`=CURRENT_DATE(),`elite_daily_completed`,0) "
        "FROM `hunt_stats` WHERE `guid`={}", player->GetGUID().GetCounter()))
        return q->Fetch()[0].Get<uint32>() == 0;
    return true;
}

bool HuntManager::RequestEliteHunt(Player* player, Creature* giver, std::string& message)
{
    if(!_enabled){message="The Hunt system is disabled.";return false;}
    if(!player||!giver||!IsHuntGiver(giver->GetEntry())){message="That creature is not a Living World Huntmaster.";return false;}
    if(player->GetLevel()<_minimumLevel){message="You are not yet ready for an Elite Hunt.";return false;}
    if(HasActiveHunt(player)){message="You already have an active hunt.";return false;}
    if(!IsEliteUnlocked(player)){message="Elite Hunts unlock after 10 completed normal hunts.";return false;}
    if(!IsEliteAvailableToday(player)){message="You have already completed an Elite Hunt today. Return tomorrow for another challenge.";return false;}

    std::vector<HuntDefinition const*> eligible;
    for(auto const& [id,h]:_hunts)
        if(h.Enabled && h.Tier == 2 && player->GetLevel()>=h.MinLevel && player->GetLevel()<=h.MaxLevel)
            eligible.push_back(&h);
    if(eligible.empty()){message="I have no Elite prey suitable for you right now.";return false;}

    auto giverIdIt=_giverEntries.find(giver->GetEntry());
    auto giverDefIt=giverIdIt==_giverEntries.end()?_givers.end():_givers.find(giverIdIt->second);
    if(giverDefIt==_givers.end()){message="This Huntmaster is not configured correctly.";return false;}
    HuntZoneDefinition const* zone=SelectZone(player->GetLevel(),giverDefIt->second);
    if(!zone){message="I have no suitable Elite hunting ground for your level.";return false;}

    HuntDefinition const& hunt=*eligible[urand(0,static_cast<uint32>(eligible.size()-1))];
    HuntRuntime r; r.CharacterGuid=player->GetGUID().GetCounter(); r.PreyId=hunt.Id; r.GiverEntry=giver->GetEntry();
    r.GiverSpawnId=giver->GetSpawnId(); r.ZoneId=zone->ZoneId; r.State=HuntState::Tracking;
    _runtimes[r.CharacterGuid]=r; SaveRuntime(r);
    message="Elite quarry: "+hunt.Name+". Travel to "+zone->Name+
        ". This prey is more dangerous than an ordinary Hunt target. At level 80, the final challenge is yours alone.";
    return true;
}

bool HuntManager::AbandonHunt(Player* player, std::string& message)
{
    if(!player||!HasActiveHunt(player)){message="You do not have an active hunt.";return false;}
    auto it=_runtimes.find(player->GetGUID().GetCounter());
    if(it!=_runtimes.end()) RemoveFinalActivator(player,it->second);
    DeleteRuntime(player->GetGUID().GetCounter()); message="Your hunt has been abandoned."; return true;
}

bool HuntManager::TurnInHunt(Player* player, Creature* giver, std::string& message)
{
    if(!player||!giver){message="Invalid hunt turn-in.";return false;}
    auto it=_runtimes.find(player->GetGUID().GetCounter()); if(it==_runtimes.end()){message="You have no hunt to turn in.";return false;}
    HuntRuntime const& r=it->second;
    if(r.State!=HuntState::ReadyToTurnIn){message="Your quarry still lives.";return false;}
    if(r.GiverEntry!=giver->GetEntry() || (r.GiverSpawnId && r.GiverSpawnId!=giver->GetSpawnId())){message="Return to the Huntmaster who gave you this hunt.";return false;}
    HuntRuntime& mutableRuntime=it->second; RemoveFinalActivator(player,mutableRuntime);

    HuntDefinition const* hunt = GetDefinition(r.PreyId);
    float rewardMultiplier = hunt ? hunt->RewardMultiplier : 1.0f;
    bool const eliteHunt = hunt && hunt->Tier == 2;

    // Determine how many hunts were already completed today before this turn-in.
    // Reward quality deliberately diminishes across repeated same-day hunts.
    uint32 dailyCompletedBefore = 0;
    if (QueryResult stats = CharacterDatabase.Query(
        "SELECT IF(`daily_reset_date`=CURRENT_DATE(),`daily_completed`,0) FROM `hunt_stats` WHERE `guid`={}", r.CharacterGuid))
        dailyCompletedBefore = stats->Fetch()[0].Get<uint32>();

    // XP baseline: 8% of the XP required for the player's current level, then
    // apply the server-wide Hunt XP multiplier and the prey reward multiplier.
    // This keeps Hunts useful while leaving normal questing as the default faster path.
    uint32 xpReward = 0;
    if (player->GetLevel() < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL) && _xpMultiplier > 0.0f)
    {
        uint32 nextLevelXp = player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
        xpReward = static_cast<uint32>(std::round(nextLevelXp * 0.08f * _xpMultiplier * rewardMultiplier));
        if (xpReward)
            player->GiveXP(xpReward, nullptr, 1.0f);
    }

    // Gold scales quadratically with level: 20 copper * level^2 at 1.0x.
    // Examples: level 10 = 20s, level 40 = 3g20s, level 80 = 12g80s.
    uint32 level = player->GetLevel();
    uint32 moneyReward = static_cast<uint32>(std::round(20.0f * level * level * rewardMultiplier));
    if (moneyReward)
        player->ModifyMoney(static_cast<int32>(moneyReward));

    // Roll item quality. First hunt: 80/19/1 green/blue/epic. Repeated hunts
    // progressively suppress high-quality rewards without removing item rewards.
    uint32 qualityRoll = urand(1, 1000);
    uint32 desiredQuality = ITEM_QUALITY_UNCOMMON;
    if (eliteHunt)
        desiredQuality = qualityRoll <= 100 ? ITEM_QUALITY_EPIC : ITEM_QUALITY_RARE; // 10% epic, otherwise guaranteed blue
    else if (dailyCompletedBefore == 0)
        desiredQuality = qualityRoll <= 10 ? ITEM_QUALITY_EPIC : (qualityRoll <= 200 ? ITEM_QUALITY_RARE : ITEM_QUALITY_UNCOMMON);
    else if (dailyCompletedBefore == 1)
        desiredQuality = qualityRoll <= 5 ? ITEM_QUALITY_EPIC : (qualityRoll <= 120 ? ITEM_QUALITY_RARE : ITEM_QUALITY_UNCOMMON);
    else if (dailyCompletedBefore == 2)
        desiredQuality = qualityRoll <= 2 ? ITEM_QUALITY_EPIC : (qualityRoll <= 60 ? ITEM_QUALITY_RARE : ITEM_QUALITY_UNCOMMON);
    else
        desiredQuality = qualityRoll <= 20 ? ITEM_QUALITY_RARE : ITEM_QUALITY_UNCOMMON;

    // Build a spec-aware pool from existing Blizzard equipment. First use the
    // core's own CanUseItem() rules as a hard gate (proficiency, class, level,
    // skill/reputation requirements, etc.), then score the survivors for the
    // hunter's active talent tree. This deliberately prefers the right *type*
    // of gear without requiring every Hunt reward to be an upgrade.
    struct ScoredRewardItem
    {
        uint32 ItemId = 0;
        float Score = 0.0f;
    };

    std::vector<ScoredRewardItem> scoredCandidates;
    uint32 minRequiredLevel = level > 5 ? level - 5 : 1;
    uint32 activeTalentTree = player->GetSpec();
    RewardRole rewardRole = GetRewardRole(player, activeTalentTree);

    for (auto const& [itemId, itemTemplate] : *sObjectMgr->GetItemTemplateStore())
    {
        if (itemTemplate.Quality != desiredQuality)
            continue;
        if (itemTemplate.Class != ITEM_CLASS_WEAPON && itemTemplate.Class != ITEM_CLASS_ARMOR)
            continue;
        if (itemTemplate.InventoryType == INVTYPE_NON_EQUIP || itemTemplate.InventoryType == INVTYPE_BAG ||
            itemTemplate.InventoryType == INVTYPE_TABARD || itemTemplate.InventoryType == INVTYPE_AMMO ||
            itemTemplate.InventoryType == INVTYPE_QUIVER)
            continue;
        if (itemTemplate.RequiredLevel > level || itemTemplate.RequiredLevel < minRequiredLevel)
            continue;
        if (player->CanUseItem(&itemTemplate) != EQUIP_ERR_OK)
            continue;

        scoredCandidates.push_back({itemId, ScoreRewardItem(player, activeTalentTree, rewardRole, itemTemplate)});
    }

    std::sort(scoredCandidates.begin(), scoredCandidates.end(), [](ScoredRewardItem const& a, ScoredRewardItem const& b)
    {
        return a.Score > b.Score;
    });

    // Keep some randomness so Hunt rewards do not collapse into the same item
    // every time. Pick from the strongest slice of the appropriate pool.
    std::vector<uint32> candidates;
    if (!scoredCandidates.empty())
    {
        float bestScore = scoredCandidates.front().Score;
        float cutoff = bestScore - 12.0f;
        for (ScoredRewardItem const& candidate : scoredCandidates)
        {
            if (candidate.Score < cutoff || candidates.size() >= 12)
                break;
            candidates.push_back(candidate.ItemId);
        }

        if (candidates.empty())
            candidates.push_back(scoredCandidates.front().ItemId);
    }

    uint32 rewardedItemId = 0;
    if (!candidates.empty())
    {
        rewardedItemId = candidates[urand(0, static_cast<uint32>(candidates.size() - 1))];
        ItemPosCountVec dest;
        InventoryResult storeResult = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, rewardedItemId, 1);
        if (storeResult == EQUIP_ERR_OK)
        {
            if (Item* item = player->StoreNewItem(dest, rewardedItemId, true))
                player->SendNewItem(item, 1, true, false);
            else
                rewardedItemId = 0;
        }
        else
        {
            // Do not block Hunt completion because the player's bags are full.
            // The reward remains XP/gold; the message explains the missing item.
            rewardedItemId = 0;
        }
    }

    char const* qualityColumn = nullptr;
    if (rewardedItemId)
    {
        if (desiredQuality == ITEM_QUALITY_EPIC) qualityColumn = "epics_received";
        else if (desiredQuality == ITEM_QUALITY_RARE) qualityColumn = "blues_received";
        else qualityColumn = "greens_received";
    }

    std::ostringstream statsSql;
    statsSql << "INSERT INTO `hunt_stats` (`guid`,`total_completed`,`daily_completed`,`daily_reset_date`,`greens_received`,`blues_received`,`epics_received`,`elite_total_completed`,`elite_daily_completed`,`elite_daily_reset_date`,`last_completed_at`) "
             << "VALUES (" << r.CharacterGuid << ",1,1,CURRENT_DATE(),"
             << (qualityColumn && std::string(qualityColumn)=="greens_received" ? 1 : 0) << ","
             << (qualityColumn && std::string(qualityColumn)=="blues_received" ? 1 : 0) << ","
             << (qualityColumn && std::string(qualityColumn)=="epics_received" ? 1 : 0) << ","
             << (eliteHunt ? 1 : 0) << "," << (eliteHunt ? 1 : 0) << ","
             << (eliteHunt ? "CURRENT_DATE()" : "NULL") << ",CURRENT_TIMESTAMP()) "
             << "ON DUPLICATE KEY UPDATE `total_completed`=`total_completed`+1, "
             << "`daily_completed`=IF(`daily_reset_date`=CURRENT_DATE(),`daily_completed`+1,1), "
             << "`daily_reset_date`=CURRENT_DATE(),";
    if (eliteHunt)
        statsSql << "`elite_total_completed`=`elite_total_completed`+1,"
                 << "`elite_daily_completed`=IF(`elite_daily_reset_date`=CURRENT_DATE(),`elite_daily_completed`+1,1),"
                 << "`elite_daily_reset_date`=CURRENT_DATE(),";
    if (qualityColumn)
        statsSql << "`" << qualityColumn << "`=`" << qualityColumn << "`+1,";
    statsSql << "`last_completed_at`=CURRENT_TIMESTAMP()";
    CharacterDatabase.DirectExecute(statsSql.str().c_str());

    std::ostringstream rewardMessage;
    rewardMessage << "A fine hunt. Reward: ";
    if (xpReward) rewardMessage << xpReward << " XP, ";
    rewardMessage << (moneyReward / 10000) << "g " << ((moneyReward / 100) % 100) << "s " << (moneyReward % 100) << "c";
    if (rewardedItemId)
    {
        if (ItemTemplate const* rewardTemplate = sObjectMgr->GetItemTemplate(rewardedItemId))
            rewardMessage << ", and " << rewardTemplate->Name1;
    }
    else if (candidates.empty())
        rewardMessage << ". No suitable item reward was found for this level/quality roll";
    else
        rewardMessage << ". Your bags were too full for the item reward";
    rewardMessage << ".";

    DeleteRuntime(r.CharacterGuid); message=rewardMessage.str(); return true;
}

uint8 HuntManager::GetNextAmbushThreshold(HuntRuntime const& r, HuntDefinition const& h) const
{
    if(r.AmbushesCompleted>=h.AmbushCount || h.AmbushCount==0) return 100;
    return static_cast<uint8>((100u*(r.AmbushesCompleted+1u))/(h.AmbushCount+1u));
}

void HuntManager::OnCreatureKill(Player* player, Creature* killed)
{
    if (!_enabled || !player || !killed)
        return;

    // Final prey belongs to the hunt runtime that spawned it, not to whichever
    // player happened to land the killing blow. Resolve encounter ownership by
    // GUID first so a grouped hunter can receive credit when a party member
    // finishes the prey. The creature's normal tap rules are also honored, so a
    // hunter who tagged the prey can still receive credit when another player
    // helps finish it.
    HuntRuntime* owningRuntime = nullptr;
    for (auto& [guid, runtime] : _runtimes)
    {
        if (runtime.ActivePreyGuid == killed->GetGUID())
        {
            owningRuntime = &runtime;
            break;
        }
    }

    if (owningRuntime)
    {
        HuntRuntime& ownerRuntime = *owningRuntime;
        bool finalEncounter = ownerRuntime.ActivePreyFinal;

        // Ambush prey remains owner-specific. Its death is not a successful
        // completion; ambushes are expected to escape at the configured HP
        // threshold. If one somehow dies, just clear the runtime creature state.
        if (!finalEncounter)
        {
            _abilityTimers.erase(ownerRuntime.CharacterGuid);
            ownerRuntime.ActivePreyGuid.Clear();
            SaveRuntime(ownerRuntime);
            return;
        }

        Player* owner = ObjectAccessor::FindConnectedPlayer(
            ObjectGuid::Create<HighGuid::Player>(ownerRuntime.CharacterGuid));

        bool ownerEligible = false;
        if (owner)
        {
            HuntDefinition const* ownerHunt = GetDefinition(ownerRuntime.PreyId);
            bool const maxLevelElite = ownerHunt && ownerHunt->Tier == 2 &&
                owner->GetLevel() >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
            bool sameGroup = !maxLevelElite && owner->GetGroup() && player->GetGroup() && owner->GetGroup() == player->GetGroup();
            ownerEligible = (owner == player) || sameGroup || killed->isTappedBy(owner);
        }

        if (ownerEligible)
        {
            uint32 preyId = ownerRuntime.PreyId;
            uint32 zoneId = ownerRuntime.ZoneId;
            auto* creditedGroup = owner->GetGroup();

            // Credit the encounter owner and any nearby party member who is in
            // the same final-stage hunt for the same prey/zone. This makes Tier-1
            // hunt encounters genuinely cooperative while outsiders may still
            // help without receiving hunt completion.
            for (auto& [guid, runtime] : _runtimes)
            {
                if (runtime.PreyId != preyId || runtime.ZoneId != zoneId || runtime.State != HuntState::FinalLocated)
                    continue;

                Player* hunter = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(guid));
                if (!hunter || hunter->GetMapId() != killed->GetMapId())
                    continue;

                bool isOwner = guid == ownerRuntime.CharacterGuid;
                HuntDefinition const* creditedHunt = GetDefinition(runtime.PreyId);
                bool const soloElite = creditedHunt && creditedHunt->Tier == 2 &&
                    owner->GetLevel() >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
                if (soloElite && !isOwner)
                    continue;
                bool groupedWithOwner = creditedGroup && hunter->GetGroup() == creditedGroup;
                bool tapped = killed->isTappedBy(hunter);
                if (!isOwner && !groupedWithOwner && !tapped)
                    continue;

                // Do not grant remote group credit from across the map.
                if (!isOwner && hunter->GetDistance(killed) > 200.0f)
                    continue;

                _abilityTimers.erase(runtime.CharacterGuid);

                // A second hunter on the same contract may already have spawned
                // their own copy. Remove it now that the shared kill satisfied
                // their contract as well.
                if (!runtime.ActivePreyGuid.IsEmpty() && runtime.ActivePreyGuid != killed->GetGUID())
                {
                    if (Creature* otherPrey = ObjectAccessor::GetCreature(*hunter, runtime.ActivePreyGuid))
                        otherPrey->DespawnOrUnsummon();
                }

                runtime.ActivePreyGuid.Clear();
                runtime.ActivePreyFinal = false;
                RemoveFinalActivator(hunter, runtime);
                runtime.State = HuntState::ReadyToTurnIn;
                SaveRuntime(runtime);

                ChatHandler(hunter->GetSession()).SendSysMessage(
                    "|cff00ff00[Hunts]|r Your quarry is dead. Return to the Huntmaster who gave you the contract.");
            }
        }
        else
        {
            // The prey was killed by an unrelated outsider and the hunter did
            // not have the tap. Leave the hunt in FinalLocated; once the corpse
            // disappears the activator may return and the hunter can try again.
            _abilityTimers.erase(ownerRuntime.CharacterGuid);
            ownerRuntime.ActivePreyGuid.Clear();
            ownerRuntime.ActivePreyFinal = false;
            SaveRuntime(ownerRuntime);
        }

        // Hunt prey should never also count as ordinary tracking progress.
        return;
    }

    // Ordinary tracking credit is shared with nearby party members. AzerothCore's
    // creature-kill hook identifies the player credited with the kill; without
    // propagating that event, a hunter grouped with other players/playerbots can
    // miss most tracking progress simply because somebody else landed the kill.
    //
    // Each eligible hunter is evaluated independently: they must be actively
    // tracking this zone, be on the same map, be within 100 yards of the kill,
    // and the creature must be non-grey to that hunter. Group size never divides
    // the normal 3-7% tracking gain.
    auto* killingGroup = player->GetGroup();

    for (auto& [guid, runtime] : _runtimes)
    {
        if (runtime.State != HuntState::Tracking)
            continue;

        Player* hunter = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(guid));
        if (!hunter)
            continue;

        bool isKiller = hunter == player;
        bool groupedWithKiller = killingGroup && hunter->GetGroup() == killingGroup;
        if (!isKiller && !groupedWithKiller)
            continue;

        if (hunter->GetMapId() != killed->GetMapId() || hunter->GetZoneId() != runtime.ZoneId)
            continue;

        if (hunter->GetDistance(killed) > 100.0f)
            continue;

        // Ordinary tracking progress only comes from creatures that are non-grey
        // to this hunter. Evaluate the XP color separately for every group member,
        // because the same creature can be green to one hunter and grey to another.
        if (Acore::XP::GetColorCode(hunter->GetLevel(), killed->GetLevel()) == XP_GRAY)
            continue;

        std::string ignored;
        AddProgress(hunter, static_cast<uint8>(urand(3, 7)), ignored);
    }
}

bool HuntManager::AddProgress(Player* player, uint8 amount, std::string& message)
{
    if(!player){message="Player required.";return false;}
    auto it=_runtimes.find(player->GetGUID().GetCounter()); if(it==_runtimes.end()){message="You have no active hunt.";return false;}
    HuntRuntime& r=it->second; if(r.State!=HuntState::Tracking){message="Tracking is already complete.";return false;}
    HuntDefinition const* h=GetDefinition(r.PreyId); if(!h){message="Hunt definition is missing.";return false;}
    uint8 old=r.TrackingProgress; r.TrackingProgress=static_cast<uint8>(std::min<uint32>(100, r.TrackingProgress+amount));
    uint8 threshold=GetNextAmbushThreshold(r,*h);
    if(r.TrackingProgress>=100)
    {
        if (!LocateFinal(player, r))
        {
            // Never persist a FinalLocated state without a valid authored site.
            // Keep tracking at 100% so the server can retry selection later.
            SaveRuntime(r);
            message="Tracking is complete, but no valid final hunt location is currently available.";
            return false;
        }
        message="Tracking reached 100%.";
        return true;
    }
    SaveRuntime(r);
    if(old<threshold && r.TrackingProgress>=threshold && r.AmbushesCompleted<h->AmbushCount)
    {
        std::string ambush; SpawnPrey(player,r,false,ambush); message=ambush; return true;
    }
    message="Tracking progress: "+std::to_string(r.TrackingProgress)+"%."; return true;
}

uint32 HuntManager::ResolvePreyEntry(HuntDefinition const& hunt) const
{
    if (hunt.PreyTemplateId)
        return sHuntCreatureTemplateMgr.ResolveEntry(hunt.PreyTemplateId);
    return hunt.PreyCreatureEntry;
}

void HuntManager::ApplyFinalLocationLevelAnalysis(
    HuntFinalLocationDefinition& location, uint32 samples, double avgMinLevel, double avgMaxLevel)
{
    constexpr uint32 MinAutoLevelSamples = 8;

    HuntZoneDefinition const* zone = GetZone(location.ZoneId);
    uint8 const fallbackMin = zone ? zone->MinLevel : 1;
    uint8 const fallbackMax = zone ? zone->MaxLevel : 80;

    location.MinLevel = fallbackMin;
    location.MaxLevel = fallbackMax;
    location.NearbyMobSamples = samples;
    location.AutoDerivedLevels = true;
    location.LevelSelectionEligible = true;

    if (!samples || samples < MinAutoLevelSamples)
        return;

    int32 const rawMin = static_cast<int32>(std::lround(avgMinLevel)) - 2;
    int32 const rawMax = static_cast<int32>(std::lround(avgMaxLevel)) + 2;
    if (rawMax < fallbackMin || rawMin > fallbackMax)
    {
        location.LevelSelectionEligible = false;
        return;
    }

    int32 const effectiveMin = std::max<int32>(rawMin, fallbackMin);
    int32 const effectiveMax = std::min<int32>(rawMax, fallbackMax);
    if (effectiveMin > effectiveMax)
    {
        location.LevelSelectionEligible = false;
        return;
    }

    location.MinLevel = static_cast<uint8>(effectiveMin);
    location.MaxLevel = static_cast<uint8>(effectiveMax);
}

void HuntManager::AnalyzeFinalLocationLevels(HuntFinalLocationDefinition& location)
{
    // Authored non-zero bounds are an explicit override.
    if (location.MinLevel && location.MaxLevel && !location.AutoDerivedLevels)
        return;

    QueryResult nearby = WorldDatabase.Query(
        "SELECT COUNT(*), AVG(ct.`minlevel`), AVG(ct.`maxlevel`) "
        "FROM `creature` c JOIN `creature_template` ct ON ct.`entry`=c.`id` "
        "WHERE c.`map`={} AND ct.`npcflag`=0 AND ct.`rank`=0 AND ct.`type`<>8 "
        "AND ct.`maxlevel`>=5 "
        "AND ((c.`position_x`-{})*(c.`position_x`-{}) + (c.`position_y`-{})*(c.`position_y`-{})) <= 40000",
        location.MapId, location.X, location.X, location.Y, location.Y);

    if (!nearby)
    {
        ApplyFinalLocationLevelAnalysis(location, 0, 0.0, 0.0);
        return;
    }

    Field* f = nearby->Fetch();
    uint32 const samples = f[0].Get<uint32>();
    double const avgMin = (!samples || f[1].IsNull()) ? 0.0 : f[1].Get<double>();
    double const avgMax = (!samples || f[2].IsNull()) ? 0.0 : f[2].Get<double>();
    ApplyFinalLocationLevelAnalysis(location, samples, avgMin, avgMax);
}

HuntFinalLocationDefinition const* HuntManager::GetFinalLocation(uint32 finalLocationId) const
{
    for (auto const& location : _finalLocations)
        if (location.Id == finalLocationId)
            return &location;
    return nullptr;
}

bool HuntManager::AddFinalLocationAtPlayer(Player* player, std::string& message)
{
    if (!player)
    {
        message = "[Hunts] This command must be used in game.";
        return false;
    }

    uint32 const zoneId = player->GetZoneId();
    uint16 const mapId = player->GetMapId();
    if (!zoneId)
    {
        message = "[Hunts] Cannot create a final location here because the current zone could not be resolved.";
        return false;
    }

    uint32 nextId = 1;
    if (QueryResult result = WorldDatabase.Query("SELECT COALESCE(MAX(`id`),0)+1 FROM `hunt_final_location`"))
        nextId = result->Fetch()[0].Get<uint32>();

    std::ostringstream insertSql;
    insertSql << "INSERT INTO `hunt_final_location` "
                 "(`id`,`zone_id`,`map_id`,`x`,`y`,`z`,`orientation`,`location_name`,`min_level`,`max_level`,`weight`,`enabled`,`comment`) VALUES ("
              << nextId << ',' << zoneId << ',' << mapId << ','
              << player->GetPositionX() << ',' << player->GetPositionY() << ',' << player->GetPositionZ() << ',' << player->GetOrientation()
              << ",'',0,0,100,1,'Added in-game with .hunt set final point')";
    WorldDatabase.DirectExecute(insertSql.str().c_str());

    // Analyze and register only the newly-created point. A full LoadDefinitions()
    // would re-run the nearby-mob query for every final location in the world.
    HuntFinalLocationDefinition createdLocation;
    createdLocation.Id = nextId;
    createdLocation.ZoneId = zoneId;
    createdLocation.MapId = mapId;
    createdLocation.X = player->GetPositionX();
    createdLocation.Y = player->GetPositionY();
    createdLocation.Z = player->GetPositionZ();
    createdLocation.Orientation = player->GetOrientation();
    createdLocation.MinLevel = 0;
    createdLocation.MaxLevel = 0;
    createdLocation.Weight = 100;
    createdLocation.Enabled = true;
    AnalyzeFinalLocationLevels(createdLocation);
    _finalLocations.push_back(createdLocation);

    std::string zoneName = "Unknown zone";
    if (AreaTableEntry const* zone = sAreaTableStore.LookupEntry(zoneId))
    {
        std::string const localized = zone->area_name[player->GetSession()->GetSessionDbcLocale()];
        if (!localized.empty())
            zoneName = localized;
    }

    std::string areaName;
    if (Map* map = player->GetMap())
    {
        uint32 const areaId = map->GetAreaId(player->GetPhaseMask(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ());
        if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(areaId))
            areaName = area->area_name[player->GetSession()->GetSessionDbcLocale()];
    }

    bool const configuredZone = GetZone(zoneId) != nullptr;
    HuntFinalLocationDefinition const* created = GetFinalLocation(nextId);

    std::ostringstream out;
    out << "[Hunts] Final hunt location created. ID: " << nextId
        << " | Zone: " << zoneName << " (" << zoneId << ")"
        << " | Map: " << mapId
        << " | XYZ: " << player->GetPositionX() << ", " << player->GetPositionY() << ", " << player->GetPositionZ()
        << " | O: " << player->GetOrientation();
    if (!areaName.empty())
        out << " | Area: " << areaName;
    if (created)
    {
        out << " | Levels: " << static_cast<uint32>(created->MinLevel) << '-' << static_cast<uint32>(created->MaxLevel);
        if (created->AutoDerivedLevels)
        {
            out << " auto (" << created->NearbyMobSamples << " nearby mobs";
            if (!created->LevelSelectionEligible)
                out << ", OUTSIDE_ZONE";
            out << ')';
        }
    }
    if (!configuredZone)
        out << " | WARNING: this zone is not currently defined/enabled in hunt_zone, so the point will not enter Hunt rotation yet.";

    message = out.str();
    return true;
}

std::string HuntManager::BuildFinalLocationList(Player const* player) const
{
    if (!player)
        return "[Hunts] This command must be used in game.";

    uint32 const zoneId = player->GetZoneId();
    std::string zoneName = "Unknown zone";
    if (AreaTableEntry const* zone = sAreaTableStore.LookupEntry(zoneId))
    {
        std::string const localized = zone->area_name[player->GetSession()->GetSessionDbcLocale()];
        if (!localized.empty())
            zoneName = localized;
    }

    std::ostringstream out;
    out << "[Hunts] Final locations for " << zoneName << " (zone " << zoneId << "):";
    uint32 count = 0;
    for (HuntFinalLocationDefinition const& location : _finalLocations)
    {
        if (location.ZoneId != zoneId || !location.Enabled)
            continue;

        out << "\n " << location.Id << " - "
            << location.X << ", " << location.Y << ", " << location.Z
            << " (map " << location.MapId << ")"
            << " | levels " << static_cast<uint32>(location.MinLevel) << '-' << static_cast<uint32>(location.MaxLevel);

        if (location.AutoDerivedLevels)
        {
            if (!location.NearbyMobSamples)
                out << " | auto NO_MOBS";
            else if (location.NearbyMobSamples < 8)
                out << " | auto SPARSE (" << location.NearbyMobSamples << " mobs)";
            else if (!location.LevelSelectionEligible)
                out << " | auto OUTSIDE_ZONE (" << location.NearbyMobSamples << " mobs)";
            else
                out << " | auto GOOD (" << location.NearbyMobSamples << " mobs)";
        }
        else
            out << " | authored";

        ++count;
    }

    if (!count)
        out << "\n No enabled final locations are currently defined for this zone.";
    else
        out << "\n[Hunts] " << count << " enabled final location(s).";

    return out.str();
}

std::string HuntManager::BuildFinalLocationNeeds(std::string const& zoneFilter) const
{
    constexpr uint32 MinAutoLevelSamples = 8;
    constexpr uint8 CoverageToleranceLevels = 1;

    auto lowerCopy = [](std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    };

    std::string const filter = lowerCopy(zoneFilter);
    bool const hasFilter = !filter.empty();

    auto zoneMatches = [&](HuntZoneDefinition const& zone)
    {
        if (!hasFilter)
            return true;

        if (filter == std::to_string(zone.ZoneId))
            return true;

        return lowerCopy(zone.Name).find(filter) != std::string::npos;
    };

    auto formatIdList = [](char const* label, std::vector<uint32> const& ids)
    {
        std::ostringstream out;
        if (ids.empty())
            return out.str();

        out << label << ' ';
        for (size_t i = 0; i < ids.size(); ++i)
        {
            if (i)
                out << ',';
            out << '#' << ids[i];
        }
        return out.str();
    };

    auto formatRanges = [](std::vector<std::pair<uint8, uint8>> const& ranges)
    {
        std::ostringstream out;
        for (size_t i = 0; i < ranges.size(); ++i)
        {
            if (i)
                out << ',';
            uint32 const first = ranges[i].first;
            uint32 const last = ranges[i].second;
            out << first;
            if (last != first)
                out << '-' << last;
        }
        return out.str();
    };

    struct ZoneNeed
    {
        HuntZoneDefinition const* Zone = nullptr;
        uint32 EnabledLocationCount = 0;
        std::vector<uint32> NoMobIds;
        std::vector<uint32> SparseIds;
        std::vector<uint32> OutsideIds;
        std::vector<std::pair<uint8, uint8>> UncoveredRanges;
    };

    uint32 matchingZones = 0;
    uint32 cleanZones = 0;
    std::vector<ZoneNeed> coverageNeeds;
    std::vector<ZoneNeed> reviewOnly;

    for (HuntZoneDefinition const& zone : _zones)
    {
        if (!zone.Enabled || !zoneMatches(zone))
            continue;

        ++matchingZones;
        ZoneNeed need;
        need.Zone = &zone;
        std::vector<HuntFinalLocationDefinition const*> trustedLocations;

        for (HuntFinalLocationDefinition const& location : _finalLocations)
        {
            if (!location.Enabled || location.ZoneId != zone.ZoneId)
                continue;

            ++need.EnabledLocationCount;

            if (!location.AutoDerivedLevels)
            {
                trustedLocations.push_back(&location);
                continue;
            }

            if (!location.NearbyMobSamples)
            {
                need.NoMobIds.push_back(location.Id);
                continue;
            }

            if (location.NearbyMobSamples < MinAutoLevelSamples)
            {
                need.SparseIds.push_back(location.Id);
                continue;
            }

            if (!location.LevelSelectionEligible)
            {
                need.OutsideIds.push_back(location.Id);
                continue;
            }

            trustedLocations.push_back(&location);
        }

        bool inGap = false;
        uint8 gapStart = 0;
        for (uint16 level = zone.MinLevel; level <= zone.MaxLevel; ++level)
        {
            bool covered = false;
            for (HuntFinalLocationDefinition const* location : trustedLocations)
            {
                uint16 const minLevel = location->MinLevel > CoverageToleranceLevels
                    ? static_cast<uint16>(location->MinLevel - CoverageToleranceLevels)
                    : 1;
                uint16 const maxLevel = std::min<uint16>(80, static_cast<uint16>(location->MaxLevel) + CoverageToleranceLevels);
                if (level >= minLevel && level <= maxLevel)
                {
                    covered = true;
                    break;
                }
            }

            if (!covered && !inGap)
            {
                inGap = true;
                gapStart = static_cast<uint8>(level);
            }
            else if (covered && inGap)
            {
                need.UncoveredRanges.emplace_back(gapStart, static_cast<uint8>(level - 1));
                inGap = false;
            }
        }

        if (inGap)
            need.UncoveredRanges.emplace_back(gapStart, zone.MaxLevel);

        bool const hasCoverageWork = !need.EnabledLocationCount || !need.UncoveredRanges.empty();
        bool const hasReview = !need.NoMobIds.empty() || !need.SparseIds.empty() || !need.OutsideIds.empty();

        if (!hasCoverageWork && !hasReview)
        {
            ++cleanZones;
            continue;
        }

        if (hasCoverageWork)
            coverageNeeds.push_back(std::move(need));
        else
            reviewOnly.push_back(std::move(need));
    }

    std::ostringstream out;
    if (hasFilter && !matchingZones)
    {
        out << "[Hunts] No enabled Hunt zone matched '" << zoneFilter
            << "'. Use the zone name (or part of it) or the numeric zone ID.";
        return out.str();
    }

    auto appendZone = [&](ZoneNeed const& need)
    {
        HuntZoneDefinition const& zone = *need.Zone;
        out << "\n" << zone.Name << " (" << static_cast<uint32>(zone.MinLevel) << '-'
            << static_cast<uint32>(zone.MaxLevel) << "):";

        if (!need.EnabledLocationCount)
            out << " | TODO NO FINAL LOCATIONS";
        else if (!need.UncoveredRanges.empty())
            out << " | TODO NEEDS LEVELS " << formatRanges(need.UncoveredRanges);

        if (!need.OutsideIds.empty())
            out << " | WARN " << formatIdList("OUTSIDE_ZONE", need.OutsideIds);

        if (!need.SparseIds.empty())
            out << " | INFO " << formatIdList("SPARSE", need.SparseIds);

        if (!need.NoMobIds.empty())
            out << " | INFO " << formatIdList("NO_MOBS", need.NoMobIds);
    };

    if (hasFilter)
        out << "[Hunts] Final-location authoring to-do for '" << zoneFilter << "':";
    else
        out << "[Hunts] Final-location authoring TO-DO list:";

    if (!coverageNeeds.empty())
    {
        out << "\n[Hunts] PRIORITY - add final sites for missing level coverage:";
        for (ZoneNeed const& need : coverageNeeds)
            appendZone(need);
    }

    if (!reviewOnly.empty())
    {
        out << "\n[Hunts] REVIEW - coverage is complete; inspect warnings/info when convenient:";
        for (ZoneNeed const& need : reviewOnly)
            appendZone(need);
    }

    if (coverageNeeds.empty() && reviewOnly.empty())
        out << "\n[Hunts] No authoring work detected.";

    if (!hasFilter)
    {
        out << "\n[Hunts] " << coverageNeeds.size() << " zone(s) need coverage work; "
            << reviewOnly.size() << " zone(s) only need review; " << cleanZones << " zone(s) are clean.";
    }

    out << "\n[Hunts] TODO = missing playable coverage; WARN = possible difficulty mismatch; "
        << "INFO = auto-level confidence only. Coverage allows +/-"
        << static_cast<uint32>(CoverageToleranceLevels) << " level around trusted final-site bands.";
    return out.str();
}

std::string HuntManager::BuildFinalLocationExport(std::string const& zoneFilter) const
{
    uint32 filterZoneId = 0;
    std::string filter = zoneFilter;
    if (!filter.empty())
    {
        bool numeric = std::all_of(filter.begin(), filter.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
        if (numeric)
            filterZoneId = static_cast<uint32>(std::stoul(filter));
        else
        {
            std::string lowered = filter;
            std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            for (HuntZoneDefinition const& zone : _zones)
            {
                std::string zoneName = zone.Name;
                std::transform(zoneName.begin(), zoneName.end(), zoneName.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (zoneName.find(lowered) != std::string::npos)
                {
                    filterZoneId = zone.ZoneId;
                    break;
                }
            }
            if (!filterZoneId)
                return "[Hunts] Export filter did not match a configured Hunt zone: " + filter;
        }
    }

    std::ostringstream query;
    query << "SELECT `id`,`zone_id`,`map_id`,`x`,`y`,`z`,`orientation`,`location_name`,`min_level`,`max_level`,`weight`,`enabled`,`comment` "
             "FROM `hunt_final_location` WHERE `comment` LIKE 'Added in-game with .hunt set final point%'";
    if (filterZoneId)
        query << " AND `zone_id`=" << filterZoneId;
    query << " ORDER BY `id`";

    QueryResult result = WorldDatabase.Query(query.str().c_str());
    if (!result)
        return filterZoneId ? "[Hunts] No in-game-authored final locations found for that zone."
                            : "[Hunts] No in-game-authored final locations found.";

    auto escapeSql = [](std::string value)
    {
        std::string escaped;
        escaped.reserve(value.size() + 8);
        for (char c : value)
        {
            if (c == '\'')
                escaped += "''";
            else if (c == '\\')
                escaped += "\\\\";
            else
                escaped += c;
        }
        return escaped;
    };

    std::ostringstream out;
    out << "[Hunts] SQL export for in-game-authored final locations";
    if (filterZoneId)
        out << " in zone " << filterZoneId;
    out << ":\nREPLACE INTO `hunt_final_location` "
           "(`id`,`zone_id`,`map_id`,`x`,`y`,`z`,`orientation`,`location_name`,`min_level`,`max_level`,`weight`,`enabled`,`comment`) VALUES\n";

    uint32 count = 0;
    do
    {
        Field* f = result->Fetch();
        if (count)
            out << ",\n";
        out << '(' << f[0].Get<uint32>() << ',' << f[1].Get<uint32>() << ',' << f[2].Get<uint16>() << ','
            << f[3].Get<float>() << ',' << f[4].Get<float>() << ',' << f[5].Get<float>() << ',' << f[6].Get<float>()
            << ",'" << escapeSql(f[7].Get<std::string>()) << "',"
            << static_cast<uint32>(f[8].Get<uint8>()) << ',' << static_cast<uint32>(f[9].Get<uint8>()) << ','
            << f[10].Get<uint32>() << ',' << static_cast<uint32>(f[11].Get<uint8>()) << ",'"
            << escapeSql(f[12].Get<std::string>()) << "')";
        ++count;
    } while (result->NextRow());

    out << ";\n[Hunts] Exported " << count << " in-game-authored final location(s).";
    return out.str();
}

bool HuntManager::SetFinalLocationLevels(uint32 locationId, uint8 minLevel, uint8 maxLevel, bool automatic, std::string& message)
{
    auto itr = std::find_if(_finalLocations.begin(), _finalLocations.end(),
        [locationId](HuntFinalLocationDefinition const& location) { return location.Id == locationId; });
    if (itr == _finalLocations.end())
    {
        message = "[Hunts] Final location " + std::to_string(locationId) + " does not exist.";
        return false;
    }

    if (!automatic && (!minLevel || !maxLevel || minLevel > maxLevel || maxLevel > 80))
    {
        message = "[Hunts] Invalid level range. Use levels <id> <min 1-80> <max 1-80>, or levels <id> auto.";
        return false;
    }

    if (automatic)
    {
        WorldDatabase.DirectExecute(
            "UPDATE `hunt_final_location` SET `min_level`=0,`max_level`=0 WHERE `id`={}", locationId);
        itr->MinLevel = 0;
        itr->MaxLevel = 0;
        itr->AutoDerivedLevels = false;
        AnalyzeFinalLocationLevels(*itr);

        std::ostringstream out;
        out << "[Hunts] Final location " << locationId << " returned to automatic level analysis."
            << " Effective levels: " << static_cast<uint32>(itr->MinLevel) << '-' << static_cast<uint32>(itr->MaxLevel);
        if (!itr->NearbyMobSamples)
            out << " | NO_MOBS";
        else if (itr->NearbyMobSamples < 8)
            out << " | SPARSE (" << itr->NearbyMobSamples << " mobs)";
        else if (!itr->LevelSelectionEligible)
            out << " | OUTSIDE_ZONE (" << itr->NearbyMobSamples << " mobs)";
        else
            out << " | GOOD (" << itr->NearbyMobSamples << " mobs)";
        message = out.str();
        return true;
    }

    WorldDatabase.DirectExecute(
        "UPDATE `hunt_final_location` SET `min_level`={},`max_level`={} WHERE `id`={}",
        static_cast<uint32>(minLevel), static_cast<uint32>(maxLevel), locationId);

    itr->MinLevel = minLevel;
    itr->MaxLevel = maxLevel;
    itr->NearbyMobSamples = 0;
    itr->AutoDerivedLevels = false;
    itr->LevelSelectionEligible = true;

    std::ostringstream out;
    out << "[Hunts] Final location " << locationId << " now uses authored levels "
        << static_cast<uint32>(minLevel) << '-' << static_cast<uint32>(maxLevel) << '.';
    message = out.str();
    return true;
}

bool HuntManager::TeleportToFinalLocation(Player* player, uint32 locationId, std::string& message) const
{
    if (!player)
    {
        message = "[Hunts] This command must be used in game.";
        return false;
    }

    HuntFinalLocationDefinition const* location = GetFinalLocation(locationId);
    if (!location)
    {
        message = "[Hunts] Final location " + std::to_string(locationId) + " does not exist.";
        return false;
    }

    player->TeleportTo(location->MapId, location->X, location->Y, location->Z, location->Orientation);

    std::ostringstream out;
    out << "[Hunts] Teleporting to final location " << locationId
        << " (zone " << location->ZoneId << ", map " << location->MapId << ")"
        << " at " << location->X << ", " << location->Y << ", " << location->Z << '.';
    message = out.str();
    return true;
}

bool HuntManager::DeleteFinalLocation(uint32 locationId, std::string& message)
{
    QueryResult result = WorldDatabase.Query(
        "SELECT `zone_id`,`map_id`,`x`,`y`,`z` FROM `hunt_final_location` WHERE `id`={}", locationId);
    if (!result)
    {
        message = "[Hunts] Final location " + std::to_string(locationId) + " does not exist.";
        return false;
    }

    Field* fields = result->Fetch();
    uint32 const zoneId = fields[0].Get<uint32>();
    uint16 const mapId = fields[1].Get<uint16>();
    float const x = fields[2].Get<float>();
    float const y = fields[3].Get<float>();
    float const z = fields[4].Get<float>();

    std::string const deleteSql = "DELETE FROM `hunt_final_location` WHERE `id`=" + std::to_string(locationId);
    WorldDatabase.DirectExecute(deleteSql.c_str());
    _finalLocations.erase(
        std::remove_if(_finalLocations.begin(), _finalLocations.end(),
            [locationId](HuntFinalLocationDefinition const& location) { return location.Id == locationId; }),
        _finalLocations.end());

    uint32 remaining = 0;
    if (QueryResult countResult = WorldDatabase.Query(
        "SELECT COUNT(*) FROM `hunt_final_location` WHERE `zone_id`={} AND `enabled`=1", zoneId))
        remaining = countResult->Fetch()[0].Get<uint32>();

    std::string zoneName = "zone " + std::to_string(zoneId);
    if (HuntZoneDefinition const* zone = GetZone(zoneId))
        zoneName = zone->Name;

    std::ostringstream out;
    out << "[Hunts] Final location " << locationId << " deleted from " << zoneName
        << " (map " << mapId << ", " << x << ", " << y << ", " << z << ").";
    if (!remaining)
        out << " WARNING: This was the last enabled final location for the zone. The zone will no longer be eligible for hunts.";
    else
        out << ' ' << remaining << " enabled final location(s) remain in the zone.";

    message = out.str();
    return true;
}

std::string HuntManager::ResolveFinalLocationName(Player* player, HuntFinalLocationDefinition const& location) const
{
    // Explicit authored names remain supported for special locations. Most Hunt
    // sites leave this blank and use the client's AreaTable name dynamically.
    if (!location.LocationName.empty())
        return location.LocationName;

    if (player && player->GetSession() && player->GetMapId() == location.MapId)
    {
        if (Map* map = player->GetMap())
        {
            uint32 const areaId = map->GetAreaId(player->GetPhaseMask(), location.X, location.Y, location.Z);
            if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(areaId))
            {
                LocaleConstant const locale = player->GetSession()->GetSessionDbcLocale();
                std::string const areaName = area->area_name[locale];
                if (!areaName.empty())
                    return areaName;
            }
        }
    }

    if (HuntZoneDefinition const* zone = GetZone(location.ZoneId))
        return zone->Name;

    return "the marked hunting grounds";
}

void HuntManager::RemoveFinalActivator(Player* player, HuntRuntime& r)
{
    if (r.FinalActivatorGuid.IsEmpty())
        return;

    if (player)
        if (GameObject* go = ObjectAccessor::GetGameObject(*player, r.FinalActivatorGuid))
            go->Delete();

    r.FinalActivatorGuid.Clear();
}

bool HuntManager::EnsureFinalActivator(Player* player, HuntRuntime& r)
{
    if (!player || r.State != HuntState::FinalLocated || !r.FinalLocationId)
        return false;

    HuntDefinition const* hunt = GetDefinition(r.PreyId);
    HuntFinalLocationDefinition const* location = GetFinalLocation(r.FinalLocationId);
    if (!hunt || !location || !hunt->ActivationGameObjectEntry)
        return false;

    if (!r.FinalActivatorGuid.IsEmpty())
    {
        if (GameObject* existing = ObjectAccessor::GetGameObject(*player, r.FinalActivatorGuid))
            if (existing->IsInWorld())
                return true;
        r.FinalActivatorGuid.Clear();
    }

    if (player->GetMapId() != location->MapId)
        return false;

    float const dx = player->GetPositionX() - location->X;
    float const dy = player->GetPositionY() - location->Y;
    if ((dx * dx + dy * dy) > (180.0f * 180.0f))
        return false;

    float z = location->Z;
    if (Map* map = player->GetMap())
    {
        float const groundZ = map->GetHeight(location->X, location->Y, z + 10.0f, true, 50.0f);
        if (groundZ > INVALID_HEIGHT)
            z = groundZ + 0.15f;
    }

    GameObject* activator = player->SummonGameObject(
        hunt->ActivationGameObjectEntry,
        location->X, location->Y, z, location->Orientation,
        0.0f, 0.0f,
        std::sin(location->Orientation * 0.5f),
        std::cos(location->Orientation * 0.5f),
        3600);

    if (!activator)
    {
        LOG_ERROR("server.loading", "[Hunts] Failed to spawn final activation object {} for character {} hunt {}.",
            hunt->ActivationGameObjectEntry, r.CharacterGuid, r.PreyId);
        return false;
    }

    r.FinalActivatorGuid = activator->GetGUID();
    ChatHandler(player->GetSession()).PSendSysMessage(
        "|cff00ff00[Hunts]|r The signs of {} are unmistakable. Interact with the hunt marker to begin the final confrontation.",
        hunt->Name);
    return true;
}

bool HuntManager::OnFinalActivatorUsed(Player* player, GameObject* gameObject, std::string& message)
{
    if (!player || !gameObject)
    {
        message = "Invalid hunt activation.";
        return false;
    }

    auto it = _runtimes.find(player->GetGUID().GetCounter());
    if (it == _runtimes.end())
    {
        message = "You are not tracking any prey.";
        return false;
    }

    HuntRuntime& r = it->second;
    if (r.State != HuntState::FinalLocated)
    {
        message = "Your hunt is not ready for the final confrontation.";
        return false;
    }

    if (r.FinalActivatorGuid.IsEmpty() || r.FinalActivatorGuid != gameObject->GetGUID())
    {
        message = "This is not your prey's trail.";
        return false;
    }

    if (!SpawnPrey(player, r, true, message))
        return false;

    gameObject->Delete();
    r.FinalActivatorGuid.Clear();
    return true;
}

bool HuntManager::SpawnPrey(Player* player, HuntRuntime& r, bool finalEncounter, std::string& message)
{
    HuntDefinition const* h=GetDefinition(r.PreyId);
    if(!player||!h){message="Unable to resolve hunt prey.";return false;}

    // Recover automatically from a prey creature that vanished or fell far below
    // the player.  This keeps a bad summon from permanently locking the hunt.
    if(!r.ActivePreyGuid.IsEmpty())
    {
        Creature* active=ObjectAccessor::GetCreature(*player,r.ActivePreyGuid);
        if(!active || !active->IsInWorld() || active->GetMapId()!=player->GetMapId() ||
           active->GetDistance2d(player)>120.0f || std::fabs(active->GetPositionZ()-player->GetPositionZ())>25.0f)
        {
            if(active) active->DespawnOrUnsummon();
            _abilityTimers.erase(r.CharacterGuid);
            r.ActivePreyGuid.Clear();
            r.ActivePreyFinal=false;
            SaveRuntime(r);
        }
        else
        {
            message="Your prey is already active.";
            return false;
        }
    }

    float angle=frand(0.0f,6.2831853f), dist=frand(8.0f,14.0f);
    float x=player->GetPositionX()+std::cos(angle)*dist;
    float y=player->GetPositionY()+std::sin(angle)*dist;
    float z=player->GetPositionZ();

    if(finalEncounter && r.FinalLocationId)
    {
        HuntFinalLocationDefinition const* finalLocation=nullptr;
        for(auto const& l:_finalLocations)
            if(l.Id==r.FinalLocationId){finalLocation=&l;break;}

        if(!finalLocation){message="The final hunt location could not be resolved.";return false;}
        if(player->GetMapId()!=finalLocation->MapId){message="Travel to the marked prey location before starting the final encounter.";return false;}

        float dx=player->GetPositionX()-finalLocation->X;
        float dy=player->GetPositionY()-finalLocation->Y;
        if((dx*dx+dy*dy)>(120.0f*120.0f)){message="Travel to the marked prey location before starting the final encounter.";return false;}

        // The player is standing at the authored site, so summon near the player
        // instead of trusting a hand-entered Z value for the actual creature.
        // The database coordinates still define the POI and activation site.
        angle=frand(0.0f,6.2831853f);
        dist=frand(7.0f,11.0f);
        x=player->GetPositionX()+std::cos(angle)*dist;
        y=player->GetPositionY()+std::sin(angle)*dist;
    }

    if(Map* map=player->GetMap())
    {
        float groundZ=map->GetHeight(x,y,z+10.0f,true,50.0f);
        if(groundZ>INVALID_HEIGHT)
            z=groundZ+0.5f;
    }

    uint32 const preyEntry = ResolvePreyEntry(*h);
    if (!preyEntry) { message="The prey creature template could not be resolved."; return false; }
    TempSummon* prey=player->SummonCreature(preyEntry,x,y,z,player->GetOrientation(),TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,300000);
    if(!prey){message="The prey could not be spawned.";return false;}
    prey->SetLevel(player->GetLevel());
    prey->UpdateAllStats();

    // SetLevel + UpdateAllStats makes the derived template use the hunter's
    // level.  Also enforce a player-relative health floor so a high-level
    // hunter cannot accidentally one-shot a prey whose visual base was a
    // low-level creature such as Hogger.
    float ambushScale = 1.0f;
    float finalScale = 1.0f;
    uint8 const hunterLevel = player->GetLevel();
    if (hunterLevel < 20)      { ambushScale = 0.375f; finalScale = 0.50f; }
    else if (hunterLevel < 40) { ambushScale = 0.625f; finalScale = 0.667f; }
    else if (hunterLevel < 60) { ambushScale = 0.750f; finalScale = 0.750f; }
    else if (hunterLevel < 70) { ambushScale = 0.875f; finalScale = 0.833f; }

    float const baseMultiplier = finalEncounter ? h->FinalHealthMultiplier : h->AmbushHealthMultiplier;
    float const levelScale = finalEncounter ? finalScale : ambushScale;
    float const healthMultiplier = std::max(1.0f, baseMultiplier * levelScale);
    uint64 const playerScaledHealth = static_cast<uint64>(healthMultiplier * player->GetMaxHealth());
    // The elite flag remains presentation/identity; hunt difficulty owns the health pool.
    // Do not let the cloned elite template's derived health override our hunt scaling.
    uint32 const desiredMaxHealth = static_cast<uint32>(std::min<uint64>(std::numeric_limits<uint32>::max(), playerScaledHealth));
    prey->SetMaxHealth(desiredMaxHealth);
    prey->SetFullHealth();

    r.ActivePreyGuid=prey->GetGUID(); r.ActivePreyFinal=finalEncounter;
    InitializeAbilityTimers(r, finalEncounter);
    if(!finalEncounter){r.AmbushesCompleted++; SaveRuntime(r); ChatHandler(player->GetSession()).PSendSysMessage("|cffff8000[Hunts]|r {} has found YOU! Drive it off!",h->Name);}
    else ChatHandler(player->GetSession()).PSendSysMessage("|cffff0000[Hunts]|r {} emerges for the final confrontation!",h->Name);
    prey->AI()->AttackStart(player);
    message=finalEncounter?"Final prey spawned.":"Ambush spawned."; return true;
}

void HuntManager::InitializeAbilityTimers(HuntRuntime const& runtime, bool finalEncounter)
{
    _abilityUsed.erase(runtime.CharacterGuid);
    auto& timers = _abilityTimers[runtime.CharacterGuid];
    timers.clear();

    auto abilityIt = _preyAbilities.find(runtime.PreyId);
    if (abilityIt == _preyAbilities.end())
        return;

    uint8 const encounterBit = finalEncounter ? 2 : 1;
    for (HuntPreyAbilityDefinition const& ability : abilityIt->second)
    {
        if (!ability.Enabled || !(ability.EncounterMask & encounterBit))
            continue;

        uint32 const minMs = std::min(ability.InitialMinMs, ability.InitialMaxMs);
        uint32 const maxMs = std::max(ability.InitialMinMs, ability.InitialMaxMs);
        timers[ability.Id] = minMs == maxMs ? minMs : urand(minMs, maxMs);
    }
}

void HuntManager::UpdatePreyAbilities(Player* player, HuntRuntime& runtime, Creature* prey, uint32 elapsedMs)
{
    if (!player || !prey || !prey->IsAlive() || !prey->IsInCombat())
        return;

    auto abilityIt = _preyAbilities.find(runtime.PreyId);
    if (abilityIt == _preyAbilities.end())
        return;

    auto timerOwner = _abilityTimers.find(runtime.CharacterGuid);
    if (timerOwner == _abilityTimers.end())
    {
        InitializeAbilityTimers(runtime, runtime.ActivePreyFinal);
        timerOwner = _abilityTimers.find(runtime.CharacterGuid);
        if (timerOwner == _abilityTimers.end())
            return;
    }

    uint8 const encounterBit = runtime.ActivePreyFinal ? 2 : 1;
    for (HuntPreyAbilityDefinition const& ability : abilityIt->second)
    {
        if (!ability.Enabled || !(ability.EncounterMask & encounterBit) || !ability.SpellId)
            continue;
        uint8 const hunterLevel = player->GetLevel();
        if (hunterLevel < ability.MinHunterLevel || hunterLevel > ability.MaxHunterLevel)
            continue;
        if (ability.OncePerEncounter && _abilityUsed[runtime.CharacterGuid][ability.Id])
            continue;
        if (ability.HealthBelowPct && prey->GetHealthPct() > ability.HealthBelowPct)
            continue;
        if (ability.RequireMelee && !prey->IsWithinMeleeRange(player))
            continue;
        Unit* target = ability.Target == 1 ? static_cast<Unit*>(prey) : static_cast<Unit*>(player);
        if (ability.RequireAuraMissing && target->HasAura(ability.SpellId))
            continue;

        uint32& timer = timerOwner->second[ability.Id];
        if (timer > elapsedMs)
        {
            timer -= elapsedMs;
            continue;
        }

        if (ability.ChancePct >= 100 || urand(1, 100) <= ability.ChancePct)
        {
            prey->CastSpell(target, ability.SpellId, false);
            if (ability.OncePerEncounter)
                _abilityUsed[runtime.CharacterGuid][ability.Id] = true;
        }

        uint32 const minMs = std::min(ability.CooldownMinMs, ability.CooldownMaxMs);
        uint32 const maxMs = std::max(ability.CooldownMinMs, ability.CooldownMaxMs);
        timer = minMs == maxMs ? minMs : urand(minMs, maxMs);
    }
}

bool HuntManager::SendFinalLocationPoi(Player* player, HuntRuntime const& r) const
{
    if (!player || !player->GetSession() || r.State != HuntState::FinalLocated || !r.FinalLocationId)
        return false;

    HuntFinalLocationDefinition const* location = GetFinalLocation(r.FinalLocationId);
    HuntDefinition const* hunt = GetDefinition(r.PreyId);
    if (!location || !hunt || player->GetMapId() != location->MapId)
        return false;

    // SMSG_GOSSIP_POI is the same native 3.3.5a marker used by guard directions.
    // The client removes it when the player gets close, so FinalLocated hunts
    // periodically resend this packet until the prey is actually credited dead.
    WorldPacket poi(SMSG_GOSSIP_POI, 64);
    poi << uint32(6);
    poi << float(location->X);
    poi << float(location->Y);
    poi << uint32(7);
    poi << uint32(0);
    poi << std::string("Prey: ") + hunt->Name;
    player->GetSession()->SendPacket(&poi);
    return true;
}

bool HuntManager::LocateFinal(Player* player, HuntRuntime& r)
{
    HuntFinalLocationDefinition const* location = SelectFinalLocation(r, player ? player->GetLevel() : 1);
    if (!location)
    {
        LOG_ERROR("server.loading", "[Hunts] Character {} reached final tracking for prey {} in zone {}, but no enabled final location is available.",
            r.CharacterGuid, r.PreyId, r.ZoneId);
        return false;
    }

    HuntDefinition const* hunt = GetDefinition(r.PreyId);
    if (!hunt)
        return false;

    r.TrackingProgress = 100;
    r.FinalLocationId = location->Id;
    r.State = HuntState::FinalLocated;
    SaveRuntime(r);

    std::string const locationName = ResolveFinalLocationName(player, *location);
    if (player && player->GetSession())
    {
        ChatHandler(player->GetSession()).PSendSysMessage(
            "|cff00ff00[Hunts]|r Your tracking is complete. {} has been located near {}.",
            hunt->Name, locationName);

        SendFinalLocationPoi(player, r);
        ChatHandler(player->GetSession()).SendSysMessage(
            "|cff00ff00[Hunts]|r The prey location has been marked on your map. The marker will be refreshed until the quarry is slain.");
        EnsureFinalActivator(player, r);
    }

    return true;
}

bool HuntManager::ForceAmbush(Player* player, std::string& message)
{
    if(!player){message="Player required.";return false;} auto it=_runtimes.find(player->GetGUID().GetCounter()); if(it==_runtimes.end()){message="No active hunt.";return false;} return SpawnPrey(player,it->second,false,message);
}

bool HuntManager::ForceFinal(Player* player, std::string& message)
{
    if(!player){message="Player required.";return false;} auto it=_runtimes.find(player->GetGUID().GetCounter()); if(it==_runtimes.end()){message="No active hunt.";return false;} HuntRuntime& r=it->second;
    if(r.State==HuntState::Tracking && !LocateFinal(player,r))
    {
        message="No valid final hunt location is available for this hunt.";
        return false;
    }
    if(r.State!=HuntState::FinalLocated){message="The hunt is not ready for a final encounter.";return false;}
    if(!SpawnPrey(player,r,true,message)) return false;
    RemoveFinalActivator(player,r);
    return true;
}

void HuntManager::Update(uint32 diff)
{
    if(!_enabled) return; if(_updateTimerMs>diff){_updateTimerMs-=diff;return;} _updateTimerMs=250;

    bool refreshFinalPoi = false;
    if (_finalPoiRefreshTimerMs <= 250)
    {
        _finalPoiRefreshTimerMs = 5000;
        refreshFinalPoi = true;
    }
    else
        _finalPoiRefreshTimerMs -= 250;

    for(auto& [guid,r]:_runtimes)
    {
        Player* p=ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(guid)); if(!p) continue;

        // TrackingProgress is the recovery source of truth. A hunt that has
        // reached 100% must either be ReadyToTurnIn or have a valid final site.
        // This repairs both observed broken forms:
        //   100%, state=Tracking,      final_location_id=0
        //   100%, state=FinalLocated,  final_location_id=0
        if (r.TrackingProgress >= 100 && r.State != HuntState::ReadyToTurnIn)
        {
            HuntFinalLocationDefinition const* finalLocation =
                r.FinalLocationId ? GetFinalLocation(r.FinalLocationId) : nullptr;

            if (!finalLocation)
            {
                if (refreshFinalPoi)
                {
                    LOG_WARN("server.loading",
                        "[Hunts] Repairing incomplete final state for character {} prey {} zone {} (state={}, final_location_id={}).",
                        r.CharacterGuid, r.PreyId, r.ZoneId, static_cast<uint32>(r.State), r.FinalLocationId);
                    LocateFinal(p, r);
                }
            }
            else
            {
                // If the authored final site was persisted but the state was not,
                // promote the runtime without choosing a different location.
                if (r.State != HuntState::FinalLocated)
                {
                    LOG_WARN("server.loading",
                        "[Hunts] Promoting recovered 100% hunt for character {} prey {} from state {} to FinalLocated using final location {}.",
                        r.CharacterGuid, r.PreyId, static_cast<uint32>(r.State), r.FinalLocationId);
                    r.State = HuntState::FinalLocated;
                    SaveRuntime(r);
                }

                if (refreshFinalPoi)
                    SendFinalLocationPoi(p, r);

                if (r.ActivePreyGuid.IsEmpty())
                    EnsureFinalActivator(p,r);
            }
        }

        if(r.ActivePreyGuid.IsEmpty()) continue;
        Creature* prey=ObjectAccessor::GetCreature(*p,r.ActivePreyGuid);
        if(!prey)
        {
            _abilityTimers.erase(r.CharacterGuid);
            r.ActivePreyGuid.Clear();
            continue;
        }

        HuntDefinition const* h=GetDefinition(r.PreyId); if(!h) continue;
        UpdatePreyAbilities(p, r, prey, 250);

        if(!r.ActivePreyFinal && prey->GetHealthPct()<=h->EscapeHealthPct)
        {
            prey->CombatStop(true); prey->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_IMMUNE_TO_PC);
            ChatHandler(p->GetSession()).PSendSysMessage("|cffffff00[Hunts]|r {} breaks away and disappears. Continue tracking it.",h->Name);
            prey->DespawnOrUnsummon(Milliseconds(1500));
            _abilityTimers.erase(r.CharacterGuid);
            r.ActivePreyGuid.Clear(); SaveRuntime(r);
        }
    }
}

std::string HuntManager::BuildStats(Player const* player) const
{
    if (!player) return "No hunting record is available.";
    uint32 guid = player->GetGUID().GetCounter();
    QueryResult result = CharacterDatabase.Query(
        "SELECT `total_completed`,IF(`daily_reset_date`=CURRENT_DATE,`daily_completed`,0),`greens_received`,`blues_received`,`epics_received` "
        "FROM `hunt_stats` WHERE `guid`={}", guid);
    if (!result) return "Hunting Record: 0 completed hunts. No rewards recorded yet.";
    Field* f = result->Fetch();
    std::ostringstream out;
    out << "Hunting Record: " << f[0].Get<uint32>() << " total | " << f[1].Get<uint32>() << " today"
        << " | green rewards " << f[2].Get<uint32>() << " | blue rewards " << f[3].Get<uint32>()
        << " | epic rewards " << f[4].Get<uint32>();
    return out.str();
}

std::string HuntManager::BuildStatus(Player const* player) const
{
    std::ostringstream s; if(!_enabled){s<<"Hunts: disabled";return s.str();} if(!player){s<<"Hunts enabled; minimum level "<<uint32(_minimumLevel)<<", XP multiplier "<<_xpMultiplier;return s.str();}
    HuntRuntime const* r=GetRuntime(player); if(!r){s<<"No active hunt.";return s.str();} HuntDefinition const* h=GetDefinition(r->PreyId);
    HuntZoneDefinition const* z=GetZone(r->ZoneId);
    s<<"Hunt: "<<(h?h->Name:"unknown")<<" | zone="<<(z?z->Name:std::to_string(r->ZoneId))<<" | tracking="<<uint32(r->TrackingProgress)<<"% | ambushes="<<uint32(r->AmbushesCompleted)<<" | state="<<uint32(static_cast<uint8>(r->State)); return s.str();
}
}
