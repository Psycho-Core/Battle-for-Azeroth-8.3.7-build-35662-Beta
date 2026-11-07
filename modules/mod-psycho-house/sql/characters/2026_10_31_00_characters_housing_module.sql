-- ---------------------------------------------------------------------------
-- Player Housing - character state
--
-- Neighbourhoods, houses (one per owned plot), placed decor, the account-wide decor
-- collection, guest lists and neighbourhood membership.
--
-- Reference: legacy-prototype/03-database-schema.md
--
-- Nothing in this file alters an existing core table.
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `character_housing_neighborhood` (
 `id`       INT UNSIGNED NOT NULL COMMENT 'module allocated, see doc 03 sec.2.7',
 `type`      TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 public, 1 guild, 2 private',
 `name`      VARCHAR(64) NOT NULL,
 `faction`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
 `map_id`     INT UNSIGNED NOT NULL DEFAULT 0,
 `slot`      SMALLINT UNSIGNED NOT NULL COMMENT 'phase slot - unique, the identity of the phases',
 `guildId`     BIGINT UNSIGNED NOT NULL DEFAULT 0,
 `ownerAccount`  INT UNSIGNED NOT NULL DEFAULT 0,
 `created`     INT UNSIGNED NOT NULL DEFAULT 0,
 PRIMARY KEY (`id`),
 UNIQUE KEY `idx_housing_neighborhood_slot` (`slot`),
 KEY `idx_housing_neighborhood_guild` (`guildId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Housing neighbourhoods (one phase slot each)';

CREATE TABLE IF NOT EXISTS `character_housing` (
 `id`       INT UNSIGNED NOT NULL COMMENT 'housing id, server allocated',
 `neighborhoodId` INT UNSIGNED NOT NULL,
 `plotIndex`    TINYINT UNSIGNED NOT NULL,
 `ownerAccount`  INT UNSIGNED NOT NULL,
 `ownerGuid`    BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'character that bought it, display only',
 `ownerName`    VARCHAR(32) NOT NULL DEFAULT '',
 `tier`      TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'house level, 1-12',
`xp`        INT UNSIGNED NOT NULL DEFAULT 0,
`budget_room`  INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'cached, recomputed on load',
`shell_id`  INT UNSIGNED NOT NULL DEFAULT 0,
`rooms_owned` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'rooms bought at the Contractor',
`armillary` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Hearthlight Armillary preset: 0 default, 1 natural light, 2 unnatural dark, 3 evening calm, 4 midnight',
`endeavor_id`  INT UNSIGNED NOT NULL DEFAULT 0,
 `blueprint`    INT UNSIGNED NOT NULL DEFAULT 0,
 `name`      VARCHAR(64) NOT NULL DEFAULT '',
 `locked`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
 `plotMask`   TINYINT UNSIGNED NOT NULL DEFAULT 63 COMMENT 'audiences allowed on the yard, bit per audience',
 `houseMask`  TINYINT UNSIGNED NOT NULL DEFAULT 62 COMMENT 'audiences allowed inside, bit per audience',
 `budget_indoor`  INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'cached, recomputed on load',
 `budget_outdoor` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'cached, recomputed on load',
 `purchase_time`  INT UNSIGNED NOT NULL DEFAULT 0,
 `last_edited`   INT UNSIGNED NOT NULL DEFAULT 0,
 `evict_warned`  TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '1 once the inactivity letter has been sent',
 PRIMARY KEY (`id`),
 UNIQUE KEY `idx_housing_plot_unique` (`neighborhoodId`, `plotIndex`),
 KEY `idx_housing_owner_account` (`ownerAccount`),
 KEY `idx_housing_owner_guid` (`ownerGuid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Owned plots and their houses';

CREATE TABLE IF NOT EXISTS `character_housing_decor` (
 `id`    BIGINT UNSIGNED NOT NULL COMMENT 'placement id, server allocated',
 `housingId` INT UNSIGNED NOT NULL,
 `decorId`  INT UNSIGNED NOT NULL,
 `x`     FLOAT NOT NULL DEFAULT 0,
 `y`     FLOAT NOT NULL DEFAULT 0,
 `z`     FLOAT NOT NULL DEFAULT 0,
 `o`     FLOAT NOT NULL DEFAULT 0,
 `pitch`   FLOAT NOT NULL DEFAULT 0 COMMENT 'tilt on the forward axis, radians, 0 = upright',
 `roll`    FLOAT NOT NULL DEFAULT 0 COMMENT 'tilt on the side axis, radians, 0 = upright',
 `scale`   FLOAT NOT NULL DEFAULT 1,
 `surface`  TINYINT UNSIGNED NOT NULL DEFAULT 0,
 `indoor`  TINYINT UNSIGNED NOT NULL DEFAULT 0,
 `placed`  INT UNSIGNED NOT NULL DEFAULT 0,
 `dye_group` TINYINT UNSIGNED NOT NULL DEFAULT 0,
 `dye_color` TINYINT UNSIGNED NOT NULL DEFAULT 0,
 `variant_entry` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'the object actually spawned after dyeing',
 `pet_entry` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'companion assigned to the bed, 0 = the bed default',
 `pet_roaming` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '1 = the assigned companion wanders',
 PRIMARY KEY (`id`),
 KEY `idx_housing_decor_house` (`housingId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Placed decor pieces';

CREATE TABLE IF NOT EXISTS `character_housing_collection` (
 `accountId`  INT UNSIGNED NOT NULL,
 `decorId`   INT UNSIGNED NOT NULL,
 `source_type` TINYINT UNSIGNED NOT NULL DEFAULT 0,
 `source_id`  INT UNSIGNED NOT NULL DEFAULT 0,
 `acquired`  INT UNSIGNED NOT NULL DEFAULT 0,
 PRIMARY KEY (`accountId`, `decorId`),
 KEY `idx_housing_collection_decor` (`decorId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Account-wide decor collection';

CREATE TABLE IF NOT EXISTS `character_housing_guest` (
 `housingId` INT UNSIGNED NOT NULL,
 `accountId` INT UNSIGNED NOT NULL,
 `flags`   TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'bit0 enter, bit1 edit, bit2 manage guests',
 `added`   INT UNSIGNED NOT NULL DEFAULT 0,
 PRIMARY KEY (`housingId`, `accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Per-house guest permissions';

CREATE TABLE IF NOT EXISTS `character_housing_neighbor_member` (
 `neighborhoodId` INT UNSIGNED NOT NULL,
 `accountId`   INT UNSIGNED NOT NULL,
 `rank`      TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 member, 1 officer, 2 leader',
 `joined`     INT UNSIGNED NOT NULL DEFAULT 0,
 PRIMARY KEY (`neighborhoodId`, `accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Guild/private neighbourhood membership';

-- ---------------------------------------------------------------------------
-- Owned decor copies. Retail's rule is that four chairs are four chairs: buying a piece
-- adds a copy, placing one spends it, removing one gives it back, and the budget is what
-- limits how many of them can stand in the house at once. The collection row records what
-- has ever been learned; this row records what is in storage right now.
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `character_housing_storage` (
 `accountId` INT UNSIGNED NOT NULL,
 `decorId`   INT UNSIGNED NOT NULL,
 `count`     INT UNSIGNED NOT NULL DEFAULT 0,
 `acquired`  INT UNSIGNED NOT NULL DEFAULT 0,
 PRIMARY KEY (`accountId`, `decorId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Owned decor copies';

-- ---------------------------------------------------------------------------
-- Endeavor state per neighbourhood, progress per task, and the claims that stop a reward
-- being taken twice. One endeavor is active per neighbourhood at a time, exactly as retail.
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `character_housing_endeavor` (
 `neighborhoodId` INT UNSIGNED NOT NULL,
 `endeavorId` INT UNSIGNED NOT NULL DEFAULT 0,
 `milestone`  TINYINT UNSIGNED NOT NULL DEFAULT 0,
 `started`    INT UNSIGNED NOT NULL DEFAULT 0,
 `ends`       INT UNSIGNED NOT NULL DEFAULT 0,
 PRIMARY KEY (`neighborhoodId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Active Endeavor per neighbourhood';

CREATE TABLE IF NOT EXISTS `character_housing_endeavor_task` (
 `neighborhoodId` INT UNSIGNED NOT NULL,
 `taskId`     INT UNSIGNED NOT NULL,
 `progress`   INT UNSIGNED NOT NULL DEFAULT 0,
 PRIMARY KEY (`neighborhoodId`, `taskId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Endeavor task progress';

CREATE TABLE IF NOT EXISTS `character_housing_endeavor_claim` (
 `neighborhoodId` INT UNSIGNED NOT NULL,
 `milestone`  TINYINT UNSIGNED NOT NULL,
 `accountId`  INT UNSIGNED NOT NULL,
 `claimed`    INT UNSIGNED NOT NULL DEFAULT 0,
 PRIMARY KEY (`neighborhoodId`, `milestone`, `accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Milestone rewards already taken';

-- ---------------------------------------------------------------------------
-- Saved builds. The code is the shareable string; the row is the server's copy so the
-- owner can re-issue it and an import can be validated against the real catalogue.
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `character_housing_blueprint` (
 `id`        INT UNSIGNED NOT NULL AUTO_INCREMENT,
 `accountId` INT UNSIGNED NOT NULL,
 `name`      VARCHAR(64) NOT NULL DEFAULT '',
 `kind`      TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 build, 1 room, 2 exterior',
 `faction`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
 `code`      TEXT NOT NULL,
 `created`   INT UNSIGNED NOT NULL DEFAULT 0,
 PRIMARY KEY (`id`),
 KEY `idx_housing_blueprint_account` (`accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Exported build codes';

-- ---------------------------------------------------------------------------
-- Relinquished and evicted houses, kept for the regret window. A sold house is not
-- destroyed: the placements are parked here and RestorePlot puts them back, which is what
-- retail's "I sold my house and regret it" timer is.
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `character_housing_archive` (
 `id`        INT UNSIGNED NOT NULL,
 `accountId` INT UNSIGNED NOT NULL,
 `plotIndex` TINYINT UNSIGNED NOT NULL DEFAULT 0,
 `neighborhoodId` INT UNSIGNED NOT NULL DEFAULT 0,
 `name`      VARCHAR(64) NOT NULL DEFAULT '',
 `tier`      TINYINT UNSIGNED NOT NULL DEFAULT 1,
 `xp`        INT UNSIGNED NOT NULL DEFAULT 0,
 `shell_id`  INT UNSIGNED NOT NULL DEFAULT 0,
 `rooms_owned` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'rooms bought at the Contractor',
 `armillary` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Hearthlight Armillary preset at the sale: 0 default, 1 natural light, 2 unnatural dark, 3 evening calm, 4 midnight',
 `decor`     MEDIUMTEXT COMMENT 'packed placements, same format as a blueprint code',
 `price_paid` INT UNSIGNED NOT NULL DEFAULT 0,
 `archived`  INT UNSIGNED NOT NULL DEFAULT 0,
 PRIMARY KEY (`id`),
 KEY `idx_housing_archive_account` (`accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Sold houses kept for the regret window';

-- ---------------------------------------------------------------------------
-- Note for an install created before this revision.
--
-- The archive used to carry `night_mode`, a lit/unlit flag. It carries `armillary` now, the
-- preset itself, because a flag cannot tell the four presets apart and a regret window that
-- restores the wrong one is a house that comes back the wrong colour.
--
-- `CREATE TABLE IF NOT EXISTS` leaves an existing table alone, so an install that already has
-- `character_housing_archive` needs one statement by hand. It is not in this file because it
-- would be MariaDB-only syntax, and every statement here has to import on MySQL as well:
--
--   ALTER TABLE `character_housing_archive`
--     ADD COLUMN `armillary` TINYINT UNSIGNED NOT NULL DEFAULT 0 AFTER `rooms_owned`;
--
-- The old `night_mode` column can be dropped afterwards; nothing reads it.
-- ---------------------------------------------------------------------------
