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
  `subcategory`      VARCHAR(32) NOT NULL DEFAULT '' COMMENT 'second level of the catalogue, empty when the piece has none',
  `source_type`      TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 vendor, 1 quest, 2 achievement, 3 drop, 4 treasure hunt, 5 GM',
  `source_id`        INT UNSIGNED NOT NULL DEFAULT 0,
  `price`            INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'gold',
  `currency`         INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = gold, else currency id (3363 = community coupons)',
  `faction_mask`     TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'bit0 alliance, bit1 horde',
  `scale_min`        FLOAT NOT NULL DEFAULT 0.5,
  `scale_max`        FLOAT NOT NULL DEFAULT 3,
  `dye_group`        TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = not dyable, see housing_decor_variant',
  `flags`            INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'bit0 interactive, bit1 no float, bit2 hidden, bit4 pet bed, bit5 jukebox, bit6 mount stand, bit7 mannequin, bit8 dyable',
  `night_entry`      INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'gameobject entry spawned instead while the Armillary is dark, 0 = the same piece',
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
  `milestone` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = always on sale, 1-4 = Endeavor milestone',
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
  `gameobject_entry` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'object spawned at x,y,z, 0 = pin only',
  `creature_entry`   INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'NPC spawned at x,y,z, 0 = none',
  `go_name`     VARCHAR(128) NOT NULL DEFAULT '' COMMENT 'name the bind statement searches for',
  `creature_name` VARCHAR(128) NOT NULL DEFAULT '' COMMENT 'name the bind statement searches for',
  `flags`    INT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'bit0 pin, bit1 sanctuary, bit2 portal',
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

-- Plot 0..54. The coordinates are the anchor layout written down: the same
-- concentric rings HousingMgr::AnchorForPlot builds at runtime, frozen into the table so a plot
-- can be authored against a surveyed position instead of guessed at. spawn_z is NULL on every
-- row on purpose - the ground is read from the map the first time somebody walks into the plot,
-- so these rows work on a map whose terrain is not final yet. Replace any row with a surveyed
-- position and the module uses it as written.
DELETE FROM `housing_plot` WHERE `map_id` = 900;
INSERT INTO `housing_plot`
(`map_id`, `plot_index`, `biome_id`, `size_class`, `price`, `spawn_x`, `spawn_y`, `spawn_z`, `spawn_o`, `yard_radius`, `flags`, `comment`)
VALUES
(900, 0, 1, 0, 150000, 90.0, 0.0, NULL, 3.1416, 25.0, 0, 'Brumewood Hollow'),
(900, 1, 2, 1, 275000, 72.81, 52.9, NULL, 3.7699, 40.0, 0, 'Gilded Oaks'),
(900, 2, 1, 0, 150000, 27.81, 85.6, NULL, 4.3982, 25.0, 0, 'Brumewood Hollow'),
(900, 3, 5, 2, 400000, -27.81, 85.6, NULL, 5.0265, 60.0, 0, 'Stoneveil Ridge - verify bridge-only access during authoring'),
(900, 4, 6, 2, 350000, -72.81, 52.9, NULL, 5.6549, 60.0, 0, 'Outer Banks - cliff/coastal plot'),
(900, 5, 2, 1, 275000, -90.0, 0.0, NULL, 0.0000, 40.0, 0, 'Gilded Oaks'),
(900, 6, 1, 0, 150000, -72.81, -52.9, NULL, 0.6283, 25.0, 0, 'Brumewood Hollow'),
(900, 7, 3, 1, 250000, -27.81, -85.6, NULL, 1.2566, 40.0, 0, 'Mear''s Farm'),
(900, 8, 1, 0, 150000, 27.81, -85.6, NULL, 1.8849, 25.0, 0, 'Brumewood Hollow'),
(900, 9, 3, 1, 250000, 72.81, -52.9, NULL, 2.5133, 40.0, 0, 'Mear''s Farm'),
(900, 10, 1, 0, 150000, 150.3, 54.86, NULL, 3.4916, 25.0, 0, 'Brumewood Hollow'),
(900, 11, 4, 1, 225000, 102.73, 122.66, NULL, 4.0152, 40.0, 0, 'Small Aerie'),
(900, 12, 6, 2, 350000, 27.64, 157.6, NULL, 4.5388, 60.0, 0, 'Outer Banks - cliff/coastal plot'),
(900, 13, 2, 1, 275000, -54.86, 150.3, NULL, 5.0624, 40.0, 0, 'Backs onto the apple orchard; plot ends at the cliff edge above the bay'),
(900, 14, 2, 1, 275000, -122.66, 102.73, NULL, 5.5860, 40.0, 0, 'Gilded Oaks'),
(900, 15, 1, 0, 150000, -157.6, 27.64, NULL, 6.1096, 25.0, 0, 'Brumewood Hollow'),
(900, 16, 4, 1, 225000, -150.3, -54.86, NULL, 0.3500, 40.0, 0, 'Small Aerie'),
(900, 17, 1, 0, 150000, -102.73, -122.66, NULL, 0.8736, 25.0, 0, 'Brumewood Hollow'),
(900, 18, 1, 0, 150000, -27.64, -157.6, NULL, 1.3972, 25.0, 0, 'Brumewood Hollow'),
(900, 19, 6, 2, 350000, 54.86, -150.3, NULL, 1.9208, 60.0, 0, 'Outer Banks - cliff/coastal plot'),
(900, 20, 1, 0, 150000, 122.66, -102.73, NULL, 2.4444, 25.0, 0, 'Brumewood Hollow'),
(900, 21, 1, 0, 150000, 157.6, -27.64, NULL, 2.9680, 25.0, 0, 'Brumewood Hollow'),
(900, 22, 5, 2, 400000, 175.91, 148.17, NULL, 3.8416, 60.0, 0, 'Stoneveil Ridge - verify bridge-only access during authoring'),
(900, 23, 5, 2, 400000, 94.2, 209.82, NULL, 4.2904, 60.0, 0, 'Stoneveil Ridge - verify bridge-only access during authoring'),
(900, 24, 6, 2, 350000, -6.16, 229.92, NULL, 4.7392, 60.0, 0, 'Outer Banks - confirm private island vs beachfront during authoring'),
(900, 25, 6, 2, 350000, -105.31, 204.47, NULL, 5.1880, 60.0, 0, 'Outer Banks - confirm private island vs beachfront during authoring'),
(900, 26, 2, 1, 275000, -183.6, 138.53, NULL, 5.6368, 40.0, 0, 'Gilded Oaks'),
(900, 27, 3, 1, 250000, -225.52, 45.15, NULL, 6.0856, 40.0, 0, 'Mear''s Farm'),
(900, 28, 6, 2, 350000, -222.78, -57.17, NULL, 0.2512, 60.0, 0, 'Outer Banks - cliff/coastal plot'),
(900, 29, 6, 2, 350000, -175.91, -148.17, NULL, 0.7000, 60.0, 0, 'Outer Banks - cliff/coastal plot'),
(900, 30, 2, 1, 275000, -94.2, -209.82, NULL, 1.1488, 40.0, 0, 'Gilded Oaks'),
(900, 31, 5, 2, 400000, 6.16, -229.92, NULL, 1.5976, 60.0, 0, 'Stoneveil Ridge - verify bridge-only access during authoring'),
(900, 32, 5, 2, 400000, 105.31, -204.47, NULL, 2.0464, 60.0, 0, 'Stoneveil Ridge - verify bridge-only access during authoring'),
(900, 33, 1, 0, 150000, 183.6, -138.53, NULL, 2.4952, 25.0, 0, 'Brumewood Hollow'),
(900, 34, 3, 1, 250000, 225.52, -45.15, NULL, 2.9440, 40.0, 0, 'Mear''s Farm'),
(900, 35, 2, 1, 275000, 222.78, 57.17, NULL, 3.3928, 40.0, 0, 'Gilded Oaks'),
(900, 36, 2, 1, 275000, 149.27, 260.23, NULL, 4.1916, 40.0, 0, 'Gilded Oaks'),
(900, 37, 2, 1, 275000, 38.32, 297.54, NULL, 4.5843, 40.0, 0, 'Gilded Oaks'),
(900, 38, 1, 0, 150000, -78.46, 289.56, NULL, 4.9770, 25.0, 0, 'Brumewood Hollow'),
(900, 39, 3, 1, 250000, -183.29, 237.49, NULL, 5.3697, 40.0, 0, 'Mear''s Farm'),
(900, 40, 6, 2, 350000, -260.23, 149.27, NULL, 5.7624, 60.0, 0, 'Outer Banks - cliff/coastal plot'),
(900, 41, 1, 0, 150000, -297.54, 38.32, NULL, 6.1551, 25.0, 0, 'Brumewood Hollow'),
(900, 42, 3, 1, 250000, -289.56, -78.46, NULL, 0.2646, 40.0, 0, 'Mear''s Farm'),
(900, 43, 6, 2, 350000, -237.49, -183.29, NULL, 0.6573, 60.0, 0, 'Outer Banks - cliff/coastal plot'),
(900, 44, 3, 1, 250000, -149.27, -260.23, NULL, 1.0500, 40.0, 0, 'Mear''s Farm'),
(900, 45, 3, 1, 250000, -38.32, -297.54, NULL, 1.4427, 40.0, 0, 'Mear''s Farm'),
(900, 46, 4, 1, 225000, 78.46, -289.56, NULL, 1.8354, 40.0, 0, 'Small Aerie'),
(900, 47, 4, 1, 225000, 183.29, -237.49, NULL, 2.2281, 40.0, 0, 'Small Aerie'),
(900, 48, 4, 1, 225000, 260.23, -149.27, NULL, 2.6208, 40.0, 0, 'Adjoins plot 53 - the pair can read as one estate across the shared border'),
(900, 49, 4, 1, 225000, 297.54, -38.32, NULL, 3.0135, 40.0, 0, 'Small Aerie'),
(900, 50, 6, 2, 350000, 289.56, 78.46, NULL, 3.4062, 60.0, 0, 'Outer Banks - cliff/coastal plot'),
(900, 51, 4, 1, 225000, 237.49, 183.29, NULL, 3.7989, 40.0, 0, 'Small Aerie'),
(900, 52, 6, 2, 350000, 62.89, 364.62, NULL, 4.5416, 60.0, 0, 'Outer Banks - cliff/coastal plot'),
(900, 53, 4, 1, 225000, -65.61, 364.14, NULL, 4.8907, 40.0, 0, 'Adjoins plot 48 - the pair can read as one estate across the shared border'),
(900, 54, 4, 1, 225000, -186.2, 319.74, NULL, 5.2397, 40.0, 0, 'Small Aerie');

-- ---------------------------------------------------------------------------
-- House levels 1-12. Retail's ladder: the XP needed for each level and the three decor
-- budgets the level hands out. Loaded at boot; the values in HousingDefines.h are the
-- fallback if this table is empty.
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `housing_house_level` (
  `level`         TINYINT UNSIGNED NOT NULL,
  `xp`            INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'total XP needed to reach this level',
  `budget_indoor` INT UNSIGNED NOT NULL DEFAULT 0,
  `budget_room`   INT UNSIGNED NOT NULL DEFAULT 0,
  `budget_outdoor` INT UNSIGNED NOT NULL DEFAULT 0,
  `rooms`         TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'rooms unlocked',
  PRIMARY KEY (`level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='House level ladder';

-- ---------------------------------------------------------------------------
-- Exterior shells and the rooms inside them. `gameobject_entry` stays 0 until
-- tools/sql/bind_housing_content.sql fills it from the realm's own gameobject_template,
-- the same way draft C binds its facilities: the statement says what it is looking for and
-- reports when it matched nothing.
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `housing_shell` (
  `id`        INT UNSIGNED NOT NULL,
  `name`      VARCHAR(64) NOT NULL,
  `gameobject_entry` INT UNSIGNED NOT NULL DEFAULT 0,
  `go_name`   VARCHAR(128) NOT NULL DEFAULT '' COMMENT 'name the bind statement searches for',
  `size_class` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '0 small, 1 standard, 2 large',
  `faction_mask` TINYINT UNSIGNED NOT NULL DEFAULT 3,
  `level_required` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `flags`     INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_housing_shell_size` (`size_class`, `faction_mask`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='House exterior shells';

CREATE TABLE IF NOT EXISTS `housing_room` (
  `id`        INT UNSIGNED NOT NULL,
  `shell_id`  INT UNSIGNED NOT NULL,
  `name`      VARCHAR(64) NOT NULL,
  `x`         FLOAT NOT NULL DEFAULT 0 COMMENT 'offset from the interior anchor',
  `y`         FLOAT NOT NULL DEFAULT 0,
  `z`         FLOAT NOT NULL DEFAULT 0,
  `radius`    FLOAT NOT NULL DEFAULT 12 COMMENT 'the room the player must stand in to decorate it',
  `budget_cost` TINYINT UNSIGNED NOT NULL DEFAULT 5 COMMENT 'rooms come out of the room budget',
  `level_required` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`id`),
  KEY `idx_housing_room_shell` (`shell_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Rooms inside a shell';

-- ---------------------------------------------------------------------------
-- Dyes. A game object cannot be recoloured at runtime, so a dye is a different object with
-- the same footprint. Only pieces with rows here are dyable, and the catalogue says so per
-- piece instead of promising seventy-seven colours to everything.
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `housing_decor_variant` (
  `decor_id`  INT UNSIGNED NOT NULL,
  `dye_group` TINYINT UNSIGNED NOT NULL,
  `dye_color` TINYINT UNSIGNED NOT NULL,
  `dye_name`  VARCHAR(32) NOT NULL DEFAULT '',
  `gameobject_entry` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`decor_id`, `dye_group`, `dye_color`),
  KEY `idx_housing_variant_group` (`dye_group`, `dye_color`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Dyed appearances of a decor piece';

-- Pet beds: the creature a bed owns. A bed without a row here is furniture.
CREATE TABLE IF NOT EXISTS `housing_dye` (
  `id`     TINYINT UNSIGNED NOT NULL,
  `name`   VARCHAR(16) NOT NULL COMMENT 'the colour word the game object name starts with',
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_housing_dye_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Dye colours the module can swap a piece to';

CREATE TABLE IF NOT EXISTS `housing_decor_creature` (
  `decor_id`  INT UNSIGNED NOT NULL,
  `kind`      TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 pet bed, 1 mount stand, 2 mannequin',
  `creature_entry` INT UNSIGNED NOT NULL DEFAULT 0,
  `creature_name` VARCHAR(128) NOT NULL DEFAULT '' COMMENT 'name the bind statement searches for',
  `roaming`   TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `radius`    FLOAT NOT NULL DEFAULT 8,
  PRIMARY KEY (`decor_id`, `kind`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Creatures a piece of decor owns';

-- ---------------------------------------------------------------------------
-- Endeavors. One definition per monthly rotation, tasks that count what players already do
-- in 8.3.7, and rewards gated by milestone - the milestone is what unlocks vendor stock
-- (housing_vendor_stock.milestone).
--
-- `target` is 0 in this file on purpose: a creature, quest or item name is searchable in
-- the realm's own tables by tools/sql/bind_housing_content.sql, an id written from memory
-- is not. A task with target 0 and no bind match never completes rather than completing on
-- the wrong kill.
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `housing_endeavor` (
  `id`         INT UNSIGNED NOT NULL,
  `name`       VARCHAR(64) NOT NULL,
  `description` TEXT,
  `duration_days` INT UNSIGNED NOT NULL DEFAULT 30,
  `base_required` INT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'scaled by neighbourhood size',
  `flags`      INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Endeavor definitions';

CREATE TABLE IF NOT EXISTS `housing_endeavor_task` (
  `id`         INT UNSIGNED NOT NULL,
  `endeavor_id` INT UNSIGNED NOT NULL,
  `type`       TINYINT UNSIGNED NOT NULL COMMENT '0 kill, 1 boss, 2 quest, 3 loot, 4 craft, 5 pet battle, 6 deliver',
  `target`     INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = not bound yet',
  `target_name` VARCHAR(128) NOT NULL DEFAULT '' COMMENT 'name the bind statement searches for',
  `required`   INT UNSIGNED NOT NULL DEFAULT 1,
  `text`       VARCHAR(160) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  KEY `idx_housing_endeavor_task` (`endeavor_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Endeavor tasks';

CREATE TABLE IF NOT EXISTS `housing_endeavor_reward` (
  `endeavor_id` INT UNSIGNED NOT NULL,
  `milestone`  TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `item_entry` INT UNSIGNED NOT NULL DEFAULT 0,
  `item_count` INT UNSIGNED NOT NULL DEFAULT 1,
  `currency_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `currency_amount` INT UNSIGNED NOT NULL DEFAULT 0,
  `xp`         INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`endeavor_id`, `milestone`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Endeavor milestone rewards';

-- ---------------------------------------------------------------------------
-- The portal amenity's destination list. One object, a gossip menu, no client patch.
-- map/x/y/z/o are absolute; faction_mask 3 = both.
-- ---------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `housing_hunt` (
  `id`           INT UNSIGNED NOT NULL,
  `name`         VARCHAR(64) NOT NULL,
  `riddle`       VARCHAR(200) NOT NULL DEFAULT '' COMMENT 'the line the riddle hunt is built around',
  `quest_id`     INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'quest_template id, 0 = the hunt cannot be taken',
  `reward_decor` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'housing_decor id the cache rewards, 0 = this realm has no such piece',
  `cache_entry`  INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'gameobject entry the cache is spawned from',
  `x`            FLOAT NOT NULL DEFAULT 0 COMMENT '0,0 places the cache on the hub ring instead',
  `y`            FLOAT NOT NULL DEFAULT 0,
  `z`            FLOAT NOT NULL DEFAULT 0,
  `faction_mask` TINYINT UNSIGNED NOT NULL DEFAULT 3 COMMENT 'bit0 alliance, bit1 horde',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Riddle treasure hunts and the cache each one hides';

CREATE TABLE IF NOT EXISTS `housing_portal` (
  `id`         INT UNSIGNED NOT NULL,
  `amenity_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `name`       VARCHAR(64) NOT NULL,
  `map`        INT UNSIGNED NOT NULL,
  `x`          FLOAT NOT NULL DEFAULT 0,
  `y`          FLOAT NOT NULL DEFAULT 0,
  `z`          FLOAT NOT NULL DEFAULT 0,
  `o`          FLOAT NOT NULL DEFAULT 0,
  `faction_mask` TINYINT UNSIGNED NOT NULL DEFAULT 3,
  `level_required` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `tele_name`  VARCHAR(64) NOT NULL DEFAULT '' COMMENT 'game_tele name the bind statement copies the position from',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Portal destinations';

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

-- `go_name` and `creature_name` are search strings, not ids: bind_housing_content.sql looks
-- them up in the realm's own tables and reports what it could not find. An amenity with
-- neither stays a pin on the addon map, which is what most of them were before.
DELETE FROM `housing_amenity` WHERE `id` BETWEEN 1 AND 22;
INSERT INTO `housing_amenity` (`id`, `biome_id`, `name`, `type`, `radius`, `flags`,
    `go_name`, `creature_name`) VALUES
(1,  0, 'Town Center',            4, 60, 1, '', ''),
(2,  0, 'Bulletin Board',         0,  3, 1, 'Notice Board', ''),
(3,  0, 'General Contractor',     2,  3, 9, '', ''),
(4,  0, 'Portal to Stormwind',    1,  4, 5, 'Portal', ''),
(5,  0, 'Endeavors Area',         7,  8, 1, 'Banner', ''),
(6,  0, 'Stage',                  4, 10, 1, 'Stage', ''),
(7,  0, 'Moonwell',               5,  6, 1, 'Moonwell', ''),
(8,  1, 'Brumewood Graveyard',    5, 15, 1, '', ''),
(9,  1, 'Nightwell',              5,  6, 1, 'Moonwell', ''),
(10, 1, 'Mushroom Ring',          5,  5, 1, 'Mushroom', ''),
(11, 1, 'Breakwater Bight',       6, 20, 1, 'Dock', ''),
(12, 1, 'Brumewood Flight Point', 3,  5, 1, 'Gryphon', ''),
(13, 2, 'Beer Garden',            4, 12, 1, 'Banner', ''),
(14, 2, 'Windmills',              5, 10, 1, 'Windmill', ''),
(15, 2, 'Lighthouse',             5,  8, 1, 'Lighthouse', ''),
(16, 2, 'Gilded Oaks Flight Point',3, 5, 1, 'Gryphon', ''),
(17, 3, 'Vineyards',              5, 20, 1, 'Vineyard', ''),
(18, 3, 'Orchards',               5, 20, 1, 'Apple Tree', ''),
(19, 3, 'Mear''s Farm Flight Point',3,5, 1, 'Gryphon', ''),
(20, 4, 'Gryphon Aerie',          3, 12, 1, 'Gryphon', ''),
(21, 5, 'Stoneveil Sheep Pasture',5, 25, 1, '', 'Sheep'),
(22, 6, 'Outer Banks Flight Point',3, 5, 1, 'Gryphon', '');

-- ---------------------------------------------------------------------------
-- Public neighbourhood name pool
-- ---------------------------------------------------------------------------

DELETE FROM `housing_neighborhood_name` WHERE `faction` = 0;
INSERT INTO `housing_neighborhood_name` (`faction`, `name`) VALUES
(0, 'Elwynn Rest'), (0, 'Hollowmere'), (0, 'Oakhollow'), (0, 'Brightwater'),
(0, 'Greenfield'), (0, 'Stormward'), (0, 'Amberfall'), (0, 'Westbrook'),
(0, 'Goldvale'), (0, 'Cliffhaven'), (0, 'Ridgewatch'), (0, 'Bramblewood');

-- The Horde pool. Both factions get a pool of their own: a Horde neighbourhood named
-- after an Alliance harbour is the kind of thing nobody notices until they do.
DELETE FROM `housing_neighborhood_name` WHERE `faction` = 1;
INSERT INTO `housing_neighborhood_name` (`faction`, `name`) VALUES
(1, 'Razorwind Rest'), (1, 'Durotar Hold'), (1, 'Ashenplain'), (1, 'Thunderstep'),
(1, 'Redrock'), (1, 'Darkspear Landing'), (1, 'Bonefire'), (1, 'Warcamp'),
(1, 'Sunveil'), (1, 'Grimhollow'), (1, 'Ashfall'), (1, 'Wolfsbane');

-- ---------------------------------------------------------------------------
-- NPC roles.
--
-- An NPC only answers gossip if its `creature_template.ScriptName` names one of the module's
-- scripts, so this is the half of the NPC work that lives in the world database. Every
-- statement is written as an UPDATE by name: a realm that has the creature gets the
-- behaviour, a realm that does not loses nothing, and no creature is created or deleted.
--
-- The contractors are the two retail General Contractors. The Architect takes the coffer,
-- the sphere and the armillary, which is what retail gives that NPC. The stewards are bound
-- where they exist; where they do not, the commands do the same job.
-- ---------------------------------------------------------------------------

UPDATE `creature_template` SET `ScriptName` = 'npc_housing_contractor'
 WHERE `name` IN ('Jorvan Longmoor', 'Rotha')
   AND (`ScriptName` = '' OR `ScriptName` = 'npc_housing_contractor');

UPDATE `creature_template` SET `ScriptName` = 'npc_housing_coffer'
 WHERE `name` IN ('The Last Architect')
   AND (`ScriptName` = '' OR `ScriptName` = 'npc_housing_coffer');

UPDATE `creature_template` SET `ScriptName` = 'npc_housing_steward'
 WHERE `name` IN ('Lyssabel Dawnpetal', 'Tocho Cloudhide')
   AND (`ScriptName` = '' OR `ScriptName` = 'npc_housing_steward');

-- The dye station: doc 09's two Dye Crafters. A realm without them keeps the menu on the
-- Dyes tab and `.housing dye`, which do the same thing.
UPDATE `creature_template` SET `ScriptName` = 'npc_housing_dyecrafter'
 WHERE `name` IN ('Helmi Cooper', 'Haleth Turnwater')
   AND (`ScriptName` = '' OR `ScriptName` = 'npc_housing_dyecrafter');

-- Every decor vendor in `housing_vendor`, in one statement, so a shop added later is bound
-- by the act of adding it.
UPDATE `creature_template` ct
  JOIN `housing_vendor` hv ON hv.`name` = ct.`name`
   SET ct.`ScriptName` = 'npc_housing_vendor'
 WHERE ct.`ScriptName` = '';

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

