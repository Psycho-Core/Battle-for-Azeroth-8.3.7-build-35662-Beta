-- mod-psychohouse - character schema
--
-- Two tables, both in the characters database, because a house belongs to a character and
-- travels with it. Nothing about a house is written to the world database: the module
-- spawns its NPCs and objects at run time into a phase, so there are no `creature` or
-- `gameobject` rows to orphan when a house is sold, and no grid bookkeeping to get wrong.
--
-- `phase` is the isolation key. It is allocated at purchase, stored, and never derived from
-- the character id, so two characters can never share one by arithmetic accident.
-- 50000-64999 is the module's range: 15 000 houses, inside the uint16 phase space, and
-- deliberately above the ranges the neighbourhood module uses.
--
-- Two widths are not free choices:
--
--   `guid` is BIGINT UNSIGNED because `characters.guid` in this build's own schema is
--   bigint(20) unsigned and ObjectGuid::LowType is uint64. A 32-bit column here would work
--   for a while on a young realm and truncate silently on an old one.
--
--   `owner` in psychohouse_purchase is the same width as `guid`, because it is that value.
--
-- The collation is written out rather than left to the server. MariaDB 11.8 defaults
-- utf8mb4 to utf8mb4_general_ci and MySQL 8 defaults it to utf8mb4_0900_ai_ci; naming it
-- means the tables behave the same on both. `name` is display text only - it is never
-- compared or joined against `characters.name`, which is utf8_bin, and mixing those two in a
-- comparison is the one way to get "Illegal mix of collations" out of these tables.

CREATE TABLE IF NOT EXISTS `psychohouse_owner` (
  `guid`    bigint unsigned NOT NULL,
  `account` int unsigned NOT NULL DEFAULT 0,
  `team`    tinyint unsigned NOT NULL DEFAULT 0 COMMENT '0 Alliance, 1 Horde',
  `phase`   smallint unsigned NOT NULL,
  `name`    varchar(64) NOT NULL DEFAULT '',
  `created` int unsigned NOT NULL DEFAULT 0 COMMENT 'unix time of the purchase',
  PRIMARY KEY (`guid`),
  UNIQUE KEY `phase` (`phase`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci
  COMMENT='One island house per character';

CREATE TABLE IF NOT EXISTS `psychohouse_purchase` (
  `owner`    bigint unsigned NOT NULL COMMENT 'psychohouse_owner.guid',
  `facility` smallint unsigned NOT NULL COMMENT 'psychohouse_facility.id',
  `bought`   int unsigned NOT NULL DEFAULT 0 COMMENT 'unix time of the purchase',
  PRIMARY KEY (`owner`, `facility`),
  KEY `facility` (`facility`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci
  COMMENT='Facilities bought for a house';

-- Objects a player put in their own house.
--
-- `slot` is the identity, not `entry`: `(guid, slot)` is the primary key, so a piece keeps its
-- number for as long as it stands and a removal always names exactly one piece. The range is
-- whatever `PsychoHouse.Objects.Max` was when the piece went down - the table does not cap it,
-- the module does - and the lowest free slot is reused, so a house that has had pieces
-- removed and re-added does not climb past its own limit.
--
-- The position is absolute, not an offset from the anchor, because that is what a client sends
-- and what `.psychostand` prints back. The radius check in the module is what keeps an absolute
-- position inside the yard.
--
-- No foreign key to `psychohouse_owner`: rows are deleted in the same call that deletes the
-- house, and a FK would only make the sell path fail on a realm where the two tables were
-- created at different times.

CREATE TABLE IF NOT EXISTS `psychohouse_object` (
  `guid`  bigint unsigned NOT NULL COMMENT 'psychohouse_owner.guid',
  `slot`  tinyint unsigned NOT NULL COMMENT '0-199, stable for the life of the piece',
  `entry` int unsigned NOT NULL COMMENT 'gameobject_template.entry',
  `x`     float NOT NULL DEFAULT 0,
  `y`     float NOT NULL DEFAULT 0,
  `z`     float NOT NULL DEFAULT 0,
  `o`     float NOT NULL DEFAULT 0 COMMENT 'orientation in radians',
  `scale` float NOT NULL DEFAULT 1,
  PRIMARY KEY (`guid`, `slot`),
  KEY `entry` (`entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci
  COMMENT='Objects a player placed in their own house';
