-- Persistent Hunt statistics. Reward-quality counters are created now so the
-- later reward system can apply daily/recent anti-farming weights without a
-- second redesign of the character data model.
CREATE TABLE IF NOT EXISTS `hunt_stats` (
  `guid` INT UNSIGNED NOT NULL,
  `total_completed` INT UNSIGNED NOT NULL DEFAULT 0,
  `daily_completed` INT UNSIGNED NOT NULL DEFAULT 0,
  `daily_reset_date` DATE NULL,
  `greens_received` INT UNSIGNED NOT NULL DEFAULT 0,
  `blues_received` INT UNSIGNED NOT NULL DEFAULT 0,
  `epics_received` INT UNSIGNED NOT NULL DEFAULT 0,
  `elite_total_completed` INT UNSIGNED NOT NULL DEFAULT 0,
  `elite_daily_completed` INT UNSIGNED NOT NULL DEFAULT 0,
  `elite_daily_reset_date` DATE NULL,
  `huntmaster_seals` INT UNSIGNED NOT NULL DEFAULT 0,
  `last_completed_at` TIMESTAMP NULL DEFAULT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;