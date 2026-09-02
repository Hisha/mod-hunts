-- Hunts 1.0.8-dev - Elite Hunt prey roster
-- Canonical Elite prey content only. Schema changes belong in db-world/updates.
SET @HUNT_ACTIVATOR_ENTRY := 14999010;

DELETE FROM `hunt_prey_ability` WHERE `prey_id` IN (100,101,102,103,104);
DELETE FROM `hunt_prey` WHERE `id` IN (100,101,102,103,104);
DELETE FROM `hunt_creature_template` WHERE `id` IN (1016,1017,1018,1019,1020);

-- ---------------------------------------------------------------------------
-- The Oathbreaker - Retribution Paladin archetype
-- ---------------------------------------------------------------------------
INSERT INTO `hunt_creature_template`
 (`id`,`name`,`base_creature_entry`,`name_override`,`subname_override`,`faction_override`,`rank_override`,
  `health_modifier_override`,`armor_modifier_override`,`damage_modifier_override`,`enabled`,`comment`) VALUES
(1016,'Elite Hunt - The Oathbreaker',3976,'The Oathbreaker','Fallen Hand of the Light',14,1,1.0,1.20,1.12,1,
 'Elite Retribution-Paladin-style prey; Scarlet Commander Mograine provides an existing armored humanoid shell.');

INSERT INTO `hunt_prey`
 (`id`,`name`,`min_level`,`max_level`,`prey_creature_entry`,`prey_template_id`,`activation_gameobject_entry`,
  `ambush_health_multiplier`,`final_health_multiplier`,`reward_multiplier`,`tier`,`combat_style`,`preferred_range`,
  `escape_health_pct`,`ambush_count`,`enabled`,`comment`) VALUES
(100,'The Oathbreaker',10,80,0,1016,@HUNT_ACTIVATOR_ENTRY,5.0,8.0,2.5,2,0,0,50,2,1,
 'Elite Retribution-Paladin-style prey; once-daily Elite Hunt.');

INSERT INTO `hunt_prey_ability`
 (`id`,`prey_id`,`spell_id`,`target`,`initial_min_ms`,`initial_max_ms`,`cooldown_min_ms`,`cooldown_max_ms`,
  `chance_pct`,`encounter_mask`,`min_hunter_level`,`max_hunter_level`,`health_below_pct`,`victim_health_below_pct`,`require_melee`,
  `once_per_encounter`,`require_aura_missing`,`enabled`,`comment`) VALUES
(100001,100,20375,1,0,500,30000,30000,100,3,10,80,0,0,0,0,1,1,'Seal of Command - keep seal active'),
(100002,100,20271,0,2500,4500,8000,11000,100,3,10,80,0,0,0,0,0,1,'Judgement'),
(100003,100,35395,0,1500,3000,6000,8000,100,3,20,80,0,0,1,0,0,1,'Crusader Strike'),
(100004,100,48819,1,5000,8000,12000,16000,85,2,30,80,0,0,1,0,0,1,'Consecration - final'),
(100005,100,10308,0,7000,11000,28000,35000,70,2,30,80,0,0,1,0,0,1,'Hammer of Justice - final'),
(100006,100,53385,0,4000,6500,10000,13000,100,2,40,80,0,0,1,0,0,1,'Divine Storm - final'),
(100007,100,31884,1,0,0,60000,60000,100,2,60,80,40,0,0,1,0,1,'Avenging Wrath below 40% - once per final');

-- ---------------------------------------------------------------------------
-- The Winterborn - Frost Mage archetype
-- Archmage Arugal supplies a stock 3.3.5a robed caster shell. Combat style 1
-- tells the Hunt runtime to maintain a ranged chase band instead of melee.
-- ---------------------------------------------------------------------------
INSERT INTO `hunt_creature_template`
 (`id`,`name`,`base_creature_entry`,`name_override`,`subname_override`,`faction_override`,`rank_override`,
  `health_modifier_override`,`armor_modifier_override`,`damage_modifier_override`,`enabled`,`comment`) VALUES
(1017,'Elite Hunt - The Winterborn',4275,'The Winterborn','Exiled Master of the Frozen Arts',14,1,0.90,0.85,1.08,1,
 'Elite Frost-Mage-style prey; ranged control and defensive magic define the encounter.');

INSERT INTO `hunt_prey`
 (`id`,`name`,`min_level`,`max_level`,`prey_creature_entry`,`prey_template_id`,`activation_gameobject_entry`,
  `ambush_health_multiplier`,`final_health_multiplier`,`reward_multiplier`,`tier`,`combat_style`,`preferred_range`,
  `escape_health_pct`,`ambush_count`,`enabled`,`comment`) VALUES
(101,'The Winterborn',10,80,0,1017,@HUNT_ACTIVATOR_ENTRY,4.5,7.25,2.5,2,1,22.0,50,2,1,
 'Elite Frost Mage archetype; keeps range, roots/slows, shields, and uses Ice Block late in the final.');

INSERT INTO `hunt_prey_ability`
 (`id`,`prey_id`,`spell_id`,`target`,`initial_min_ms`,`initial_max_ms`,`cooldown_min_ms`,`cooldown_max_ms`,
  `chance_pct`,`encounter_mask`,`min_hunter_level`,`max_hunter_level`,`health_below_pct`,`victim_health_below_pct`,`require_melee`,
  `once_per_encounter`,`require_aura_missing`,`enabled`,`comment`) VALUES
(101001,101,42842,0,1000,2500,2500,4000,100,3,10,80,0,0,0,0,0,1,'Frostbolt - primary ranged pressure'),
(101002,101,42917,0,3500,6000,18000,24000,100,3,10,80,0,0,0,0,0,1,'Frost Nova - create distance'),
(101006,101,1953,1,1500,3000,15000,15000,100,3,20,80,0,0,0,0,0,1,'Blink - escape when pressured inside panic range'),
(101003,101,43039,1,500,1500,24000,32000,100,3,20,80,0,0,0,0,1,1,'Ice Barrier - keep defensive shield available'),
(101004,101,42931,0,6000,9000,10000,14000,75,2,30,80,0,0,0,0,0,1,'Cone of Cold - final close-range punishment'),
(101005,101,45438,1,0,0,60000,60000,100,2,50,80,25,0,0,1,0,1,'Ice Block below 25% - once per final');

-- ---------------------------------------------------------------------------
-- The Headsman - Arms Warrior archetype
-- Herod supplies a stock armored two-handed melee shell. Default combat style
-- remains melee; Charge/Hamstring/Mortal Strike keep pressure on the hunter.
-- ---------------------------------------------------------------------------
INSERT INTO `hunt_creature_template`
 (`id`,`name`,`base_creature_entry`,`name_override`,`subname_override`,`faction_override`,`rank_override`,
  `health_modifier_override`,`armor_modifier_override`,`damage_modifier_override`,`enabled`,`comment`) VALUES
(1018,'Elite Hunt - The Headsman',3975,'The Headsman','Executioner Without a Banner',14,1,1.10,1.20,1.15,1,
 'Elite Arms-Warrior-style prey; closes distance and maintains heavy melee pressure.');

INSERT INTO `hunt_prey`
 (`id`,`name`,`min_level`,`max_level`,`prey_creature_entry`,`prey_template_id`,`activation_gameobject_entry`,
  `ambush_health_multiplier`,`final_health_multiplier`,`reward_multiplier`,`tier`,`combat_style`,`preferred_range`,
  `escape_health_pct`,`ambush_count`,`enabled`,`comment`) VALUES
(102,'The Headsman',10,80,0,1018,@HUNT_ACTIVATOR_ENTRY,5.25,8.50,2.5,2,0,0,50,2,1,
 'Elite Arms Warrior archetype; Charge, Hamstring, Mortal Strike, fear, and Execute-style finisher.');

INSERT INTO `hunt_prey_ability`
 (`id`,`prey_id`,`spell_id`,`target`,`initial_min_ms`,`initial_max_ms`,`cooldown_min_ms`,`cooldown_max_ms`,
  `chance_pct`,`encounter_mask`,`min_hunter_level`,`max_hunter_level`,`health_below_pct`,`victim_health_below_pct`,`require_melee`,
  `once_per_encounter`,`require_aura_missing`,`enabled`,`comment`) VALUES
(102001,102,11578,0,0,1200,14000,18000,100,3,10,80,0,0,0,0,0,1,'Charge - close distance'),
(102002,102,25212,0,1000,2200,8000,10000,100,3,10,80,0,0,1,0,1,1,'Hamstring - hold the hunter in melee'),
(102006,102,47465,0,1800,3200,12000,15000,100,3,10,80,0,0,1,0,1,1,'Rend - maintain a visible bleed'),
(102003,102,47486,0,2500,4000,6000,8000,100,3,20,80,0,0,1,0,0,1,'Mortal Strike - primary Arms pressure'),
(102007,102,1680,1,4500,6500,9000,12000,100,3,20,80,0,0,1,0,0,1,'Whirlwind - unmistakable heavy melee pressure'),
(102004,102,5246,0,8000,12000,28000,36000,70,2,30,80,0,0,1,0,0,1,'Intimidating Shout - final control'),
(102005,102,47471,0,0,0,5000,7000,100,2,40,80,0,20,1,0,0,1,'Execute - finisher when the hunter is below 20%');


-- ---------------------------------------------------------------------------
-- The Veiled Knife - Subtlety/Assassination Rogue archetype
-- Edwin VanCleef supplies a stock dual-wield humanoid shell. The Hunt AI
-- triggers resource/combo-driven Rogue techniques so the creature does not
-- depend on player-only energy/combo-point state.
-- ---------------------------------------------------------------------------
INSERT INTO `hunt_creature_template`
 (`id`,`name`,`base_creature_entry`,`name_override`,`subname_override`,`faction_override`,`rank_override`,
  `health_modifier_override`,`armor_modifier_override`,`damage_modifier_override`,`enabled`,`comment`) VALUES
(1019,'Elite Hunt - The Veiled Knife',639,'The Veiled Knife','Blade in the Dark',14,1,0.95,0.90,1.16,1,
 'Elite Rogue-style prey; control, bleeds, avoidance, and burst define the encounter.');

INSERT INTO `hunt_prey`
 (`id`,`name`,`min_level`,`max_level`,`prey_creature_entry`,`prey_template_id`,`activation_gameobject_entry`,
  `ambush_health_multiplier`,`final_health_multiplier`,`reward_multiplier`,`tier`,`combat_style`,`preferred_range`,
  `escape_health_pct`,`ambush_count`,`enabled`,`comment`) VALUES
(103,'The Veiled Knife',10,80,0,1019,@HUNT_ACTIVATOR_ENTRY,4.75,7.75,2.5,2,0,0,50,2,1,
 'Elite Rogue archetype; fast melee control, persistent bleed pressure, Evasion, and finishing burst.');

INSERT INTO `hunt_prey_ability`
 (`id`,`prey_id`,`spell_id`,`target`,`initial_min_ms`,`initial_max_ms`,`cooldown_min_ms`,`cooldown_max_ms`,
  `chance_pct`,`encounter_mask`,`min_hunter_level`,`max_hunter_level`,`health_below_pct`,`victim_health_below_pct`,`require_melee`,
  `once_per_encounter`,`require_aura_missing`,`enabled`,`comment`) VALUES
(103001,103,48638,0,900,1800,3500,5000,100,3,10,80,0,0,1,0,0,1,'Sinister Strike - basic melee pressure'),
(103002,103,48672,0,1800,3200,11000,14000,100,3,20,80,0,0,1,0,1,1,'Rupture - maintain a bleed'),
(103003,103,1776,0,4500,6500,18000,24000,80,3,20,80,0,0,1,0,0,1,'Gouge - short melee control'),
(103004,103,26669,1,0,0,45000,45000,100,3,30,80,35,0,0,1,0,1,'Evasion below 35% - once per encounter'),
(103005,103,8643,0,6500,9000,22000,28000,80,2,40,80,0,0,1,0,0,1,'Kidney Shot - final control'),
(103006,103,48668,0,3500,5500,7000,9000,100,2,40,80,0,30,1,0,0,1,'Eviscerate - finishing burst when hunter is below 30%');

-- ---------------------------------------------------------------------------
-- The Ashen Pact - Affliction/Destruction Warlock archetype
-- Grand Warlock Nethekurse supplies a stock demonic-caster shell. Combat style
-- 1 keeps casting distance, but unlike Winterborn this prey controls space with
-- fear and damage-over-time pressure instead of roots/Blink.
-- ---------------------------------------------------------------------------
INSERT INTO `hunt_creature_template`
 (`id`,`name`,`base_creature_entry`,`name_override`,`subname_override`,`faction_override`,`rank_override`,
  `health_modifier_override`,`armor_modifier_override`,`damage_modifier_override`,`enabled`,`comment`) VALUES
(1020,'Elite Hunt - The Ashen Pact',16807,'The Ashen Pact','Bearer of the Black Covenant',14,1,1.00,0.90,1.10,1,
 'Elite Warlock-style prey; ranged curses, damage over time, fear, and life-draining pressure.');

INSERT INTO `hunt_prey`
 (`id`,`name`,`min_level`,`max_level`,`prey_creature_entry`,`prey_template_id`,`activation_gameobject_entry`,
  `ambush_health_multiplier`,`final_health_multiplier`,`reward_multiplier`,`tier`,`combat_style`,`preferred_range`,
  `escape_health_pct`,`ambush_count`,`enabled`,`comment`) VALUES
(104,'The Ashen Pact',10,80,0,1020,@HUNT_ACTIVATOR_ENTRY,4.75,7.75,2.5,2,1,24.0,50,2,1,
 'Elite Warlock archetype; maintains range while layering DoTs, fear, direct shadow damage, and Drain Life.');

INSERT INTO `hunt_prey_ability`
 (`id`,`prey_id`,`spell_id`,`target`,`initial_min_ms`,`initial_max_ms`,`cooldown_min_ms`,`cooldown_max_ms`,
  `chance_pct`,`encounter_mask`,`min_hunter_level`,`max_hunter_level`,`health_below_pct`,`victim_health_below_pct`,`require_melee`,
  `once_per_encounter`,`require_aura_missing`,`enabled`,`comment`) VALUES
(104001,104,47809,0,800,1800,3000,4500,100,3,10,80,0,0,0,0,0,1,'Shadow Bolt - primary ranged cast'),
(104002,104,47813,0,1200,2400,15000,18000,100,3,10,80,0,0,0,0,1,1,'Corruption - maintain damage over time'),
(104003,104,47864,0,2200,3800,18000,22000,100,3,20,80,0,0,0,0,1,1,'Curse of Agony - maintain curse pressure'),
(104004,104,6215,0,5500,8000,22000,28000,85,3,20,80,0,0,0,0,0,1,'Fear - create casting space'),
(104005,104,47811,0,3500,5500,12000,16000,90,2,30,80,0,0,0,0,1,1,'Immolate - final additional DoT pressure'),
(104006,104,47857,0,6500,9000,14000,18000,85,2,40,80,45,0,0,0,0,1,'Drain Life below 45% - sustain in final'),
(104007,104,47860,0,0,0,26000,32000,100,2,50,80,30,0,0,1,0,1,'Death Coil below 30% - once per final');
