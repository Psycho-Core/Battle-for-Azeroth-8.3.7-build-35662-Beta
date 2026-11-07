-- ---------------------------------------------------------------------------
-- mod-psycho-house - lit variants, bound from the realm's own game objects
--
-- The Hearthlight Armillary's dark presets do not recolour anything: they respawn every
-- placed piece, and a piece whose `housing_decor.night_entry` is set comes back as that
-- game object instead. A piece is therefore lit-capable exactly when the realm's own
-- `gameobject_template` has the same object in a lit form.
--
-- Nothing here guesses an entry. Two names are a pair only when one is the other with the
-- word "Lit" in front:
--
--   "Lantern"      <-> "Lit Lantern"
--   "Stone Brazier" <-> "Lit Stone Brazier"
--
-- and both game objects have to be the same type and in the same map-independent family.
-- A catalogue with no pairs leaves the column at 0 and the Armillary's dark presets change
-- only the world state, which is the truth rather than a silent no-op with a fake entry.
--
-- Run after the draft's world files:
--
--   mariadb world < tools/sql/bind_night_variants.sql
-- ---------------------------------------------------------------------------

UPDATE `housing_decor` SET `night_entry` = 0;

-- The lit twin of a piece: same type, name = the piece's name with "Lit " in front.
UPDATE `housing_decor` d
JOIN `gameobject_template` own ON own.`entry` = d.`gameobject_entry`
JOIN `gameobject_template` lit
  ON lit.`name` = CONCAT('Lit ', own.`name`)
 AND lit.`type` = own.`type`
SET d.`night_entry` = lit.`entry`
WHERE d.`gameobject_entry` <> 0;

-- The other direction: the catalogue holds the lit one and the unlit one is the twin. That
-- happens when the harvest picked the brighter object first.
UPDATE `housing_decor` d
JOIN `gameobject_template` own ON own.`entry` = d.`gameobject_entry`
JOIN `gameobject_template` unlit
  ON SUBSTRING(own.`name`, 5) = unlit.`name`
 AND unlit.`type` = own.`type`
SET d.`night_entry` = unlit.`entry`
WHERE d.`gameobject_entry` <> 0
  AND own.`name` LIKE 'Lit %';

-- The report.

SELECT 'pieces with a lit variant' AS `what`, COUNT(*) AS `rows`
  FROM `housing_decor` WHERE `night_entry` <> 0
UNION ALL
SELECT 'catalogue pieces', COUNT(*) FROM `housing_decor`;

-- If the first number is 0 the realm's gameobject_template has no lit twins for the pieces
-- this catalogue uses. Set one by hand - `UPDATE housing_decor SET night_entry = <entry>
-- WHERE id = <decor>` - and that piece relights with the rest.
