-- ============================================================================
-- Hunts Huntmasters - permanent capital-city service NPCs
-- No quest flags are used. Each Huntmaster is gossip-only and is discoverable
-- through the city's normal guard-direction gossip via hunt_guard_locator.
-- ============================================================================
SET @SW_ENTRY := 14999980;
SET @IF_ENTRY := 14999981;
SET @DN_ENTRY := 14999982;
SET @EX_ENTRY := 14999983;
SET @OG_ENTRY := 14999984;
SET @TB_ENTRY := 14999985;
SET @UC_ENTRY := 14999986;
SET @SM_ENTRY := 14999987;
SET @SH_ENTRY := 14999988;
SET @DA_ENTRY := 14999989;

-- Existing race/city-appropriate visual shells.
SET @SW_BASE := 68;     -- Stormwind City Guard
SET @IF_BASE := 5595;   -- Ironforge Guard
SET @DN_BASE := 4262;   -- Darnassus Sentinel
SET @EX_BASE := 16733;  -- Exodar Peacekeeper
SET @OG_BASE := 3296;   -- Orgrimmar Grunt
SET @TB_BASE := 3084;   -- Bluffwatcher
SET @UC_BASE := 5624;   -- Undercity Guardian
SET @SM_BASE := 16222;  -- Silvermoon City Guardian
SET @SH_BASE := 19687;  -- Shattrath City Peacekeeper
SET @DA_BASE := 30659;  -- Violet Hold Guard (Dalaran)

DELETE FROM `creature_template_model` WHERE `CreatureID` IN (@SW_ENTRY,@IF_ENTRY,@DN_ENTRY,@EX_ENTRY,@OG_ENTRY,@TB_ENTRY,@UC_ENTRY,@SM_ENTRY,@SH_ENTRY,@DA_ENTRY);
DELETE FROM `creature_template` WHERE `entry` IN (@SW_ENTRY,@IF_ENTRY,@DN_ENTRY,@EX_ENTRY,@OG_ENTRY,@TB_ENTRY,@UC_ENTRY,@SM_ENTRY,@SH_ENTRY,@DA_ENTRY);

INSERT INTO `creature_template` (
    `entry`,`difficulty_entry_1`,`difficulty_entry_2`,`difficulty_entry_3`,`KillCredit1`,`KillCredit2`,`name`,`subname`,`IconName`,`gossip_menu_id`,
    `minlevel`,`maxlevel`,`exp`,`faction`,`npcflag`,`speed_walk`,`speed_run`,`speed_swim`,`speed_flight`,`detection_range`,`rank`,`dmgschool`,
    `DamageModifier`,`BaseAttackTime`,`RangeAttackTime`,`BaseVariance`,`RangeVariance`,`unit_class`,`unit_flags`,`unit_flags2`,`dynamicflags`,`family`,`type`,`type_flags`,
    `lootid`,`pickpocketloot`,`skinloot`,`PetSpellDataId`,`VehicleId`,`mingold`,`maxgold`,`AIName`,`MovementType`,`HoverHeight`,`HealthModifier`,`ManaModifier`,`ArmorModifier`,
    `ExperienceModifier`,`RacialLeader`,`movementId`,`RegenHealth`,`CreatureImmunitiesId`,`flags_extra`,`ScriptName`,`VerifiedBuild`)
SELECT m.entry,0,0,0,0,0,m.npc_name,'Master of the Hunt','Speak',0,
    60,60,b.`exp`,b.`faction`,1,b.`speed_walk`,b.`speed_run`,b.`speed_swim`,b.`speed_flight`,b.`detection_range`,0,b.`dmgschool`,
    b.`DamageModifier`,b.`BaseAttackTime`,b.`RangeAttackTime`,b.`BaseVariance`,b.`RangeVariance`,b.`unit_class`,b.`unit_flags`,b.`unit_flags2`,
    b.`dynamicflags`,b.`family`,b.`type`,b.`type_flags`,0,0,0,0,0,0,0,'',0,b.`HoverHeight`,b.`HealthModifier`,b.`ManaModifier`,b.`ArmorModifier`,
    0,0,0,1,b.`CreatureImmunitiesId`,b.`flags_extra`,'mod_hunts_huntmaster',0
FROM (
    SELECT @SW_ENTRY entry,@SW_BASE base_entry,'Huntmaster Corvin' npc_name UNION ALL
    SELECT @IF_ENTRY,@IF_BASE,'Huntmaster Brannoc' UNION ALL
    SELECT @DN_ENTRY,@DN_BASE,'Huntmistress Shalara' UNION ALL
    SELECT @EX_ENTRY,@EX_BASE,'Huntmaster Veylan' UNION ALL
    SELECT @OG_ENTRY,@OG_BASE,'Huntmaster Gorrak' UNION ALL
    SELECT @TB_ENTRY,@TB_BASE,'Huntmaster Tahu' UNION ALL
    SELECT @UC_ENTRY,@UC_BASE,'Huntmaster Morcant' UNION ALL
    SELECT @SM_ENTRY,@SM_BASE,'Huntmistress Vaelith' UNION ALL
    SELECT @SH_ENTRY,@SH_BASE,'Huntmaster Raleth' UNION ALL
    SELECT @DA_ENTRY,@DA_BASE,'Huntmaster Varyn'
) m
JOIN `creature_template` b ON b.`entry`=m.base_entry;

INSERT INTO `creature_template_model` (`CreatureID`,`Idx`,`CreatureDisplayID`,`DisplayScale`,`Probability`,`VerifiedBuild`)
SELECT m.entry,ctm.`Idx`,ctm.`CreatureDisplayID`,ctm.`DisplayScale`,ctm.`Probability`,0
FROM (
    SELECT @SW_ENTRY entry,@SW_BASE base_entry UNION ALL SELECT @IF_ENTRY,@IF_BASE UNION ALL
    SELECT @DN_ENTRY,@DN_BASE UNION ALL SELECT @EX_ENTRY,@EX_BASE UNION ALL
    SELECT @OG_ENTRY,@OG_BASE UNION ALL SELECT @TB_ENTRY,@TB_BASE UNION ALL
    SELECT @UC_ENTRY,@UC_BASE UNION ALL SELECT @SM_ENTRY,@SM_BASE UNION ALL
    SELECT @SH_ENTRY,@SH_BASE UNION ALL SELECT @DA_ENTRY,@DA_BASE
) m
JOIN `creature_template_model` ctm ON ctm.`CreatureID`=m.base_entry;

-- Remove/recreate our permanent spawns. GUIDs are allocated above the current
-- database maximum at apply time so the module does not claim a fixed core GUID range.
DELETE FROM `creature` WHERE `id` IN (@SW_ENTRY,@IF_ENTRY,@DN_ENTRY,@EX_ENTRY,@OG_ENTRY,@TB_ENTRY,@UC_ENTRY,@SM_ENTRY,@SH_ENTRY,@DA_ENTRY);
SET @LW_HUNTMASTER_CGUID := (SELECT COALESCE(MAX(`guid`),0) FROM `creature`);
INSERT INTO `creature`
(`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curhealth`,`curmana`,`MovementType`,`npcflag`,`unit_flags`,`dynamicflags`,`ScriptName`,`VerifiedBuild`,`CreateObject`,`Comment`) VALUES
(@LW_HUNTMASTER_CGUID+1,@SW_ENTRY,0,0,0,1,1,0,-8771.718,637.399,97.22381,3.0327725,300,0,0,1,0,0,0,0,0,'',0,0,'Huntsmaster - Stormwind'),
(@LW_HUNTMASTER_CGUID+2,@IF_ENTRY,0,0,0,1,1,0,-5036.303,-1189.7064,507.4897,5.23103,300,0,0,1,0,0,0,0,0,'',0,0,'Huntsmaster - Ironforge'),
(@LW_HUNTMASTER_CGUID+3,@DN_ENTRY,1,0,0,1,1,0,9947.34,2272.71,1341.47,0.017453,300,0,0,1,0,0,0,0,0,'',0,0,'Huntsmaster - Darnassus'),
(@LW_HUNTMASTER_CGUID+4,@EX_ENTRY,530,0,0,1,1,0,-4185.3833,-11559.71,-125.5796,3.652105,300,0,0,1,0,0,0,0,0,'',0,0,'Huntsmaster - Exodar'),
(@LW_HUNTMASTER_CGUID+5,@OG_ENTRY,1,0,0,1,1,0,1857.5254,-4516.36,24.02204,3.5012627,300,0,0,1,0,0,0,0,0,'',0,0,'Huntsmaster - Orgrimmar'),
(@LW_HUNTMASTER_CGUID+6,@TB_ENTRY,1,0,0,1,1,0,-1401.8701,-144.85625,159.25444,1.7206132,300,0,0,1,0,0,0,0,0,'',0,0,'Huntsmaster - Thunder Bluff'),
(@LW_HUNTMASTER_CGUID+7,@UC_ENTRY,0,0,0,1,1,0,1476.3986,36.700302,-62.353333,1.599211,300,0,0,1,0,0,0,0,0,'',0,0,'Huntsmaster - Undercity'),
(@LW_HUNTMASTER_CGUID+8,@SM_ENTRY,530,0,0,1,1,0,9801.341,-7324.399,14.6854105,2.006077,300,0,0,1,0,0,0,0,0,'',0,0,'Huntsmaster - Silvermoon'),
(@LW_HUNTMASTER_CGUID+9,@SH_ENTRY,530,0,0,1,1,0,-2019.2322,5203.5225,-35.69525,5.916366,300,0,0,1,0,0,0,0,0,'',0,0,'Huntsmaster - Shattrath'),
(@LW_HUNTMASTER_CGUID+10,@DA_ENTRY,571,0,0,1,1,0,5773.8013,548.9488,651.6386,0.8435841,300,0,0,1,0,0,0,0,0,'',0,0,'Huntsmaster - Dalaran');

DELETE FROM `hunt_giver` WHERE `id` BETWEEN 1 AND 10;
INSERT INTO `hunt_giver` (`id`,`creature_entry`,`city_name`,`map_id`,`continent_id`,`x`,`y`,`z`,`enabled`,`comment`) VALUES
(1,@SW_ENTRY,'Stormwind City',0,1,-8771.718,637.399,97.22381,1,'Alliance capital Huntmaster'),
(2,@IF_ENTRY,'Ironforge',0,1,-5036.303,-1189.7064,507.4897,1,'Alliance capital Huntmaster'),
(3,@DN_ENTRY,'Darnassus',1,2,9947.34,2272.71,1341.47,1,'Alliance capital Huntmaster'),
(4,@EX_ENTRY,'The Exodar',530,2,-4185.3833,-11559.71,-125.5796,1,'Alliance capital Huntmaster'),
(5,@OG_ENTRY,'Orgrimmar',1,2,1857.5254,-4516.36,24.02204,1,'Horde capital Huntmaster'),
(6,@TB_ENTRY,'Thunder Bluff',1,2,-1401.8701,-144.85625,159.25444,1,'Horde capital Huntmaster'),
(7,@UC_ENTRY,'Undercity',0,1,1476.3986,36.700302,-62.353333,1,'Horde capital Huntmaster'),
(8,@SM_ENTRY,'Silvermoon City',530,1,9801.341,-7324.399,14.6854105,1,'Horde capital Huntmaster'),
(9,@SH_ENTRY,'Shattrath City',530,3,-2019.2322,5203.5225,-35.69525,1,'Neutral Outland hub Huntmaster'),
(10,@DA_ENTRY,'Dalaran',571,4,5773.8013,548.9488,651.6386,1,'Neutral Northrend hub Huntmaster');

DELETE FROM `hunt_guard_locator` WHERE `id` BETWEEN 1 AND 14;
INSERT INTO `hunt_guard_locator` (`id`,`guard_creature_entry`,`hunt_giver_id`,`enabled`,`comment`) VALUES
(1,68,1,1,'Stormwind City Guard'),
(2,1976,1,1,'Stormwind City Patroller'),
(3,5595,2,1,'Ironforge Guard'),
(4,4262,3,1,'Darnassus Sentinel'),
(5,16733,4,1,'Exodar Peacekeeper'),
(6,20674,4,1,'Shield of Velen'),
(7,3296,5,1,'Orgrimmar Grunt'),
(8,3084,6,1,'Bluffwatcher'),
(9,5624,7,1,'Undercity Guardian'),
(10,36213,7,1,'Kor''kron Overseer - Wrath-era Undercity'),
(11,16222,8,1,'Silvermoon City Guardian'),
(12,19687,9,1,'Shattrath City Peacekeeper'),
(13,30659,10,1,'Violet Hold Guard - Dalaran'),
(14,32691,10,1,'Magus Fansy Goodbringer - Dalaran information giver');