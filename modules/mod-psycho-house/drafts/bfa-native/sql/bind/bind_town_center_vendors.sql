-- ---------------------------------------------------------------------------
-- mod-psycho-house - bind the 39 town centre vendors to creature models
--
-- Draft A ships every vendor with creature_entry 0, which means "not spawned": the row
-- carries a retail npc id in its comment, and a retail npc id is not an 8.3.7 creature
-- entry. This script fills the column from creature_template by role, not by name, the
-- same way tools/sql/bind_native_vendors.sql does for the eight native stalls.
--
-- Why by role and not by name: 8.3.7 has no npc called "Faarden the Builder", so a name
-- match would return nothing every time, and a name I guessed would bind a random citizen.
-- A subname query either finds real vendors or finds nothing, and nothing leaves the row
-- unspawned with a warning rather than wrong.
--
-- Run after 03_vendors_retail.sql. If the world database localises subname, widen the IN
-- list - the query is the only thing that needs editing. 39 vendors, ids 101 to 139, one
-- model each so no two stalls are the same person.
-- ---------------------------------------------------------------------------
-- Every lookup is wrapped in COALESCE(..., 0): a subquery that matches nothing returns
-- NULL and `creature_entry` is NOT NULL, so an unwrapped statement fails with error
-- 1048 on a realm that has fewer vendor NPCs than this file has stalls.
-- ---------------------------------------------------------------------------



UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 0), 0)
WHERE `id` = 101;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 1), 0)
WHERE `id` = 102;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 2), 0)
WHERE `id` = 103;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 3), 0)
WHERE `id` = 104;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 4), 0)
WHERE `id` = 105;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 5), 0)
WHERE `id` = 106;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 6), 0)
WHERE `id` = 107;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 7), 0)
WHERE `id` = 108;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 8), 0)
WHERE `id` = 109;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 9), 0)
WHERE `id` = 110;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 10), 0)
WHERE `id` = 111;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 11), 0)
WHERE `id` = 112;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 12), 0)
WHERE `id` = 113;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 13), 0)
WHERE `id` = 114;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 14), 0)
WHERE `id` = 115;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 15), 0)
WHERE `id` = 116;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 16), 0)
WHERE `id` = 117;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 17), 0)
WHERE `id` = 118;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 18), 0)
WHERE `id` = 119;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 19), 0)
WHERE `id` = 120;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 20), 0)
WHERE `id` = 121;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 21), 0)
WHERE `id` = 122;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 22), 0)
WHERE `id` = 123;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 23), 0)
WHERE `id` = 124;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 24), 0)
WHERE `id` = 125;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 25), 0)
WHERE `id` = 126;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 26), 0)
WHERE `id` = 127;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 27), 0)
WHERE `id` = 128;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 28), 0)
WHERE `id` = 129;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 29), 0)
WHERE `id` = 130;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 30), 0)
WHERE `id` = 131;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 31), 0)
WHERE `id` = 132;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 32), 0)
WHERE `id` = 133;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 33), 0)
WHERE `id` = 134;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 34), 0)
WHERE `id` = 135;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 35), 0)
WHERE `id` = 136;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 36), 0)
WHERE `id` = 137;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 37), 0)
WHERE `id` = 138;

UPDATE `housing_vendor` SET `creature_entry` =
    COALESCE((SELECT `entry` FROM `creature_template` WHERE (`npcflag` & 128) <> 0
       AND `subname` IN ('General Goods', 'Trade Supplies', 'Tradesman', 'Decor')
     ORDER BY `entry` LIMIT 1 OFFSET 38), 0)
WHERE `id` = 139;

-- Report: which stalls still have no model after the run above.
SELECT `id`, `name`, `creature_entry`, `role`
FROM `housing_vendor`
WHERE `id` BETWEEN 101 AND 139 AND `creature_entry` = 0;
