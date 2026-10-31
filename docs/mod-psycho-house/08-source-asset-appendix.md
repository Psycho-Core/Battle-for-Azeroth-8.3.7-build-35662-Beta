# Appendix - decor candidates that already have ids in the source tree

Extracted from `src/` by `tools/housing/index_source_assets.py`; every id below appears
in the tree as a named constant. Machine-readable: `source-asset-index.csv`.

**767 game-object candidates**, **221 creature candidates** (NPCs and pets).

Read the constant name as the label; the display name comes from the world database at
load time. Verify a row exists in `gameobject_template` before relying on it.

## Accents / Food and Drink (2)

| Id | Constant | Cost | File |
| --- | --- | --- | --- |
| 170607 | `GO_BAR_KEG_SHOT` | 1 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackrockDepths/instance_blackrock_depths.cpp` |
| 171941 | `GO_BAR_KEG_TRAP` | 1 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackrockDepths/instance_blackrock_depths.cpp` |

## Accents / Ornamental (53)

| Id | Constant | Cost | File |
| --- | --- | --- | --- |
| 148830 | `GO_ATALAI_STATUE1` | 2 | `./src/server/scripts/EasternKingdoms/SunkenTemple/instance_sunken_temple.cpp` |
| 148831 | `GO_ATALAI_STATUE2` | 2 | `./src/server/scripts/EasternKingdoms/SunkenTemple/instance_sunken_temple.cpp` |
| 148832 | `GO_ATALAI_STATUE3` | 2 | `./src/server/scripts/EasternKingdoms/SunkenTemple/instance_sunken_temple.cpp` |
| 148833 | `GO_ATALAI_STATUE4` | 2 | `./src/server/scripts/EasternKingdoms/SunkenTemple/instance_sunken_temple.cpp` |
| 148834 | `GO_ATALAI_STATUE5` | 2 | `./src/server/scripts/EasternKingdoms/SunkenTemple/instance_sunken_temple.cpp` |
| 148835 | `GO_ATALAI_STATUE6` | 2 | `./src/server/scripts/EasternKingdoms/SunkenTemple/instance_sunken_temple.cpp` |
| 178205 | `GO_BANNER` | 2 | `./src/server/scripts/Kalimdor/zone_ashenvale.cpp` |
| 184596 | `GO_KAEL_STATUE_RIGHT` | 2 | `./src/server/scripts/Outland/TempestKeep/Eye/the_eye.h` |
| 184597 | `GO_KAEL_STATUE_LEFT` | 2 | `./src/server/scripts/Outland/TempestKeep/Eye/the_eye.h` |
| 188165 | `GO_KAEL_STATUE_1` | 2 | `./src/server/scripts/EasternKingdoms/MagistersTerrace/magisters_terrace.h` |
| 188166 | `GO_KAEL_STATUE_2` | 2 | `./src/server/scripts/EasternKingdoms/MagistersTerrace/magisters_terrace.h` |
| 190475 | `GO_WINTERGRASP_FACTORY_BANNER_NE` | 2 | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 190487 | `GO_WINTERGRASP_FACTORY_BANNER_NW` | 2 | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 192564 | `GO_SLAD_RAN_STATUE` | 2 | `./src/server/scripts/Northrend/Gundrak/gundrak.h` |
| 192565 | `GO_MOORABI_STATUE` | 2 | `./src/server/scripts/Northrend/Gundrak/gundrak.h` |
| ... | *38 more in the CSV* | | |

## Functional / Utility (7)

| Id | Constant | Cost | File |
| --- | --- | --- | --- |
| 151951 | `GO_IDOL_OVEN_FIRE` | 4 | `./src/server/scripts/Kalimdor/RazorfenDowns/razorfen_downs.h` |
| 186609 | `GO_GLOWING_ANVIL_1` | 4 | `./src/server/scripts/Northrend/UtgardeKeep/UtgardeKeep/utgarde_keep.h` |
| 186610 | `GO_GLOWING_ANVIL_2` | 4 | `./src/server/scripts/Northrend/UtgardeKeep/UtgardeKeep/utgarde_keep.h` |
| 186611 | `GO_GLOWING_ANVIL_3` | 4 | `./src/server/scripts/Northrend/UtgardeKeep/UtgardeKeep/utgarde_keep.h` |
| 186691 | `GO_FORGEFIRE_3` | 4 | `./src/server/scripts/Northrend/UtgardeKeep/UtgardeKeep/utgarde_keep.h` |
| 186692 | `GO_FORGEFIRE_1` | 4 | `./src/server/scripts/Northrend/UtgardeKeep/UtgardeKeep/utgarde_keep.h` |
| 186693 | `GO_FORGEFIRE_2` | 4 | `./src/server/scripts/Northrend/UtgardeKeep/UtgardeKeep/utgarde_keep.h` |

## Furnishings / Seating (22)

| Id | Constant | Cost | File |
| --- | --- | --- | --- |
| 160410 | `GO_CHAIR_6` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 160415 | `GO_CHAIR_3` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 160416 | `GO_CHAIR_8` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 160418 | `GO_CHAIR_7` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 160419 | `GO_CHAIR_9` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 170575 | `GO_THRONE_ROOM` | 2 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackrockDepths/instance_blackrock_depths.cpp` |
| 181640 | `GO_KELTHUZAD_THRONE` | 2 | `./src/server/scripts/Northrend/Naxxramas/naxxramas.h` |
| 186896 | `GO_BENCH_1` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 186897 | `GO_BENCH_5` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 186899 | `GO_BENCH_3` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 186904 | `GO_BENCH_4` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 186922 | `GO_BENCH_2` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 192654 | `GO_LOKEN_THRONE` | 2 | `./src/server/scripts/Northrend/Ulduar/HallsOfLightning/halls_of_lightning.h` |
| 195410 | `GO_CHAIR_1` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195414 | `GO_CHAIR_2` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| ... | *7 more in the CSV* | | |

## Furnishings / Storage (157)

| Id | Constant | Cost | File |
| --- | --- | --- | --- |
| 144111 | `GO_MR_SMITE_CHEST` | 3 | `./src/server/scripts/EasternKingdoms/Deadmines/deadmines.h` |
| 169243 | `GO_CHEST_SEVEN` | 3 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackrockDepths/instance_blackrock_depths.cpp` |
| 181366 | `GO_HORSEMEN_CHEST` | 3 | `./src/server/scripts/Northrend/Naxxramas/naxxramas.h` |
| 185119 | `GO_DUST_COVERED_CHEST` | 3 | `./src/server/scripts/EasternKingdoms/Karazhan/karazhan.h` |
| 185168 | `GO_FEL_IRON_CHEST_NORMAL` | 3 | `./src/server/scripts/Outland/HellfireCitadel/HellfireRamparts/hellfire_ramparts.h` |
| 185169 | `GO_FEL_IRON_CHEST_HEROIC` | 3 | `./src/server/scripts/Outland/HellfireCitadel/HellfireRamparts/hellfire_ramparts.h` |
| 190094 | `GO_SUSPICIOUS_CRATE` | 3 | `./src/server/scripts/Kalimdor/CavernsOfTime/CullingOfStratholme/culling_of_stratholme.h` |
| 190095 | `GO_PLAGUED_CRATE` | 3 | `./src/server/scripts/Kalimdor/CavernsOfTime/CullingOfStratholme/culling_of_stratholme.h` |
| 190586 | `GO_TRIBUNAL_CHEST` | 3 | `./src/server/scripts/Northrend/Ulduar/HallsOfStone/halls_of_stone.h` |
| 190663 | `GO_MALGANIS_CHEST_N` | 3 | `./src/server/scripts/Kalimdor/CavernsOfTime/CullingOfStratholme/culling_of_stratholme.h` |
| 193426 | `GO_HORSEMEN_CHEST_HERO` | 3 | `./src/server/scripts/Northrend/Naxxramas/naxxramas.h` |
| 193597 | `GO_MALGANIS_CHEST_H` | 3 | `./src/server/scripts/Kalimdor/CavernsOfTime/CullingOfStratholme/culling_of_stratholme.h` |
| 193996 | `GO_TRIBUNAL_CHEST_HERO` | 3 | `./src/server/scripts/Northrend/Ulduar/HallsOfStone/halls_of_stone.h` |
| 194307 | `GO_HODIR_CHEST` | 3 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194308 | `GO_HODIR_CHEST_HERO` | 3 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| ... | *142 more in the CSV* | | |

## Furnishings / Tables and Desks (4)

| Id | Constant | Cost | File |
| --- | --- | --- | --- |
| 330208 | `GO_ORGOZOA_ENCOUNTER_FINAL` | 3 | `./src/server/scripts/Nazjatar/EternalPalace/eternal_palace.h` |
| 332139 | `GO_LADY_ASHVANE_ENCOUNTER` | 3 | `./src/server/scripts/Nazjatar/EternalPalace/eternal_palace.h` |
| 332144 | `GO_ORGOZOA_ENCOUNTER_BEH` | 3 | `./src/server/scripts/Nazjatar/EternalPalace/eternal_palace.h` |
| 334101 | `GO_SIVARA_ENCOUNTER` | 3 | `./src/server/scripts/Nazjatar/EternalPalace/eternal_palace.h` |

## Lighting / Small Lights (15)

| Id | Constant | Cost | File |
| --- | --- | --- | --- |
| 152097 | `GO_BELNISTRASZS_BRAZIER` | 2 | `./src/server/scripts/Kalimdor/RazorfenDowns/razorfen_downs.h` |
| 175564 | `GO_BRAZIER_OF_THE_HERALD` | 2 | `./src/server/scripts/EasternKingdoms/Scholomance/scholomance.h` |
| 176093 | `GO_BEACON_TORCH` | 2 | `./src/server/scripts/EasternKingdoms/zone_western_plaguelands.cpp` |
| 178247 | `GO_NAGA_BRAZIER` | 2 | `./src/server/scripts/Kalimdor/zone_ashenvale.cpp` |
| 195207 | `GO_BONFIRE_4` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195208 | `GO_BONFIRE_2` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195209 | `GO_BONFIRE_5` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195210 | `GO_BONFIRE_3` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195376 | `GO_BONFIRE_1` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195377 | `GO_BONFIRE_6` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195402 | `GO_BRAZIER_1` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195403 | `GO_BRAZIER_2` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195424 | `GO_BRAZIER_4` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195425 | `GO_BRAZIER_3` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 207408 | `GO_MAGIC_BRAZIER` | 2 | `./src/server/scripts/Kalimdor/TheVortexPinnacle/the_vortex_pinnacle.h` |

## Nature / Large Foliage (6)

| Id | Constant | Cost | File |
| --- | --- | --- | --- |
| 185584 | `GO_NAJENTUS_SPINE` | 5 | `./src/server/scripts/Outland/BlackTemple/black_temple.h` |
| 191144 | `GO_INCONSPICUOUS_TREE` | 5 | `./src/server/scripts/EasternKingdoms/ScarletEnclave/chapter2.cpp` |
| 203034 | `GO_BURNING_TREE_1` | 5 | `./src/server/scripts/Northrend/ChamberOfAspects/RubySanctum/ruby_sanctum.h` |
| 203035 | `GO_BURNING_TREE_2` | 5 | `./src/server/scripts/Northrend/ChamberOfAspects/RubySanctum/ruby_sanctum.h` |
| 203036 | `GO_BURNING_TREE_3` | 5 | `./src/server/scripts/Northrend/ChamberOfAspects/RubySanctum/ruby_sanctum.h` |
| 203037 | `GO_BURNING_TREE_4` | 5 | `./src/server/scripts/Northrend/ChamberOfAspects/RubySanctum/ruby_sanctum.h` |

## Nature / Small Foliage (1)

| Id | Constant | Cost | File |
| --- | --- | --- | --- |
| 266938 | `GO_GOROTH_INFERNAL_SPIKE` | 2 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |

## Structural / Doors (337)

| Id | Constant | Cost | File |
| --- | --- | --- | --- |
| 123 | `GO_DOOR_2` | 4 | `./src/server/scripts/BrokenIsles/Scenario/scenario_artifact_karazhan_catacombs.cpp` |
| 13965 | `GO_FACTORY_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/Deadmines/deadmines.h` |
| 16397 | `GO_IRONCLAD_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/Deadmines/deadmines.h` |
| 16399 | `GO_FOUNDRY_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/Deadmines/deadmines.h` |
| 16400 | `GO_HEAVY_DOOR_HELIX` | 4 | `./src/server/scripts/EasternKingdoms/Deadmines/deadmines.h` |
| 17154 | `GO_HEAVY_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/Deadmines/deadmines.h` |
| 18895 | `GO_BARON_ASHBURY_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/ShadowfangKeep/shadowfang_keep.h` |
| 18971 | `GO_LORD_GODFREY_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/ShadowfangKeep/shadowfang_keep.h` |
| 18972 | `GO_LORD_WALDEN_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/ShadowfangKeep/shadowfang_keep.h` |
| 21117 | `GO_AKU_MAI_DOOR` | 4 | `./src/server/scripts/Kalimdor/BlackfathomDeeps/blackfathom_deeps.h` |
| 25064 | `GO_DOOR_1` | 4 | `./src/server/scripts/BrokenIsles/Scenario/scenario_artifact_ruins_of_falanaar.cpp` |
| 104600 | `GO_HIGH_INQUISITORS_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/ScarletMonastery/scarlet_monastery.h` |
| 124367 | `GO_ALTAR_OF_THE_KEEPER_TEMPLE_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/Uldaman/uldaman.h` |
| 124369 | `GO_ANCIENT_VAULT_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/Uldaman/uldaman.h` |
| 124372 | `GO_IRONAYA_SEAL_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/Uldaman/uldaman.h` |
| ... | *322 more in the CSV* | | |

## Structural / Large Structures (111)

| Id | Constant | Cost | File |
| --- | --- | --- | --- |
| 175357 | `GO_GAUNTLET_GATE1` | 5 | `./src/server/scripts/EasternKingdoms/Stratholme/stratholme.h` |
| 175570 | `GO_GATE_KIRTONOS` | 5 | `./src/server/scripts/EasternKingdoms/Scholomance/scholomance.h` |
| 176146 | `GO_GATE_OF_AHN_QIRAJ` | 5 | `./src/server/scripts/Kalimdor/zone_silithus.cpp` |
| 177371 | `GO_GATE_ILLUCIA` | 5 | `./src/server/scripts/EasternKingdoms/Scholomance/scholomance.h` |
| 177372 | `GO_GATE_RAVENIAN` | 5 | `./src/server/scripts/EasternKingdoms/Scholomance/scholomance.h` |
| 177373 | `GO_GATE_BAROV` | 5 | `./src/server/scripts/EasternKingdoms/Scholomance/scholomance.h` |
| 177374 | `GO_GATE_GANDLING` | 5 | `./src/server/scripts/EasternKingdoms/Scholomance/scholomance.h` |
| 177375 | `GO_GATE_MALICIA` | 5 | `./src/server/scripts/EasternKingdoms/Scholomance/scholomance.h` |
| 177376 | `GO_GATE_POLKELT` | 5 | `./src/server/scripts/EasternKingdoms/Scholomance/scholomance.h` |
| 177377 | `GO_GATE_THEOLEN` | 5 | `./src/server/scripts/EasternKingdoms/Scholomance/scholomance.h` |
| 180424 | `GO_ALTERAC_VALLEY_GATE` | 5 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackwingLair/blackwing_lair.h` |
| 181125 | `GO_VACCUUM_EXIT_GATE` | 5 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackwingLair/blackwing_lair.h` |
| 181170 | `GO_GOTHIK_GATE` | 5 | `./src/server/scripts/Northrend/Naxxramas/naxxramas.h` |
| 185483 | `GO_GATE` | 5 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackwingLair/blackwing_lair.h` |
| 185483 | `GO_NAJENTUS_GATE` | 5 | `./src/server/scripts/Outland/BlackTemple/black_temple.h` |
| ... | *96 more in the CSV* | | |

## Structural / Walls and Columns (51)

| Id | Constant | Cost | File |
| --- | --- | --- | --- |
| 184634 | `GO_MAGTHERIDON_COLUMN_3` | 4 | `./src/server/scripts/Outland/HellfireCitadel/MagtheridonsLair/magtheridons_lair.h` |
| 184635 | `GO_MAGTHERIDON_COLUMN_2` | 4 | `./src/server/scripts/Outland/HellfireCitadel/MagtheridonsLair/magtheridons_lair.h` |
| 184636 | `GO_MAGTHERIDON_COLUMN_4` | 4 | `./src/server/scripts/Outland/HellfireCitadel/MagtheridonsLair/magtheridons_lair.h` |
| 184637 | `GO_MAGTHERIDON_COLUMN_5` | 4 | `./src/server/scripts/Outland/HellfireCitadel/MagtheridonsLair/magtheridons_lair.h` |
| 184638 | `GO_MAGTHERIDON_COLUMN_0` | 4 | `./src/server/scripts/Outland/HellfireCitadel/MagtheridonsLair/magtheridons_lair.h` |
| 184639 | `GO_MAGTHERIDON_COLUMN_1` | 4 | `./src/server/scripts/Outland/HellfireCitadel/MagtheridonsLair/magtheridons_lair.h` |
| 194323 | `GO_WINTERGRASP_KEEP_COLLISION_WALL` | 4 | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 201385 | `GO_ICE_WALL` | 4 | `./src/server/scripts/Northrend/FrozenHalls/HallsOfReflection/halls_of_reflection.h` |
| 201885 | `GO_ICE_WALL` | 4 | `./src/server/scripts/Northrend/FrozenHalls/PitOfSaron/pit_of_saron.h` |
| 201910 | `GO_DOODAD_ICECROWN_ICEWALL02` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201911 | `GO_ICEWALL` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 202396 | `GO_ICE_WALL` | 4 | `./src/server/scripts/Kalimdor/CavernsOfTime/HourofTwilight/hour_of_twilight.h` |
| 202396 | `GO_ICE_WALL` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 202437 | `GO_LAVAMAN_PILLARS_CHAINED` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 202438 | `GO_LAVAMAN_PILLARS_UNCHAINED` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| ... | *36 more in the CSV* | | |

## Structural / Windows (1)

| Id | Constant | Cost | File |
| --- | --- | --- | --- |
| 184069 | `GO_TEMPEST_BRIDDGE_WINDOW` | 4 | `./src/server/scripts/Outland/TempestKeep/Eye/the_eye.h` |

## Creatures usable as neighbourhood NPCs, pets or ambient life

| Id | Constant | Suggested role | File |
| --- | --- | --- | --- |
| 89 | `NPC_INFERNAL` | ambient life | `./src/server/scripts/Spells/spell_generic.cpp` |
| 1981 | `NPC_DARK_IRON_AMBUSHER` | ambient life | `./src/server/scripts/EasternKingdoms/zone_loch_modan.cpp` |
| 4823 | `NPC_BARBED_CRUSTACEAN` | prop creature / ambient | `./src/server/scripts/Kalimdor/BlackfathomDeeps/blackfathom_deeps.h` |
| 6207 | `NPC_CAVERNDEEP_AMBUSHER` | ambient life | `./src/server/scripts/EasternKingdoms/Gnomeregan/gnomeregan.h` |
| 7809 | `NPC_VILE_AMBUSHER` | ambient life | `./src/server/scripts/EasternKingdoms/zone_hinterlands.cpp` |
| 12717 | `NPC_MUGLASH` | prop creature / ambient | `./src/server/scripts/Kalimdor/zone_ashenvale.cpp` |
| 13301 | `NPC_HIVE_AMBUSHER` | ambient life | `./src/server/scripts/World/go_scripts.cpp` |
| 14776 | `NPC_TOWER_POINT_WARMASTER` | large prop creature | `./src/server/scripts/EasternKingdoms/AlteracValley/alterac_valley.cpp` |
| 15334 | `NPC_GIANT_EYE_TENTACLE` | large prop creature | `./src/server/scripts/Kalimdor/TempleOfAhnQiraj/temple_of_ahnqiraj.h` |
| 15725 | `NPC_CLAW_TENTACLE` | large prop creature | `./src/server/scripts/Kalimdor/TempleOfAhnQiraj/temple_of_ahnqiraj.h` |
| 15726 | `NPC_EYE_TENTACLE` | large prop creature | `./src/server/scripts/Kalimdor/TempleOfAhnQiraj/temple_of_ahnqiraj.h` |
| 15728 | `NPC_GIANT_CLAW_TENTACLE` | large prop creature | `./src/server/scripts/Kalimdor/TempleOfAhnQiraj/temple_of_ahnqiraj.h` |
| 15802 | `NPC_FLESH_TENTACLE` | large prop creature | `./src/server/scripts/Kalimdor/TempleOfAhnQiraj/temple_of_ahnqiraj.h` |
| 16343 | `NPC_SHADOWPINE_ORACLE` | ambient life | `./src/server/scripts/EasternKingdoms/zone_ghostlands.cpp` |
| 17375 | `NPC_STILLPINE_CAPITIVE` | ambient life | `./src/server/scripts/Kalimdor/zone_azuremyst_isle.cpp` |
| 20886 | `NPC_SOCCOTHRATES` | prop creature / ambient | `./src/server/scripts/Outland/TempestKeep/arcatraz/arcatraz.h` |
| 21419 | `NPC_INFERNAL_ATTACKER` | ambient life | `./src/server/scripts/Outland/zone_shadowmoon_valley.cpp` |
| 21865 | `NPC_COILFANG_AMBUSHER` | ambient life | `./src/server/scripts/Outland/CoilfangReservoir/SerpentShrine/boss_lurker_below.cpp` |
| 23700 | `NPC_FESTIVE_KEG_2` | prop creature / ambient | `./src/server/scripts/Events/brewfest.cpp` |
| 23702 | `NPC_FESTIVE_KEG_1` | prop creature / ambient | `./src/server/scripts/Events/brewfest.cpp` |
| 23706 | `NPC_FESTIVE_KEG_3` | prop creature / ambient | `./src/server/scripts/Events/brewfest.cpp` |
| 24372 | `NPC_FESTIVE_KEG_5` | prop creature / ambient | `./src/server/scripts/Events/brewfest.cpp` |
| 24373 | `NPC_FESTIVE_KEG_4` | prop creature / ambient | `./src/server/scripts/Events/brewfest.cpp` |
| 24710 | `NPC_IPFELKOFER_IRONKEG` | prop creature / ambient | `./src/server/scripts/World/areatrigger_scripts.cpp` |
| 24711 | `NPC_TAPPER_SWINDLEKEG` | prop creature / ambient | `./src/server/scripts/World/areatrigger_scripts.cpp` |
| 25060 | `NPC_DARKSPINE_MYRIDON` | ambient life | `./src/server/scripts/EasternKingdoms/zone_isle_of_queldanas.cpp` |
| 26529 | `NPC_MEATHOOK` | prop creature / ambient | `./src/server/scripts/Kalimdor/CavernsOfTime/CullingOfStratholme/culling_of_stratholme.h` |
| 27273 | `NPC_FLAME_BRAZIER` | ambient light source | `./src/server/scripts/Northrend/UtgardeKeep/UtgardePinnacle/boss_svala.cpp` |
| 27979 | `NPC_FORGED_IRON_TROGG` | service NPC | `./src/server/scripts/Northrend/Ulduar/HallsOfStone/boss_sjonnir.cpp` |
| 27982 | `NPC_FORGED_IRON_DWARF` | service NPC | `./src/server/scripts/Northrend/Ulduar/HallsOfStone/boss_sjonnir.cpp` |
| 28166 | `NPC_UNSTABLE_SPHERE` | prop creature / ambient | `./src/server/scripts/Northrend/Nexus/Oculus/boss_drakos.cpp` |
| 28366 | `NPC_WINTERGRASP_TOWER_CANNON` | large prop creature | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 28525 | `NPC_NEW_AVALON_FORGE` | service NPC | `./src/server/scripts/Spells/spell_quest.cpp` |
| 28823 | `NPC_VOLKHAN_ANVIL` | service NPC | `./src/server/scripts/Northrend/Ulduar/HallsOfLightning/boss_volkhan.cpp` |
| 29178 | `NPC_LORD_MAXWELL_TYROSUS` | large prop creature | `./src/server/scripts/EasternKingdoms/ScarletEnclave/chapter5.cpp` |
| 29240 | `NPC_STORMFORGED_LIEUTENANT` | service NPC | `./src/server/scripts/Northrend/Ulduar/HallsOfLightning/boss_bjarngrim.cpp` |
| 29254 | `NPC_SILVER_COVENANT_GUARDIAN_MAGE` | service NPC | `./src/server/scripts/Northrend/zone_dalaran.cpp` |
| 29861 | `NPC_STORMFORGED_ERADICTOR` | service NPC | `./src/server/scripts/World/areatrigger_scripts.cpp` |
| 29862 | `NPC_STORMFORGED_MONITOR` | service NPC | `./src/server/scripts/World/areatrigger_scripts.cpp` |
| 29920 | `NPC_RUIN_DWELLER` | large prop creature | `./src/server/scripts/Northrend/Gundrak/gundrak.h` |
| 30391 | `NPC_HEALTHY_MUSHROOM` | ambient life | `./src/server/scripts/Northrend/AzjolNerub/Ahnkahet/ahnkahet.h` |
| 30435 | `NPC_POISONOUS_MUSHROOM` | ambient life | `./src/server/scripts/Northrend/AzjolNerub/Ahnkahet/ahnkahet.h` |
| 30891 | `NPC_BLESSED_BANNER` | prop creature / ambient | `./src/server/scripts/Northrend/zone_icecrown.cpp` |
| 32823 | `NPC_BOUNTIFUL_TABLE` | prop creature / ambient | `./src/server/scripts/Events/pilgrims_bounty.cpp` |
| 33050 | `NPC_UNSTABLE_SUN_BEAM` | prop creature / ambient | `./src/server/scripts/Northrend/Ulduar/Ulduar/boss_freya.cpp` |
| 33114 | `NPC_FLAME_LEVIATHAN_SEAT` | prop creature / ambient | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 33114 | `NPC_SEAT` | prop creature / ambient | `./src/server/scripts/Northrend/Ulduar/Ulduar/boss_flame_leviathan.cpp` |
| 33236 | `NPC_STEELFORGED_DEFFENDER` | service NPC | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 33316 | `NPC_IRONFORGE_RAM` | service NPC | `./src/server/scripts/Spells/spell_generic.cpp` |
| 33487 | `NPC_IRONBAND_TABLET` | prop creature / ambient | `./src/server/scripts/EasternKingdoms/zone_loch_modan.cpp` |
| 33943 | `NPC_INFLUENCE_TENTACLE` | large prop creature | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 33962 | `NPC_TURNED_CHAMPION` | prop creature / ambient | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 33966 | `NPC_CRUSHER_TENTACLE` | large prop creature | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 33983 | `NPC_CONSTRICTOR_TENTACLE` | large prop creature | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 33985 | `NPC_CORRUPTOR_TENTACLE` | large prop creature | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 33991 | `NPC_SANITY_WELL` | large prop creature | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 34812 | `NPC_THE_TURKEY_CHAIR` | prop creature / ambient | `./src/server/scripts/Events/pilgrims_bounty.cpp` |
| 34813 | `NPC_INFERNAL_VOLCANO` | ambient life | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/boss_lord_jaraxxus.cpp` |
| 34815 | `NPC_FEL_INFERNAL` | ambient life | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/boss_lord_jaraxxus.cpp` |
| 34819 | `NPC_THE_STUFFING_CHAIR` | prop creature / ambient | `./src/server/scripts/Events/pilgrims_bounty.cpp` |
