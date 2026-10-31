-- ---------------------------------------------------------------------------
-- mod-psycho-house : decor harvest (pure SQL)
--
-- Run against the WORLD database of my 8.3.7 build 35662 realm.
-- Read-only. Produces candidate decor rows from gameobject_template.
--
-- Usage:
--   mysql -u user -p world < tools/sql/psycho_house_harvest.sql
--
-- The Python tool tools/housing/harvest_decor.py does the same thing with
-- scoring, dedup and INSERT generation; this file exists so the numbers can be
-- checked by hand and so the queries can be re-run on any mysql client.
-- ---------------------------------------------------------------------------

-- 0. Sanity: how many game objects do I have to work with?
SELECT COUNT(*) AS gameobject_templates FROM `gameobject_template`;

-- 1. Category candidates -----------------------------------------------------
-- The name patterns are the ones retail's decor categories imply (doc 03 S.6.1).

DROP TEMPORARY TABLE IF EXISTS `tmp_decor_candidates`;
CREATE TEMPORARY TABLE `tmp_decor_candidates` (
  `entry`       INT UNSIGNED NOT NULL,
  `name`        VARCHAR(100) NOT NULL,
  `type`        TINYINT UNSIGNED NOT NULL,
  `displayId`   INT UNSIGNED NOT NULL,
  `category`    VARCHAR(32)  NOT NULL,
  `subcategory` VARCHAR(48)  NOT NULL,
  PRIMARY KEY (`entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Furnishings', 'Seating'
FROM `gameobject_template`
WHERE `name` REGEXP '(chair|stool|bench|seat|throne|sofa|couch)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Furnishings', 'Tables and Desks'
FROM `gameobject_template`
WHERE `name` REGEXP '(table|desk|counter|workbench)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Furnishings', 'Storage'
FROM `gameobject_template`
WHERE `name` REGEXP '(chest|crate|barrel|box|cabinet|bookcase|bookshelf|shelf|coffer)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Furnishings', 'Beds'
FROM `gameobject_template`
WHERE `name` REGEXP '(bed|cot|hammock|bunk|mattress)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Accents', 'Ornamental'
FROM `gameobject_template`
WHERE `name` REGEXP '(vase|urn|bowl|goblet|trophy|statue|painting|banner)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Accents', 'Wall Hangings'
FROM `gameobject_template`
WHERE `name` REGEXP '(rug|tapestry|curtain|drape|banner)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Accents', 'Food and Drink'
FROM `gameobject_template`
WHERE `name` REGEXP '(food|fruit|meat|bread|keg|bottle|mug|kettle|cauldron|feast|barrel)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Accents', 'Floor'
FROM `gameobject_template`
WHERE `name` REGEXP '(carpet|mat)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Structural', 'Doors'
FROM `gameobject_template`
WHERE `name` REGEXP '(door|gate|portcullis|archway)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Structural', 'Walls and Columns'
FROM `gameobject_template`
WHERE `name` REGEXP '(wall|pillar|column|fence|barricade|palisade)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Structural', 'Windows'
FROM `gameobject_template`
WHERE `name` REGEXP '(window|shutter|lattice|trellis)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Structural', 'Large Structures'
FROM `gameobject_template`
WHERE `name` REGEXP '(tower|gazebo|tent|pavilion|windmill|bridge|dock|well|fountain)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Lighting', 'Small Lights'
FROM `gameobject_template`
WHERE `name` REGEXP '(lantern|lamp|torch|candle|brazier|campfire|chandelier|beacon|light)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Nature', 'Large Foliage'
FROM `gameobject_template`
WHERE `name` REGEXP '(tree|oak|pine|willow|palm|ancient)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Nature', 'Small Foliage'
FROM `gameobject_template`
WHERE `name` REGEXP '(bush|shrub|fern|plant|flower|herb|vine|ivy|grass|moss)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Nature', 'Misc. Nature'
FROM `gameobject_template`
WHERE `name` REGEXP '(mushroom|leaves|snow|dirt|lilypad|coral|kelp)' AND `displayId` <> 0;

INSERT IGNORE INTO `tmp_decor_candidates`
SELECT `entry`, `name`, `type`, `displayId`, 'Functional', 'Utility'
FROM `gameobject_template`
WHERE `name` REGEXP '(forge|anvil|oven|mailbox)' AND `displayId` <> 0;

-- 2. Report -----------------------------------------------------------------

SELECT `category`, `subcategory`, COUNT(*) AS candidates
FROM `tmp_decor_candidates`
GROUP BY `category`, `subcategory`
ORDER BY `category`, `subcategory`;

SELECT COUNT(*) AS total_candidates FROM `tmp_decor_candidates`;

-- 3. Inspect one family to check name quality and display variety -------------

SELECT `entry`, `name`, `type`, `displayId`, `subcategory`
FROM `tmp_decor_candidates`
WHERE `subcategory` = 'Seating'
ORDER BY `name`
LIMIT 60;

-- 4. Find near-duplicates (same name family, digits stripped) ----------------
-- These are the rows the Python tool collapses. Each group should keep one.

SELECT
  REGEXP_REPLACE(LOWER(`name`), '[0-9]+', '') AS name_family,
  COUNT(*) AS variants,
  GROUP_CONCAT(`entry` ORDER BY `entry` SEPARATOR ',') AS entries
FROM `tmp_decor_candidates`
GROUP BY name_family
HAVING variants > 1
ORDER BY variants DESC
LIMIT 40;

-- 5. Emit ready-to-run rows for one category --------------------------------
-- Budget cost by keyword weight, matching doc 03 S.6.2. Ids start at 100000.

SELECT CONCAT(
  '(', 100000 + ROW_NUMBER() OVER (ORDER BY `entry`), ', ',
  QUOTE(`name`), ', ', `entry`, ', ',
  CASE
    WHEN `subcategory` IN ('Large Structures') THEN 5
    WHEN `subcategory` IN ('Large Foliage', 'Walls and Columns', 'Windows', 'Doors') THEN 4
    WHEN `subcategory` IN ('Tables and Desks', 'Storage', 'Beds') THEN 3
    WHEN `subcategory` IN ('Seating', 'Small Foliage', 'Wall Hangings') THEN 2
    ELSE 1
  END, ', ',
  0, ', 2, ',   -- surface: 0 = indoor-only default, placement: 2 = any
  QUOTE(`category`), ', ', QUOTE(`subcategory`),
  '),'
) AS `values_row`
FROM `tmp_decor_candidates`
WHERE `subcategory` = 'Seating'
LIMIT 50;
