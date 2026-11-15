-- ---------------------------------------------------------------------------
-- mod-psycho-house - bind the eight native vendor stalls to creature models
--
-- Draft B ships its vendors with creature_entry 0, which means "not spawned". This
-- script fills the column from creature_template by role, not by name: it takes vendor
-- NPCs whose subname says they sell general goods or trade supplies and walks down that
-- list one stall at a time.
--
-- Why by role and not by name: a name I get wrong binds a random citizen to a shop, and
-- that is worse than no model at all. A subname query either finds real vendors or finds
-- nothing, and nothing leaves the stall unspawned with a warning in the log.
--
-- Run after 04_vendors_native.sql. If the world database localises subname, widen the IN
-- list - the query is the only thing that needs editing.
--
-- Every subquery is wrapped in COALESCE(..., 0). A subquery that matches nothing returns
-- NULL, and `creature_entry` is NOT NULL: MariaDB in strict mode raises 1048 and stops the
-- file half-applied, which is the opposite of what a bind pass is for. There are more than
-- eight General Goods vendors on a normal realm, so the eight lookups fill; a realm with
-- fewer now leaves the extras at 0 and the report at the end names them.
-- ---------------------------------------------------------------------------


UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman')
     ORDER BY `entry` LIMIT 1), 0)
WHERE `id` = 201;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman')
     ORDER BY `entry` LIMIT 1 OFFSET 1), 0)
WHERE `id` = 202;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman')
     ORDER BY `entry` LIMIT 1 OFFSET 2), 0)
WHERE `id` = 203;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman')
     ORDER BY `entry` LIMIT 1 OFFSET 3), 0)
WHERE `id` = 204;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman')
     ORDER BY `entry` LIMIT 1 OFFSET 4), 0)
WHERE `id` = 205;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman')
     ORDER BY `entry` LIMIT 1 OFFSET 5), 0)
WHERE `id` = 206;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman')
     ORDER BY `entry` LIMIT 1 OFFSET 6), 0)
WHERE `id` = 207;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman')
     ORDER BY `entry` LIMIT 1 OFFSET 7), 0)
WHERE `id` = 208;

-- Report: which stalls still have no model after the run above. `creature_entry` is NOT NULL,
-- so the test is against 0 rather than NULL - the column cannot hold a NULL to test for.
SELECT `id`, `name`, `creature_entry`, `role`
FROM `housing_vendor`
WHERE `id` BETWEEN 201 AND 208 AND `creature_entry` = 0;
