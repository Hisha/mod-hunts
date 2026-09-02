-- ============================================================================
-- Northrend authored Hunt final encounter sites
-- Authored final encounter sites for the nine configured Northrend hunt zones.
-- Three manually verified locations per hunt zone, authored in-game with
-- .hunt set final point.
--
-- location_name is intentionally blank. HuntManager resolves the sub-area name
-- from AreaTable.dbc at runtime using the authored coordinates and the player's
-- DBC locale, with the hunt-zone name as a fallback.
-- ============================================================================

DELETE FROM `hunt_final_location` WHERE `zone_id` IN (495,394,66,67,210,3711,3537,65,2817);

INSERT INTO `hunt_final_location` (`id`,`zone_id`,`map_id`,`x`,`y`,`z`,`orientation`,`location_name`,`weight`,`enabled`,`comment`) VALUES
(1153,495,571,1986.16,-4905.5,201.038,0.170128,'',100,1,'Howling Fjord authored final site 1'),
(1154,495,571,1710.23,-6011.74,7.22847,5.69933,'',100,1,'Howling Fjord authored final site 2'),
(1155,495,571,159.404,-4358.6,255.804,2.32595,'',100,1,'Howling Fjord authored final site 3'),
(1156,394,571,4489.43,-3839.42,211.651,5.56622,'',100,1,'Grizzly Hills authored final site 1'),
(1157,394,571,4268.92,-4997.2,26.0822,4.94185,'',100,1,'Grizzly Hills authored final site 2'),
(1158,394,571,3906.27,-2988.64,279.747,1.11696,'',100,1,'Grizzly Hills authored final site 3'),
(1159,66,571,4888.25,-3289.33,290.178,5.8262,'',100,1,'Zul''Drak authored final site 1'),
(1160,66,571,5997.32,-4145.6,376.67,5.48064,'',100,1,'Zul''Drak authored final site 2'),
(1161,66,571,5983.65,-2251.99,235.511,2.00132,'',100,1,'Zul''Drak authored final site 3'),
(1162,67,571,7019.8,-2144.56,744.868,0.100658,'',100,1,'The Storm Peaks authored final site 1'),
(1163,67,571,7851.12,-1809.34,1271.45,1.61255,'',100,1,'The Storm Peaks authored final site 2'),
(1164,67,571,7061.74,-578.163,731.003,1.58506,'',100,1,'The Storm Peaks authored final site 3'),
(1165,210,571,6645.62,842.681,360.465,0.819297,'',100,1,'Icecrown authored final site 1'),
(1166,210,571,7670.2,1759.94,348.668,0.612737,'',100,1,'Icecrown authored final site 2'),
(1167,210,571,6824.48,2882.53,450.764,2.48905,'',100,1,'Icecrown authored final site 3'),
(1168,3711,571,5922.05,3679.75,11.4575,2.51262,'',100,1,'Sholazar Basin authored final site 1'),
(1169,3711,571,5105.71,4522.76,-98.7845,1.64082,'',100,1,'Sholazar Basin authored final site 2'),
(1170,3711,571,5444.05,5755.82,-76.7616,1.57406,'',100,1,'Sholazar Basin authored final site 3'),
(1171,3537,571,4305.07,5698.29,91.8078,2.92495,'',100,1,'Borean Tundra authored final site 1'),
(1172,3537,571,3882.6,6690.55,151.577,1.04785,'',100,1,'Borean Tundra authored final site 2'),
(1173,3537,571,3206.12,4583.84,27.7831,4.64498,'',100,1,'Borean Tundra authored final site 3'),
(1174,65,571,4384.88,1061.03,149.464,1.34103,'',100,1,'Dragonblight authored final site 1'),
(1175,65,571,3414.06,1559.01,81.8544,1.92223,'',100,1,'Dragonblight authored final site 2'),
(1176,65,571,3215.48,306.041,80.457,3.91715,'',100,1,'Dragonblight authored final site 3'),
(1177,2817,571,5361.75,-103.072,147.133,4.27453,'',100,1,'Crystalsong Forest authored final site 1'),
(1178,2817,571,5825.42,290.937,188.35,1.50992,'',100,1,'Crystalsong Forest authored final site 2'),
(1179,2817,571,5449.95,817.86,176.593,1.79267,'',100,1,'Crystalsong Forest authored final site 3');