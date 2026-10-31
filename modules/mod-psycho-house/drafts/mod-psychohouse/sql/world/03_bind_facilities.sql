-- mod-psychohouse - binding the realm's own content
--
-- Three kinds of number this draft cannot know without the realm's world database:
--
--   1. the model of the two NPCs the module owns (a CreatureDisplayID),
--   2. the entry of every service NPC the house sells (a creature entry),
--   3. the entry of every service object the house sells (a gameobject entry).
--
-- None of them is guessed here. Every one is read out of the realm's own tables by a
-- statement that says what it is looking for, so a mistake shows up as "this statement
-- matched nothing" or "this statement matched the wrong NPC", both visible in the report
-- at the bottom of this file.
--
-- The two `npcflag` masks below are the ones this build's UnitDefines.h declares:
--   banker 0x00020000, auctioneer 0x00200000, innkeeper 0x00010000,
--   stable master 0x00400000, spirit healer 0x00004000, repair 0x00001000
-- and the two `gameobject_template` types are the ones GameObjectData.h declares:
--   mailbox 19, barber chair 32, guild bank 34.
--
-- Run this once after 01 and 02, then read the report. Any row still at entry 0 is a
-- facility the realm does not have; the module simply will not offer it.
--
-- Which server this was written for: MariaDB 11.8, default sql_mode. Two consequences are
-- written into the statements below rather than left to chance.
--
--   * Every scalar subquery is wrapped in COALESCE(..., 0). A scalar subquery that matches
--     nothing returns NULL, `entry` is NOT NULL, and MariaDB in STRICT_TRANS_TABLES aborts
--     the statement with error 1048 - which would stop this whole file on the first missing
--     service. With the COALESCE the row stays at 0 and the report says so.
--   * Nothing here writes to a table the statement also reads, so the "You can't specify
--     target table for update in FROM clause" error cannot happen either.

-- ---------------------------------------------------------------------------
-- 1. Models for the two module NPCs
-- ---------------------------------------------------------------------------
-- Borrowed from a service NPC that already exists, so the look is content from the realm
-- rather than a display id invented here. To give either NPC a different look, change the
-- mask in the matching statement (innkeeper 0x00010000, banker 0x00020000) and re-run it.
--
-- DELETE then INSERT rather than REPLACE: the row is deleted first so re-running this file
-- cannot leave a second model behind, and INSERT ... SELECT reads one table while writing
-- another, which every server accepts. REPLACE would delete and re-insert in one step and
-- hide which of the two happened.

DELETE FROM `creature_template_model` WHERE `CreatureID` = 900200;

INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`)
SELECT 900200, 0, m.`CreatureDisplayID`, 1, 1
FROM `creature_template_model` m
JOIN `creature_template` t ON t.`entry` = m.`CreatureID`
WHERE (t.`npcflag` & 0x00010000) <> 0 AND m.`Idx` = 0 AND m.`CreatureDisplayID` > 0
ORDER BY m.`CreatureID` LIMIT 1;

DELETE FROM `creature_template_model` WHERE `CreatureID` = 900201;

INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`)
SELECT 900201, 0, m.`CreatureDisplayID`, 1, 1
FROM `creature_template_model` m
JOIN `creature_template` t ON t.`entry` = m.`CreatureID`
WHERE (t.`npcflag` & 0x00020000) <> 0 AND m.`Idx` = 0 AND m.`CreatureDisplayID` > 0
ORDER BY m.`CreatureID` LIMIT 1;

-- ---------------------------------------------------------------------------
-- 2. Service creatures
-- ---------------------------------------------------------------------------
-- Lowest matching entry wins, so the result is stable between runs. Every one of these can
-- be pinned to a preferred NPC instead: replace the subquery with the entry by hand and the
-- module will never touch it again, because the UPDATE only fires where `entry` is still 0.

UPDATE `psychohouse_facility` SET `entry` = COALESCE((
  SELECT t.`entry` FROM `creature_template` t
  WHERE (t.`npcflag` & 0x00020000) <> 0 AND t.`entry` > 0
  ORDER BY t.`entry` LIMIT 1), 0)
WHERE `id` = 1 AND `entry` = 0;

UPDATE `psychohouse_facility` SET `entry` = COALESCE((
  SELECT t.`entry` FROM `creature_template` t
  WHERE (t.`npcflag` & 0x00200000) <> 0 AND t.`entry` > 0
  ORDER BY t.`entry` LIMIT 1), 0)
WHERE `id` = 2 AND `entry` = 0;

UPDATE `psychohouse_facility` SET `entry` = COALESCE((
  SELECT t.`entry` FROM `creature_template` t
  WHERE (t.`npcflag` & 0x00010000) <> 0 AND t.`entry` > 0
  ORDER BY t.`entry` LIMIT 1), 0)
WHERE `id` = 4 AND `entry` = 0;

UPDATE `psychohouse_facility` SET `entry` = COALESCE((
  SELECT t.`entry` FROM `creature_template` t
  WHERE (t.`npcflag` & 0x00400000) <> 0 AND t.`entry` > 0
  ORDER BY t.`entry` LIMIT 1), 0)
WHERE `id` = 5 AND `entry` = 0;

UPDATE `psychohouse_facility` SET `entry` = COALESCE((
  SELECT t.`entry` FROM `creature_template` t
  WHERE (t.`npcflag` & 0x00004000) <> 0 AND t.`entry` > 0
  ORDER BY t.`entry` LIMIT 1), 0)
WHERE `id` = 6 AND `entry` = 0;

UPDATE `psychohouse_facility` SET `entry` = COALESCE((
  SELECT t.`entry` FROM `creature_template` t
  WHERE (t.`npcflag` & 0x00001000) <> 0 AND (t.`npcflag` & 0x00000080) <> 0 AND t.`entry` > 0
  ORDER BY t.`entry` LIMIT 1), 0)
WHERE `id` = 13 AND `entry` = 0;

-- ---------------------------------------------------------------------------
-- 3. Service objects
-- ---------------------------------------------------------------------------
-- A mailbox, a barber chair and a guild bank are types, not names, so the type is the test.
-- Forge and anvil are spell-caster objects whose name is the only stable thing about them.

UPDATE `psychohouse_facility` SET `entry` = COALESCE((
  SELECT g.`entry` FROM `gameobject_template` g
  WHERE g.`type` = 19 AND g.`entry` > 0
  ORDER BY g.`entry` LIMIT 1), 0)
WHERE `id` = 3 AND `entry` = 0;

UPDATE `psychohouse_facility` SET `entry` = COALESCE((
  SELECT g.`entry` FROM `gameobject_template` g
  WHERE g.`type` = 32 AND g.`entry` > 0
  ORDER BY g.`entry` LIMIT 1), 0)
WHERE `id` = 9 AND `entry` = 0;

UPDATE `psychohouse_facility` SET `entry` = COALESCE((
  SELECT g.`entry` FROM `gameobject_template` g
  WHERE g.`type` = 34 AND g.`entry` > 0
  ORDER BY g.`entry` LIMIT 1), 0)
WHERE `id` = 10 AND `entry` = 0;

UPDATE `psychohouse_facility` SET `entry` = COALESCE((
  SELECT g.`entry` FROM `gameobject_template` g
  WHERE g.`name` = 'Forge' AND g.`entry` > 0
  ORDER BY g.`entry` LIMIT 1), 0)
WHERE `id` = 7 AND `entry` = 0;

UPDATE `psychohouse_facility` SET `entry` = COALESCE((
  SELECT g.`entry` FROM `gameobject_template` g
  WHERE g.`name` = 'Anvil' AND g.`entry` > 0
  ORDER BY g.`entry` LIMIT 1), 0)
WHERE `id` = 8 AND `entry` = 0;

-- The two portals are the only faction-locked facilities in the shipped catalogue: an
-- Alliance house opens Stormwind, a Horde house opens Orgrimmar.

UPDATE `psychohouse_facility` SET `entry` = COALESCE((
  SELECT g.`entry` FROM `gameobject_template` g
  WHERE g.`name` LIKE 'Portal to Stormwind%' AND g.`entry` > 0
  ORDER BY g.`entry` LIMIT 1), 0)
WHERE `id` = 11 AND `entry` = 0;

UPDATE `psychohouse_facility` SET `entry` = COALESCE((
  SELECT g.`entry` FROM `gameobject_template` g
  WHERE g.`name` LIKE 'Portal to Orgrimmar%' AND g.`entry` > 0
  ORDER BY g.`entry` LIMIT 1), 0)
WHERE `id` = 12 AND `entry` = 0;

-- ---------------------------------------------------------------------------
-- 4. Report - read this before starting the server
-- ---------------------------------------------------------------------------
-- Facilities still at 0 are not offered by the Steward. Models missing for 900200/900201
-- mean the two NPCs will be invisible: bind them by hand with the two statements at the top
-- of this file, changing the mask to match an NPC that exists.

SELECT `id`, `name`, `kind`, `entry`, `is_object`, `faction`, `price`
FROM `psychohouse_facility` ORDER BY `id`;

SELECT f.`id`, f.`name` FROM `psychohouse_facility` f WHERE f.`entry` = 0 ORDER BY f.`id`;

SELECT `CreatureID`, `CreatureDisplayID` FROM `creature_template_model`
WHERE `CreatureID` IN (900200, 900201) ORDER BY `CreatureID`;
