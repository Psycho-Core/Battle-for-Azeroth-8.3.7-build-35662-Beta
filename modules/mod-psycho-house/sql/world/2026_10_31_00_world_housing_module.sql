-- ---------------------------------------------------------------------------
-- Player Housing - Founder's Point (alliance neighbourhood)
--
-- World content for the housing module: biomes, the 55 plot templates, the decor
-- catalogue, vendor placement and the amenity pins.
--
-- Reference: legacy-prototype/01-founders-point-reference.md
-- Schema:    legacy-prototype/03-database-schema.md
--
-- Coordinates are intentionally NULL / 0.0 in this update. The retail guides do not
-- publish per-plot world coordinates and the custom Founder's Point map does not exist
-- yet, so the module refuses to open any plot whose spawn_h* columns are NULL (see
-- Housing.SetOwner / HasSpawn). Edit these rows from the map editor once the terrain is
-- in place; nothing else has to change.
--
-- map_id 900 is the placeholder for the custom housing map. It must match
-- `Housing.Map.Id` in worldserver.conf.
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `housing_biome` (
  `id`          INT UNSIGNED NOT NULL,
  `name`        VARCHAR(64) NOT NULL,
  `description` TEXT,
  `faction`     TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 alliance, 1 horde',
  `zone_id`     INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'retail zone id - reference only',
  `ui_map_id`   INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'retail UI map id - reference only',
  `map_id`      INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'emulated map id the module uses',
  `flight_x`    FLOAT NOT NULL DEFAULT 0,
  `flight_y`    FLOAT NOT NULL DEFAULT 0,
  `flight_z`    FLOAT NOT NULL DEFAULT 0,
  `flight_o`    FLOAT NOT NULL DEFAULT 0,
  `center_x`    FLOAT NOT NULL DEFAULT 0,
  `center_y`    FLOAT NOT NULL DEFAULT 0,
  `flags`       INT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'bit0 sanctuary, bit1 no fly',
  PRIMARY KEY (`id`),
  KEY `idx_housing_biome_map` (`map_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Housing biomes (neighbourhood sub-zones)';

CREATE TABLE IF NOT EXISTS `housing_plot` (
  `map_id`      INT UNSIGNED NOT NULL,
  `plot_index`  TINYINT UNSIGNED NOT NULL COMMENT '0-54, as shown on the neighbourhood map',
  `biome_id`    INT UNSIGNED NOT NULL,
  `size_class`  TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '0 small, 1 standard, 2 large - my pricing split, not a retail value',
  `price`       INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'gold',
  `spawn_x`     FLOAT DEFAULT NULL COMMENT 'plot front door anchor, author on the custom map',
  `spawn_y`     FLOAT DEFAULT NULL,
  `spawn_z`     FLOAT DEFAULT NULL,
  `spawn_o`     FLOAT DEFAULT NULL,
  `yard_radius` FLOAT DEFAULT NULL COMMENT 'yard trigger + decor bound radius',
  `flags`       INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'bit0 island, bit1 bridge only, bit2 no outdoor decor',
  `comment`     VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`map_id`, `plot_index`),
  KEY `idx_housing_plot_biome` (`biome_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Housing plot templates';

CREATE TABLE IF NOT EXISTS `housing_decor` (
  `id`               INT UNSIGNED NOT NULL,
  `name`             VARCHAR(128) NOT NULL,
  `gameobject_entry` INT UNSIGNED NOT NULL COMMENT 'gameobject_template entry spawned when placed, 0 = not mapped yet',
  `item_entry`       INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'retail decor item id when the piece is a port, 0 for native pieces',
  `budget_cost`      TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'retail cost bands: 1, 3 or 5',
  `surface`          TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 floor, 1 wall, 2 ceiling, 3 any',
  `placement`        TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 outdoor, 1 indoor, 2 both',
  `category`         VARCHAR(32) NOT NULL DEFAULT '',
  `source_type`      TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 vendor, 1 quest, 2 achievement, 3 drop, 4 treasure hunt, 5 GM',
  `source_id`        INT UNSIGNED NOT NULL DEFAULT 0,
  `price`            INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'gold',
  `currency`         INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = gold, else currency id (3363 = community coupons)',
  `faction_mask`     TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'bit0 alliance, bit1 horde',
  `scale_min`        FLOAT NOT NULL DEFAULT 0.5,
  `scale_max`        FLOAT NOT NULL DEFAULT 3,
  `flags`            INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'bit0 interactive, bit1 no float, bit2 hidden',
  PRIMARY KEY (`id`),
  KEY `idx_housing_decor_category` (`category`),
  KEY `idx_housing_decor_source` (`source_type`, `source_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Housing decor catalogue';

CREATE TABLE IF NOT EXISTS `housing_vendor` (
  `id`             INT UNSIGNED NOT NULL,
  `name`           VARCHAR(64) NOT NULL,
  `biome_id`       INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = town center, else the biome it sits in',
  `role`           TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 decor, 1 contractor, 2 steward, 3 smuggler, 4 pet, 5 holiday',
  `creature_entry` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'existing 8.3.7 creature template used for appearance; 0 = not spawned',
  `x`              FLOAT NOT NULL DEFAULT 0,
  `y`              FLOAT NOT NULL DEFAULT 0,
  `z`              FLOAT NOT NULL DEFAULT 0,
  `o`              FLOAT NOT NULL DEFAULT 0,
  `retail_name`    VARCHAR(64) DEFAULT NULL,
  `comment`        VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_housing_vendor_biome` (`biome_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Housing vendor placement';

CREATE TABLE IF NOT EXISTS `housing_vendor_stock` (
  `vendor_id` INT UNSIGNED NOT NULL,
  `decor_id`  INT UNSIGNED NOT NULL,
  `price`     INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = use housing_decor.price',
  PRIMARY KEY (`vendor_id`, `decor_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Housing vendor stock';

CREATE TABLE IF NOT EXISTS `housing_amenity` (
  `id`       INT UNSIGNED NOT NULL,
  `biome_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = town center',
  `name`     VARCHAR(64) NOT NULL,
  `type`     TINYINT UNSIGNED NOT NULL DEFAULT 5 COMMENT '0 bulletin, 1 portal, 2 contractor, 3 flight point, 4 gathering, 5 scenery, 6 vendor, 7 endeavor',
  `x`        FLOAT NOT NULL DEFAULT 0,
  `y`        FLOAT NOT NULL DEFAULT 0,
  `z`        FLOAT NOT NULL DEFAULT 0,
  `o`        FLOAT NOT NULL DEFAULT 0,
  `radius`   FLOAT NOT NULL DEFAULT 0,
  `flags`    INT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'bit0 show as map pin',
  PRIMARY KEY (`id`),
  KEY `idx_housing_amenity_biome` (`biome_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Housing amenity pins';

CREATE TABLE IF NOT EXISTS `housing_neighborhood_name` (
  `id`      INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `faction` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `name`    VARCHAR(64) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_housing_name_faction` (`faction`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Public neighbourhood name pool';

-- ---------------------------------------------------------------------------
-- Biomes: six named sub-zones of Founder's Point
-- ---------------------------------------------------------------------------

DELETE FROM `housing_biome` WHERE `id` BETWEEN 1 AND 6;
INSERT INTO `housing_biome` (`id`, `name`, `description`, `faction`, `zone_id`, `ui_map_id`, `map_id`, `flags`) VALUES
(1, 'Brumewood Hollow', 'Duskwood with a Drustvar edge: deep forest, a graveyard, a nightwell and a mushroom ring. Klasa keeps a basement shop by the graveyard entrance and smugglers work Breakwater Bight.', 0, 16105, 2352, 900, 1),
(2, 'Gilded Oaks', 'Westfall autumn colours above the water: a beer garden with a water wheel, windmills, the lighthouse, and plots that end at the cliff over the bay.', 0, 16105, 2352, 900, 1),
(3, 'Mear''s Farm', 'Elwynn farmland: green fields, cows, vineyards with grape-stomping vats and orchards. The best biome for a decorated yard.', 0, 16105, 2352, 900, 1),
(4, 'Small Aerie', 'Green and lush, Elwynn charm, and the gryphon aerie with its flight point. Plots 48 and 53 sit close enough to build across the border.', 0, 16105, 2352, 900, 1),
(5, 'Stoneveil Ridge', 'Pastoral and quiet: sheep pasture, mountain views, few neighbours, and some plots reachable only by their own bridge.', 0, 16105, 2352, 900, 1),
(6, 'The Outer Banks', 'High cliffs, strong wind, a gothic romance mood, beachfront and private islands. The most expensive ground in the neighbourhood.', 0, 16105, 2352, 900, 1);

-- ---------------------------------------------------------------------------
-- The 55 plots. Biome assignment comes straight from the guide's per-biome plot
-- lists; every index 0-54 appears exactly once.
-- ---------------------------------------------------------------------------

DELETE FROM `housing_plot` WHERE `map_id` = 900;
INSERT INTO `housing_plot`
(`map_id`, `plot_index`, `biome_id`, `size_class`, `price`, `spawn_x`, `spawn_y`, `spawn_z`, `spawn_o`, `yard_radius`, `flags`, `comment`)
VALUES
(900, 0, 1, 0, 150000, NULL, NULL, NULL, NULL, NULL, 0, 'Brumewood Hollow'),
(900, 1, 2, 1, 275000, NULL, NULL, NULL, NULL, NULL, 0, 'Gilded Oaks'),
(900, 2, 1, 0, 150000, NULL, NULL, NULL, NULL, NULL, 0, 'Brumewood Hollow'),
(900, 3, 5, 2, 400000, NULL, NULL, NULL, NULL, NULL, 0, 'Stoneveil Ridge - verify bridge-only access during authoring'),
(900, 4, 6, 2, 350000, NULL, NULL, NULL, NULL, NULL, 0, 'Outer Banks - cliff/coastal plot'),
(900, 5, 2, 1, 275000, NULL, NULL, NULL, NULL, NULL, 0, 'Gilded Oaks'),
(900, 6, 1, 0, 150000, NULL, NULL, NULL, NULL, NULL, 0, 'Brumewood Hollow'),
(900, 7, 3, 1, 250000, NULL, NULL, NULL, NULL, NULL, 0, 'Mear''s Farm'),
(900, 8, 1, 0, 150000, NULL, NULL, NULL, NULL, NULL, 0, 'Brumewood Hollow'),
(900, 9, 3, 1, 250000, NULL, NULL, NULL, NULL, NULL, 0, 'Mear''s Farm'),
(900, 10, 1, 0, 150000, NULL, NULL, NULL, NULL, NULL, 0, 'Brumewood Hollow'),
(900, 11, 4, 1, 225000, NULL, NULL, NULL, NULL, NULL, 0, 'Small Aerie'),
(900, 12, 6, 2, 350000, NULL, NULL, NULL, NULL, NULL, 0, 'Outer Banks - cliff/coastal plot'),
(900, 13, 2, 1, 275000, NULL, NULL, NULL, NULL, NULL, 0, 'Backs onto the apple orchard; plot ends at the cliff edge above the bay'),
(900, 14, 2, 1, 275000, NULL, NULL, NULL, NULL, NULL, 0, 'Gilded Oaks'),
(900, 15, 1, 0, 150000, NULL, NULL, NULL, NULL, NULL, 0, 'Brumewood Hollow'),
(900, 16, 4, 1, 225000, NULL, NULL, NULL, NULL, NULL, 0, 'Small Aerie'),
(900, 17, 1, 0, 150000, NULL, NULL, NULL, NULL, NULL, 0, 'Brumewood Hollow'),
(900, 18, 1, 0, 150000, NULL, NULL, NULL, NULL, NULL, 0, 'Brumewood Hollow'),
(900, 19, 6, 2, 350000, NULL, NULL, NULL, NULL, NULL, 0, 'Outer Banks - cliff/coastal plot'),
(900, 20, 1, 0, 150000, NULL, NULL, NULL, NULL, NULL, 0, 'Brumewood Hollow'),
(900, 21, 1, 0, 150000, NULL, NULL, NULL, NULL, NULL, 0, 'Brumewood Hollow'),
(900, 22, 5, 2, 400000, NULL, NULL, NULL, NULL, NULL, 0, 'Stoneveil Ridge - verify bridge-only access during authoring'),
(900, 23, 5, 2, 400000, NULL, NULL, NULL, NULL, NULL, 0, 'Stoneveil Ridge - verify bridge-only access during authoring'),
(900, 24, 6, 2, 350000, NULL, NULL, NULL, NULL, NULL, 0, 'Outer Banks - confirm private island vs beachfront during authoring'),
(900, 25, 6, 2, 350000, NULL, NULL, NULL, NULL, NULL, 0, 'Outer Banks - confirm private island vs beachfront during authoring'),
(900, 26, 2, 1, 275000, NULL, NULL, NULL, NULL, NULL, 0, 'Gilded Oaks'),
(900, 27, 3, 1, 250000, NULL, NULL, NULL, NULL, NULL, 0, 'Mear''s Farm'),
(900, 28, 6, 2, 350000, NULL, NULL, NULL, NULL, NULL, 0, 'Outer Banks - cliff/coastal plot'),
(900, 29, 6, 2, 350000, NULL, NULL, NULL, NULL, NULL, 0, 'Outer Banks - cliff/coastal plot'),
(900, 30, 2, 1, 275000, NULL, NULL, NULL, NULL, NULL, 0, 'Gilded Oaks'),
(900, 31, 5, 2, 400000, NULL, NULL, NULL, NULL, NULL, 0, 'Stoneveil Ridge - verify bridge-only access during authoring'),
(900, 32, 5, 2, 400000, NULL, NULL, NULL, NULL, NULL, 0, 'Stoneveil Ridge - verify bridge-only access during authoring'),
(900, 33, 1, 0, 150000, NULL, NULL, NULL, NULL, NULL, 0, 'Brumewood Hollow'),
(900, 34, 3, 1, 250000, NULL, NULL, NULL, NULL, NULL, 0, 'Mear''s Farm'),
(900, 35, 2, 1, 275000, NULL, NULL, NULL, NULL, NULL, 0, 'Gilded Oaks'),
(900, 36, 2, 1, 275000, NULL, NULL, NULL, NULL, NULL, 0, 'Gilded Oaks'),
(900, 37, 2, 1, 275000, NULL, NULL, NULL, NULL, NULL, 0, 'Gilded Oaks'),
(900, 38, 1, 0, 150000, NULL, NULL, NULL, NULL, NULL, 0, 'Brumewood Hollow'),
(900, 39, 3, 1, 250000, NULL, NULL, NULL, NULL, NULL, 0, 'Mear''s Farm'),
(900, 40, 6, 2, 350000, NULL, NULL, NULL, NULL, NULL, 0, 'Outer Banks - cliff/coastal plot'),
(900, 41, 1, 0, 150000, NULL, NULL, NULL, NULL, NULL, 0, 'Brumewood Hollow'),
(900, 42, 3, 1, 250000, NULL, NULL, NULL, NULL, NULL, 0, 'Mear''s Farm'),
(900, 43, 6, 2, 350000, NULL, NULL, NULL, NULL, NULL, 0, 'Outer Banks - cliff/coastal plot'),
(900, 44, 3, 1, 250000, NULL, NULL, NULL, NULL, NULL, 0, 'Mear''s Farm'),
(900, 45, 3, 1, 250000, NULL, NULL, NULL, NULL, NULL, 0, 'Mear''s Farm'),
(900, 46, 4, 1, 225000, NULL, NULL, NULL, NULL, NULL, 0, 'Small Aerie'),
(900, 47, 4, 1, 225000, NULL, NULL, NULL, NULL, NULL, 0, 'Small Aerie'),
(900, 48, 4, 1, 225000, NULL, NULL, NULL, NULL, NULL, 0, 'Adjoins plot 53 - the pair can read as one estate across the shared border'),
(900, 49, 4, 1, 225000, NULL, NULL, NULL, NULL, NULL, 0, 'Small Aerie'),
(900, 50, 6, 2, 350000, NULL, NULL, NULL, NULL, NULL, 0, 'Outer Banks - cliff/coastal plot'),
(900, 51, 4, 1, 225000, NULL, NULL, NULL, NULL, NULL, 0, 'Small Aerie'),
(900, 52, 6, 2, 350000, NULL, NULL, NULL, NULL, NULL, 0, 'Outer Banks - cliff/coastal plot'),
(900, 53, 4, 1, 225000, NULL, NULL, NULL, NULL, NULL, 0, 'Adjoins plot 48 - the pair can read as one estate across the shared border'),
(900, 54, 4, 1, 225000, NULL, NULL, NULL, NULL, NULL, 0, 'Small Aerie');

-- ---------------------------------------------------------------------------
-- Vendors. Retail entries (Faarden 255213, Argan 255218, Balen 255216, Devin 257332,
-- Perry 267795, Jorvan 255104) do not exist in the 8.3.7 client, so creature_entry stays
-- 0 until an appearance template is chosen. The retail Town Center coordinates are kept
-- in the comment because they are the only published positions.
-- ---------------------------------------------------------------------------

DELETE FROM `housing_vendor` WHERE `id` BETWEEN 1 AND 12;
INSERT INTO `housing_vendor` (`id`, `name`, `biome_id`, `role`, `creature_entry`, `retail_name`, `comment`) VALUES
(1,  'Jorvan Longmoor',        0, 1, 0, 'Jorvan Longmoor',    'General Contractor, Town Center. Retail entry 255104. TODO(client): place + appearance template.'),
(2,  'Argan Hammerfist',       0, 0, 0, 'Argan Hammerfist',   'Elven furniture (tables, benches, barrels, weapon racks). Retail entry 255218, /way #2352 52.2 37.8.'),
(3,  'Balen Starfinder',       0, 0, 0, 'Balen Starfinder',   'Bel''ameth architecture: platforms, walls, doorways, rugs, bath. Retail entry 255216, /way #2352 52.2 37.8.'),
(4,  'Faarden the Builder',    0, 0, 0, 'Faarden the Builder','Local and Stormwind decor: tents, market stands, fountains, gryphon roost, signposts. Retail entry 255213, /way #2352 52.0 38.4.'),
(5,  'Xiao Dan',               0, 0, 0, 'Xiao Dan',           'Pandaren and local furnishings. Retail coords 52.0 38.4. Entry not captured - read from client DB2.'),
(6,  'Trevor Grenner',         0, 0, 0, 'Trevor Grenner',     'Plants and nature: Elwynn apple trees, ivy, bellflowers, boulders. Retail coords 52.0 38.4. Entry not captured.'),
(7,  'Devin Slatesmith',       0, 0, 0, 'Devin Slatesmith',   'Carved wood and stone interior surfaces, staircases. Retail entry 257332, 52.0 38.4.'),
(8,  'Klasa',                  1, 0, 0, 'Klasa',              'Cobwebs and spooky accents. The only vendor outside the Town Center: basement of the house by the Brumewood Hollow graveyard entrance.'),
(9,  'Perry Winkles',          0, 4, 0, 'Perry Winkles',      'Pet decor, including extra copies of Pepe for 10 Community Coupons (3363). Retail entry 267795.'),
(10, 'Fel''orei Moonstrider',  0, 0, 0, 'Fel''orei Moonstrider','Elven / nightborne decor. Entry not captured.'),
(11, 'Holly Twinklebright',    0, 5, 0, 'Holly Twinklebright','Holiday decor (Hallow''s End stock observed). Entry not captured.'),
(12, 'Breakwater Smuggler',    1, 3, 0, NULL,                 'Breakwater Bight. Sells Horde-oriented decor to alliance players; the Horde mirror is "Len" Splinthoof in Razorwind Shores at 39.9 73.3.');

-- ---------------------------------------------------------------------------
-- Amenities. Types drive the addon map pins; positions are authored with the map.
-- ---------------------------------------------------------------------------

DELETE FROM `housing_amenity` WHERE `id` BETWEEN 1 AND 22;
INSERT INTO `housing_amenity` (`id`, `biome_id`, `name`, `type`, `radius`, `flags`) VALUES
(1,  0, 'Town Center',            4, 60, 1),
(2,  0, 'Bulletin Board',         0,  3, 1),
(3,  0, 'General Contractor',     2,  3, 1),
(4,  0, 'Portal to Stormwind',    1,  4, 1),
(5,  0, 'Endeavors Area',         7,  8, 1),
(6,  0, 'Stage',                  4, 10, 1),
(7,  0, 'Moonwell',               5,  6, 1),
(8,  1, 'Brumewood Graveyard',    5, 15, 1),
(9,  1, 'Nightwell',              5,  6, 1),
(10, 1, 'Mushroom Ring',          5,  5, 1),
(11, 1, 'Breakwater Bight',       6, 20, 1),
(12, 1, 'Brumewood Flight Point', 3,  5, 1),
(13, 2, 'Beer Garden',            4, 12, 1),
(14, 2, 'Windmills',              5, 10, 1),
(15, 2, 'Lighthouse',             5,  8, 1),
(16, 2, 'Gilded Oaks Flight Point',3, 5, 1),
(17, 3, 'Vineyards',              5, 20, 1),
(18, 3, 'Orchards',               5, 20, 1),
(19, 3, 'Mear''s Farm Flight Point',3,5, 1),
(20, 4, 'Gryphon Aerie',          3, 12, 1),
(21, 5, 'Stoneveil Sheep Pasture',5, 25, 1),
(22, 6, 'Outer Banks Flight Point',3, 5, 1);

-- ---------------------------------------------------------------------------
-- Public neighbourhood name pool
-- ---------------------------------------------------------------------------

DELETE FROM `housing_neighborhood_name` WHERE `faction` = 0;
INSERT INTO `housing_neighborhood_name` (`faction`, `name`) VALUES
(0, 'Elwynn Rest'), (0, 'Hollowmere'), (0, 'Oakhollow'), (0, 'Brightwater'),
(0, 'Greenfield'), (0, 'Stormward'), (0, 'Amberfall'), (0, 'Westbrook'),
(0, 'Goldvale'), (0, 'Cliffhaven'), (0, 'Ridgewatch'), (0, 'Bramblewood');

-- ---------------------------------------------------------------------------
-- Decor catalogue
--
-- Deliberately shipped empty. Every row needs a `gameobject_entry` that exists in the
-- 8.3.7 client, and I am not going to invent entries - a wrong number spawns nothing and
-- logs an error on every load. The rows are authored from the client's gameobject table,
-- e.g.:
--
--   SELECT entry, name FROM gameobject_template
--   WHERE name LIKE '%Chair%' OR name LIKE '%Table%' OR name LIKE '%Bench%'
--   ORDER BY name LIMIT 100;
--
-- Insert them with budget/surface/placement/category and a vendor (see
-- legacy-prototype/06-build-test-operations.md, "Authoring the decor catalogue").
--
-- Template row, copy and edit:
--
--   INSERT INTO housing_decor (id, name, gameobject_entry, budget_cost, surface, placement,
--       category, source_type, source_id, price, currency, faction_mask, scale_min, scale_max, flags)
--   VALUES (1001, 'Elwynn Apple Tree', <go entry>, 3, 0, 0, 'Nature', 0, 6, 75, 0, 1, 0.5, 2.0, 0);
-- ---------------------------------------------------------------------------
