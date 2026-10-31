-- ---------------------------------------------------------------------------
-- Player Housing - character state
--
-- Neighbourhoods, houses (one per owned plot), placed decor, the account-wide decor
-- collection, guest lists and neighbourhood membership.
--
-- Reference: docs/player-housing/03-database-schema.md
--
-- Nothing in this file alters an existing core table.
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `character_housing_neighborhood` (
  `id`              INT UNSIGNED NOT NULL COMMENT 'module allocated, see docs 03 §2.7',
  `type`            TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 public, 1 guild, 2 private',
  `name`            VARCHAR(64) NOT NULL,
  `faction`         TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `map_id`          INT UNSIGNED NOT NULL DEFAULT 0,
  `slot`            SMALLINT UNSIGNED NOT NULL COMMENT 'phase slot - unique, the identity of the phases',
  `guildId`         BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `ownerAccount`    INT UNSIGNED NOT NULL DEFAULT 0,
  `created`         INT UNSIGNED NOT NULL DEFAULT 0,
  `endeavorId`      INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'reserved, phase 3',
  `endeavorProgress` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'reserved, phase 3',
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_housing_neighborhood_slot` (`slot`),
  KEY `idx_housing_neighborhood_guild` (`guildId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Housing neighbourhoods (one phase slot each)';

CREATE TABLE IF NOT EXISTS `character_housing` (
  `id`              INT UNSIGNED NOT NULL COMMENT 'housing id, server allocated',
  `neighborhoodId`  INT UNSIGNED NOT NULL,
  `plotIndex`       TINYINT UNSIGNED NOT NULL,
  `ownerAccount`    INT UNSIGNED NOT NULL,
  `ownerGuid`       BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'character that bought it, display only',
  `ownerName`       VARCHAR(32) NOT NULL DEFAULT '',
  `tier`            TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `blueprint`       INT UNSIGNED NOT NULL DEFAULT 0,
  `name`            VARCHAR(64) NOT NULL DEFAULT '',
  `locked`          TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `budget_indoor`   INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'cached, recomputed on load',
  `budget_outdoor`  INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'cached, recomputed on load',
  `purchase_time`   INT UNSIGNED NOT NULL DEFAULT 0,
  `last_edited`     INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_housing_plot_unique` (`neighborhoodId`, `plotIndex`),
  KEY `idx_housing_owner_account` (`ownerAccount`),
  KEY `idx_housing_owner_guid` (`ownerGuid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Owned plots and their houses';

CREATE TABLE IF NOT EXISTS `character_housing_decor` (
  `id`        BIGINT UNSIGNED NOT NULL COMMENT 'placement id, server allocated',
  `housingId` INT UNSIGNED NOT NULL,
  `decorId`   INT UNSIGNED NOT NULL,
  `x`         FLOAT NOT NULL DEFAULT 0,
  `y`         FLOAT NOT NULL DEFAULT 0,
  `z`         FLOAT NOT NULL DEFAULT 0,
  `o`         FLOAT NOT NULL DEFAULT 0,
  `scale`     FLOAT NOT NULL DEFAULT 1,
  `surface`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `indoor`    TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `placed`    INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_housing_decor_house` (`housingId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Placed decor pieces';

CREATE TABLE IF NOT EXISTS `character_housing_collection` (
  `accountId`   INT UNSIGNED NOT NULL,
  `decorId`     INT UNSIGNED NOT NULL,
  `source_type` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `source_id`   INT UNSIGNED NOT NULL DEFAULT 0,
  `acquired`    INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`accountId`, `decorId`),
  KEY `idx_housing_collection_decor` (`decorId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Account-wide decor collection';

CREATE TABLE IF NOT EXISTS `character_housing_guest` (
  `housingId` INT UNSIGNED NOT NULL,
  `accountId` INT UNSIGNED NOT NULL,
  `flags`     TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'bit0 enter, bit1 edit, bit2 manage guests',
  `added`     INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`housingId`, `accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Per-house guest permissions';

CREATE TABLE IF NOT EXISTS `character_housing_neighbor_member` (
  `neighborhoodId` INT UNSIGNED NOT NULL,
  `accountId`      INT UNSIGNED NOT NULL,
  `rank`           TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 member, 1 officer, 2 leader',
  `joined`         INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`neighborhoodId`, `accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Guild/private neighbourhood membership';
