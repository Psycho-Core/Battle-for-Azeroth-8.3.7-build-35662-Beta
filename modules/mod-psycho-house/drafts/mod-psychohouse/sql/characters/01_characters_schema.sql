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

CREATE TABLE IF NOT EXISTS `psychohouse_owner` (
  `guid`    int unsigned NOT NULL,
  `account` int unsigned NOT NULL DEFAULT 0,
  `team`    tinyint unsigned NOT NULL DEFAULT 0,
  `phase`   smallint unsigned NOT NULL,
  `name`    varchar(64) NOT NULL,
  `created` int unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`),
  UNIQUE KEY `phase` (`phase`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `psychohouse_purchase` (
  `owner`    int unsigned NOT NULL,
  `facility` smallint unsigned NOT NULL,
  `bought`   int unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`owner`, `facility`),
  KEY `facility` (`facility`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
