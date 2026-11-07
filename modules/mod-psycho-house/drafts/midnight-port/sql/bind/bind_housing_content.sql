-- ---------------------------------------------------------------------------
-- mod-psycho-house - binding the module's content to the realm's content
--
-- The drafts ship with every realm-owned id at 0 and the *name* the module wants in the
-- row beside it. This file turns those names into ids out of the realm's own world
-- database, and then prints a report of what it could not find.
--
-- Run it after the draft's world files, once, and re-run it after any content patch.
--
-- Two rules run through every statement here, both learned the hard way:
--
--   * every scalar subquery is wrapped in COALESCE(..., 0). A subquery that matches nothing
--     returns NULL, the target column is NOT NULL, and MariaDB in STRICT_TRANS_TABLES
--     aborts on error 1048 - which would stop the file at the first missing name;
--   * no statement writes to the table it reads from, so MariaDB's restriction on a target
--     table inside a FROM clause never applies.
--
-- Written for MariaDB 11.8, default sql_mode.
-- ---------------------------------------------------------------------------

-- ---------------------------------------------------------------------------
-- 1. Amenity objects and NPCs
-- ---------------------------------------------------------------------------

UPDATE `housing_amenity` a
SET a.`gameobject_entry` = COALESCE((
        SELECT MIN(g.`entry`) FROM `gameobject_template` g
        WHERE g.`name` = a.`go_name` AND a.`go_name` <> ''), 0)
WHERE a.`go_name` <> '';

UPDATE `housing_amenity` a
SET a.`creature_entry` = COALESCE((
        SELECT MIN(c.`entry`) FROM `creature_template` c
        WHERE c.`name` = a.`creature_name` AND a.`creature_name` <> ''), 0)
WHERE a.`creature_name` <> '';

-- ---------------------------------------------------------------------------
-- 2. Exterior shells
--
-- A shell is a building, so the search is on the game object name and takes the lowest
-- entry: the oldest model of that name is the one that has been in the client longest and
-- is the least likely to be a later expansion's placeholder. Change `MIN` to `MAX` for the
-- newest, or set the row by hand - both are supported and neither is guessed here.
-- ---------------------------------------------------------------------------

UPDATE `housing_shell` s
SET s.`gameobject_entry` = COALESCE((
        SELECT MIN(g.`entry`) FROM `gameobject_template` g
        WHERE g.`name` LIKE CONCAT('%', s.`go_name`, '%')
          AND g.`type` IN (0, 5, 6, 8, 14) AND s.`go_name` <> ''), 0)
WHERE s.`go_name` <> '';

-- ---------------------------------------------------------------------------
-- 3. Portals: the destination list comes from `game_tele`
--
-- game_tele is "a named place with a map and coordinates", which is exactly what a portal
-- destination is. A name the realm does not have stays at map 0 and is not offered.
-- ---------------------------------------------------------------------------

UPDATE `housing_portal` p
SET p.`map` = COALESCE((SELECT t.`map` FROM `game_tele` t WHERE t.`name` = p.`tele_name` LIMIT 1), 0),
    p.`x`   = COALESCE((SELECT t.`position_x` FROM `game_tele` t WHERE t.`name` = p.`tele_name` LIMIT 1), 0),
    p.`y`   = COALESCE((SELECT t.`position_y` FROM `game_tele` t WHERE t.`name` = p.`tele_name` LIMIT 1), 0),
    p.`z`   = COALESCE((SELECT t.`position_z` FROM `game_tele` t WHERE t.`name` = p.`tele_name` LIMIT 1), 0),
    p.`o`   = COALESCE((SELECT t.`orientation` FROM `game_tele` t WHERE t.`name` = p.`tele_name` LIMIT 1), 0)
WHERE p.`tele_name` <> '';

-- ---------------------------------------------------------------------------
-- 4. Decor creatures: pet beds, mount stands and mannequins
--
-- The creature is looked up by name. kind 0 is a pet bed, 1 a mount stand, 2 a mannequin;
-- a pet bed with no match stays furniture, which the audit report lists.
-- ---------------------------------------------------------------------------

UPDATE `housing_decor_creature` d
SET d.`creature_entry` = COALESCE((
        SELECT MIN(c.`entry`) FROM `creature_template` c
        WHERE c.`name` = d.`creature_name` AND d.`creature_name` <> ''), 0)
WHERE d.`creature_name` <> '';

-- ---------------------------------------------------------------------------
-- 5. Endeavor targets
--
-- The bind depends on the task type: 0 and 1 are creatures, 2 is a quest, 3 and 4 are
-- items. A task with no match stays at 0 and never completes, which is the safe failure -
-- it cannot complete on the wrong kill.
-- ---------------------------------------------------------------------------

UPDATE `housing_endeavor_task` t
SET t.`target` = COALESCE((
        SELECT MIN(c.`entry`) FROM `creature_template` c
        WHERE c.`name` = t.`target_name` AND t.`target_name` <> ''), 0)
WHERE t.`type` IN (0, 1) AND t.`target_name` <> '';

UPDATE `housing_endeavor_task` t
SET t.`target` = COALESCE((
        SELECT MIN(q.`ID`) FROM `quest_template` q
        WHERE q.`LogTitle` = t.`target_name` AND t.`target_name` <> ''), 0)
WHERE t.`type` = 2 AND t.`target_name` <> '';

UPDATE `housing_endeavor_task` t
SET t.`target` = COALESCE((
        SELECT MIN(i.`entry`) FROM `item_template` i
        WHERE i.`name` = t.`target_name` AND t.`target_name` <> ''), 0)
WHERE t.`type` IN (3, 4) AND t.`target_name` <> '';

-- ---------------------------------------------------------------------------
-- 7. Game object scripts
--
-- A game object's script is attached by `gameobject_template.ScriptName` and by nothing else.
-- The module summons its objects from template entries, so an object whose template names no
-- script does nothing at all: the bulletin board is a board, the portal is scenery, the coffer
-- cannot be opened and a hunt cache cannot be searched. Every statement is guarded on an empty
-- ScriptName, so a realm that has its own script on one of these keeps it.
--
-- This runs after section 1, because it binds whatever entries section 1 resolved.
-- ---------------------------------------------------------------------------

UPDATE `gameobject_template` g
  JOIN `housing_amenity` a ON a.`gameobject_entry` = g.`entry`
   SET g.`ScriptName` = CASE a.`type`
        WHEN 0 THEN 'go_housing_bulletin'
        WHEN 1 THEN 'go_housing_portal'
        WHEN 7 THEN 'go_housing_coffer'
        ELSE g.`ScriptName` END
 WHERE g.`ScriptName` = '' AND a.`gameobject_entry` <> 0 AND a.`type` IN (0, 1, 7);

UPDATE `gameobject_template` g
  JOIN `housing_hunt` h ON h.`cache_entry` = g.`entry`
   SET g.`ScriptName` = 'go_housing_huntcache'
 WHERE g.`ScriptName` = '';

-- The plot cornerstone is chosen by `Housing.Plot.CornerstoneEntry` rather than by a row, so
-- there is nothing here to join to. Set it by hand on the entry that key names:
--
--   UPDATE `gameobject_template` SET `ScriptName` = 'go_housing_cornerstone'
--    WHERE `entry` = <the entry in Housing.Plot.CornerstoneEntry>;
--
-- An entry whose `go_name` the realm does not have stays at 0 and is not offered, which is the
-- behaviour section 1's report describes.

-- ---------------------------------------------------------------------------
-- 6. The report
-- ---------------------------------------------------------------------------

SELECT 'amenities with an object' AS `what`, COUNT(*) AS `rows`
FROM `housing_amenity` WHERE `gameobject_entry` <> 0
UNION ALL SELECT 'amenities with an NPC', COUNT(*) FROM `housing_amenity` WHERE `creature_entry` <> 0
UNION ALL SELECT 'amenities still a pin only', COUNT(*) FROM `housing_amenity`
    WHERE `gameobject_entry` = 0 AND `creature_entry` = 0
UNION ALL SELECT 'shells bound', COUNT(*) FROM `housing_shell` WHERE `gameobject_entry` <> 0
UNION ALL SELECT 'portal destinations bound', COUNT(*) FROM `housing_portal` WHERE `map` <> 0
UNION ALL SELECT 'decor creatures bound', COUNT(*) FROM `housing_decor_creature` WHERE `creature_entry` <> 0
UNION ALL SELECT 'endeavor tasks bound', COUNT(*) FROM `housing_endeavor_task` WHERE `target` <> 0
UNION ALL SELECT 'endeavor tasks unbound (never complete)', COUNT(*)
    FROM `housing_endeavor_task` WHERE `target` = 0 AND `type` <> 6 AND `type` <> 5
UNION ALL SELECT 'amenity objects with a housing script', COUNT(*)
    FROM `gameobject_template` WHERE `ScriptName` LIKE 'go_housing_%'
UNION ALL SELECT 'hunt caches with a housing script', COUNT(*)
    FROM `housing_hunt` h JOIN `gameobject_template` g ON g.`entry` = h.`cache_entry`
    WHERE g.`ScriptName` = 'go_housing_huntcache';

-- What the report means:
--
--   * an amenity that is still a pin only appears on the addon map and has no object in the
--     world. Fill `go_name` or `creature_name` for it and re-run;
--   * a shell with entry 0 means the plot has no building on it. Set `gameobject_entry`
--     by hand from gameobject_template, which is the honest way to choose a house model;
--   * an unbound Endeavor task never completes. Fix `target_name` and re-run, or set
--     `target` by hand.
--
-- None of these is a crash: the module runs with all of them at 0 and simply has less
-- content, which is why the report exists rather than an assertion.
