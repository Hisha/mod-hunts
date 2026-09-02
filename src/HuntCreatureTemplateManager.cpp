#include "HuntCreatureTemplateManager.h"

#include "DatabaseEnv.h"
#include "Field.h"
#include "QueryResult.h"
#include "Log.h"

#include <limits>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace hunts
{
namespace
{
// Keep generated entries well away from normal WotLK IDs, but DO NOT assume
// ownership of this range. Every candidate is checked against creature_template
// and the persistent Hunt mapping table before use.
constexpr uint32 AllocationStart = 15000000;
constexpr uint32 AllocationEnd = 16777215;

void ExecuteSql(std::string const& sql)
{
    WorldDatabase.DirectExecute(sql.c_str());
}
}

HuntCreatureTemplateManager& HuntCreatureTemplateManager::Instance()
{
    static HuntCreatureTemplateManager instance;
    return instance;
}

bool HuntCreatureTemplateManager::MaterializeStartupTemplates()
{
    _entryByLwTemplate.clear();

    LOG_INFO("server.loading",
        "[Hunts Template] Reconciling dynamically allocated creature templates before ObjectMgr creature-template load.");

    if (!RetireInactiveMappings())
    {
        return false;
    }

    if (!MaterializeEnabledDefinitions())
    {
        return false;
    }

    LoadMappings();

    LOG_INFO("server.loading",
        "[Hunts Template] Reconciliation complete. {} active Hunt creature template mapping(s).",
        _entryByLwTemplate.size());

    return true;
}

bool HuntCreatureTemplateManager::RetireInactiveMappings()
{
    QueryResult result = WorldDatabase.Query(
        "SELECT m.`hunt_template_id`, m.`allocated_entry` "
        "FROM `hunt_creature_template_map` m "
        "LEFT JOIN `hunt_creature_template` d ON d.`id` = m.`hunt_template_id` "
        "WHERE d.`id` IS NULL OR d.`enabled` = 0");

    if (!result)
    {
        return true;
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 const lwTemplateId = fields[0].Get<uint32>();
        uint32 const allocatedEntry = fields[1].Get<uint32>();

        std::ostringstream sql;
        sql << "DELETE FROM `creature_template_model` WHERE `CreatureID` = " << allocatedEntry;
        ExecuteSql(sql.str());

        sql.str("");
        sql.clear();
        sql << "DELETE FROM `creature_equip_template` WHERE `CreatureID` = " << allocatedEntry;
        ExecuteSql(sql.str());

        sql.str("");
        sql.clear();
        sql << "DELETE FROM `creature_template` WHERE `entry` = " << allocatedEntry;
        ExecuteSql(sql.str());

        sql.str("");
        sql.clear();
        sql << "UPDATE `hunt_creature_template_map` SET `retired` = 1 "
            << "WHERE `hunt_template_id` = " << lwTemplateId;
        ExecuteSql(sql.str());

        LOG_INFO("server.loading",
            "[Hunts Template] Retired logical template {} and removed generated creature entry {}. "
            "The allocated ID remains reserved and will not be reused.",
            lwTemplateId,
            allocatedEntry);
    } while (result->NextRow());

    return true;
}

bool HuntCreatureTemplateManager::MaterializeEnabledDefinitions()
{
    QueryResult result = WorldDatabase.Query(
        "SELECT d.`id`, d.`base_creature_entry`, m.`allocated_entry` "
        "FROM `hunt_creature_template` d "
        "LEFT JOIN `hunt_creature_template_map` m ON m.`hunt_template_id` = d.`id` "
        "WHERE d.`enabled` = 1 ORDER BY d.`id`");

    if (!result)
    {
        LOG_INFO("server.loading", "[Hunts Template] No enabled derived creature templates are defined.");
        return true;
    }

    struct Definition
    {
        uint32 LwTemplateId = 0;
        uint32 BaseEntry = 0;
        uint32 AllocatedEntry = 0;
    };

    std::vector<Definition> definitions;
    do
    {
        Field* fields = result->Fetch();
        Definition definition;
        definition.LwTemplateId = fields[0].Get<uint32>();
        definition.BaseEntry = fields[1].Get<uint32>();
        definition.AllocatedEntry = fields[2].IsNull() ? 0 : fields[2].Get<uint32>();
        definitions.push_back(definition);
    } while (result->NextRow());

    // Allocation is exceptional after the first clean startup. Keep the safe
    // allocator for new logical definitions, but do not perform per-template
    // existence/materialization queries for the normal already-mapped path.
    for (Definition& definition : definitions)
    {
        if (definition.AllocatedEntry)
            continue;

        if (!WorldDatabase.Query(
            ("SELECT 1 FROM `creature_template` WHERE `entry` = " +
             std::to_string(definition.BaseEntry) + " LIMIT 1").c_str()))
        {
            LOG_ERROR("server.loading",
                "[Hunts Template] Logical template {} references missing base creature entry {}; skipped.",
                definition.LwTemplateId, definition.BaseEntry);
            continue;
        }

        definition.AllocatedEntry = AllocateEntry();
        if (!definition.AllocatedEntry)
        {
            LOG_ERROR("server.loading",
                "[Hunts Template] Could not allocate a free creature_template entry for logical template {}.",
                definition.LwTemplateId);
            return false;
        }

        std::ostringstream insertMap;
        insertMap << "INSERT INTO `hunt_creature_template_map` "
            << "(`hunt_template_id`,`allocated_entry`,`base_creature_entry`,`retired`) VALUES ("
            << definition.LwTemplateId << ',' << definition.AllocatedEntry << ','
            << definition.BaseEntry << ",0)";
        ExecuteSql(insertMap.str());

        LOG_INFO("server.loading", "[Hunts Template] Allocated creature entry {} to logical template {}.",
            definition.AllocatedEntry, definition.LwTemplateId);
    }

    // 0.6.4.9: rebuild every active mapped LW template with set-based SQL.
    // Previously each logical template performed deletes, inserts, validation
    // SELECTs and a mapping UPDATE separately. On a synchronous world DB
    // connection that turned 22 templates into ~45 seconds of startup work.
    // These statements preserve the same inheritance/override rules while
    // reducing the normal startup path to a handful of DB round trips.
    ExecuteSql(
        "DELETE ctm FROM `creature_template_model` ctm "
        "JOIN `hunt_creature_template_map` m ON m.`allocated_entry` = ctm.`CreatureID` "
        "JOIN `hunt_creature_template` d ON d.`id` = m.`hunt_template_id` "
        "WHERE d.`enabled` = 1");

    ExecuteSql(
        "DELETE cet FROM `creature_equip_template` cet "
        "JOIN `hunt_creature_template_map` m ON m.`allocated_entry` = cet.`CreatureID` "
        "JOIN `hunt_creature_template` d ON d.`id` = m.`hunt_template_id` "
        "WHERE d.`enabled` = 1");

    ExecuteSql(
        "DELETE ct FROM `creature_template` ct "
        "JOIN `hunt_creature_template_map` m ON m.`allocated_entry` = ct.`entry` "
        "JOIN `hunt_creature_template` d ON d.`id` = m.`hunt_template_id` "
        "WHERE d.`enabled` = 1");

    ExecuteSql(
        "INSERT INTO `creature_template` ("
        "`entry`,`difficulty_entry_1`,`difficulty_entry_2`,`difficulty_entry_3`,"
        "`KillCredit1`,`KillCredit2`,`name`,`subname`,`IconName`,`gossip_menu_id`,"
        "`minlevel`,`maxlevel`,`exp`,`faction`,`npcflag`,`speed_walk`,`speed_run`,"
        "`speed_swim`,`speed_flight`,`detection_range`,`rank`,`dmgschool`,"
        "`DamageModifier`,`BaseAttackTime`,`RangeAttackTime`,`BaseVariance`,`RangeVariance`,"
        "`unit_class`,`unit_flags`,`unit_flags2`,`dynamicflags`,`family`,`type`,`type_flags`,"
        "`lootid`,`pickpocketloot`,`skinloot`,`PetSpellDataId`,`VehicleId`,`mingold`,`maxgold`,"
        "`AIName`,`MovementType`,`HoverHeight`,`HealthModifier`,`ManaModifier`,`ArmorModifier`,"
        "`ExperienceModifier`,`RacialLeader`,`movementId`,`RegenHealth`,`CreatureImmunitiesId`,"
        "`flags_extra`,`ScriptName`,`VerifiedBuild`) "
        "SELECT m.`allocated_entry`,0,0,0,0,0,"
        "COALESCE(d.`name_override`,b.`name`),COALESCE(d.`subname_override`,b.`subname`),"
        "b.`IconName`,0,b.`minlevel`,b.`maxlevel`,b.`exp`,"
        "COALESCE(d.`faction_override`,b.`faction`),0,"
        "b.`speed_walk`,b.`speed_run`,b.`speed_swim`,b.`speed_flight`,b.`detection_range`,"
        "COALESCE(d.`rank_override`,b.`rank`),b.`dmgschool`,"
        "COALESCE(d.`damage_modifier_override`,b.`DamageModifier`),"
        "b.`BaseAttackTime`,b.`RangeAttackTime`,b.`BaseVariance`,b.`RangeVariance`,"
        "COALESCE(d.`unit_class_override`,b.`unit_class`),"
        "b.`unit_flags`,b.`unit_flags2`,b.`dynamicflags`,b.`family`,b.`type`,b.`type_flags`,"
        "0,0,0,0,0,0,0,'',0,b.`HoverHeight`,"
        "COALESCE(d.`health_modifier_override`,b.`HealthModifier`),"
        "COALESCE(d.`mana_modifier_override`,b.`ManaModifier`),"
        "COALESCE(d.`armor_modifier_override`,b.`ArmorModifier`),"
        "b.`ExperienceModifier`,0,0,b.`RegenHealth`,"
        "CASE WHEN EXISTS (SELECT 1 FROM `hunt_prey` hp WHERE hp.`prey_template_id`=d.`id` AND hp.`tier`>=2 AND hp.`enabled`=1) "
        "THEN 0 ELSE b.`CreatureImmunitiesId` END,b.`flags_extra`,'',b.`VerifiedBuild` "
        "FROM `hunt_creature_template` d "
        "JOIN `hunt_creature_template_map` m ON m.`hunt_template_id`=d.`id` "
        "JOIN `creature_template` b ON b.`entry`=d.`base_creature_entry` "
        "WHERE d.`enabled`=1");

    ExecuteSql(
        "INSERT INTO `creature_template_model` "
        "(`CreatureID`,`Idx`,`CreatureDisplayID`,`DisplayScale`,`Probability`,`VerifiedBuild`) "
        "SELECT m.`allocated_entry`,bm.`Idx`,bm.`CreatureDisplayID`,bm.`DisplayScale`,bm.`Probability`,bm.`VerifiedBuild` "
        "FROM `hunt_creature_template` d "
        "JOIN `hunt_creature_template_map` m ON m.`hunt_template_id`=d.`id` "
        "JOIN `creature_template_model` bm ON bm.`CreatureID`=d.`base_creature_entry` "
        "WHERE d.`enabled`=1");

    ExecuteSql(
        "INSERT INTO `creature_equip_template` "
        "(`CreatureID`,`ID`,`ItemID1`,`ItemID2`,`ItemID3`,`VerifiedBuild`) "
        "SELECT m.`allocated_entry`,be.`ID`,be.`ItemID1`,be.`ItemID2`,be.`ItemID3`,be.`VerifiedBuild` "
        "FROM `hunt_creature_template` d "
        "JOIN `hunt_creature_template_map` m ON m.`hunt_template_id`=d.`id` "
        "JOIN `creature_equip_template` be ON be.`CreatureID`=d.`base_creature_entry` "
        "WHERE d.`enabled`=1");

    ExecuteSql(
        "UPDATE `hunt_creature_template_map` m "
        "JOIN `hunt_creature_template` d ON d.`id`=m.`hunt_template_id` "
        "SET m.`base_creature_entry`=d.`base_creature_entry`,m.`retired`=0 "
        "WHERE d.`enabled`=1");

    QueryResult validation = WorldDatabase.Query(
        "SELECT COUNT(*), COUNT(DISTINCT ctm.`CreatureID`) "
        "FROM `hunt_creature_template` d "
        "JOIN `hunt_creature_template_map` m ON m.`hunt_template_id`=d.`id` "
        "LEFT JOIN `creature_template` ct ON ct.`entry`=m.`allocated_entry` "
        "LEFT JOIN `creature_template_model` ctm ON ctm.`CreatureID`=m.`allocated_entry` "
        "WHERE d.`enabled`=1 AND ct.`entry` IS NOT NULL");

    if (!validation)
        return false;

    Field* fields = validation->Fetch();
    uint32 const materializedRows = fields[0].Get<uint32>();
    uint32 const templatesWithModels = fields[1].Get<uint32>();
    uint32 const expected = static_cast<uint32>(definitions.size());

    // COUNT(*) can exceed expected because a template may have multiple model
    // rows, so validate the generated creature rows separately.
    QueryResult creatureCountResult = WorldDatabase.Query(
        "SELECT COUNT(*) FROM `hunt_creature_template` d "
        "JOIN `hunt_creature_template_map` m ON m.`hunt_template_id`=d.`id` "
        "JOIN `creature_template` ct ON ct.`entry`=m.`allocated_entry` "
        "WHERE d.`enabled`=1");
    uint32 const creatureCount = creatureCountResult ? creatureCountResult->Fetch()[0].Get<uint32>() : 0;

    if (creatureCount != expected || templatesWithModels != expected)
    {
        LOG_ERROR("server.loading",
            "[Hunts Template] Bulk materialization validation failed: expected {} template(s), found {} generated creature row(s) and {} template(s) with models ({} joined row(s)).",
            expected, creatureCount, templatesWithModels, materializedRows);
        return false;
    }

    LOG_INFO("server.loading", "[Hunts Template] Bulk-materialized {} logical creature template(s).", expected);
    return true;
}

uint32 HuntCreatureTemplateManager::AllocateEntry() const
{
    // Reserved mappings are included even if their generated row is currently
    // retired, preventing automatic ID recycling.
    QueryResult used = WorldDatabase.Query(
        "SELECT `id_value` FROM ("
        " SELECT `entry` AS `id_value` FROM `creature_template` WHERE `entry` >= 15000000"
        " UNION "
        " SELECT `allocated_entry` AS `id_value` FROM `hunt_creature_template_map` WHERE `allocated_entry` >= 15000000"
        ") ids ORDER BY `id_value`");

    uint32 candidate = AllocationStart;

    if (used)
    {
        do
        {
            uint32 const usedEntry = used->Fetch()[0].Get<uint32>();

            if (usedEntry < candidate)
            {
                continue;
            }

            if (usedEntry == candidate)
            {
                if (candidate == AllocationEnd)
                {
                    return 0;
                }

                ++candidate;
                continue;
            }

            // First gap.
            break;

        } while (used->NextRow());
    }

    return candidate <= AllocationEnd ? candidate : 0;
}

bool HuntCreatureTemplateManager::MaterializeDefinition(uint32 lwTemplateId, uint32 allocatedEntry)
{
    // We deliberately inherit the physical/combat shell but strip world-service
    // identity and entry-specific scripting:
    //   - no gossip/vendor/quest/trainer/etc npcflags
    //   - no loot/pickpocket/skinning/gold
    //   - no SmartAI/ScriptName inheritance
    //   - no permanent movement path
    //
    // We DO inherit:
    //   - creature_template_model
    //   - creature_equip_template
    //
    // Level remains a spawn-time LW concern via level_override.
    std::ostringstream sql;
    sql <<
        "INSERT INTO `creature_template` ("
        "`entry`,`difficulty_entry_1`,`difficulty_entry_2`,`difficulty_entry_3`,"
        "`KillCredit1`,`KillCredit2`,`name`,`subname`,`IconName`,`gossip_menu_id`,"
        "`minlevel`,`maxlevel`,`exp`,`faction`,`npcflag`,`speed_walk`,`speed_run`,"
        "`speed_swim`,`speed_flight`,`detection_range`,`rank`,`dmgschool`,"
        "`DamageModifier`,`BaseAttackTime`,`RangeAttackTime`,`BaseVariance`,`RangeVariance`,"
        "`unit_class`,`unit_flags`,`unit_flags2`,`dynamicflags`,`family`,`type`,`type_flags`,"
        "`lootid`,`pickpocketloot`,`skinloot`,`PetSpellDataId`,`VehicleId`,`mingold`,`maxgold`,"
        "`AIName`,`MovementType`,`HoverHeight`,`HealthModifier`,`ManaModifier`,`ArmorModifier`,"
        "`ExperienceModifier`,`RacialLeader`,`movementId`,`RegenHealth`,`CreatureImmunitiesId`,"
        "`flags_extra`,`ScriptName`,`VerifiedBuild`) "
        "SELECT "
        << allocatedEntry << ","
        "0,0,0,0,0,"
        "COALESCE(d.`name_override`, b.`name`),"
        "COALESCE(d.`subname_override`, b.`subname`),"
        "b.`IconName`,"
        "0,"
        "b.`minlevel`,b.`maxlevel`,b.`exp`,"
        "COALESCE(d.`faction_override`, b.`faction`),"
        "0,"
        "b.`speed_walk`,b.`speed_run`,b.`speed_swim`,b.`speed_flight`,b.`detection_range`,"
        "COALESCE(d.`rank_override`, b.`rank`),"
        "b.`dmgschool`,"
        "COALESCE(d.`damage_modifier_override`, b.`DamageModifier`),"
        "b.`BaseAttackTime`,b.`RangeAttackTime`,b.`BaseVariance`,b.`RangeVariance`,"
        "COALESCE(d.`unit_class_override`, b.`unit_class`),"
        "b.`unit_flags`,b.`unit_flags2`,b.`dynamicflags`,b.`family`,b.`type`,b.`type_flags`,"
        "0,0,0,0,0,0,0,"
        "'',"
        "0,"
        "b.`HoverHeight`,"
        "COALESCE(d.`health_modifier_override`, b.`HealthModifier`),"
        "COALESCE(d.`mana_modifier_override`, b.`ManaModifier`),"
        "COALESCE(d.`armor_modifier_override`, b.`ArmorModifier`),"
        "b.`ExperienceModifier`,"
        "0,0,b.`RegenHealth`,"
        "CASE WHEN EXISTS (SELECT 1 FROM `hunt_prey` hp WHERE hp.`prey_template_id`=d.`id` AND hp.`tier`>=2 AND hp.`enabled`=1) "
        "THEN 0 ELSE b.`CreatureImmunitiesId` END,b.`flags_extra`,'',b.`VerifiedBuild` "
        "FROM `hunt_creature_template` d "
        "JOIN `creature_template` b ON b.`entry` = d.`base_creature_entry` "
        "WHERE d.`id` = " << lwTemplateId << " AND d.`enabled` = 1";

    ExecuteSql(sql.str());

    if (!WorldDatabase.Query(
        ("SELECT 1 FROM `creature_template` WHERE `entry` = " + std::to_string(allocatedEntry) + " LIMIT 1").c_str()))
    {
        return false;
    }

    std::ostringstream models;
    models <<
        "INSERT INTO `creature_template_model` "
        "(`CreatureID`,`Idx`,`CreatureDisplayID`,`DisplayScale`,`Probability`,`VerifiedBuild`) "
        "SELECT " << allocatedEntry << ",`Idx`,`CreatureDisplayID`,`DisplayScale`,`Probability`,`VerifiedBuild` "
        "FROM `creature_template_model` "
        "WHERE `CreatureID` = (SELECT `base_creature_entry` FROM `hunt_creature_template` "
        "WHERE `id` = " << lwTemplateId << ")";

    ExecuteSql(models.str());

    if (!WorldDatabase.Query(
        ("SELECT 1 FROM `creature_template_model` WHERE `CreatureID` = " +
         std::to_string(allocatedEntry) + " LIMIT 1").c_str()))
    {
        LOG_ERROR("server.loading",
            "[Hunts Template] Logical template {} generated creature entry {} but no creature_template_model rows were copied.",
            lwTemplateId,
            allocatedEntry);
        return false;
    }

    std::ostringstream equipment;
    equipment <<
        "INSERT INTO `creature_equip_template` "
        "(`CreatureID`,`ID`,`ItemID1`,`ItemID2`,`ItemID3`,`VerifiedBuild`) "
        "SELECT " << allocatedEntry << ",`ID`,`ItemID1`,`ItemID2`,`ItemID3`,`VerifiedBuild` "
        "FROM `creature_equip_template` "
        "WHERE `CreatureID` = (SELECT `base_creature_entry` FROM `hunt_creature_template` "
        "WHERE `id` = " << lwTemplateId << ")";

    ExecuteSql(equipment.str());

    LOG_INFO("server.loading",
        "[Hunts Template] Materialized logical template {} as creature entry {}.",
        lwTemplateId,
        allocatedEntry);

    return true;
}

void HuntCreatureTemplateManager::LoadMappings()
{
    _entryByLwTemplate.clear();

    QueryResult result = WorldDatabase.Query(
        "SELECT m.`hunt_template_id`, m.`allocated_entry` "
        "FROM `hunt_creature_template_map` m "
        "JOIN `hunt_creature_template` d ON d.`id` = m.`hunt_template_id` "
        "JOIN `creature_template` c ON c.`entry` = m.`allocated_entry` "
        "WHERE m.`retired` = 0 AND d.`enabled` = 1");

    if (!result)
    {
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        _entryByLwTemplate.emplace(
            fields[0].Get<uint32>(),
            fields[1].Get<uint32>());
    } while (result->NextRow());
}

uint32 HuntCreatureTemplateManager::ResolveEntry(uint32 lwTemplateId) const
{
    auto const itr = _entryByLwTemplate.find(lwTemplateId);
    return itr != _entryByLwTemplate.end() ? itr->second : 0;
}

uint32 HuntCreatureTemplateManager::GetMappedTemplateCount() const
{
    return static_cast<uint32>(_entryByLwTemplate.size());
}
}
