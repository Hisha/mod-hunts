-- ============================================================================
-- Outland authored Hunt final encounter sites
-- Authored final encounter sites for the seven primary Outland hunt zones.
-- Three manually verified locations per hunt zone, authored in-game with
-- .hunt set final point.
--
-- location_name is intentionally blank. HuntManager resolves the sub-area name
-- from AreaTable.dbc at runtime using the authored coordinates and the player's
-- DBC locale, with the hunt-zone name as a fallback.
-- ============================================================================

DELETE FROM `hunt_final_location` WHERE `zone_id` IN (3483,3521,3518,3519,3520,3522,3523);

INSERT INTO `hunt_final_location` (`id`,`zone_id`,`map_id`,`x`,`y`,`z`,`orientation`,`location_name`,`weight`,`enabled`,`comment`) VALUES
(1132,3483,530,-537.548,1905.77,83.0214,4.81797,'',100,1,'Hellfire Peninsula authored final site 1'),
(1133,3483,530,-138.055,2983.86,6.82544,3.79695,'',100,1,'Hellfire Peninsula authored final site 2'),
(1134,3483,530,-422.915,3915.63,71.0766,1.13445,'',100,1,'Hellfire Peninsula authored final site 3'),
(1135,3521,530,-230.038,5824.36,22.7232,2.62985,'',100,1,'Zangarmarsh authored final site 1'),
(1136,3521,530,272.25,6847.66,33.7908,1.62061,'',100,1,'Zangarmarsh authored final site 2'),
(1137,3521,530,145.258,8286.53,23.8421,1.71093,'',100,1,'Zangarmarsh authored final site 3'),
(1138,3518,530,-678.817,8254.41,52.2484,3.54484,'',100,1,'Nagrand authored final site 1'),
(1139,3518,530,-2240.04,8271.19,-7.43892,5.91124,'',100,1,'Nagrand authored final site 2'),
(1140,3518,530,-2140.8,6358.29,49.8974,5.51854,'',100,1,'Nagrand authored final site 3'),
(1141,3519,530,-3319.85,5890.55,-21.6576,3.02883,'',100,1,'Terokkar Forest authored final site 1'),
(1142,3519,530,-3348.32,4922.83,-101.216,4.3915,'',100,1,'Terokkar Forest authored final site 2'),
(1143,3519,530,-2609.96,3740.38,2.36805,5.65206,'',100,1,'Terokkar Forest authored final site 3'),
(1144,3520,530,-3290.66,2583.18,59.8307,4.19437,'',100,1,'Shadowmoon Valley authored final site 1'),
(1145,3520,530,-3067.38,1345.07,13.0099,5.85627,'',100,1,'Shadowmoon Valley authored final site 2'),
(1146,3520,530,-4003.16,729.326,3.91577,3.36656,'',100,1,'Shadowmoon Valley authored final site 3'),
(1147,3522,530,2910.94,5735.32,141.896,5.97707,'',100,1,'Blade''s Edge Mountains authored final site 1'),
(1148,3522,530,2378.18,6633.02,9.24248,2.61164,'',100,1,'Blade''s Edge Mountains authored final site 2'),
(1149,3522,530,1703.25,5092.37,264.996,3.64444,'',100,1,'Blade''s Edge Mountains authored final site 3'),
(1150,3523,530,2610.55,3842.56,135.941,5.14458,'',100,1,'Netherstorm authored final site 1'),
(1151,3523,530,3145.96,2295.49,144.238,4.63802,'',100,1,'Netherstorm authored final site 2'),
(1152,3523,530,4788.63,3032.99,130.518,4.86187,'',100,1,'Netherstorm authored final site 3');