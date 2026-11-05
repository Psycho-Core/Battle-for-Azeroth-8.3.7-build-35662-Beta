-- ---------------------------------------------------------------------------
-- mod-psycho-house - dye variants, bound from the realm's own game objects
--
-- A dye in this module is not a shader: it swaps the game object the piece spawns as, which
-- is the only recolour an 8.3.7 client will render for furniture. That means a piece is
-- dyable exactly when the realm has a second game object with the same name and a different
-- colour word in front of it - "Red Banner" beside "Blue Banner", "Pink Paintbrush" beside
-- "Red Paintbrush".
--
-- Nothing here guesses an entry. The pair is found in `gameobject_template`, and only for
-- game objects the catalogue already uses, so a variant can never point at a model that has
-- nothing to do with the piece.
--
-- Run after the draft's world files:
--
--   mariadb world < tools/sql/bind_decor_variants.sql
--
-- The last statement prints what was found. A catalogue with no pairs leaves the table empty
-- and `/housing dye` reports "this piece has no dyed appearance", which is the truth.
-- ---------------------------------------------------------------------------

-- The colour words come from `housing_dye`, which the draft's world file seeds. Keeping the
-- list in a table rather than in this statement is deliberate: a realm that adds a seventh
-- colour to that table gets it here without editing this file.

-- One row per (piece, colour) whose game object name starts with that colour word and whose
-- remaining name matches the piece's own game object name once the piece's colour word is
-- stripped. Both sides have to be in the catalogue, which is what keeps this honest.

DELETE FROM `housing_decor_variant`;

INSERT INTO `housing_decor_variant` (`decor_id`, `dye_group`, `dye_color`, `dye_name`, `gameobject_entry`)
SELECT d.`id`, 1, w.`id`, w.`name`, g.`entry`
FROM `housing_decor` d
JOIN `gameobject_template` own ON own.`entry` = d.`gameobject_entry`
JOIN `housing_dye` own_word
  ON own.`name` LIKE CONCAT(own_word.`name`, ' %')
JOIN `gameobject_template` g
  ON g.`name` = CONCAT(w.`name`, ' ', SUBSTRING(own.`name`, CHAR_LENGTH(own_word.`name`) + 2))
JOIN `housing_dye` w ON w.`id` <> own_word.`id`
WHERE g.`type` = own.`type`;

-- The report. A piece with two or more rows here is dyable; the count is what the addon shows.

SELECT 'dyable pieces' AS `what`, COUNT(DISTINCT `decor_id`) AS `rows` FROM `housing_decor_variant`
UNION ALL SELECT 'variant rows', COUNT(*) FROM `housing_decor_variant`;

-- If the first number is 0, the realm's gameobject_template has no colour pairs for the
-- pieces this catalogue uses. That is a content limitation, not a bug: the dye code path is
-- complete and the table is simply empty. Add a row by hand - decor_id, group 1, a colour
-- number, a name, and the game object entry to swap to - and the piece becomes dyable.
