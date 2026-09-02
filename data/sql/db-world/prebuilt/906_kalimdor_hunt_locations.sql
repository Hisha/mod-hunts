-- ============================================================================
-- Kalimdor authored Hunt final encounter sites
-- Includes Kalimdor proper plus Azuremyst Isle and Bloodmyst Isle, which are treated as Kalimdor by Hunts hunt scope.
-- Three manually verified locations per hunt zone, authored in-game with
-- .hunt set final point.
--
-- location_name is intentionally blank. HuntManager resolves the sub-area name
-- from AreaTable.dbc at runtime using the authored coordinates and the player's
-- DBC locale, with the hunt-zone name as a fallback.
-- ============================================================================

DELETE FROM `hunt_final_location` WHERE `zone_id` IN (440,490,1377,357,400,15,17,215,405,406,331,14,16,618,361,148,141,3524,3525);

INSERT INTO `hunt_final_location` (`id`,`zone_id`,`map_id`,`x`,`y`,`z`,`orientation`,`location_name`,`min_level`,`max_level`,`weight`,`enabled`,`comment`) VALUES
(1075,440,1,-8734.69,-4624.83,8.88232,0.66522,'',0,0,100,1,'Tanaris authored final site 1'),
(1076,440,1,-7887.28,-3491.25,70.8391,0.66522,'',0,0,100,1,'Tanaris authored final site 2'),
(1077,440,1,-8835.06,-2445.92,10.8496,5.24017,'',0,0,100,1,'Tanaris authored final site 3'),
(1078,490,1,-8242.51,-1520.92,-217.891,5.24017,'',0,0,100,1,'Un''Goro Crater authored final site 1'),
(1079,490,1,-7440.58,-675.164,-271.298,5.24017,'',0,0,100,1,'Un''Goro Crater authored final site 2'),
(1080,490,1,-6966.61,-1408.84,-270.408,5.24017,'',0,0,100,1,'Un''Goro Crater authored final site 3'),
(1081,1377,1,-8035.16,1404.81,-1.02089,5.24017,'',0,0,100,1,'Silithus authored final site 1'),
(1082,1377,1,-6726.45,1764,-0.131079,5.24017,'',0,0,100,1,'Silithus authored final site 2'),
(1083,1377,1,-6603.2,232.883,6.93787,5.24017,'',0,0,100,1,'Silithus authored final site 3'),
(1084,357,1,-5540.43,1367.3,37.0197,6.23667,'',0,0,100,1,'Feralas authored final site 1'),
(1085,357,1,-4957.65,3301.72,2.08032,0.076972,'',0,0,100,1,'Feralas authored final site 2'),
(1086,357,1,-2973.31,1925.43,29.4601,5.91865,'',0,0,100,1,'Feralas authored final site 3'),
(1087,400,1,-4776.68,-995.974,-52.5865,5.31777,'',0,0,100,1,'Thousand Needles authored final site 1'),
(1088,400,1,-5362.69,-2787.01,-43.477,5.20702,'',0,0,100,1,'Thousand Needles authored final site 2'),
(1089,400,1,-5897.39,-3984.75,-58.7498,3.31027,'',0,0,100,1,'Thousand Needles authored final site 3'),
(1090,15,1,-4018.48,-2921.48,39.4915,3.91244,'',0,0,100,1,'Dustwallow Marsh authored final site 1'),
(1091,15,1,-4624.04,-3600.86,32.5479,0.853311,'',0,0,100,1,'Dustwallow Marsh authored final site 2'),
(1092,15,1,-2755.02,-3506.14,50.1019,2.97918,'',0,0,100,1,'Dustwallow Marsh authored final site 3'),
(1093,17,1,639.45,-2326.39,105.478,6.04909,'',0,0,100,1,'The Barrens authored final site 1'),
(1094,17,1,-697.473,-3264.5,97.2435,4.63931,'',0,0,100,1,'The Barrens authored final site 2'),
(1095,17,1,-1713.03,-2494.94,82.5758,3.54604,'',0,0,100,1,'The Barrens authored final site 3'),
(1096,215,1,-2192.62,-736.317,-13.3274,3.99438,'',0,0,100,1,'Mulgore authored final site 1'),
(1097,215,1,-2617.44,-964.992,-5.5982,4.39021,'',0,0,100,1,'Mulgore authored final site 2'),
(1098,215,1,-1013.61,-981.506,7.41105,1.69943,'',0,0,100,1,'Mulgore authored final site 3'),
(1099,405,1,-448.933,2461.19,104.433,0.962899,'',0,0,100,1,'Desolace authored final site 1'),
(1100,405,1,-1885.99,2277.31,61.1747,2.92247,'',0,0,100,1,'Desolace authored final site 2'),
(1101,405,1,-1302.83,1367.92,61.6085,1.80718,'',0,0,100,1,'Desolace authored final site 3'),
(1102,406,1,1623.54,898.132,123.228,1.08383,'',0,0,100,1,'Stonetalon Mountains authored final site 1'),
(1103,406,1,1156.03,-71.5774,-6.66115,6.00983,'',0,0,100,1,'Stonetalon Mountains authored final site 2'),
(1104,406,1,2428.54,1105.76,321.543,2.53994,'',0,0,100,1,'Stonetalon Mountains authored final site 3'),
(1105,331,1,1331.9,-2094.37,100.175,5.34066,'',0,0,100,1,'Ashenvale authored final site 1'),
(1106,331,1,2679.13,-3313.94,131.319,6.10956,'',0,0,100,1,'Ashenvale authored final site 2'),
(1107,331,1,3041.91,-658.45,166.508,0.504181,'',0,0,100,1,'Ashenvale authored final site 3'),
(1108,14,1,901.19,-4193.81,26.2281,4.8104,'',0,0,100,1,'Durotar authored final site 1'),
(1109,14,1,79.5683,-4255,59.4092,0.105869,'',0,0,100,1,'Durotar authored final site 2'),
(1110,14,1,-102.037,-5161.04,30.1188,1.5785,'',0,0,100,1,'Durotar authored final site 3'),
(1111,16,1,3577,-4599.91,103.13,0.737883,'',0,0,100,1,'Azshara authored final site 1'),
(1112,16,1,4430.86,-5765.92,107.106,5.3765,'',0,0,100,1,'Azshara authored final site 2'),
(1113,16,1,2515.91,-6064.72,97.8122,4.43951,'',0,0,100,1,'Azshara authored final site 3'),
(1114,618,1,6759.18,-4419.63,763.214,4.12845,'',0,0,100,1,'Winterspring authored final site 1'),
(1115,618,1,5579.73,-4542.83,774.727,5.79114,'',0,0,100,1,'Winterspring authored final site 2'),
(1116,618,1,7851.09,-4645.94,716.626,1.66234,'',0,0,100,1,'Winterspring authored final site 3'),
(1117,361,1,4259.84,-1153.87,330.447,2.23283,'',0,0,100,1,'Felwood authored final site 1'),
(1118,361,1,4811.39,-507.773,323.183,5.17649,'',0,0,100,1,'Felwood authored final site 2'),
(1119,361,1,6383.1,-1220.16,374.675,3.05984,'',0,0,100,1,'Felwood authored final site 3'),
(1120,148,1,4362.48,523.547,58.2381,4.37304,'',0,0,100,1,'Darkshore authored final site 1'),
(1121,148,1,5911.58,460.052,12.0974,1.23694,'',0,0,100,1,'Darkshore authored final site 2'),
(1122,148,1,6792.52,-478.373,43.09,0.885094,'',0,0,100,1,'Darkshore authored final site 3'),
(1123,141,1,9912.95,1305.89,1298.68,4.25051,'',0,0,100,1,'Teldrassil authored final site 1'),
(1124,141,1,9459.67,871.303,1253.16,3.54916,'',0,0,100,1,'Teldrassil authored final site 2'),
(1125,141,1,10693.2,1597.62,1284.4,0.753136,'',0,0,100,1,'Teldrassil authored final site 3'),
(1126,3524,530,-4559.14,-12365.7,20.9414,3.06699,'',0,0,100,1,'Azuremyst Isle authored final site 1'),
(1127,3524,530,-3271.53,-12808.9,20.6905,0.157066,'',0,0,100,1,'Azuremyst Isle authored final site 2'),
(1128,3524,530,-4773.92,-11926.1,28.4556,3.06306,'',0,0,100,1,'Azuremyst Isle authored final site 3'),
(1129,3525,530,-1993.62,-11475.8,63.9657,4.10449,'',0,0,100,1,'Bloodmyst Isle authored final site 1'),
(1130,3525,530,-1308.43,-11480.8,24.6384,6.20857,'',0,0,100,1,'Bloodmyst Isle authored final site 2'),
(1131,3525,530,-2643.03,-11731.2,10.4663,3.58534,'',0,0,100,1,'Bloodmyst Isle authored final site 3'),
(1187,17,1,-3550.33,-2335.34,98.5986,1.7145,'',0,0,100,1,'The Barrens in-game authored final site 4'),
(1188,440,1,-7042.16,-3595.47,10.3134,2.80935,'',0,0,100,1,'Tanaris in-game authored final site 4'),
(1191,405,1,-2285.92,1323.54,63.6063,0.446091,'',0,0,100,1,'Desolace in-game authored final site 4'),
(1192,357,1,-3063.6,2683.82,60.5281,0.582655,'',0,0,100,1,'Feralas in-game authored final site 4'),
(1193,406,1,716.475,1820.22,-11.5923,1.45531,'',0,0,100,1,'Stonetalon Mountains in-game authored final site 4'),
(1194,15,1,-4154.32,-4035.8,7.80383,1.09444,'',0,0,100,1,'Dustwallow Marsh in-game authored final site 4');
