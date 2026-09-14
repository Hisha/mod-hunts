-- Hunts custom object range: activation marker 14999010, return rift 14999011.
-- Stock 3.3.5a display 9529: gameobject_template 19529, Instance Portal Red.
-- Deliberately a scripted goober, with no spell, quest, shared cooldown or charges.
-- Persistent, convergent content; no character schema changes or permanent spawns.
DELETE FROM `gameobject_template` WHERE `entry` = 14999011;
INSERT INTO `gameobject_template`
    (`entry`,`type`,`displayId`,`name`,`IconName`,`castBarCaption`,`unk1`,`size`,
     `Data0`,`Data1`,`Data2`,`Data3`,`Data4`,`Data5`,`Data6`,`Data7`,
     `Data8`,`Data9`,`Data10`,`Data11`,`Data12`,`Data13`,`Data14`,`Data15`,
     `Data16`,`Data17`,`Data18`,`Data19`,`Data20`,`Data21`,`Data22`,`Data23`,
     `AIName`,`ScriptName`,`VerifiedBuild`)
VALUES
    (14999011,10,1327,'Return Rift','','','',1.0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     '','mod_hunts_return_rift',12340);
