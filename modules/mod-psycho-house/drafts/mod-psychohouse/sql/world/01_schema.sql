-- mod-psychohouse - world schema and the facility catalogue
--
-- One row per buyable facility. The position columns are offsets from the house anchor,
-- never absolute coordinates, so moving a whole house is one config change and not a
-- rewrite of this table.
--
-- `entry` is 0 until 03_bind_facilities.sql fills it from the realm's own world database.
-- A row with entry 0 is never spawned and never offered; it does not break anything.
--
-- `faction`: 0 = Alliance only, 1 = Horde only, 2 = both. The owner's team picks the rows.

CREATE TABLE IF NOT EXISTS `psychohouse_facility` (
  `id`             smallint unsigned NOT NULL,
  `name`           varchar(64)  NOT NULL,
  `kind`           tinyint unsigned NOT NULL,
  `entry`          int unsigned NOT NULL DEFAULT 0,
  `is_object`      tinyint unsigned NOT NULL DEFAULT 0,
  `faction`        tinyint unsigned NOT NULL DEFAULT 2,
  `price`          int unsigned NOT NULL DEFAULT 0,
  `pos_x`          float NOT NULL DEFAULT 0,
  `pos_y`          float NOT NULL DEFAULT 0,
  `pos_z`          float NOT NULL DEFAULT 0,
  `pos_o`          float NOT NULL DEFAULT 0,
  `required_level` tinyint unsigned NOT NULL DEFAULT 0,
  `sort`           tinyint unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- kind: 1 bank, 2 auctioneer, 3 mailbox, 4 innkeeper, 5 stable master, 6 spirit healer,
--       7 forge, 8 anvil, 9 barber chair, 10 guild vault, 11 portal, 12 repair
--
-- prices are copper: 10000 copper = 1 gold. The house itself is priced in the config file,
-- not here, because it is not a facility.
DELETE FROM `psychohouse_facility`;

INSERT INTO `psychohouse_facility`
  (`id`, `name`, `kind`, `entry`, `is_object`, `faction`, `price`, `pos_x`, `pos_y`, `pos_z`, `pos_o`, `required_level`, `sort`)
VALUES
  (1,  'Bank',           1,  0, 0, 2, 1000000,  -9.0, -6.0, 0, 0, 0, 1),
  (2,  'Auctioneer',     2,  0, 0, 2, 1000000,  -6.0, -6.0, 0, 0, 0, 2),
  (3,  'Mailbox',        3,  0, 1, 2,  250000,  -3.0, -6.0, 0, 0, 0, 3),
  (4,  'Innkeeper',      4,  0, 0, 2,  500000,   0.0, -6.0, 0, 0, 0, 4),
  (5,  'Stable Master',  5,  0, 0, 2,  500000,   3.0, -6.0, 0, 0, 0, 5),
  (6,  'Spirit Healer',  6,  0, 0, 2,  100000,   6.0, -6.0, 0, 0, 0, 6),
  (7,  'Forge',          7,  0, 1, 2,  250000,   9.0, -6.0, 0, 0, 0, 7),
  (8,  'Anvil',          8,  0, 1, 2,  100000,  -9.0,  6.0, 0, 0, 0, 8),
  (9,  'Barber Chair',   9,  0, 1, 2,  500000,  -6.0,  6.0, 0, 0, 0, 9),
  (10, 'Guild Vault',   10,  0, 1, 2, 1500000,  -3.0,  6.0, 0, 0, 0, 10),
  (11, 'Portal West',   11,  0, 1, 0, 1000000,   0.0,  6.0, 0, 0, 0, 11),
  (12, 'Portal East',   11,  0, 1, 1, 1000000,   0.0,  6.0, 0, 0, 0, 12),
  (13, 'Repair',        12,  0, 0, 2,  250000,   3.0,  6.0, 0, 0, 0, 13);
