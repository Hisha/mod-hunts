-- Hunts 0.7.0-dev - first Elite Hunt prey
SET @HUNT_ACTIVATOR_ENTRY := 14999010;

DELETE FROM `hunt_prey_ability` WHERE `prey_id`=100;
DELETE FROM `hunt_prey` WHERE `id`=100;
DELETE FROM `hunt_creature_template` WHERE `id`=1016;

INSERT INTO `hunt_creature_template`
 (`id`,`name`,`base_creature_entry`,`name_override`,`subname_override`,`faction_override`,`rank_override`,
  `health_modifier_override`,`armor_modifier_override`,`damage_modifier_override`,`enabled`,`comment`) VALUES
(1016,'Elite Hunt - The Oathbreaker',3976,'The Oathbreaker','Fallen Hand of the Light',14,1,1.0,1.20,1.12,1,
 'Elite humanoid prey; Scarlet Commander Mograine provides an existing armored humanoid shell.');

INSERT INTO `hunt_prey`
 (`id`,`name`,`min_level`,`max_level`,`prey_creature_entry`,`prey_template_id`,`activation_gameobject_entry`,
  `ambush_health_multiplier`,`final_health_multiplier`,`reward_multiplier`,`tier`,`escape_health_pct`,`ambush_count`,`enabled`,`comment`) VALUES
(100,'The Oathbreaker',10,80,0,1016,@HUNT_ACTIVATOR_ENTRY,5.0,8.0,2.5,2,50,2,1,
 'Elite Retribution-Paladin-style prey; once-daily Elite Hunt.');

-- Conditions allow the same prey to gain class-kit complexity as the hunter levels.
INSERT INTO `hunt_prey_ability`
 (`id`,`prey_id`,`spell_id`,`target`,`initial_min_ms`,`initial_max_ms`,`cooldown_min_ms`,`cooldown_max_ms`,
  `chance_pct`,`encounter_mask`,`min_hunter_level`,`max_hunter_level`,`health_below_pct`,`require_melee`,
  `once_per_encounter`,`require_aura_missing`,`enabled`,`comment`) VALUES
(100001,100,20375,1,0,500,30000,30000,100,3,10,80,0,0,0,1,1,'Seal of Command - keep seal active'),
(100002,100,20271,0,2500,4500,8000,11000,100,3,10,80,0,0,0,0,1,'Judgement'),
(100003,100,35395,0,1500,3000,6000,8000,100,3,20,80,0,1,0,0,1,'Crusader Strike'),
(100004,100,48819,1,5000,8000,12000,16000,85,2,30,80,0,1,0,0,1,'Consecration - final'),
(100005,100,10308,0,7000,11000,28000,35000,70,2,30,80,0,1,0,0,1,'Hammer of Justice - final'),
(100006,100,53385,0,4000,6500,10000,13000,100,2,40,80,0,1,0,0,1,'Divine Storm - final'),
(100007,100,31884,1,0,0,60000,60000,100,2,60,80,40,0,1,0,1,'Avenging Wrath below 40% - once per final');
