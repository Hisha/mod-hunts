-- ============================================================================
-- Hunts Hunt - legendary beast prey collection
--
-- All visuals and spells use existing 3.3.5a assets; no client patch is needed.
-- Combat abilities are data-driven through hunt_prey_ability.
-- ============================================================================
SET @HUNT_ACTIVATOR_ENTRY := 14999010;

DELETE FROM `hunt_prey_ability` WHERE `prey_id` BETWEEN 1 AND 16;
DELETE FROM `hunt_prey` WHERE `id` BETWEEN 1 AND 16;
DELETE FROM `hunt_creature_template` WHERE `id` BETWEEN 1000 AND 1015;

INSERT INTO `hunt_creature_template`
 (`id`,`name`,`base_creature_entry`,`name_override`,`subname_override`,`faction_override`,`rank_override`,
  `health_modifier_override`,`armor_modifier_override`,`damage_modifier_override`,`enabled`,`comment`) VALUES
(1000,'Hunt Prey - Ashfang',448,'Ashfang','The Grey Terror',14,1,1.0,1.15,1.05,1,'Legendary wolf prey; Hogger supplies the existing client model shell.'),
(1001,'Hunt Prey - Silkmaw',30,'Silkmaw','The Webbed Horror',14,1,1.0,1.05,0.95,1,'Legendary spider prey; Forest Spider supplies the existing client model shell.'),
(1002,'Hunt Prey - Gorehide',157,'Gorehide','The Iron Tusk',14,1,1.0,1.25,1.10,1,'Legendary boar prey; Goretusk supplies the existing client model shell.'),
(1003,'Hunt Prey - Whiteclaw',1130,'Whiteclaw','The Mountain Ghost',14,1,1.0,1.25,1.10,1,'Legendary bear prey; Bjarn supplies the existing client model shell.'),
(1004,'Hunt Prey - Tidefang',391,'Tidefang','Terror of the Shore',14,1,1.0,1.10,1.15,1,'Legendary murloc prey; Old Murk-Eye supplies the existing client model shell.'),
(1005,'Hunt Prey - Stonegut',1210,'Stonegut','The Wandering Brute',14,1,1.0,1.30,1.20,1,'Legendary ogre prey; Chok''sul supplies the existing client model shell.'),
(1006,'Hunt Prey - Sootfang',1225,'Sootfang','The Black Bear',14,1,1.0,1.25,1.15,1,'Legendary bear prey; Ol'' Sooty supplies the existing client model shell.'),
(1007,'Hunt Prey - Nightfang',521,'Nightfang','The Shadow Wolf',14,1,1.0,1.15,1.20,1,'Legendary wolf prey; Lupos supplies the existing client model shell.'),
(1008,'Hunt Prey - Shadowclaw',728,'Shadowclaw','The Jungle Stalker',14,1,1.0,1.10,1.20,1,'Legendary cat prey; Bhag''thera supplies the existing client model shell.'),
(1009,'Hunt Prey - Dreadwing',10357,'Dreadwing','The Night Screamer',14,1,1.0,1.00,1.10,1,'Legendary bat prey; Ressan the Needler supplies the existing client model shell.'),
(1010,'Hunt Prey - Venomtail',5937,'Venomtail','The Death Sting',14,1,1.0,1.25,1.10,1,'Legendary scorpid prey; Vile Sting supplies the existing client model shell.'),
(1011,'Hunt Prey - Stormcoil',5834,'Stormcoil','The Thunder Serpent',14,1,1.0,1.05,1.15,1,'Legendary wind serpent prey; Azzere the Skyblade supplies the existing client model shell.'),
(1012,'Hunt Prey - Mirejaw',14233,'Mirejaw','The Ancient Maw',14,1,1.0,1.25,1.15,1,'Legendary crocolisk prey; Ripscale supplies the existing client model shell.'),
(1013,'Hunt Prey - Razortalon',14232,'Razortalon','The Blood Raptor',14,1,1.0,1.10,1.25,1,'Legendary raptor prey; Dart supplies the existing client model shell.'),
(1014,'Hunt Prey - Cliffhowl',8211,'Cliffhowl','The Ridge Hunter',14,1,1.0,1.15,1.15,1,'Legendary wolf prey; Old Cliff Jumper supplies the existing client model shell.'),
(1015,'Hunt Prey - Grimmaw',8215,'Grimmaw','The Forest Breaker',14,1,1.0,1.30,1.20,1,'Legendary bear prey; Grimungous supplies the existing client model shell.');

INSERT INTO `hunt_prey`
 (`id`,`name`,`min_level`,`max_level`,`prey_creature_entry`,`prey_template_id`,`activation_gameobject_entry`,
  `ambush_health_multiplier`,`final_health_multiplier`,`reward_multiplier`,`tier`,`escape_health_pct`,`ambush_count`,`enabled`,`comment`) VALUES
(1,'Ashfang',10,80,0,1000,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Bleed/frenzy wolf.'),
(2,'Silkmaw',10,80,0,1001,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Web/poison spider.'),
(3,'Gorehide',10,80,0,1002,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Charging boar.'),
(4,'Whiteclaw',10,80,0,1003,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Knockdown/roar bear.'),
(5,'Tidefang',10,80,0,1004,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Thrashing shoreline terror.'),
(6,'Stonegut',10,80,0,1005,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Knockdown/thrash brute.'),
(7,'Sootfang',10,80,0,1006,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Roaring/frenzied bear.'),
(8,'Nightfang',10,80,0,1007,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Bleed/frenzy wolf.'),
(9,'Shadowclaw',10,80,0,1008,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Bleeding jungle cat.'),
(10,'Dreadwing',10,80,0,1009,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Silencing/fear bat.'),
(11,'Venomtail',10,80,0,1010,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Poisonous scorpid.'),
(12,'Stormcoil',10,80,0,1011,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Fast-striking wind serpent.'),
(13,'Mirejaw',10,80,0,1012,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Knockdown crocolisk.'),
(14,'Razortalon',10,80,0,1013,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Bleed/thrash raptor.'),
(15,'Cliffhowl',10,80,0,1014,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Fear/bleed wolf.'),
(16,'Grimmaw',10,80,0,1015,@HUNT_ACTIVATOR_ENTRY,4.0,6.0,1.0,1,50,2,1,'Knockdown/enrage bear.');

-- target: 0=victim, 1=self. encounter_mask: 1=ambush, 2=final, 3=both.
-- Spell IDs below are existing 3.3.5a creature abilities. Long cooldowns keep
-- control effects flavorful without allowing them to chain-lock the hunter.
INSERT INTO `hunt_prey_ability`
 (`id`,`prey_id`,`spell_id`,`target`,`initial_min_ms`,`initial_max_ms`,`cooldown_min_ms`,`cooldown_max_ms`,`chance_pct`,`encounter_mask`,`min_hunter_level`,`max_hunter_level`,`health_below_pct`,`require_melee`,`once_per_encounter`,`require_aura_missing`,`enabled`,`comment`) VALUES
(1001,1,13443,0,4000,7000,14000,19000,80,3,1,80,0,0,0,0,1,'Ashfang - Rend'),
(1002,1,19615,1,9000,13000,24000,32000,70,2,1,80,0,0,0,0,1,'Ashfang - Frenzy Effect (final only)'),
(2001,2,4167,0,5000,8000,18000,24000,100,3,1,80,0,0,0,0,1,'Silkmaw - Web'),
(2002,2,18197,0,2500,5000,12000,17000,80,3,1,80,0,0,0,0,1,'Silkmaw - Poison'),
(3001,3,25999,0,500,1500,18000,24000,100,3,1,80,0,0,0,0,1,'Gorehide - Charge'),
(3002,3,3391,1,8000,12000,22000,30000,60,2,1,80,0,0,0,0,1,'Gorehide - Thrash (final only)'),
(4001,4,5164,0,5000,8000,18000,24000,75,3,1,80,0,0,0,0,1,'Whiteclaw - Knockdown'),
(4002,4,15971,1,7000,11000,24000,32000,75,3,1,80,0,0,0,0,1,'Whiteclaw - Demoralizing Roar'),
(5001,5,3391,1,5000,9000,16000,22000,75,3,1,80,0,0,0,0,1,'Tidefang - Thrash'),
(5002,5,11428,0,9000,13000,24000,32000,60,2,1,80,0,0,0,0,1,'Tidefang - Knockdown (final only)'),
(6001,6,5164,0,5000,8000,18000,24000,70,3,1,80,0,0,0,0,1,'Stonegut - Knockdown'),
(6002,6,3391,1,7000,11000,18000,26000,70,3,1,80,0,0,0,0,1,'Stonegut - Thrash'),
(7001,7,15971,1,4000,7000,22000,30000,80,3,1,80,0,0,0,0,1,'Sootfang - Demoralizing Roar'),
(7002,7,19615,1,9000,13000,24000,32000,70,2,1,80,0,0,0,0,1,'Sootfang - Frenzy Effect (final only)'),
(8001,8,13443,0,3500,6500,13000,18000,85,3,1,80,0,0,0,0,1,'Nightfang - Rend'),
(8002,8,19615,1,7000,11000,22000,30000,70,3,1,80,0,0,0,0,1,'Nightfang - Frenzy Effect'),
(9001,9,13443,0,3000,6000,12000,17000,90,3,1,80,0,0,0,0,1,'Shadowclaw - Rend'),
(9002,9,5164,0,8000,12000,24000,32000,60,2,1,80,0,0,0,0,1,'Shadowclaw - Knockdown (final only)'),
(10001,10,8281,1,5000,9000,22000,30000,70,3,1,80,0,0,0,0,1,'Dreadwing - Sonic Burst'),
(10002,10,14100,1,10000,15000,30000,40000,55,2,1,80,0,0,0,0,1,'Dreadwing - Terrifying Roar (final only)'),
(11001,11,18197,0,2500,5000,12000,17000,85,3,1,80,0,0,0,0,1,'Venomtail - Poison'),
(11002,11,5164,0,8000,12000,22000,30000,60,2,1,80,0,0,0,0,1,'Venomtail - Knockdown (final only)'),
(12001,12,3391,1,5000,8000,16000,22000,80,3,1,80,0,0,0,0,1,'Stormcoil - Thrash'),
(12002,12,19615,1,9000,13000,24000,32000,65,2,1,80,0,0,0,0,1,'Stormcoil - Frenzy Effect (final only)'),
(13001,13,5164,0,4500,7500,18000,24000,75,3,1,80,0,0,0,0,1,'Mirejaw - Knockdown'),
(13002,13,13443,0,7000,11000,16000,22000,75,3,1,80,0,0,0,0,1,'Mirejaw - Rend'),
(14001,14,13443,0,3000,6000,12000,17000,90,3,1,80,0,0,0,0,1,'Razortalon - Rend'),
(14002,14,3391,1,6000,9000,16000,22000,80,3,1,80,0,0,0,0,1,'Razortalon - Thrash'),
(15001,15,13443,0,3500,6500,13000,18000,85,3,1,80,0,0,0,0,1,'Cliffhowl - Rend'),
(15002,15,14100,1,9000,14000,30000,40000,50,2,1,80,0,0,0,0,1,'Cliffhowl - Terrifying Roar (final only)'),
(16001,16,5164,0,4500,7500,18000,24000,75,3,1,80,0,0,0,0,1,'Grimmaw - Knockdown'),
(16002,16,8599,1,9000,13000,26000,34000,65,2,1,80,0,0,0,0,1,'Grimmaw - Enrage (final only)');
