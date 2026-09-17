-- Dedicated realm character database required, as for the existing hunt_stats.
-- Preserves every legacy balance; no ALTER/UPDATE of hunt_stats.
CREATE TABLE IF NOT EXISTS hunt_currency_realm (
 id TINYINT UNSIGNED NOT NULL PRIMARY KEY,
 migration_version INT UNSIGNED NOT NULL,
 state VARCHAR(16) NOT NULL,
 seal_item INT UNSIGNED NOT NULL,
 snapshot_count BIGINT UNSIGNED NOT NULL DEFAULT 0,
 snapshot_total BIGINT UNSIGNED NOT NULL DEFAULT 0,
 reward_serial BIGINT UNSIGNED NOT NULL DEFAULT 0,
 last_reward_guid INT UNSIGNED NOT NULL DEFAULT 0,
 last_reward_mail INT UNSIGNED NOT NULL DEFAULT 0,
 started_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
 completed_at TIMESTAMP NULL
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS hunt_currency_delivery (
 guid INT UNSIGNED NOT NULL PRIMARY KEY,
 legacy_amount INT UNSIGNED NOT NULL,
 delivered_amount INT UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB;
-- Rows above are delivery accounting, not per-character currency modes.
