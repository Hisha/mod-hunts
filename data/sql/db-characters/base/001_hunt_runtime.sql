-- Per-character active Hunts Hunt state.
CREATE TABLE IF NOT EXISTS `hunt_runtime` (
  `guid` INT UNSIGNED NOT NULL,
  `prey_id` INT UNSIGNED NOT NULL,
  `giver_entry` MEDIUMINT UNSIGNED NOT NULL,
  `giver_spawn_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `zone_id` INT UNSIGNED NOT NULL,
  `final_location_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `tracking_progress` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `ambushes_completed` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `state` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '1=tracking,2=final located,3=ready to turn in',
  `accepted_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`guid`), KEY `idx_hunt_runtime_prey` (`prey_id`,`state`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
