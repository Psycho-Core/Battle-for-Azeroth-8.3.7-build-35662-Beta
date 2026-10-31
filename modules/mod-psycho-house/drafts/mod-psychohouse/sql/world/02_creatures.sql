-- mod-psychohouse - the two NPCs the module owns
--
-- These are the only creature templates the module creates. Every other service NPC it
-- spawns is one that already exists in the realm's world database; nothing retail is
-- copied here and no entry outside this range is written to.
--
-- The column list is exactly the list this build's template loader reads
-- (WORLD_SEL_CREATURE_TEMPLATE), so the rows say everything the server looks at and
-- nothing it ignores. Level 120 is the 8.3.7 cap: these NPCs are inland services, and a
-- 120 tells the client this is current-expansion content rather than a leftover.
--
-- Models live in `creature_template_model` and are filled by 03_bind_facilities.sql from
-- an existing service NPC, because a display id is content this draft has no way to know.

DELETE FROM `creature_template` WHERE `entry` IN (900200, 900201);

INSERT INTO `creature_template`
  (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `name`, `femaleName`, `subname`, `TitleAlt`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `HealthScalingExpansion`, `RequiredExpansion`, `VignetteID`, `faction`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `dmgschool`, `BaseAttackTime`, `RangeAttackTime`, `BaseVariance`, `RangeVariance`, `unit_class`, `unit_flags`, `unit_flags2`, `unit_flags3`, `dynamicflags`, `family`, `trainer_class`, `type`, `type_flags`, `type_flags2`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `HoverHeight`, `HealthModifier`, `HealthModifierExtra`, `ManaModifier`, `ManaModifierExtra`, `ArmorModifier`, `DamageModifier`, `ExperienceModifier`, `RacialLeader`, `movementId`, `FadeRegionRadius`, `WidgetSetID`, `WidgetSetUnitConditionID`, `RegenHealth`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`)
VALUES
  (900200, 0, 0, 0, 0, 0, 'The Keeper', '', 'Island House', '', '', 0, 120, 120, 0, 0, 0, 35, 1, 1, 1.14286, 1, 0, 0, 2000, 2000, 1, 1, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 'psychohouse_keeper'),
  (900201, 0, 0, 0, 0, 0, 'The Steward', '', 'House Amenities', '', '', 0, 120, 120, 0, 0, 0, 35, 1, 1, 1.14286, 1, 0, 0, 2000, 2000, 1, 1, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 'psychohouse_steward');

DELETE FROM `creature_template_model` WHERE `CreatureID` IN (900200, 900201);
