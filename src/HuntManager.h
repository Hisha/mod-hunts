#ifndef MOD_HUNTS_HUNT_MANAGER_H
#define MOD_HUNTS_HUNT_MANAGER_H

#include "Define.h"
#include "ObjectGuid.h"

#include <string>
#include <unordered_map>
#include <vector>

class Creature;
class GameObject;
class Player;

namespace hunts
{
enum class HuntSearchScope : uint8
{
    LocalRegion = 0,
    Continent = 1,
    World = 2
};

enum class HuntState : uint8
{
    None = 0,
    Tracking = 1,
    FinalLocated = 2,
    ReadyToTurnIn = 3
};


enum class SealStoreSlot : uint8
{
    Weapon = 0,
    Head = 1,
    Neck = 2,
    Shoulder = 3,
    Back = 4,
    Chest = 5,
    Wrist = 6,
    Hands = 7,
    Waist = 8,
    Legs = 9,
    Feet = 10,
    Ring = 11,
    Trinket = 12,
    OffHand = 13,
    Relic = 14
};

struct SealStoreItem
{
    uint32 ItemId = 0;
    std::string Name;
    uint32 ItemLevel = 0;
    float Score = 0.0f;
};

struct HuntPreyAbilityDefinition
{
    uint32 Id = 0;
    uint32 PreyId = 0;
    uint32 SpellId = 0;
    uint8 Target = 0; // 0=victim, 1=self
    uint32 InitialMinMs = 0;
    uint32 InitialMaxMs = 0;
    uint32 CooldownMinMs = 10000;
    uint32 CooldownMaxMs = 10000;
    uint8 ChancePct = 100;
    uint8 EncounterMask = 3; // 1=ambush, 2=final, 3=both
    uint8 MinHunterLevel = 1;
    uint8 MaxHunterLevel = 80;
    uint8 HealthBelowPct = 0; // prey/caster health; 0=ignore
    uint8 VictimHealthBelowPct = 0; // hunter/victim health; 0=ignore
    bool RequireMelee = false;
    bool OncePerEncounter = false;
    bool RequireAuraMissing = false;
    bool Enabled = false;
};

struct HuntDefinition
{
    uint32 Id = 0;
    std::string Name;
    uint8 MinLevel = 10;
    uint8 MaxLevel = 80;
    uint32 PreyCreatureEntry = 0;
    uint32 PreyTemplateId = 0;
    uint32 ActivationGameObjectEntry = 0;
    float AmbushHealthMultiplier = 4.0f;
    float FinalHealthMultiplier = 6.0f;
    float RewardMultiplier = 1.0f;
    uint8 Tier = 1; // 1=normal, 2=elite
    uint8 CombatStyle = 0; // 0=melee/default, 1=ranged/kiting
    float PreferredRange = 0.0f; // yards; used by ranged combat style
    uint8 EscapeHealthPct = 50;
    uint8 AmbushCount = 2;
    bool Enabled = false;
};

struct HuntZoneDefinition
{
    uint32 Id = 0;
    uint32 ZoneId = 0;
    uint16 MapId = 0;
    uint8 ContinentId = 0;
    std::string Name;
    uint8 MinLevel = 1;
    uint8 MaxLevel = 80;
    uint32 Weight = 100;
    bool Enabled = false;
};

struct HuntFinalLocationDefinition
{
    uint32 Id = 0;
    uint32 ZoneId = 0;
    uint16 MapId = 0;
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;
    float Orientation = 0.0f;
    std::string LocationName;
    uint8 MinLevel = 0;
    uint8 MaxLevel = 0;
    uint32 NearbyMobSamples = 0;
    bool AutoDerivedLevels = false;
    bool LevelSelectionEligible = true;
    uint32 Weight = 100;
    bool Enabled = false;
};


struct HuntGiverDefinition
{
    uint32 Id = 0;
    uint32 CreatureEntry = 0;
    std::string CityName;
    uint16 MapId = 0;
    uint8 ContinentId = 0;
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;
    bool Enabled = false;
};

struct HuntRuntime
{
    uint32 CharacterGuid = 0;
    uint32 PreyId = 0;
    uint32 GiverEntry = 0;
    uint32 GiverSpawnId = 0;
    uint32 ZoneId = 0;
    uint32 FinalLocationId = 0;
    uint8 TrackingProgress = 0;
    uint8 AmbushesCompleted = 0;
    bool AmbushPending = false;
    HuntState State = HuntState::None;
    ObjectGuid ActivePreyGuid;
    bool ActivePreyFinal = false;
    ObjectGuid FinalActivatorGuid;
};

class HuntManager
{
public:
    static HuntManager& Instance();

    void Configure(bool enabled, uint8 minimumLevel, float xpMultiplier, HuntSearchScope searchScope, bool debug,
        uint32 eliteRequiredNormalCompletions, uint32 eliteDailyLimit, float eliteHealthMultiplier,
        float eliteDamageMultiplier, float eliteArmorMultiplier, float eliteXpMultiplier, float eliteGoldMultiplier,
        uint8 eliteSealMinimumLevel, uint32 eliteSealsPerCompletion, uint8 eliteEndgameRewardLevel,
        uint32 eliteEndgameRewardMinItemLevel, uint32 eliteEndgameRewardMaxItemLevel,
        uint8 trackingProgressMin, uint8 trackingProgressMax, float groupCreditRadius, float sharedFinalCreditRadius);
    void Reset();
    void LoadDefinitions();
    void Initialize();
    void Update(uint32 diff);

    bool IsEnabled() const { return _enabled; }
    uint8 GetMinimumLevel() const { return _minimumLevel; }
    float GetXpMultiplier() const { return _xpMultiplier; }
    bool IsDebugEnabled() const { return _debug; }

    bool HasActiveHunt(Player const* player) const;
    HuntRuntime const* GetRuntime(Player const* player) const;
    HuntDefinition const* GetDefinition(uint32 preyId) const;
    bool IsElitePreyEntry(uint32 creatureEntry) const;

    bool RequestHunt(Player* player, Creature* giver, std::string& message);
    bool RequestEliteHunt(Player* player, Creature* giver, std::string& message);
    bool IsEliteUnlocked(Player const* player) const;
    bool IsEliteAvailableToday(Player const* player) const;
    bool IsSealStoreAvailable(Player const* player) const;
    uint32 GetSealBalance(Player const* player) const;
    void ConfigureSealStoreTier(uint8 tier, uint32 cost, uint32 minItemLevel, uint32 maxItemLevel);
    void ConfigureEliteRewardTargeting(bool requireUpgrade, float upgradePoolPct);
    void ConfigureEliteCombat(float rangedPanicRange, float rangedRetreatRangePct, uint32 rangedBlinkCooldownMs, uint32 rangedReactionMs,
        float rangedArenaRadius);
    uint32 GetSealStoreTierCost(uint8 tier) const;
    uint32 GetSealStoreTierMinItemLevel(uint8 tier) const;
    uint32 GetSealStoreTierMaxItemLevel(uint8 tier) const;
    std::vector<SealStoreItem> BuildSealStoreItems(Player* player, uint32 spec, uint8 tier, SealStoreSlot slot) const;
    bool PurchaseSealStoreItem(Player* player, uint32 spec, uint8 tier, uint32 itemId, std::string& message);
    bool AbandonHunt(Player* player, std::string& message);
    bool TurnInHunt(Player* player, Creature* giver, std::string& message);
    void OnCreatureKill(Player* player, Creature* killed);
    bool OnFinalActivatorUsed(Player* player, GameObject* gameObject, std::string& message);

    bool AddProgress(Player* player, uint8 amount, std::string& message);
    bool ForceAmbush(Player* player, std::string& message);
    bool ForceFinal(Player* player, std::string& message);
    std::string BuildStatus(Player const* player) const;
    std::string BuildStats(Player const* player) const;

    // GM/debug world-authoring helpers for final hunt crystal locations.
    bool AddFinalLocationAtPlayer(Player* player, std::string& message);
    std::string BuildFinalLocationList(Player const* player) const;
    std::string BuildFinalLocationNeeds(std::string const& zoneFilter = "") const;
    std::string BuildFinalLocationExport(std::string const& zoneFilter = "") const;
    bool SetFinalLocationLevels(uint32 locationId, uint8 minLevel, uint8 maxLevel, bool automatic, std::string& message);
    bool TeleportToFinalLocation(Player* player, uint32 locationId, std::string& message) const;
    bool DeleteFinalLocation(uint32 locationId, std::string& message);

    bool IsHuntGiver(uint32 creatureEntry) const;
    bool IsGuardLocator(uint32 creatureEntry) const;
    bool SendHuntmasterLocation(Player* player, uint32 guardEntry, std::string& message) const;

private:
    HuntManager() = default;

    void ApplyFinalLocationLevelAnalysis(HuntFinalLocationDefinition& location, uint32 samples, double avgMinLevel, double avgMaxLevel);
    void AnalyzeFinalLocationLevels(HuntFinalLocationDefinition& location);
    void LoadRuntimes();
    void SaveRuntime(HuntRuntime const& runtime);
    void DeleteRuntime(uint32 characterGuid);
    HuntZoneDefinition const* SelectZone(uint8 playerLevel, HuntGiverDefinition const& giver) const;
    HuntZoneDefinition const* GetZone(uint32 zoneId) const;
    HuntFinalLocationDefinition const* SelectFinalLocation(HuntRuntime const& runtime, uint8 hunterLevel) const;
    bool SpawnPrey(Player* player, HuntRuntime& runtime, bool finalEncounter, std::string& message);
    uint32 ResolvePreyEntry(HuntDefinition const& hunt) const;
    HuntFinalLocationDefinition const* GetFinalLocation(uint32 finalLocationId) const;
    std::string ResolveFinalLocationName(Player* player, HuntFinalLocationDefinition const& location) const;
    bool EnsureFinalActivator(Player* player, HuntRuntime& runtime);
    void RemoveFinalActivator(Player* player, HuntRuntime& runtime);
    bool LocateFinal(Player* player, HuntRuntime& runtime);
    bool SendFinalLocationPoi(Player* player, HuntRuntime const& runtime) const;
    uint8 GetNextAmbushThreshold(HuntRuntime const& runtime, HuntDefinition const& hunt) const;
    void InitializeAbilityTimers(HuntRuntime const& runtime, bool finalEncounter);
    void UpdatePreyAbilities(Player* player, HuntRuntime& runtime, Creature* prey, uint32 elapsedMs);
    void UpdatePreyMovement(Player* player, HuntRuntime& runtime, Creature* prey, uint32 elapsedMs);
    bool IsSealStoreItemEligible(Player* player, uint32 spec, uint8 tier, SealStoreSlot slot, uint32 itemId) const;

    bool _enabled = false;
    bool _debug = false;
    uint8 _minimumLevel = 10;
    float _xpMultiplier = 0.75f;
    HuntSearchScope _searchScope = HuntSearchScope::World;
    uint32 _eliteRequiredNormalCompletions = 10;
    uint32 _eliteDailyLimit = 1;
    float _eliteHealthMultiplier = 1.0f;
    float _eliteDamageMultiplier = 1.0f;
    float _eliteArmorMultiplier = 1.0f;
    float _eliteXpMultiplier = 1.0f;
    float _eliteGoldMultiplier = 1.0f;
    uint8 _eliteSealMinimumLevel = 80;
    uint32 _eliteSealsPerCompletion = 1;
    uint8 _eliteEndgameRewardLevel = 80;
    uint32 _eliteEndgameRewardMinItemLevel = 200;
    uint32 _eliteEndgameRewardMaxItemLevel = 200;
    uint32 _sealStoreTierCost[4] = { 5, 10, 20, 30 };
    uint32 _sealStoreTierMinItemLevel[4] = { 213, 226, 245, 264 };
    uint32 _sealStoreTierMaxItemLevel[4] = { 219, 232, 251, 264 };
    bool _eliteRewardRequireUpgrade = true;
    float _eliteRewardUpgradePoolPct = 0.70f;
    float _rangedPanicRange = 10.0f;
    float _rangedRetreatRangePct = 1.0f;
    uint32 _rangedBlinkCooldownMs = 15000;
    uint32 _rangedReactionMs = 500;
    float _rangedArenaRadius = 35.0f;
    uint8 _trackingProgressMin = 3;
    uint8 _trackingProgressMax = 7;
    float _groupCreditRadius = 100.0f;
    float _sharedFinalCreditRadius = 200.0f;
    uint32 _updateTimerMs = 0;
    uint32 _finalPoiRefreshTimerMs = 0;

    std::unordered_map<uint32, HuntDefinition> _hunts;
    std::unordered_map<uint32, std::vector<HuntPreyAbilityDefinition>> _preyAbilities;
    std::unordered_map<uint32, std::unordered_map<uint32, uint32>> _abilityTimers;
    std::unordered_map<uint32, std::unordered_map<uint32, bool>> _abilityUsed;
    std::unordered_map<uint32, uint32> _movementReactionTimers;
    // Elite class-brain transient state. These deliberately do not persist: a
    // restarted encounter begins with fresh CC DR and signature cooldown state.
    std::unordered_map<uint32, uint8> _fearDrStage;
    std::unordered_map<uint32, uint32> _fearDrResetTimers;
    std::unordered_map<uint32, uint32> _rogueReopenTimers;
    std::unordered_map<uint32, bool> _druidBearPhase;
    std::vector<HuntZoneDefinition> _zones;
    std::vector<HuntFinalLocationDefinition> _finalLocations;
    std::unordered_map<uint32, uint32> _giverEntries;
    std::unordered_map<uint32, HuntGiverDefinition> _givers;
    std::unordered_map<uint32, uint32> _guardLocators;
    std::unordered_map<uint32, std::vector<uint32>> _giverLocalZones;
    std::unordered_map<uint32, HuntRuntime> _runtimes;
};
}

#define sHuntMgr hunts::HuntManager::Instance()

#endif
