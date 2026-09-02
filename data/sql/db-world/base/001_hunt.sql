-- Hunts Hunt/Prey subsystem - canonical schema
CREATE TABLE IF NOT EXISTS `hunt_prey` (
  `id` INT UNSIGNED NOT NULL,
  `name` VARCHAR(100) NOT NULL,
  `min_level` TINYINT UNSIGNED NOT NULL DEFAULT 10,
  `max_level` TINYINT UNSIGNED NOT NULL DEFAULT 80,
  `prey_creature_entry` INT UNSIGNED NOT NULL DEFAULT 0,
  `prey_template_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `activation_gameobject_entry` INT UNSIGNED NOT NULL DEFAULT 0,
  `ambush_health_multiplier` FLOAT NOT NULL DEFAULT 4.0,
  `final_health_multiplier` FLOAT NOT NULL DEFAULT 6.0,
  `reward_multiplier` FLOAT NOT NULL DEFAULT 1.0,
  `tier` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '1=normal,2=elite',
  `combat_style` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=melee/default,1=ranged/kiting',
  `preferred_range` FLOAT NOT NULL DEFAULT 0 COMMENT 'Desired combat range in yards for ranged style',
  `escape_health_pct` TINYINT UNSIGNED NOT NULL DEFAULT 50,
  `ambush_count` TINYINT UNSIGNED NOT NULL DEFAULT 2,
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `comment` VARCHAR(255) NULL,
  PRIMARY KEY (`id`),
  KEY `idx_hunt_prey_level` (`min_level`,`max_level`,`enabled`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Optional data-driven combat abilities for prey. The Hunt runtime casts these
-- from the active prey creature; no prey-specific C++ branches are required.
CREATE TABLE IF NOT EXISTS `hunt_prey_ability` (
  `id` INT UNSIGNED NOT NULL,
  `prey_id` INT UNSIGNED NOT NULL,
  `spell_id` INT UNSIGNED NOT NULL,
  `target` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=victim,1=self',
  `initial_min_ms` INT UNSIGNED NOT NULL DEFAULT 0,
  `initial_max_ms` INT UNSIGNED NOT NULL DEFAULT 0,
  `cooldown_min_ms` INT UNSIGNED NOT NULL DEFAULT 10000,
  `cooldown_max_ms` INT UNSIGNED NOT NULL DEFAULT 10000,
  `chance_pct` TINYINT UNSIGNED NOT NULL DEFAULT 100,
  `encounter_mask` TINYINT UNSIGNED NOT NULL DEFAULT 3 COMMENT '1=ambush,2=final,3=both',
  `min_hunter_level` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `max_hunter_level` TINYINT UNSIGNED NOT NULL DEFAULT 80,
  `health_below_pct` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Prey/caster health threshold; 0=ignore',
  `victim_health_below_pct` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Hunter/victim health threshold; 0=ignore',
  `require_melee` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `once_per_encounter` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `require_aura_missing` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `comment` VARCHAR(255) NULL,
  PRIMARY KEY (`id`),
  KEY `idx_hunt_prey_ability` (`prey_id`,`enabled`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- World hunting grounds are independent of prey definitions. Any Huntmaster
-- may assign any enabled level-appropriate zone; faction is intentionally not
-- represented here.
CREATE TABLE IF NOT EXISTS `hunt_zone` (
  `id` INT UNSIGNED NOT NULL,
  `zone_id` INT UNSIGNED NOT NULL,
  `map_id` SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  `continent_id` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '1=Eastern Kingdoms, 2=Kalimdor, 3=Outland, 4=Northrend',
  `name` VARCHAR(100) NOT NULL,
  `min_level` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `max_level` TINYINT UNSIGNED NOT NULL DEFAULT 80,
  `weight` INT UNSIGNED NOT NULL DEFAULT 100,
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `comment` VARCHAR(255) NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_hunt_zone_zone` (`zone_id`),
  KEY `idx_hunt_zone_level` (`min_level`,`max_level`,`enabled`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Authored final sites are zone assets, not prey assets. This lets every prey
-- reuse the same verified, settlement-safe encounter locations in a zone.
CREATE TABLE IF NOT EXISTS `hunt_final_location` (
  `id` INT UNSIGNED NOT NULL,
  `zone_id` INT UNSIGNED NOT NULL,
  `map_id` SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  `x` FLOAT NOT NULL,
  `y` FLOAT NOT NULL,
  `z` FLOAT NOT NULL,
  `orientation` FLOAT NOT NULL DEFAULT 0,
  `location_name` VARCHAR(120) NOT NULL DEFAULT '',
  `min_level` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=auto-derive from nearby world creatures',
  `max_level` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=auto-derive from nearby world creatures',
  `weight` INT UNSIGNED NOT NULL DEFAULT 100,
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `comment` VARCHAR(255) NULL,
  PRIMARY KEY (`id`),
  KEY `idx_hunt_final` (`zone_id`,`enabled`),
  KEY `idx_hunt_final_level` (`zone_id`,`min_level`,`max_level`,`enabled`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `hunt_giver` (
  `id` INT UNSIGNED NOT NULL,
  `creature_entry` INT UNSIGNED NOT NULL,
  `city_name` VARCHAR(80) NOT NULL,
  `map_id` SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  `continent_id` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '1=Eastern Kingdoms, 2=Kalimdor, 3=Outland, 4=Northrend',
  `x` FLOAT NOT NULL,
  `y` FLOAT NOT NULL,
  `z` FLOAT NOT NULL,
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `comment` VARCHAR(255) NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_hunt_giver_entry` (`creature_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Capital guards use this mapping to append "Where is the Huntmaster?" to
-- their normal direction gossip without replacing Blizzard/AzerothCore menus.
CREATE TABLE IF NOT EXISTS `hunt_guard_locator` (
  `id` INT UNSIGNED NOT NULL,
  `guard_creature_entry` INT UNSIGNED NOT NULL,
  `hunt_giver_id` INT UNSIGNED NOT NULL,
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `comment` VARCHAR(255) NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_hunt_guard_locator` (`guard_creature_entry`),
  KEY `idx_hunt_guard_giver` (`hunt_giver_id`,`enabled`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- Local hunt regions are intentionally data-driven and may overlap. Each row
-- says that a Huntmaster may use a zone when SearchScope=LocalRegion.
CREATE TABLE IF NOT EXISTS `hunt_local_region_zone` (
  `id` INT UNSIGNED NOT NULL,
  `hunt_giver_id` INT UNSIGNED NOT NULL,
  `zone_id` INT UNSIGNED NOT NULL,
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `comment` VARCHAR(255) NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_hunt_local_region_zone` (`hunt_giver_id`,`zone_id`),
  KEY `idx_hunt_local_region_giver` (`hunt_giver_id`,`enabled`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;