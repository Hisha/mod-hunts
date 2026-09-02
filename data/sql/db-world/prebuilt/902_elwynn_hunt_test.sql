-- ============================================================================
-- Hunts Hunt shared activation-object content
-- ============================================================================
SET @PREY_BASE_ENTRY := 448;  -- Hogger visual/combat shell only
SET @PREY_HUNT_TEMPLATE_ID := 1000;
SET @HUNT_ACTIVATOR_ENTRY := 14999010;
SET @HUNT_ACTIVATOR_VISUAL_BASE := 190558; -- Shining Crystal visual

-- Hunt final activation object: custom goober using an existing client visual.
DELETE FROM `gameobject_template` WHERE `entry`=@HUNT_ACTIVATOR_ENTRY;
INSERT INTO `gameobject_template`
    (`entry`,`type`,`displayId`,`name`,`IconName`,`castBarCaption`,`unk1`,`size`,
     `Data0`,`Data1`,`Data2`,`Data3`,`Data4`,`Data5`,`Data6`,`Data7`,
     `Data8`,`Data9`,`Data10`,`Data11`,`Data12`,`Data13`,`Data14`,`Data15`,
     `Data16`,`Data17`,`Data18`,`Data19`,`Data20`,`Data21`,`Data22`,`Data23`,
     `AIName`,`ScriptName`,`VerifiedBuild`)
SELECT
    @HUNT_ACTIVATOR_ENTRY,10,`displayId`,'Prey Trail Marker','','Inspecting','','1.0',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    '','mod_hunts_activation',0
FROM `gameobject_template` WHERE `entry`=@HUNT_ACTIVATOR_VISUAL_BASE;
