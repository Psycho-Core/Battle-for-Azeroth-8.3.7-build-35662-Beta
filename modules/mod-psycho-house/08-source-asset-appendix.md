# 08 - Source asset appendix

Every decor candidate that already has an id in my source tree. Extracted from `src/` by
`tools/housing/index_source_assets.py`; every id below appears
in the tree as a named constant. Machine-readable: `generated/source-asset-index.csv`.
Regenerate this file with `tools/housing/build_appendix.py`.

**767 game-object rows (748 distinct entries)**, **221 creature rows
(216 distinct entries)**. Every one of them is listed below, not a sample.

Read the constant name as the label; the display name comes from the world database at
load time. Verify a row exists in `gameobject_template` before relying on it. The `Cost`
column is the placement budget band the harvest assigns from the constant's family.


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
| 192566 | `GO_GAL_DARAH_STATUE` | 2 | `./src/server/scripts/Northrend/Gundrak/gundrak.h` |
| 192567 | `GO_DRAKKARI_COLOSSUS_STATUE` | 2 | `./src/server/scripts/Northrend/Gundrak/gundrak.h` |
| 194959 | `GO_WINTERGRASP_FACTORY_BANNER_SE` | 2 | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 194962 | `GO_WINTERGRASP_FACTORY_BANNER_SW` | 2 | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 195130 | `GO_HORDE_BANNER_HANGAR` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195132 | `GO_ALLIANCE_BANNER_HANGAR` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195133 | `GO_WORKSHOP_BANNER` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195144 | `GO_ALLIANCE_BANNER_HANGAR_CONT` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195145 | `GO_HORDE_BANNER_HANGAR_CONT` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195149 | `GO_ALLIANCE_BANNER_WORKSHOP` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195150 | `GO_ALLIANCE_BANNER_WORKSHOP_CONT` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195151 | `GO_HORDE_BANNER_WORKSHOP` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195152 | `GO_HORDE_BANNER_WORKSHOP_CONT` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195158 | `GO_HANGAR_BANNER` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195334 | `GO_ALLIANCE_BANNER_QUARRY` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195335 | `GO_ALLIANCE_BANNER_QUARRY_CONT` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195336 | `GO_HORDE_BANNER_QUARRY` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195337 | `GO_HORDE_BANNER_QUARRY_CONT` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195338 | `GO_QUARRY_BANNER` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195339 | `GO_ALLIANCE_BANNER_REFINERY` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195340 | `GO_ALLIANCE_BANNER_REFINERY_CONT` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195341 | `GO_HORDE_BANNER_REFINERY` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195342 | `GO_HORDE_BANNER_REFINERY_CONT` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195343 | `GO_REFINERY_BANNER` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195391 | `GO_ALLIANCE_BANNER_GRAVEYARD_H` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195392 | `GO_ALLIANCE_BANNER_GRAVEYARD_H_CONT` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195393 | `GO_HORDE_BANNER` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195393 | `GO_HORDE_BANNER_GRAVEYARD_H` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195394 | `GO_HORDE_BANNER_GRAVEYARD_H_CONT` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195396 | `GO_ALLIANCE_BANNER` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195396 | `GO_ALLIANCE_BANNER_GRAVEYARD_A` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195397 | `GO_ALLIANCE_BANNER_GRAVEYARD_A_CONT` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195398 | `GO_HORDE_BANNER_GRAVEYARD_A` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195399 | `GO_HORDE_BANNER_GRAVEYARD_A_CONT` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 202302 | `GO_FROSTMOURNE` | 2 | `./src/server/scripts/Northrend/FrozenHalls/HallsOfReflection/halls_of_reflection.h` |
| 206890 | `GO_DUSTY_PRISON_JOURNAL` | 2 | `./src/server/game/Battlefield/Zones/BattlefieldTB.h` |
| 207391 | `GO_BARADINS_WARDEN_BANNER` | 2 | `./src/server/game/Battlefield/Zones/BattlefieldTB.h` |
| 207400 | `GO_HELLSCREAM_REACH_BANNER` | 2 | `./src/server/game/Battlefield/Zones/BattlefieldTB.h` |

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
| 195416 | `GO_CHAIR_5` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195418 | `GO_CHAIR_4` | 2 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 202188 | `GO_DOODAD_ICECROWN_THRONEFROSTYWIND01` | 2 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 202189 | `GO_DOODAD_ICECROWN_THRONEFROSTYEDGE01` | 2 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 269779 | `GO_ELUNES_THRONE` | 2 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 269780 | `GO_HAMMER_THRONE` | 2 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 269782 | `GO_GOLGANETH_THRONE` | 2 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |

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
| 195046 | `GO_KOLOGARN_CHEST` | 3 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 195047 | `GO_KOLOGARN_CHEST_HERO` | 3 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 195665 | `GO_TRIBUTE_CHEST_10H_99` | 3 | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/trial_of_the_crusader.h` |
| 195666 | `GO_TRIBUTE_CHEST_10H_50` | 3 | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/trial_of_the_crusader.h` |
| 195667 | `GO_TRIBUTE_CHEST_10H_45` | 3 | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/trial_of_the_crusader.h` |
| 195668 | `GO_TRIBUTE_CHEST_10H_25` | 3 | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/trial_of_the_crusader.h` |
| 195669 | `GO_TRIBUTE_CHEST_25H_99` | 3 | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/trial_of_the_crusader.h` |
| 195670 | `GO_TRIBUTE_CHEST_25H_50` | 3 | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/trial_of_the_crusader.h` |
| 195671 | `GO_TRIBUTE_CHEST_25H_45` | 3 | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/trial_of_the_crusader.h` |
| 195672 | `GO_TRIBUTE_CHEST_25H_25` | 3 | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/trial_of_the_crusader.h` |
| 201710 | `GO_THE_CAPTAIN_CHEST_ALLIANCE_NORMAL` | 3 | `./src/server/scripts/Northrend/FrozenHalls/HallsOfReflection/halls_of_reflection.h` |
| 202212 | `GO_THE_CAPTAIN_CHEST_HORDE_NORMAL` | 3 | `./src/server/scripts/Northrend/FrozenHalls/HallsOfReflection/halls_of_reflection.h` |
| 202336 | `GO_THE_CAPTAIN_CHEST_ALLIANCE_HEROIC` | 3 | `./src/server/scripts/Northrend/FrozenHalls/HallsOfReflection/halls_of_reflection.h` |
| 202337 | `GO_THE_CAPTAIN_CHEST_HORDE_HEROIC` | 3 | `./src/server/scripts/Northrend/FrozenHalls/HallsOfReflection/halls_of_reflection.h` |
| 205216 | `GO_OZUMAT_CHEST_NORMAL` | 3 | `./src/server/scripts/EasternKingdoms/ThroneOfTheTides/throne_of_the_tides.h` |
| 206996 | `GO_CRATE_OF_CELLBLOCK_RATIONS` | 3 | `./src/server/game/Battlefield/Zones/BattlefieldTB.h` |
| 207973 | `GO_OZUMAT_CHEST_HEROIC` | 3 | `./src/server/scripts/EasternKingdoms/ThroneOfTheTides/throne_of_the_tides.h` |
| 214520 | `GO_TRIAL_CHEST` | 3 | `./src/server/scripts/Pandaria/MogushanPalace/mogu_shan_palace.h` |
| 221802 | `GO_STOUT_CRATE_OF_MANTID_1` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221803 | `GO_MASSIVE_CRATE_OF_MOGU_1` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221804 | `GO_MASSIVE_CRATE_OF_MANTID_1` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221805 | `GO_STOUT_CRATE_OF_MOGU_1` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221806 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_1` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221807 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_1` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221808 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_2` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221809 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_3` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221810 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_4` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221811 | `GO_STOUT_CRATE_OF_MANTID_2` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221812 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_5` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221813 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_6` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221814 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_7` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221815 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_8` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221816 | `GO_STOUT_CRATE_OF_MANTID_3` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221817 | `GO_STOUT_CRATE_OF_MANTID_4` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221818 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_9` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221819 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_10` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221820 | `GO_STOUT_CRATE_OF_MANTID_5` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221821 | `GO_STOUT_CRATE_OF_MANTID_6` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221822 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_11` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221823 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_12` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221824 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_13` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221825 | `GO_MASSIVE_CRATE_OF_MANTID_2` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221826 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_14` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221827 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_15` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221828 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_16` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221829 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_17` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221830 | `GO_STOUT_CRATE_OF_MOGU_2` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221831 | `GO_MASSIVE_CRATE_OF_MOGU_2` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221832 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_2` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221833 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_3` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221834 | `GO_STOUT_CRATE_OF_MOGU_3` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221835 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_4` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221836 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_5` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221837 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_6` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221838 | `GO_STOUT_CRATE_OF_MOGU_4` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221839 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_7` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221840 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_8` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221841 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_9` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221842 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_10` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221843 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_11` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221844 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_12` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221845 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_13` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221846 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_14` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221847 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_15` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221848 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_16` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221849 | `GO_STOUT_CRATE_OF_MOGU_5` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221850 | `GO_STOUT_CRATE_OF_MOGU_6` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221851 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_17` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221852 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_18` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221853 | `GO_STOUT_CRATE_OF_MANTID_7` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221854 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_18` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221855 | `GO_MASSIVE_CRATE_OF_MANTID_3` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221856 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_19` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221857 | `GO_STOUT_CRATE_OF_MANTID_8` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221858 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_20` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221859 | `GO_MASSIVE_CRATE_OF_MANTID_4` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221860 | `GO_STOUT_CRATE_OF_MANTID_9` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221861 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_21` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221862 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_22` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221863 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_23` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221864 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_24` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221865 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_25` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221866 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_26` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221867 | `GO_STOUT_CRATE_OF_MANTID_10` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221868 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_27` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221869 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_28` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221870 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_29` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221871 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_30` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221872 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_31` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221873 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_32` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221874 | `GO_STOUT_CRATE_OF_MANTID_11` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221875 | `GO_STOUT_CRATE_OF_MANTID_12` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221876 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_33` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221877 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_34` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221878 | `GO_CRATE_OF_PANDAREN_RELICS_1` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221879 | `GO_CRATE_OF_PANDAREN_RELICS_2` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221880 | `GO_CRATE_OF_PANDAREN_RELICS_3` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221881 | `GO_CRATE_OF_PANDAREN_RELICS_4` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221882 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_19` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221883 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_20` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221884 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_21` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221885 | `GO_MASSIVE_CRATE_OF_MOGU_3` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221886 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_22` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221887 | `GO_STOUT_CRATE_OF_MOGU_7` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221888 | `GO_STOUT_CRATE_OF_MOGU_8` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221889 | `GO_STOUT_CRATE_OF_MOGU_9` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221890 | `GO_STOUT_CRATE_OF_MOGU_10` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221891 | `GO_MASSIVE_CRATE_OF_MOGU_4` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221892 | `GO_STOUT_CRATE_OF_MOGU_11` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221893 | `GO_STOUT_CRATE_OF_MOGU_12` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221894 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_23` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221896 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_24` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221898 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_25` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221900 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_26` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221901 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_27` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221902 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_28` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221903 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_29` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221904 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_30` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221905 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_31` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221906 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_32` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221907 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_33` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221908 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_34` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221909 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_35` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221910 | `GO_LIGHTWEIGHT_CRATE_OF_MOGU_36` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 222691 | `GO_CRATE_OF_PANDAREN_RELICS_5` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 222692 | `GO_CRATE_OF_PANDAREN_RELICS_6` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 222693 | `GO_CRATE_OF_PANDAREN_RELICS_7` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 222694 | `GO_CRATE_OF_PANDAREN_RELICS_8` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223224 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_35` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223245 | `GO_LIGHTWEIGHT_CRATE_OF_MANTID_36` | 3 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 254168 | `GO_CENARIUS_CHEST` | 3 | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 268372 | `GO_THRASHBITE_BOOKCASE_3` | 3 | `./src/server/scripts/BrokenIsles/CathedralEternalNight/cathedral_of_eternal_night.h` |
| 268373 | `GO_THRASHBITE_BOOKCASE_4` | 3 | `./src/server/scripts/BrokenIsles/CathedralEternalNight/cathedral_of_eternal_night.h` |
| 268374 | `GO_THRASHBITE_BOOKCASE_1` | 3 | `./src/server/scripts/BrokenIsles/CathedralEternalNight/cathedral_of_eternal_night.h` |
| 268375 | `GO_THRASHBITE_BOOKCASE_2` | 3 | `./src/server/scripts/BrokenIsles/CathedralEternalNight/cathedral_of_eternal_night.h` |
| 271099 | `GO_KILJAEDEN_CHEST` | 3 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 276503 | `GO_BOSS4_BOX` | 3 | `./src/server/scripts/BrokenIsles/AntorusTheBurningThrone/antorus.h` |
| 276503 | `GO_BOX` | 3 | `./src/server/scripts/BrokenIsles/AntorusTheBurningThrone/boss_eonar.cpp` |
| 311563 | `GO_PROUDMOORE_STRONGBOX` | 3 | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 316707 | `GO_EXPEDITION_STRONGBOX_GRONG` | 3 | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 322413 | `GO_GLIMMERING_CHEST` | 3 | `./src/server/scripts/Nazjatar/zone_nazjatar.cpp` |
| 327682 | `GO_AZSHARA_CHEST` | 3 | `./src/server/scripts/Nazjatar/EternalPalace/eternal_palace.h` |

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
| 141869 | `GO_ARCHAEDAS_TEMPLE_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/Uldaman/uldaman.h` |
| 146084 | `GO_END_DOOR` | 4 | `./src/server/scripts/Kalimdor/ZulFarrak/zulfarrak.h` |
| 157923 | `GO_SHADOW_GIANT_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackrockDepths/instance_blackrock_depths.cpp` |
| 164726 | `GO_GYTH_ENTRY_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackrockSpire/blackrock_spire.h` |
| 164726 | `GO_PORTCULLIS_ACTIVE` | 4 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackrockSpire/blackrock_spire.h` |
| 164726 | `GO_PORTCULLIS_ACTIVE` | 4 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackwingLair/boss_nefarian.cpp` |
| 170571 | `GO_BAR_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackrockDepths/instance_blackrock_depths.cpp` |
| 175185 | `GO_DRAKE_RIDER_PORTCULLIS` | 4 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackwingLair/blackwing_lair.h` |
| 175185 | `GO_DR_PORTCULLIS` | 4 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackrockSpire/blackrock_spire.h` |
| 175185 | `GO_GYTH_COMBAT_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackrockSpire/blackrock_spire.h` |
| 175186 | `GO_GYTH_EXIT_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackrockSpire/blackrock_spire.h` |
| 175186 | `GO_PORTCULLIS_TOBOSSROOMS` | 4 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackrockSpire/blackrock_spire.h` |
| 175186 | `GO_PORTCULLIS_TOBOSSROOMS` | 4 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackwingLair/boss_nefarian.cpp` |
| 175705 | `GO_DOORS` | 4 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackrockSpire/blackrock_spire.h` |
| 175946 | `GO_DRAKKISATH_DOOR_1` | 4 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackrockSpire/blackrock_spire.h` |
| 175947 | `GO_DRAKKISATH_DOOR_2` | 4 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackrockSpire/blackrock_spire.h` |
| 176965 | `GO_PORTCULLIS` | 4 | `./src/server/scripts/EasternKingdoms/BlackrockMountain/BlackwingLair/blackwing_lair.h` |
| 177203 | `GO_IKISS_DOOR` | 4 | `./src/server/scripts/Outland/Auchindoun/SethekkHalls/sethekk_halls.h` |
| 181766 | `GO_PRISON_DOOR_01` | 4 | `./src/server/scripts/Outland/HellfireCitadel/BloodFurnace/blood_furnace.h` |
| 181811 | `GO_PRISON_DOOR_02` | 4 | `./src/server/scripts/Outland/HellfireCitadel/BloodFurnace/blood_furnace.h` |
| 181812 | `GO_PRISON_DOOR_03` | 4 | `./src/server/scripts/Outland/HellfireCitadel/BloodFurnace/blood_furnace.h` |
| 181813 | `GO_PRISON_CELL_DOOR_1` | 4 | `./src/server/scripts/Outland/HellfireCitadel/BloodFurnace/blood_furnace.h` |
| 181814 | `GO_PRISON_CELL_DOOR_2` | 4 | `./src/server/scripts/Outland/HellfireCitadel/BloodFurnace/blood_furnace.h` |
| 181815 | `GO_PRISON_CELL_DOOR_4` | 4 | `./src/server/scripts/Outland/HellfireCitadel/BloodFurnace/blood_furnace.h` |
| 181816 | `GO_PRISON_CELL_DOOR_3` | 4 | `./src/server/scripts/Outland/HellfireCitadel/BloodFurnace/blood_furnace.h` |
| 181817 | `GO_PRISON_CELL_DOOR_8` | 4 | `./src/server/scripts/Outland/HellfireCitadel/BloodFurnace/blood_furnace.h` |
| 181818 | `GO_PRISON_CELL_DOOR_6` | 4 | `./src/server/scripts/Outland/HellfireCitadel/BloodFurnace/blood_furnace.h` |
| 181819 | `GO_PRISON_DOOR_04` | 4 | `./src/server/scripts/Outland/HellfireCitadel/BloodFurnace/blood_furnace.h` |
| 181820 | `GO_PRISON_CELL_DOOR_7` | 4 | `./src/server/scripts/Outland/HellfireCitadel/BloodFurnace/blood_furnace.h` |
| 181821 | `GO_PRISON_CELL_DOOR_5` | 4 | `./src/server/scripts/Outland/HellfireCitadel/BloodFurnace/blood_furnace.h` |
| 181822 | `GO_PRISON_DOOR_05` | 4 | `./src/server/scripts/Outland/HellfireCitadel/BloodFurnace/blood_furnace.h` |
| 181823 | `GO_SUMMON_DOOR` | 4 | `./src/server/scripts/Outland/HellfireCitadel/BloodFurnace/blood_furnace.h` |
| 182539 | `GO_GRAND_WARLOCK_CHAMBER_DOOR_1` | 4 | `./src/server/scripts/Outland/HellfireCitadel/ShatteredHalls/shattered_halls.h` |
| 182540 | `GO_GRAND_WARLOCK_CHAMBER_DOOR_2` | 4 | `./src/server/scripts/Outland/HellfireCitadel/ShatteredHalls/shattered_halls.h` |
| 183049 | `GO_MAIN_CHAMBERS_DOOR` | 4 | `./src/server/scripts/Outland/CoilfangReservoir/SteamVault/steam_vault.h` |
| 183295 | `GO_SCREAMING_HALL_DOOR` | 4 | `./src/server/scripts/Outland/Auchindoun/ShadowLabyrinth/shadow_labyrinth.h` |
| 183296 | `GO_REFECTORY_DOOR` | 4 | `./src/server/scripts/Outland/Auchindoun/ShadowLabyrinth/shadow_labyrinth.h` |
| 183847 | `GO_MAGTHERIDON_DOOR` | 4 | `./src/server/scripts/Outland/HellfireCitadel/MagtheridonsLair/magtheridons_lair.h` |
| 184274 | `GO_MASTERS_TERRACE_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/Karazhan/karazhan.h` |
| 184275 | `GO_SIDE_ENTRANCE_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/Karazhan/karazhan.h` |
| 184276 | `GO_GAMESMAN_HALL_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/Karazhan/karazhan.h` |
| 184277 | `GO_GAMESMAN_HALL_EXIT_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/Karazhan/karazhan.h` |
| 184278 | `GO_STAGE_DOOR_LEFT` | 4 | `./src/server/scripts/EasternKingdoms/Karazhan/karazhan.h` |
| 184279 | `GO_STAGE_DOOR_RIGHT` | 4 | `./src/server/scripts/EasternKingdoms/Karazhan/karazhan.h` |
| 184280 | `GO_MASTERS_TERRACE_DOOR2` | 4 | `./src/server/scripts/EasternKingdoms/Karazhan/karazhan.h` |
| 184322 | `GO_DOOR_MOARG_2` | 4 | `./src/server/scripts/Outland/TempestKeep/Mechanar/mechanar.h` |
| 184324 | `GO_ARCANE_DOOR_LEFT` | 4 | `./src/server/scripts/Outland/TempestKeep/Eye/the_eye.h` |
| 184325 | `GO_ARCANE_DOOR_RIGHT` | 4 | `./src/server/scripts/Outland/TempestKeep/Eye/the_eye.h` |
| 184449 | `GO_DOOR_NETHERMANCER` | 4 | `./src/server/scripts/Outland/TempestKeep/Mechanar/mechanar.h` |
| 184468 | `GO_MAULGAR_DOOR` | 4 | `./src/server/scripts/Outland/GruulsLair/gruuls_lair.h` |
| 184517 | `GO_PRIVATE_LIBRARY_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/Karazhan/karazhan.h` |
| 184632 | `GO_DOOR_MOARG_1` | 4 | `./src/server/scripts/Outland/TempestKeep/Mechanar/mechanar.h` |
| 184662 | `GO_GRUUL_DOOR` | 4 | `./src/server/scripts/Outland/GruulsLair/gruuls_lair.h` |
| 185134 | `GO_NETHERSPACE_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/Karazhan/karazhan.h` |
| 185478 | `GO_SHADE_OF_AKAMA_DOOR` | 4 | `./src/server/scripts/Outland/BlackTemple/black_temple.h` |
| 185479 | `GO_DEN_OF_MORTAL_DOOR` | 4 | `./src/server/scripts/Outland/BlackTemple/black_temple.h` |
| 185480 | `GO_TERON_DOOR_1` | 4 | `./src/server/scripts/Outland/BlackTemple/black_temple.h` |
| 185481 | `GO_COUNCIL_DOOR_1` | 4 | `./src/server/scripts/Outland/BlackTemple/black_temple.h` |
| 185482 | `GO_MOTHER_SHAHRAZ_DOOR` | 4 | `./src/server/scripts/Outland/BlackTemple/black_temple.h` |
| 185521 | `GO_MASSIVE_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/Karazhan/karazhan.h` |
| 185892 | `GO_GURTOGG_DOOR` | 4 | `./src/server/scripts/Outland/BlackTemple/black_temple.h` |
| 186152 | `GO_COUNCIL_DOOR_2` | 4 | `./src/server/scripts/Outland/BlackTemple/black_temple.h` |
| 186153 | `GO_TERON_DOOR_2` | 4 | `./src/server/scripts/Outland/BlackTemple/black_temple.h` |
| 186261 | `GO_ILLIDAN_DOOR_R` | 4 | `./src/server/scripts/Outland/BlackTemple/black_temple.h` |
| 186262 | `GO_ILLIDAN_DOOR_L` | 4 | `./src/server/scripts/Outland/BlackTemple/black_temple.h` |
| 186303 | `GO_DOOR_HALAZZI` | 4 | `./src/server/scripts/EasternKingdoms/Stratholme/stratholme.h` |
| 186694 | `GO_GIANT_PORTCULLIS_2` | 4 | `./src/server/scripts/Northrend/UtgardeKeep/UtgardeKeep/utgarde_keep.h` |
| 186756 | `GO_GIANT_PORTCULLIS_1` | 4 | `./src/server/scripts/Northrend/UtgardeKeep/UtgardeKeep/utgarde_keep.h` |
| 187770 | `GO_DELRISSA_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/MagistersTerrace/magisters_terrace.h` |
| 187896 | `GO_VEXALLUS_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/MagistersTerrace/magisters_terrace.h` |
| 187979 | `GO_SELIN_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/MagistersTerrace/magisters_terrace.h` |
| 188064 | `GO_KAEL_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/MagistersTerrace/magisters_terrace.h` |
| 188065 | `GO_SELIN_ENCOUNTER_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/MagistersTerrace/magisters_terrace.h` |
| 191292 | `GO_MAIDEN_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/HallsOfStone/halls_of_stone.h` |
| 191295 | `GO_BRANN_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/HallsOfStone/halls_of_stone.h` |
| 191296 | `GO_SJONNIR_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/HallsOfStone/halls_of_stone.h` |
| 191324 | `GO_LOKEN_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/HallsOfLightning/halls_of_lightning.h` |
| 191325 | `GO_VOLKHAN_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/HallsOfLightning/halls_of_lightning.h` |
| 191326 | `GO_IONAR_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/HallsOfLightning/halls_of_lightning.h` |
| 191416 | `GO_BJARNGRIM_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/HallsOfLightning/halls_of_lightning.h` |
| 191556 | `GO_XEVOZZ_DOOR` | 4 | `./src/server/scripts/Northrend/VioletHold/violet_hold.h` |
| 191562 | `GO_EREKEM_GUARD_2_DOOR` | 4 | `./src/server/scripts/Northrend/VioletHold/violet_hold.h` |
| 191563 | `GO_EREKEM_GUARD_1_DOOR` | 4 | `./src/server/scripts/Northrend/VioletHold/violet_hold.h` |
| 191564 | `GO_EREKEM_DOOR` | 4 | `./src/server/scripts/Northrend/VioletHold/violet_hold.h` |
| 191565 | `GO_ZURAMAT_DOOR` | 4 | `./src/server/scripts/Northrend/VioletHold/violet_hold.h` |
| 191566 | `GO_LAVANTHOR_DOOR` | 4 | `./src/server/scripts/Northrend/VioletHold/violet_hold.h` |
| 191606 | `GO_MORAGG_DOOR` | 4 | `./src/server/scripts/Northrend/VioletHold/violet_hold.h` |
| 191722 | `GO_ICHORON_DOOR` | 4 | `./src/server/scripts/Northrend/VioletHold/violet_hold.h` |
| 191723 | `GO_MAIN_DOOR` | 4 | `./src/server/scripts/Northrend/VioletHold/violet_hold.h` |
| 192173 | `GO_SKADI_THE_RUTHLESS_DOOR` | 4 | `./src/server/scripts/Northrend/UtgardeKeep/UtgardePinnacle/utgarde_pinnacle.h` |
| 192174 | `GO_KING_YMIRON_DOOR` | 4 | `./src/server/scripts/Northrend/UtgardeKeep/UtgardePinnacle/utgarde_pinnacle.h` |
| 192395 | `GO_KRIKTHIR_DOOR` | 4 | `./src/server/scripts/Northrend/AzjolNerub/AzjolNerub/azjol_nerub.h` |
| 192396 | `GO_ANUBARAK_DOOR_1` | 4 | `./src/server/scripts/Northrend/AzjolNerub/AzjolNerub/azjol_nerub.h` |
| 192397 | `GO_ANUBARAK_DOOR_2` | 4 | `./src/server/scripts/Northrend/AzjolNerub/AzjolNerub/azjol_nerub.h` |
| 192398 | `GO_ANUBARAK_DOOR_3` | 4 | `./src/server/scripts/Northrend/AzjolNerub/AzjolNerub/azjol_nerub.h` |
| 192568 | `GO_GAL_DARAH_DOOR_3` | 4 | `./src/server/scripts/Northrend/Gundrak/gundrak.h` |
| 192569 | `GO_ECK_THE_FEROCIOUS_DOOR_BEHIND` | 4 | `./src/server/scripts/Northrend/Gundrak/gundrak.h` |
| 192632 | `GO_ECK_THE_FEROCIOUS_DOOR` | 4 | `./src/server/scripts/Northrend/Gundrak/gundrak.h` |
| 193188 | `GO_TRAPDOOR` | 4 | `./src/server/scripts/Northrend/Gundrak/gundrak.h` |
| 193208 | `GO_GAL_DARAH_DOOR_1` | 4 | `./src/server/scripts/Northrend/Gundrak/gundrak.h` |
| 193209 | `GO_GAL_DARAH_DOOR_2` | 4 | `./src/server/scripts/Northrend/Gundrak/gundrak.h` |
| 193995 | `GO_DRAGON_CAGE_DOOR` | 4 | `./src/server/scripts/Northrend/Nexus/Oculus/oculus.h` |
| 194253 | `GO_DOODAD_UL_ULDUAR_TRAPDOOR_03` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194441 | `GO_HODIR_ICE_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194553 | `GO_KOLOGARN_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194554 | `GO_IRON_COUNCIL_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194556 | `GO_ARCHIVUM_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194557 | `GO_THORIM_RUNIC_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194558 | `GO_THORIM_STONE_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194559 | `GO_THORIM_ENCOUNTER_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194560 | `GO_THORIM_DARK_IRON_PORTCULLIS` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194631 | `GO_XT_002_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194634 | `GO_HODIR_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194635 | `GO_BRAIN_ROOM_DOOR_1` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194636 | `GO_BRAIN_ROOM_DOOR_2` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194637 | `GO_BRAIN_ROOM_DOOR_3` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194750 | `GO_VEZAX_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194767 | `GO_DOODAD_UL_SIGILDOOR_01` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194773 | `GO_YOGG_SARON_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194774 | `GO_MIMIRON_DOOR_1` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194775 | `GO_MIMIRON_DOOR_2` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194776 | `GO_MIMIRON_DOOR_3` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194905 | `GO_LEVIATHAN_DOOR` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194910 | `GO_DOODAD_UL_SIGILDOOR_03` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194911 | `GO_DOODAD_UL_SIGILDOOR_02` | 4 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 195223 | `GO_HORDE_KEEP_PORTCULLIS` | 4 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195436 | `GO_DOODAD_HU_PORTCULLIS01` | 4 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195437 | `GO_DOODAD_VR_PORTCULLIS01` | 4 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195451 | `GO_DOODAD_PORTCULLISACTIVE01` | 4 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195452 | `GO_DOODAD_PORTCULLISACTIVE02` | 4 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195485 | `GO_WEB_DOOR` | 4 | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/trial_of_the_crusader.h` |
| 195648 | `GO_EAST_PORTCULLIS` | 4 | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/trial_of_the_crusader.h` |
| 197341 | `GO_IMPENETRABLE_DOOR` | 4 | `./src/server/scripts/Northrend/FrozenHalls/HallsOfReflection/halls_of_reflection.h` |
| 197342 | `GO_SHADOW_THRONE_DOOR` | 4 | `./src/server/scripts/Northrend/FrozenHalls/HallsOfReflection/halls_of_reflection.h` |
| 197343 | `GO_ESCAPE_DOOR` | 4 | `./src/server/scripts/Northrend/FrozenHalls/HallsOfReflection/halls_of_reflection.h` |
| 201369 | `GO_SINDRAGOSA_SHORTCUT_ENTRANCE_DOOR` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201373 | `GO_SINDRAGOSA_ENTRANCE_DOOR` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201376 | `GO_CRIMSON_HALL_DOOR` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201377 | `GO_BLOOD_ELF_COUNCIL_DOOR_RIGHT` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201378 | `GO_BLOOD_ELF_COUNCIL_DOOR` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201379 | `GO_SINDRAGOSA_SHORTCUT_EXIT_DOOR` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201380 | `GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_01` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201381 | `GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_02` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201382 | `GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_03` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201383 | `GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_04` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201612 | `GO_SCIENTIST_AIRLOCK_DOOR_COLLISION` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201613 | `GO_SCIENTIST_AIRLOCK_DOOR_ORANGE` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201614 | `GO_SCIENTIST_AIRLOCK_DOOR_GREEN` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201746 | `GO_DOODAD_ICECROWN_BLOODPRINCE_DOOR_01` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201825 | `GO_SAURFANG_S_DOOR` | 4 | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 201848 | `GO_HALLS_OF_REFLECTION_PORTCULLIS` | 4 | `./src/server/scripts/Northrend/FrozenHalls/PitOfSaron/pit_of_saron.h` |
| 201976 | `GO_ENTRANCE_DOOR` | 4 | `./src/server/scripts/Northrend/FrozenHalls/HallsOfReflection/halls_of_reflection.h` |
| 202306 | `GO_ULDUM_DOOR_14` | 4 | `./src/server/scripts/Kalimdor/HallsOfOrigination/halls_of_origination.h` |
| 202307 | `GO_ANHUURS_DOOR` | 4 | `./src/server/scripts/Kalimdor/HallsOfOrigination/halls_of_origination.h` |
| 202314 | `GO_VAULT_OF_LIGHTS_BOSS_DOOR` | 4 | `./src/server/scripts/Kalimdor/HallsOfOrigination/halls_of_origination.h` |
| 204338 | `GO_LADY_NAZJAR_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/ThroneOfTheTides/throne_of_the_tides.h` |
| 204339 | `GO_COMMANDER_ULTHOK_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/ThroneOfTheTides/throne_of_the_tides.h` |
| 204340 | `GO_ERUNAK_STONESPEAKER_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/ThroneOfTheTides/throne_of_the_tides.h` |
| 204341 | `GO_OZUMAT_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/ThroneOfTheTides/throne_of_the_tides.h` |
| 206576 | `GO_TOLBARAD_DOOR` | 4 | `./src/server/game/Battlefield/Zones/BattlefieldTB.h` |
| 207619 | `GO_ARGALOTH_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/BaradinHold/baradin_hold.h` |
| 208843 | `GO_THE_CACHE_OF_MADNESS_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/ZulGurub/zulgurub.h` |
| 208845 | `GO_ARENA_DOOR_1` | 4 | `./src/server/scripts/EasternKingdoms/ZulGurub/zulgurub.h` |
| 208846 | `GO_ARENA_DOOR_4` | 4 | `./src/server/scripts/EasternKingdoms/ZulGurub/zulgurub.h` |
| 208847 | `GO_ARENA_DOOR_2` | 4 | `./src/server/scripts/EasternKingdoms/ZulGurub/zulgurub.h` |
| 208848 | `GO_ARENA_DOOR_3` | 4 | `./src/server/scripts/EasternKingdoms/ZulGurub/zulgurub.h` |
| 208849 | `GO_ARENA_DOOR_5` | 4 | `./src/server/scripts/EasternKingdoms/ZulGurub/zulgurub.h` |
| 208850 | `GO_ZANZIL_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/ZulGurub/zulgurub.h` |
| 208873 | `GO_TRAP_DOOR` | 4 | `./src/server/scripts/Kalimdor/Firelands/firelands.h` |
| 208953 | `GO_OCCUTHAR_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/BaradinHold/baradin_hold.h` |
| 209073 | `GO_DOOR_RAGNAROS` | 4 | `./src/server/scripts/Kalimdor/Firelands/firelands.h` |
| 209849 | `GO_ALIZABAL_DOOR` | 4 | `./src/server/scripts/EasternKingdoms/BaradinHold/baradin_hold.h` |
| 210084 | `GO_COURTYARD_DOOR_1` | 4 | `./src/server/scripts/Kalimdor/CavernsOfTime/WellOfEternity/well_of_eternity.h` |
| 210800 | `GO_SNOWDRIFT_DOJO_DOOR` | 4 | `./src/server/scripts/Pandaria/ShadopanMonastery/shadopan_monastery.h` |
| 212921 | `GO_DOOR` | 4 | `./src/server/scripts/Pandaria/SiegeNiuzaoTemple/siege_of_the_niuzao_temple.h` |
| 212982 | `GO_KIPTILAK_ENTRANCE_DOOR` | 4 | `./src/server/scripts/Pandaria/GateSettingSun/gate_setting_sun.h` |
| 212983 | `GO_KIPTILAK_EXIT_DOOR` | 4 | `./src/server/scripts/Pandaria/GateSettingSun/gate_setting_sun.h` |
| 212985 | `GO_RIMAK_AFTER_DOOR` | 4 | `./src/server/scripts/Pandaria/GateSettingSun/gate_setting_sun.h` |
| 212986 | `GO_RAIGONN_DOOR` | 4 | `./src/server/scripts/Pandaria/GateSettingSun/gate_setting_sun.h` |
| 213251 | `GO_TEMPLE_INVIS_DOOR` | 4 | `./src/server/scripts/Pandaria/SiegeNiuzaoTemple/siege_of_the_niuzao_temple.h` |
| 213593 | `GO_DOOR_AFTER_TRIAL` | 4 | `./src/server/scripts/Pandaria/MogushanPalace/mogu_shan_palace.h` |
| 213594 | `GO_DOOR_BEFORE_TRIAL` | 4 | `./src/server/scripts/Pandaria/MogushanPalace/mogu_shan_palace.h` |
| 213596 | `GO_DOOR_BEFORE_KING` | 4 | `./src/server/scripts/Pandaria/MogushanPalace/mogu_shan_palace.h` |
| 214261 | `GO_RAIGONN_AFTER_DOOR` | 4 | `./src/server/scripts/Pandaria/GateSettingSun/gate_setting_sun.h` |
| 214888 | `GO_GREATDOOR_SECOND_DOOR` | 4 | `./src/server/scripts/Pandaria/GateSettingSun/gate_setting_sun.h` |
| 221245 | `GO_IMMERSEUS_DOOR` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221439 | `GO_SOUTH_EAST_DOOR` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221440 | `GO_SOUTH_WEST_DOOR` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221620 | `GO_MYSTERIOUS_ORNATE_DOOR` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221622 | `GO_MASSIVE_IRONBOUND_DOOR_4` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221663 | `GO_PRESSURE_PLATE_DOOR` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221792 | `GO_SANCTUM_DOOR` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221793 | `GO_MASSIVE_IRONBOUND_DOOR_1` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221911 | `GO_MASSIVE_IRONBOUND_DOOR_5` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221912 | `GO_MASSIVE_IRONBOUND_DOOR_7` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223056 | `GO_MASSIVE_IRONBOUND_DOOR_2` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223057 | `GO_MASSIVE_IRONBOUND_DOOR_6` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223058 | `GO_MASSIVE_IRONBOUND_DOOR_3` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223219 | `GO_WIND_DOOR` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223222 | `GO_TREX_DOOR` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223231 | `GO_RUSTED_IRON_PORTCULLIS` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223235 | `GO_MASSIVE_IRONBOUND_DOOR_8` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223276 | `GO_NAZGRIM_ENTRY_DOOR` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223805 | `GO_OGRAID_LARGE_DOOR` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 240687 | `GO_FEL_LORD_ZAKUUN_DOOR_2` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 241485 | `GO_HELLFIRE_ARCHIMONDE_DOOR_1` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 241649 | `GO_TYRANT_VELHARI_DOOR_1` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 241650 | `GO_TYRANT_VELHARI_DOOR_2` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 242280 | `GO_SHADOW_LORD_ISKAR_DOOR_3` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 242358 | `GO_TYRANT_VELHARI_DOOR_4` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 242359 | `GO_TYRANT_VELHARI_DOOR_3` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 242360 | `GO_SOULBOUND_CONSTRUCT_DOOR_3` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 242361 | `GO_SOULBOUND_CONSTRUCT_DOOR_4` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 242362 | `GO_SOULBOUND_CONSTRUCT_DOOR_1` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 242363 | `GO_SOULBOUND_CONSTRUCT_DOOR_2` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 242419 | `GO_SHADOW_LORD_ISKAR_DOOR_2` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 242454 | `GO_SIEGEMASTER_MARTAK_DOOR_2` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 242464 | `GO_SIEGEMASTER_MARTAK_DOOR_1` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 242510 | `GO_FEL_LORD_ZAKUUN_DOOR_1` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 243183 | `GO_SHADOW_LORD_ISKAR_DOOR_1` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 243482 | `GO_SHADOW_LORD_ISKAR_DOOR_4` | 4 | `./src/server/scripts/Draenor/HellfireCitadel/hellfire_citadel.h` |
| 246049 | `GO_CORDANA_DOOR` | 4 | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 246053 | `GO_BOSS01_DOOR_2` | 4 | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 246053 | `GO_SALTHERIL_DOOR_3` | 4 | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 246054 | `GO_BOSS01_DOOR_1` | 4 | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 246054 | `GO_SALTHERIL_DOOR_2` | 4 | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 246055 | `GO_SALTHERIL_DOOR_1` | 4 | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 246112 | `GO_DOOR_1` | 4 | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 246112 | `GO_TORMENTORUM_DOOR_1` | 4 | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 246834 | `GO_TRAP_DOOR_07` | 4 | `./src/server/scripts/BrokenIsles/MawOfSouls/maw_of_souls.h` |
| 246835 | `GO_TRAP_DOOR_09` | 4 | `./src/server/scripts/BrokenIsles/MawOfSouls/maw_of_souls.h` |
| 246837 | `GO_TRAP_DOOR_10` | 4 | `./src/server/scripts/BrokenIsles/MawOfSouls/maw_of_souls.h` |
| 246838 | `GO_TRAP_DOOR_06` | 4 | `./src/server/scripts/BrokenIsles/MawOfSouls/maw_of_souls.h` |
| 246839 | `GO_TRAP_DOOR_05` | 4 | `./src/server/scripts/BrokenIsles/MawOfSouls/maw_of_souls.h` |
| 246840 | `GO_TRAP_DOOR_01` | 4 | `./src/server/scripts/BrokenIsles/MawOfSouls/maw_of_souls.h` |
| 246841 | `GO_TRAP_DOOR_11` | 4 | `./src/server/scripts/BrokenIsles/MawOfSouls/maw_of_souls.h` |
| 246842 | `GO_TRAP_DOOR_02` | 4 | `./src/server/scripts/BrokenIsles/MawOfSouls/maw_of_souls.h` |
| 246843 | `GO_TRAP_DOOR_03` | 4 | `./src/server/scripts/BrokenIsles/MawOfSouls/maw_of_souls.h` |
| 246844 | `GO_TRAP_DOOR_04` | 4 | `./src/server/scripts/BrokenIsles/MawOfSouls/maw_of_souls.h` |
| 246983 | `GO_WATER_DOOR` | 4 | `./src/server/scripts/BrokenIsles/EyeOfAzshara/eye_of_azshara.h` |
| 246990 | `GO_MANASTORM_DOOR` | 4 | `./src/server/scripts/BrokenIsles/VioletHold/violet_hold_assault.h` |
| 246997 | `GO_THALENA_DOOR` | 4 | `./src/server/scripts/BrokenIsles/VioletHold/violet_hold_assault.h` |
| 246998 | `GO_FESTERFACE_DOOR` | 4 | `./src/server/scripts/BrokenIsles/VioletHold/violet_hold_assault.h` |
| 246999 | `GO_SHIVERMAW_DOOR` | 4 | `./src/server/scripts/BrokenIsles/VioletHold/violet_hold_assault.h` |
| 247000 | `GO_KAARHJ_DOOR` | 4 | `./src/server/scripts/BrokenIsles/VioletHold/violet_hold_assault.h` |
| 247080 | `GO_AFTER_BOSS01_DOOR` | 4 | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 247081 | `GO_SALTHERIL_DOOR_4` | 4 | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 247082 | `GO_SALTHERIL_DOOR_5` | 4 | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 247322 | `GO_DOOR_1` | 4 | `./src/server/scripts/BrokenIsles/Scenario/scenario_artifact_karazhan_catacombs.cpp` |
| 247385 | `GO_ANUBESSET_DOOR` | 4 | `./src/server/scripts/BrokenIsles/VioletHold/violet_hold_assault.h` |
| 248848 | `GO_ILGYNOTH_DOOR_1` | 4 | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 248849 | `GO_ILGYNOTH_DOOR_2` | 4 | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 248850 | `GO_ILGYNOTH_DOOR_3` | 4 | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 250243 | `GO_TRILLIAX_DOOR_02` | 4 | `./src/server/scripts/BrokenIsles/TheNighthold/thenighthold.h` |
| 251397 | `GO_TRILLIAX_DOOR_01` | 4 | `./src/server/scripts/BrokenIsles/TheNighthold/thenighthold.h` |
| 251519 | `GO_TORM_THE_BRUTE_DOOR` | 4 | `./src/server/scripts/BrokenIsles/TheNighthold/thenighthold.h` |
| 251533 | `GO_NYTHENDRA_DOOR_1` | 4 | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 251534 | `GO_NYTHENDRA_DOOR_2` | 4 | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 251535 | `GO_NYTHENDRA_DOOR_3` | 4 | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 251536 | `GO_NYTHENDRA_DOOR_4` | 4 | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 251555 | `GO_EYE_OF_ILGYNOTH_DOOR` | 4 | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 251847 | `GO_MELANDRUS_DOOR_1` | 4 | `./src/server/scripts/BrokenIsles/CourtOfStars/court_of_stars.h` |
| 251848 | `GO_MELANDRUS_DOOR_2` | 4 | `./src/server/scripts/BrokenIsles/CourtOfStars/court_of_stars.h` |
| 251849 | `GO_GERDO_DOOR` | 4 | `./src/server/scripts/BrokenIsles/CourtOfStars/court_of_stars.h` |
| 252042 | `GO_URSOC_DOOR_1` | 4 | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 252043 | `GO_URSOC_DOOR_2` | 4 | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 252103 | `GO_SKORPYRON_DOOR_02` | 4 | `./src/server/scripts/BrokenIsles/TheNighthold/thenighthold.h` |
| 252105 | `GO_SKORPYRON_DOOR_01` | 4 | `./src/server/scripts/BrokenIsles/TheNighthold/thenighthold.h` |
| 254849 | `GO_DOOR_ROOM_DRESARON_1` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254850 | `GO_DOOR_ROOM_DRESARON_2` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254855 | `GO_DOOR_ROOM_XAVIUS` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254984 | `GO_DOOR_ROOM_GLAIDALIS_1` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254985 | `GO_DOOR_ROOM_GLAIDALIS_2` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254986 | `GO_DOOR_ROOM_GLAIDALIS_3` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254987 | `GO_DOOR_ROOM_GLAIDALIS_4` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254988 | `GO_DOOR_ROOM_GLAIDALIS_5` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254989 | `GO_DOOR_ROOM_GLAIDALIS_6` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254990 | `GO_DOOR_ROOM_GLAIDALIS_8` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254991 | `GO_DOOR_ROOM_GLAIDALIS_10` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254992 | `GO_DOOR_ROOM_GLAIDALIS_12` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254993 | `GO_DOOR_ROOM_GLAIDALIS_16` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254994 | `GO_DOOR_ROOM_GLAIDALIS_15` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254995 | `GO_DOOR_ROOM_GLAIDALIS_14` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254996 | `GO_DOOR_ROOM_GLAIDALIS_13` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254997 | `GO_DOOR_ROOM_GLAIDALIS_7` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254998 | `GO_DOOR_ROOM_GLAIDALIS_9` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 254999 | `GO_DOOR_ROOM_GLAIDALIS_11` | 4 | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 255000 | `GO_RENFERAL_DOOR` | 4 | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 259702 | `GO_OPERA_RIGHT_DOOR` | 4 | `./src/server/scripts/BrokenIsles/ReturnToKharazan/new_karazhan.h` |
| 260550 | `GO_NYTHENDRA_DOOR_5` | 4 | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 260551 | `GO_NYTHENDRA_DOOR_6` | 4 | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 266510 | `GO_MEDIVH_BOOK_DOOR` | 4 | `./src/server/scripts/BrokenIsles/ReturnToKharazan/new_karazhan.h` |
| 266533 | `GO_GUARM_DOOR_HALYA` | 4 | `./src/server/scripts/BrokenIsles/TrialOfValor/trial_of_valor.h` |
| 266846 | `GO_MEDIVH_DOOR` | 4 | `./src/server/scripts/BrokenIsles/ReturnToKharazan/new_karazhan.h` |
| 268579 | `GO_INQUISITION_DOOR` | 4 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 268580 | `GO_INQUISITION_DOOR_INTRO` | 4 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 268680 | `GO_LAST_DOORS` | 4 | `./src/server/scripts/BrokenIsles/CathedralEternalNight/cathedral_of_eternal_night.h` |
| 268709 | `GO_THRASHBITE_DOOR_ENTER` | 4 | `./src/server/scripts/BrokenIsles/CathedralEternalNight/cathedral_of_eternal_night.h` |
| 268710 | `GO_THRASHBITE_DOOR` | 4 | `./src/server/scripts/BrokenIsles/CathedralEternalNight/cathedral_of_eternal_night.h` |
| 268749 | `GO_DESOLATE_HOST_DOOR_1` | 4 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 269045 | `GO_SISTERS_MOON_DOOR` | 4 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 269221 | `GO_AGRONOX_DOOR` | 4 | `./src/server/scripts/BrokenIsles/CathedralEternalNight/cathedral_of_eternal_night.h` |
| 269262 | `GO_AVATARA_DOOR` | 4 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 269781 | `GO_DESOLATE_HOST_DOOR_2` | 4 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 269976 | `GO_SASSZINE_DOOR` | 4 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 269988 | `GO_MAIDEN_DOOR` | 4 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 272062 | `GO_DOOR_ROOM_VICEROY_NEZHAR` | 4 | `./src/server/scripts/BrokenIsles/SeatOfTriumvirate/seat_of_triumvirate.h` |
| 273789 | `GO_DOOR_ROOM_ZURAAL_THE_ASCENDED` | 4 | `./src/server/scripts/BrokenIsles/SeatOfTriumvirate/seat_of_triumvirate.h` |
| 277358 | `GO_DOOR_4` | 4 | `./src/server/scripts/BrokenIsles/AntorusTheBurningThrone/antorus.h` |
| 277359 | `GO_DOOR_3` | 4 | `./src/server/scripts/BrokenIsles/AntorusTheBurningThrone/antorus.h` |
| 277365 | `GO_DOOR_1` | 4 | `./src/server/scripts/BrokenIsles/AntorusTheBurningThrone/antorus.h` |
| 277521 | `GO_DOOR_PORTAL_KEEPER_HASABEL` | 4 | `./src/server/scripts/BrokenIsles/AntorusTheBurningThrone/antorus.h` |
| 278488 | `GO_DOOR_2` | 4 | `./src/server/scripts/BrokenIsles/AntorusTheBurningThrone/antorus.h` |
| 287416 | `GO_TRAPDOOR_ZUL` | 4 | `./src/server/scripts/Zandalar/Uldir/uldir.h` |
| 291104 | `GO_MYTHRAX_DOOR` | 4 | `./src/server/scripts/Zandalar/Uldir/uldir.h` |
| 299084 | `GO_DOOR_ZUL_ENCOUNTER_THIRD` | 4 | `./src/server/scripts/Zandalar/Uldir/uldir.h` |
| 299085 | `GO_DOOR_ZUL_ENCOUNTER_SECOND` | 4 | `./src/server/scripts/Zandalar/Uldir/uldir.h` |
| 299086 | `GO_DOOR_ZUL_ENCOUNTER` | 4 | `./src/server/scripts/Zandalar/Uldir/uldir.h` |
| 312359 | `GO_CONCLAVE_DOOR_EXIT` | 4 | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 312360 | `GO_ENCLAVE_DOOR_ENTRANCE` | 4 | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 313898 | `GO_RASTAKHAN_DOOR_EXIT_TO_PORT_OF_ZANDALAR` | 4 | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 328388 | `GO_PALACE_DOOR` | 4 | `./src/server/scripts/Nazjatar/EternalPalace/eternal_palace.h` |

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
| 185882 | `GO_SUPREMUS_GATE` | 5 | `./src/server/scripts/Outland/BlackTemple/black_temple.h` |
| 185905 | `GO_ILLIDAN_GATE` | 5 | `./src/server/scripts/Outland/BlackTemple/black_temple.h` |
| 186728 | `GO_MASSIVE_GATE` | 5 | `./src/server/scripts/EasternKingdoms/ZulAman/zulaman.h` |
| 187711 | `GO_MALGANIS_GATE_1` | 5 | `./src/server/scripts/Kalimdor/CavernsOfTime/CullingOfStratholme/culling_of_stratholme.h` |
| 187723 | `GO_MALGANIS_GATE_2` | 5 | `./src/server/scripts/Kalimdor/CavernsOfTime/CullingOfStratholme/culling_of_stratholme.h` |
| 187990 | `GO_MURUS_GATE_1` | 5 | `./src/server/scripts/EasternKingdoms/SunwellPlateau/sunwell_plateau.h` |
| 188118 | `GO_MURUS_GATE_2` | 5 | `./src/server/scripts/EasternKingdoms/SunwellPlateau/sunwell_plateau.h` |
| 188686 | `GO_SHKAF_GATE` | 5 | `./src/server/scripts/Kalimdor/CavernsOfTime/CullingOfStratholme/culling_of_stratholme.h` |
| 190221 | `GO_WINTERGRASP_FORTRESS_TOWER_1` | 5 | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 190356 | `GO_WINTERGRASP_SHADOWSIGHT_TOWER` | 5 | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 190357 | `GO_WINTERGRASP_WINTER_S_EDGE_TOWER` | 5 | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 190358 | `GO_WINTERGRASP_FLAMEWATCH_TOWER` | 5 | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 190373 | `GO_WINTERGRASP_FORTRESS_TOWER_2` | 5 | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 190375 | `GO_WINTERGRASP_FORTRESS_GATE` | 5 | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 190377 | `GO_WINTERGRASP_FORTRESS_TOWER_3` | 5 | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 190378 | `GO_WINTERGRASP_FORTRESS_TOWER_4` | 5 | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 190722 | `GO_GATE_OF_THE_GREEN_EMERALD` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundSA.h` |
| 190723 | `GO_GATE_OF_THE_PURPLE_AMETHYST` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundSA.h` |
| 190724 | `GO_GATE_OF_THE_BLUE_SAPPHIRE` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundSA.h` |
| 190726 | `GO_GATE_OF_THE_RED_SUN` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundSA.h` |
| 190727 | `GO_GATE_OF_THE_YELLOW_MOON` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundSA.h` |
| 191788 | `GO_EXIT_GATE` | 5 | `./src/server/scripts/Kalimdor/CavernsOfTime/CullingOfStratholme/culling_of_stratholme.h` |
| 191810 | `GO_WINTERGRASP_VAULT_GATE` | 5 | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 192236 | `GO_PRINCE_TALDARAM_GATE` | 5 | `./src/server/scripts/Northrend/AzjolNerub/Ahnkahet/ahnkahet.h` |
| 194232 | `GO_KOLOGARN_BRIDGE` | 5 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194255 | `GO_ANCIENT_GATE_OF_THE_KEEPERS` | 5 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 194370 | `GO_TOWER_OF_FROST` | 5 | `./src/server/scripts/Northrend/Ulduar/Ulduar/boss_flame_leviathan.cpp` |
| 194371 | `GO_TOWER_OF_FLAMES` | 5 | `./src/server/scripts/Northrend/Ulduar/Ulduar/boss_flame_leviathan.cpp` |
| 194375 | `GO_TOWER_OF_LIFE` | 5 | `./src/server/scripts/Northrend/Ulduar/Ulduar/boss_flame_leviathan.cpp` |
| 194377 | `GO_TOWER_OF_STORMS` | 5 | `./src/server/scripts/Northrend/Ulduar/Ulduar/boss_flame_leviathan.cpp` |
| 194630 | `GO_LEVIATHAN_GATE` | 5 | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 195153 | `GO_ALLIANCE_BANNER_DOCK` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195154 | `GO_ALLIANCE_BANNER_DOCK_CONT` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195155 | `GO_HORDE_BANNER_DOCK` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195156 | `GO_HORDE_BANNER_DOCK_CONT` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195157 | `GO_DOCKS_BANNER` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195491 | `GO_DOODAD_ND_WINTERORC_WALL_GATEFX_DOOR01` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195494 | `GO_HORDE_GATE_1` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195495 | `GO_HORDE_GATE_3` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195496 | `GO_HORDE_GATE_2` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195647 | `GO_MAIN_GATE` | 5 | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheChampion/trial_of_the_champion.h` |
| 195647 | `GO_MAIN_GATE_DOOR` | 5 | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/trial_of_the_crusader.h` |
| 195698 | `GO_ALLIANCE_GATE_2` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195699 | `GO_ALLIANCE_GATE_1` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195700 | `GO_ALLIANCE_GATE_3` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 195703 | `GO_DOODAD_ND_HUMAN_GATE_CLOSEDFX_DOOR01` | 5 | `./src/server/game/Battlegrounds/Zones/BattlegroundIC.h` |
| 204927 | `GO_OZUMAT_TENTACLE_1` | 5 | `./src/server/scripts/EasternKingdoms/ThroneOfTheTides/throne_of_the_tides.h` |
| 204944 | `GO_OZUMAT_TENTACLE_2` | 5 | `./src/server/scripts/EasternKingdoms/ThroneOfTheTides/throne_of_the_tides.h` |
| 206506 | `GO_ANHUURS_BRIDGE` | 5 | `./src/server/scripts/Kalimdor/HallsOfOrigination/halls_of_origination.h` |
| 206598 | `GO_TOLBARAD_GATES` | 5 | `./src/server/game/Battlefield/Zones/BattlefieldTB.h` |
| 206843 | `GO_CURSED_DEPTHS_GATE` | 5 | `./src/server/game/Battlefield/Zones/BattlefieldTB.h` |
| 206844 | `GO_GATE_D_BLOCK` | 5 | `./src/server/game/Battlefield/Zones/BattlefieldTB.h` |
| 206845 | `GO_GATE_TO_THE_HOLE` | 5 | `./src/server/game/Battlefield/Zones/BattlefieldTB.h` |
| 207406 | `GO_STRANGE_FOUNTAIN` | 5 | `./src/server/scripts/EasternKingdoms/ThroneOfTheTides/throne_of_the_tides.h` |
| 209251 | `GO_SULFURON_BRIDGE` | 5 | `./src/server/scripts/Kalimdor/Firelands/firelands.h` |
| 209255 | `GO_BRIDGE_OF_RHYOLITH` | 5 | `./src/server/scripts/Kalimdor/Firelands/firelands.h` |
| 209277 | `GO_RAID_BRIDGE_FORMING` | 5 | `./src/server/scripts/Kalimdor/Firelands/firelands.h` |
| 218869 | `GO_TORTOS_BRIDGE` | 5 | `./src/server/scripts/Pandaria/ThroneOfThunder/boss_tortos.cpp` |
| 221794 | `GO_IRON_GATE_OUT_4` | 5 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221795 | `GO_IRON_GATE_OUT_3` | 5 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221796 | `GO_IRON_GATE_OUT_1` | 5 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221797 | `GO_IRON_GATE_OUT_2` | 5 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221798 | `GO_IRON_GATE_NORTH` | 5 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221799 | `GO_IRON_GATE_WEST` | 5 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221800 | `GO_IRON_GATE_EAST` | 5 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 221801 | `GO_IRON_GATE_SOUTH` | 5 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223032 | `GO_IRON_GATE_NORTH_WEST` | 5 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223033 | `GO_IRON_GATE_SOUTH_EAST` | 5 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223120 | `GO_ORGRIMMAR_FRONT_GATE` | 5 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223739 | `GO_GATES_OF_ORGRIMMAR` | 5 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223814 | `GO_OGRAID_FRONT_GATE_BACK` | 5 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 227851 | `GO_NHALLISH_GATE` | 5 | `./src/server/scripts/Draenor/ShadowmoonBurialGrounds/shadowmoon_burial_grounds.h` |
| 233919 | `GO_SADANA_GATE_2` | 5 | `./src/server/scripts/Draenor/ShadowmoonBurialGrounds/shadowmoon_burial_grounds.h` |
| 233920 | `GO_NERZHUL_GATE` | 5 | `./src/server/scripts/Draenor/ShadowmoonBurialGrounds/shadowmoon_burial_grounds.h` |
| 233921 | `GO_SADANA_GATE_1` | 5 | `./src/server/scripts/Draenor/ShadowmoonBurialGrounds/shadowmoon_burial_grounds.h` |
| 233988 | `GO_BONEMAW_GATE_1` | 5 | `./src/server/scripts/Draenor/ShadowmoonBurialGrounds/shadowmoon_burial_grounds.h` |
| 233989 | `GO_BONEMAW_GATE_2` | 5 | `./src/server/scripts/Draenor/ShadowmoonBurialGrounds/shadowmoon_burial_grounds.h` |
| 233990 | `GO_BONEMAW_GATE_3` | 5 | `./src/server/scripts/Draenor/ShadowmoonBurialGrounds/shadowmoon_burial_grounds.h` |
| 244664 | `GO_DOOR_BRIDGE` | 5 | `./src/server/scripts/BrokenIsles/TrialOfValor/trial_of_valor.h` |
| 246057 | `GO_ASHGOLM_BRIDGE` | 5 | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 246727 | `GO_GLAZER_BRIDGE` | 5 | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 249494 | `GO_LEGION_GATE` | 5 | `./src/server/scripts/BrokenIsles/Scenario/scenario_artifact_karazhan_catacombs.cpp` |
| 249494 | `GO_LEGION_GATE` | 5 | `./src/server/scripts/BrokenIsles/Scenario/scenario_artifact_ruins_of_falanaar.cpp` |
| 271126 | `GO_KILJAEDEN_BRIDGE_1` | 5 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 271127 | `GO_KILJAEDEN_BRIDGE_2` | 5 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 272802 | `GO_GOROTH_GATES` | 5 | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 288202 | `GO_GATE_003` | 5 | `./src/server/scripts/Zandalar/AtalDazar/atal_dazar.h` |
| 288203 | `GO_GATE_004` | 5 | `./src/server/scripts/Zandalar/AtalDazar/atal_dazar.h` |
| 288204 | `GO_GATE_001` | 5 | `./src/server/scripts/Zandalar/AtalDazar/atal_dazar.h` |
| 288205 | `GO_GATE_002` | 5 | `./src/server/scripts/Zandalar/AtalDazar/atal_dazar.h` |
| 292397 | `GO_GATE_009` | 5 | `./src/server/scripts/Zandalar/AtalDazar/atal_dazar.h` |
| 292398 | `GO_GATE_010` | 5 | `./src/server/scripts/Zandalar/AtalDazar/atal_dazar.h` |
| 292399 | `GO_GATE_006` | 5 | `./src/server/scripts/Zandalar/AtalDazar/atal_dazar.h` |
| 292400 | `GO_GATE_007` | 5 | `./src/server/scripts/Zandalar/AtalDazar/atal_dazar.h` |
| 292401 | `GO_GATE_008` | 5 | `./src/server/scripts/Zandalar/AtalDazar/atal_dazar.h` |
| 292402 | `GO_GATE_005` | 5 | `./src/server/scripts/Zandalar/AtalDazar/atal_dazar.h` |

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
| 203006 | `GO_FLAME_WALLS` | 4 | `./src/server/scripts/Northrend/ChamberOfAspects/RubySanctum/ruby_sanctum.h` |
| 207737 | `GO_SKYWALL_RAID` | 4 | `./src/server/scripts/Kalimdor/ThroneOfTheFourWinds/throne_of_the_four_winds.h` |
| 208906 | `GO_FIRE_WALL_FENDRAL` | 4 | `./src/server/scripts/Kalimdor/Firelands/firelands.h` |
| 209066 | `GO_FIRE_WALL_BALEROC` | 4 | `./src/server/scripts/Kalimdor/Firelands/firelands.h` |
| 209596 | `GO_INNER_WALL` | 4 | `./src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/dragon_soul.h` |
| 210097 | `GO_FIRE_WALL` | 4 | `./src/server/scripts/Pandaria/SiegeNiuzaoTemple/siege_of_the_niuzao_temple.h` |
| 210097 | `GO_HOLY_WALL` | 4 | `./src/server/scripts/Kalimdor/CavernsOfTime/HourofTwilight/hour_of_twilight.h` |
| 210130 | `GO_SMALL_FIREWALL_DOOR` | 4 | `./src/server/scripts/Kalimdor/CavernsOfTime/WellOfEternity/well_of_eternity.h` |
| 210234 | `GO_LARGE_FIREWALL_DOOR` | 4 | `./src/server/scripts/Kalimdor/CavernsOfTime/WellOfEternity/well_of_eternity.h` |
| 212908 | `GO_SNOWDRIFT_FIRE_WALL` | 4 | `./src/server/scripts/Pandaria/ShadopanMonastery/shadopan_monastery.h` |
| 213198 | `GO_WALL_C` | 4 | `./src/server/scripts/Pandaria/GateSettingSun/gate_setting_sun.h` |
| 214548 | `GO_WIND_WALL` | 4 | `./src/server/scripts/Pandaria/SiegeNiuzaoTemple/siege_of_the_niuzao_temple.h` |
| 214629 | `GO_KIPTILAK_WALLS` | 4 | `./src/server/scripts/Pandaria/GateSettingSun/gate_setting_sun.h` |
| 214849 | `GO_SHA_VORTEX_WALL` | 4 | `./src/server/scripts/Pandaria/TerraceofEndlessSpring/terrace_of_endless_spring.h` |
| 221422 | `GO_WALL_OF_ICE` | 4 | `./src/server/scripts/Pandaria/ThroneOfThunder/boss_durumu.cpp` |
| 221602 | `GO_SHA_ENERGY_WALL` | 4 | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 223271 | `GO_SKYWALL_WIND` | 4 | `./src/server/scripts/Kalimdor/ThroneOfTheFourWinds/throne_of_the_four_winds.h` |
| 243403 | `GO_COLLISION_WALL` | 4 | `./src/server/scripts/BrokenIsles/MawOfSouls/maw_of_souls.h` |
| 245119 | `GO_STONE_WALL` | 4 | `./src/server/scripts/BrokenIsles/Scenario/scenario_artifact_brokenshore.cpp` |
| 246435 | `GO_WALL_OF_SOULS_3` | 4 | `./src/server/scripts/BrokenIsles/MawOfSouls/maw_of_souls.h` |
| 246947 | `GO_WALL_OF_SOULS` | 4 | `./src/server/scripts/BrokenIsles/MawOfSouls/maw_of_souls.h` |
| 246948 | `GO_WALL_OF_SOULS_2` | 4 | `./src/server/scripts/BrokenIsles/MawOfSouls/maw_of_souls.h` |
| 247081 | `GO_BOSS01_FIRWALL_2` | 4 | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 247082 | `GO_BOSS01_FIRWALL_1` | 4 | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 266508 | `GO_STRANGE_WALL` | 4 | `./src/server/scripts/BrokenIsles/ReturnToKharazan/new_karazhan.h` |
| 278820 | `GO_COLLISION_WALL` | 4 | `./src/server/scripts/Zandalar/AtalDazar/atal_dazar.h` |
| 315948 | `GO_MEKKATORQUE_WALL_OF_SPEARS_ENTRANCE` | 4 | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 316663 | `GO_MEKKATORQUE_WALL_OF_SPEARS_EXIT` | 4 | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 317059 | `GO_WALL_OF_SPEARS_GRONG` | 4 | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 317064 | `GO_RAWANI_WALL_OF_SPEARS` | 4 | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 319259 | `GO_JADEFIRE_MASTERS_HORDE_WALL_OF_SPEARS_MAIN` | 4 | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 319260 | `GO_FRIDA_WALL_OF_SPEARS` | 4 | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 320585 | `GO_WALL_OF_ICE` | 4 | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 329650 | `GO_CIRCULAR_WALL_ASHVANE` | 4 | `./src/server/scripts/Nazjatar/EternalPalace/eternal_palace.h` |
| 999235 | `GO_WALL_OF_ICE_1` | 4 | `./src/server/scripts/Pandaria/ThroneOfThunder/boss_durumu.cpp` |
| 999236 | `GO_WALL_OF_ICE_2` | 4 | `./src/server/scripts/Pandaria/ThroneOfThunder/boss_durumu.cpp` |

## Structural / Windows (1)

| Id | Constant | Cost | File |
| --- | --- | --- | --- |
| 184069 | `GO_TEMPEST_BRIDDGE_WINDOW` | 4 | `./src/server/scripts/Outland/TempestKeep/Eye/the_eye.h` |

## Creatures usable as neighbourhood NPCs, pets or ambient life (221)

| Id | Constant | Role | File |
| --- | --- | --- | --- |
| 4823 | `NPC_BARBED_CRUSTACEAN` | Beds | `./src/server/scripts/Kalimdor/BlackfathomDeeps/blackfathom_deeps.h` |
| 20886 | `NPC_SOCCOTHRATES` | Beds | `./src/server/scripts/Outland/TempestKeep/arcatraz/arcatraz.h` |
| 108028 | `NPC_WEBBED_VICTIM_108028` | Beds | `./src/server/scripts/BrokenIsles/Scenario/scenario_artifact_karazhan_catacombs.cpp` |
| 108028 | `NPC_WEBBED_VICTIM_108028` | Beds | `./src/server/scripts/BrokenIsles/Scenario/scenario_artifact_ruins_of_falanaar.cpp` |
| 44930 | `NPC_DOOR_1_CREDIT` | Doors | `./src/server/scripts/Maelstrom/deepholm.cpp` |
| 44931 | `NPC_DOOR_2_CREDIT` | Doors | `./src/server/scripts/Maelstrom/deepholm.cpp` |
| 125104 | `NPC_LURA_DOOR` | Doors | `./src/server/scripts/BrokenIsles/SeatOfTriumvirate/seat_of_triumvirate.h` |
| 12717 | `NPC_MUGLASH` | Food and Drink | `./src/server/scripts/Kalimdor/zone_ashenvale.cpp` |
| 23700 | `NPC_FESTIVE_KEG_2` | Food and Drink | `./src/server/scripts/Events/brewfest.cpp` |
| 23702 | `NPC_FESTIVE_KEG_1` | Food and Drink | `./src/server/scripts/Events/brewfest.cpp` |
| 23706 | `NPC_FESTIVE_KEG_3` | Food and Drink | `./src/server/scripts/Events/brewfest.cpp` |
| 24372 | `NPC_FESTIVE_KEG_5` | Food and Drink | `./src/server/scripts/Events/brewfest.cpp` |
| 24373 | `NPC_FESTIVE_KEG_4` | Food and Drink | `./src/server/scripts/Events/brewfest.cpp` |
| 24710 | `NPC_IPFELKOFER_IRONKEG` | Food and Drink | `./src/server/scripts/World/areatrigger_scripts.cpp` |
| 24711 | `NPC_TAPPER_SWINDLEKEG` | Food and Drink | `./src/server/scripts/World/areatrigger_scripts.cpp` |
| 26529 | `NPC_MEATHOOK` | Food and Drink | `./src/server/scripts/Kalimdor/CavernsOfTime/CullingOfStratholme/culling_of_stratholme.h` |
| 47754 | `NPC_CAULDRON` | Food and Drink | `./src/server/scripts/EasternKingdoms/Deadmines/boss_captain_cookie.cpp` |
| 47754 | `NPC_COOKIES_CAULDRON` | Food and Drink | `./src/server/scripts/EasternKingdoms/Deadmines/deadmines.h` |
| 48297 | `NPC_MISTERY_MEAT` | Food and Drink | `./src/server/scripts/EasternKingdoms/Deadmines/boss_captain_cookie.cpp` |
| 48298 | `NPC_ROTTEN_MISTERY_MEAT` | Food and Drink | `./src/server/scripts/EasternKingdoms/Deadmines/boss_captain_cookie.cpp` |
| 48300 | `NPC_BREAD_LOAF` | Food and Drink | `./src/server/scripts/EasternKingdoms/Deadmines/boss_captain_cookie.cpp` |
| 61817 | `NPC_MANTID_TAR_KEG` | Food and Drink | `./src/server/scripts/Pandaria/SiegeNiuzaoTemple/siege_of_the_niuzao_temple.h` |
| 67573 | `NPC_MEATBALL` | Food and Drink | `./src/server/game/BrawlersGuild/BrawlersGuild.h` |
| 128956 | `NPC_BLOOD_TAINTED_CAULDRON` | Food and Drink | `./src/server/scripts/Zandalar/AtalDazar/boss_priestess_alunza.cpp` |
| 16343 | `NPC_SHADOWPINE_ORACLE` | Large Foliage | `./src/server/scripts/EasternKingdoms/zone_ghostlands.cpp` |
| 17375 | `NPC_STILLPINE_CAPITIVE` | Large Foliage | `./src/server/scripts/Kalimdor/zone_azuremyst_isle.cpp` |
| 25060 | `NPC_DARKSPINE_MYRIDON` | Large Foliage | `./src/server/scripts/EasternKingdoms/zone_isle_of_queldanas.cpp` |
| 37534 | `NPC_SPINESTALKER` | Large Foliage | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 53879 | `NPC_SPINE_OF_DEATHWING` | Large Foliage | `./src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/dragon_soul.h` |
| 54519 | `NPC_WINTER_VEIL_TREE` | Large Foliage | `./src/server/scripts/Events/winter_veil.cpp` |
| 79525 | `NPC_TREE` | Large Foliage | `./src/server/scripts/Draenor/zone_frostfire_ridge.cpp` |
| 79525 | `NPC_TREE` | Large Foliage | `./src/server/scripts/Draenor/zone_frostfire_ridge.cpp` |
| 103344 | `NPC_OAKHEART` | Large Foliage | `./src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.h` |
| 137043 | `NPC_ORB_OF_CORRUPTION_VISUAL_SOAK` | Large Foliage | `./src/server/scripts/Zandalar/Uldir/uldir.h` |
| 14776 | `NPC_TOWER_POINT_WARMASTER` | Large Structures | `./src/server/scripts/EasternKingdoms/AlteracValley/alterac_valley.cpp` |
| 15334 | `NPC_GIANT_EYE_TENTACLE` | Large Structures | `./src/server/scripts/Kalimdor/TempleOfAhnQiraj/temple_of_ahnqiraj.h` |
| 15725 | `NPC_CLAW_TENTACLE` | Large Structures | `./src/server/scripts/Kalimdor/TempleOfAhnQiraj/temple_of_ahnqiraj.h` |
| 15726 | `NPC_EYE_TENTACLE` | Large Structures | `./src/server/scripts/Kalimdor/TempleOfAhnQiraj/temple_of_ahnqiraj.h` |
| 15728 | `NPC_GIANT_CLAW_TENTACLE` | Large Structures | `./src/server/scripts/Kalimdor/TempleOfAhnQiraj/temple_of_ahnqiraj.h` |
| 15802 | `NPC_FLESH_TENTACLE` | Large Structures | `./src/server/scripts/Kalimdor/TempleOfAhnQiraj/temple_of_ahnqiraj.h` |
| 28366 | `NPC_WINTERGRASP_TOWER_CANNON` | Large Structures | `./src/server/game/Battlefield/Zones/BattlefieldWG.h` |
| 29178 | `NPC_LORD_MAXWELL_TYROSUS` | Large Structures | `./src/server/scripts/EasternKingdoms/ScarletEnclave/chapter5.cpp` |
| 29920 | `NPC_RUIN_DWELLER` | Large Structures | `./src/server/scripts/Northrend/Gundrak/gundrak.h` |
| 33943 | `NPC_INFLUENCE_TENTACLE` | Large Structures | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 33966 | `NPC_CRUSHER_TENTACLE` | Large Structures | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 33983 | `NPC_CONSTRICTOR_TENTACLE` | Large Structures | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 33985 | `NPC_CORRUPTOR_TENTACLE` | Large Structures | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 33991 | `NPC_SANITY_WELL` | Large Structures | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 43590 | `NPC_BLACKROCK_TOWER` | Large Structures | `./src/server/scripts/EasternKingdoms/zone_redridge_mountains.cpp` |
| 44586 | `NPC_BAD_INTENTIONS_TARGET` | Large Structures | `./src/server/scripts/Kalimdor/LostCityOfTheTolvir/boss_general_husam.cpp` |
| 44824 | `NPC_GRAVITY_WELL` | Large Structures | `./src/server/scripts/EasternKingdoms/BastionOfTwilight/boss_council_of_ascendents.cpp` |
| 45467 | `NPC_TROGG_DWELLER` | Large Structures | `./src/server/scripts/EasternKingdoms/GrimBatol/boss_general_umbriss.cpp` |
| 45492 | `NPC_TOWER_RANGE_FINDER` | Large Structures | `./src/server/game/Battlefield/Zones/BattlefieldTB.h` |
| 45561 | `NPC_TOWER_CANNON_TARGET` | Large Structures | `./src/server/game/Battlefield/Zones/BattlefieldTB.h` |
| 50264 | `NPC_SPIKED_TENTACLE` | Large Structures | `./src/server/scripts/EasternKingdoms/BastionOfTwilight/boss_chogall.cpp` |
| 54696 | `NPC_FLAILING_TENTACLE` | Large Structures | `./src/server/scripts/Kalimdor/CavernsOfTime/HourofTwilight/hour_of_twilight.h` |
| 55971 | `NPC_DEATHWING_TOWER` | Large Structures | `./src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/dragon_soul.h` |
| 56144 | `NPC_PORTENT_OF_TWILIGHT` | Large Structures | `./src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/dragon_soul.h` |
| 56167 | `NPC_ARM_TENTACLE_2` | Large Structures | `./src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/boss_madness_of_deathwing.cpp` |
| 56168 | `NPC_WING_TENTACLE` | Large Structures | `./src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/boss_madness_of_deathwing.cpp` |
| 56188 | `NPC_BLISTERING_TENTACLE` | Large Structures | `./src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/boss_madness_of_deathwing.cpp` |
| 56724 | `NPC_IMPALING_TENTACLE` | Large Structures | `./src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/boss_madness_of_deathwing.cpp` |
| 56844 | `NPC_TAIL_TENTACLE` | Large Structures | `./src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/boss_madness_of_deathwing.cpp` |
| 56846 | `NPC_ARM_TENTACLE_1` | Large Structures | `./src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/boss_madness_of_deathwing.cpp` |
| 57693 | `NPC_COSMETIC_TENTACLE` | Large Structures | `./src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/boss_madness_of_deathwing.cpp` |
| 57751 | `NPC_RUIN_TENTACLE` | Large Structures | `./src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/dragon_soul.cpp` |
| 57836 | `NPC_TENTACLE_TOSS_STALKER` | Large Structures | `./src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/dragon_soul.cpp` |
| 59262 | `NPC_WARLOCK_DEMONIC_GATEWAY_GREEN` | Large Structures | `./src/server/scripts/Spells/spell_warlock.cpp` |
| 59271 | `NPC_WARLOCK_DEMONIC_GATEWAY_PURPLE` | Large Structures | `./src/server/scripts/Spells/spell_warlock.cpp` |
| 73565 | `NPC_TOWER_LEFT` | Large Structures | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/boss_galakras.cpp` |
| 73628 | `NPC_TOWER_RIGHT` | Large Structures | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/boss_galakras.cpp` |
| 73670 | `NPC_GATECRUSHER_SAVAH` | Large Structures | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar_part_d.cpp` |
| 92909 | `NPC_LORD_MAXWELL_TYROSUS` | Large Structures | `./src/server/scripts/BrokenIsles/zone_dalaran_legion.cpp` |
| 92909 | `NPC_LORD_MAXWELL_TYROSUS` | Large Structures | `./src/server/scripts/BrokenIsles/ClassHalls/class_hall_paladin.cpp` |
| 94406 | `NPC_LEGION_GATEWAY_KILL_CREDIT` | Large Structures | `./src/server/scripts/BrokenIsles/DemonHunterZones/zone_mardum.cpp` |
| 98363 | `NPC_GRASPING_TENTACLE_3` | Large Structures | `./src/server/scripts/BrokenIsles/MawOfSouls/boss_helya_maw.cpp` |
| 99800 | `NPC_GRASPING_TENTACLE_DUMMY_3` | Large Structures | `./src/server/scripts/BrokenIsles/MawOfSouls/boss_helya_maw.cpp` |
| 99801 | `NPC_DESTRUCTOR_TENTACLE` | Large Structures | `./src/server/scripts/BrokenIsles/MawOfSouls/boss_helya_maw.cpp` |
| 99803 | `NPC_DESTRUCTOR_TENTACLE_DUMMY` | Large Structures | `./src/server/scripts/BrokenIsles/MawOfSouls/boss_helya_maw.cpp` |
| 100188 | `NPC_PIERCING_TENTACLE` | Large Structures | `./src/server/scripts/BrokenIsles/MawOfSouls/boss_helya_maw.cpp` |
| 100359 | `NPC_GRASPING_TENTACLE_DUMMY` | Large Structures | `./src/server/scripts/BrokenIsles/MawOfSouls/boss_helya_maw.cpp` |
| 100360 | `NPC_GRASPING_TENTACLE` | Large Structures | `./src/server/scripts/BrokenIsles/MawOfSouls/boss_helya_maw.cpp` |
| 100361 | `NPC_GRASPING_TENTACLE_DUMMY_2` | Large Structures | `./src/server/scripts/BrokenIsles/MawOfSouls/boss_helya_maw.cpp` |
| 100362 | `NPC_GRASPING_TENTACLE_2` | Large Structures | `./src/server/scripts/BrokenIsles/MawOfSouls/boss_helya_maw.cpp` |
| 104592 | `NPC_NIGHTMARE_TENTACLE` | Large Structures | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 104738 | `NPC_THE_NIGHTWELL` | Large Structures | `./src/server/scripts/BrokenIsles/TheNighthold/thenighthold.h` |
| 105304 | `NPC_DOMINATOR_TENTACLE` | Large Structures | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 105322 | `NPC_DEATHGLARE_TENTACLE` | Large Structures | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 105383 | `NPC_CORRUPTOR_TENTACLE` | Large Structures | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 111109 | `NPC_EMISSARY_AULDBRIDGE_111109` | Large Structures | `./src/server/scripts/BrokenIsles/zone_dalaran_legion.cpp` |
| 111439 | `NPC_TWISTING_SHADOWS_BRIDGE` | Large Structures | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 114900 | `NPC_GRIPPING_TENTACLE` | Large Structures | `./src/server/scripts/BrokenIsles/TrialOfValor/trial_of_valor.h` |
| 122494 | `NPC_GATEWAY_XOROTH_ENTERING` | Large Structures | `./src/server/scripts/BrokenIsles/AntorusTheBurningThrone/boss_hasabel.cpp` |
| 122533 | `NPC_GATEWAY_XOROTH_LEAVING` | Large Structures | `./src/server/scripts/BrokenIsles/AntorusTheBurningThrone/boss_hasabel.cpp` |
| 122543 | `NPC_GATEWAY_RANCORA_ENTERING` | Large Structures | `./src/server/scripts/BrokenIsles/AntorusTheBurningThrone/boss_hasabel.cpp` |
| 122555 | `NPC_GATEWAY_RANCORA_LEAVING` | Large Structures | `./src/server/scripts/BrokenIsles/AntorusTheBurningThrone/boss_hasabel.cpp` |
| 122558 | `NPC_GATEWAY_NATHREZA_ENTERING` | Large Structures | `./src/server/scripts/BrokenIsles/AntorusTheBurningThrone/boss_hasabel.cpp` |
| 122559 | `NPC_GATEWAY_NATHREZA_LEAVING` | Large Structures | `./src/server/scripts/BrokenIsles/AntorusTheBurningThrone/boss_hasabel.cpp` |
| 128649 | `NPC_SERGEANT_BAINBRIDGE` | Large Structures | `./src/server/scripts/KulTiras/SiegeOfBoralus/siege_of_boralus.h` |
| 134612 | `NPC_GRASPING_TENTACLES` | Large Structures | `./src/server/scripts/KulTiras/ShrineOfTheStorm/boss_aqusirr.cpp` |
| 135987 | `NPC_TENTACLE` | Large Structures | `./src/server/scripts/KulTiras/ShrineOfTheStorm/boss_volzith_the_whisperer.cpp` |
| 152192 | `NPC_FEAR_GATE` | Large Structures | `./src/server/scripts/Nazjatar/EternalPalace/eternal_palace.h` |
| 152574 | `NPC_TENTACLE_DELIRIUM` | Large Structures | `./src/server/scripts/Nazjatar/EternalPalace/eternal_palace.h` |
| 153335 | `NPC_POTENT_SPARK` | Large Structures | `./src/server/scripts/Nazjatar/EternalPalace/eternal_palace.h` |
| 157614 | `NPC_TENTACLE_OF_DRESTAGATH` | Large Structures | `./src/server/scripts/Nyalotha/nyalotha.h` |
| 158367 | `NPC_BASHER_TENTACLE` | Large Structures | `./src/server/scripts/Nyalotha/nyalotha.h` |
| 158375 | `NPC_CORRUPTOR_TENTACLE` | Large Structures | `./src/server/scripts/Nyalotha/nyalotha.h` |
| 160249 | `NPC_SPIKE_TENTACLE` | Large Structures | `./src/server/scripts/Nyalotha/nyalotha.h` |
| 161148 | `NPC_DARK_GATEWAY` | Large Structures | `./src/server/scripts/Nyalotha/nyalotha.h` |
| 162159 | `NPC_THRASHING_TENTACLE` | Large Structures | `./src/server/scripts/Nyalotha/nyalotha.h` |
| 30891 | `NPC_BLESSED_BANNER` | Ornamental | `./src/server/scripts/Northrend/zone_icecrown.cpp` |
| 33962 | `NPC_TURNED_CHAMPION` | Ornamental | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 36823 | `NPC_TERENAS_MENETHIL_FROSTMOURNE` | Ornamental | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 39217 | `NPC_TERENAS_MENETHIL_FROSTMOURNE_H` | Ornamental | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 39892 | `NPC_ENSLAVED_BURNING_EMBER` | Ornamental | `./src/server/scripts/EasternKingdoms/GrimBatol/grim_batol.h` |
| 42046 | `NPC_BURNING_BOMBER_CREDIT` | Ornamental | `./src/server/scripts/EasternKingdoms/zone_dun_morogh.cpp` |
| 42704 | `NPC_MASK_BURNT_CREDIT` | Ornamental | `./src/server/scripts/EasternKingdoms/zone_hinterlands.cpp` |
| 53216 | `NPC_BURNING_ORB` | Ornamental | `./src/server/scripts/Kalimdor/Firelands/boss_majordomo_staghelm.cpp` |
| 53375 | `NPC_HERALD_OF_THE_BURNING_END` | Ornamental | `./src/server/scripts/Kalimdor/Firelands/boss_alysrazor.cpp` |
| 56341 | `NPC_BURNING_TENDONS_1` | Ornamental | `./src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/boss_spine_of_deathwing.cpp` |
| 56575 | `NPC_BURNING_TENDONS_2` | Ornamental | `./src/server/scripts/Kalimdor/CavernsOfTime/DragonSoul/boss_spine_of_deathwing.cpp` |
| 65736 | `NPC_RETURN_TO_TERRACE` | Ornamental | `./src/server/scripts/Pandaria/TerraceofEndlessSpring/terrace_of_endless_spring.h` |
| 69467 | `NPC_JINROKH_STATUE` | Ornamental | `./src/server/scripts/Pandaria/ThroneOfThunder/throne_of_thunder.h` |
| 71382 | `NPC_BURIAL_URN` | Ornamental | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/boss_spoils_of_pandaria.cpp` |
| 71626 | `NPC_KORKRON_BANNER` | Ornamental | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/boss_general_nazgrim.cpp` |
| 72357 | `NPC_LIEUTENANT_KRUGRUK` | Ornamental | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/boss_galakras.cpp` |
| 72535 | `NPC_STONE_STATUE` | Ornamental | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/boss_spoils_of_pandaria.cpp` |
| 105103 | `NPC_BURNING_CRUSHER` | Ornamental | `./src/server/scripts/BrokenIsles/Scenario/scenario_artifact_brokenshore.cpp` |
| 115023 | `NPC_BURNSTACHIO` | Ornamental | `./src/server/game/BrawlersGuild/BrawlersGuild.h` |
| 136436 | `NPC_BURNING_SOUL` | Ornamental | `./src/server/scripts/KulTiras/WaycrestManor/waycrest_manor.h` |
| 33114 | `NPC_FLAME_LEVIATHAN_SEAT` | Seating | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 33114 | `NPC_SEAT` | Seating | `./src/server/scripts/Northrend/Ulduar/Ulduar/boss_flame_leviathan.cpp` |
| 34812 | `NPC_THE_TURKEY_CHAIR` | Seating | `./src/server/scripts/Events/pilgrims_bounty.cpp` |
| 34819 | `NPC_THE_STUFFING_CHAIR` | Seating | `./src/server/scripts/Events/pilgrims_bounty.cpp` |
| 34822 | `NPC_THE_PIE_CHAIR` | Seating | `./src/server/scripts/Events/pilgrims_bounty.cpp` |
| 34823 | `NPC_THE_CRANBERRY_CHAIR` | Seating | `./src/server/scripts/Events/pilgrims_bounty.cpp` |
| 34824 | `NPC_THE_SWEET_POTATO_CHAIR` | Seating | `./src/server/scripts/Events/pilgrims_bounty.cpp` |
| 51391 | `NPC_THRONE_TELEPORTER_2` | Seating | `./src/server/scripts/EasternKingdoms/ThroneOfTheTides/throne_of_the_tides.h` |
| 51395 | `NPC_THRONE_TELEPORTER_1` | Seating | `./src/server/scripts/EasternKingdoms/ThroneOfTheTides/throne_of_the_tides.h` |
| 54247 | `NPC_CIRCLE_OF_THRONES_PORTAL` | Seating | `./src/server/scripts/Kalimdor/Firelands/firelands.h` |
| 102498 | `NPC_KILL_CREDIT_TAKE_YOUR_SEAT_01` | Seating | `./src/server/scripts/BrokenIsles/ClassHalls/class_hall_rogue.cpp` |
| 102502 | `NPC_KILL_CREDIT_TAKE_YOUR_SEAT_02` | Seating | `./src/server/scripts/BrokenIsles/ClassHalls/class_hall_rogue.cpp` |
| 89 | `NPC_INFERNAL` | Small Foliage | `./src/server/scripts/Spells/spell_generic.cpp` |
| 1981 | `NPC_DARK_IRON_AMBUSHER` | Small Foliage | `./src/server/scripts/EasternKingdoms/zone_loch_modan.cpp` |
| 6207 | `NPC_CAVERNDEEP_AMBUSHER` | Small Foliage | `./src/server/scripts/EasternKingdoms/Gnomeregan/gnomeregan.h` |
| 7809 | `NPC_VILE_AMBUSHER` | Small Foliage | `./src/server/scripts/EasternKingdoms/zone_hinterlands.cpp` |
| 13301 | `NPC_HIVE_AMBUSHER` | Small Foliage | `./src/server/scripts/World/go_scripts.cpp` |
| 21419 | `NPC_INFERNAL_ATTACKER` | Small Foliage | `./src/server/scripts/Outland/zone_shadowmoon_valley.cpp` |
| 21865 | `NPC_COILFANG_AMBUSHER` | Small Foliage | `./src/server/scripts/Outland/CoilfangReservoir/SerpentShrine/boss_lurker_below.cpp` |
| 30391 | `NPC_HEALTHY_MUSHROOM` | Small Foliage | `./src/server/scripts/Northrend/AzjolNerub/Ahnkahet/ahnkahet.h` |
| 30435 | `NPC_POISONOUS_MUSHROOM` | Small Foliage | `./src/server/scripts/Northrend/AzjolNerub/Ahnkahet/ahnkahet.h` |
| 34813 | `NPC_INFERNAL_VOLCANO` | Small Foliage | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/boss_lord_jaraxxus.cpp` |
| 34815 | `NPC_FEL_INFERNAL` | Small Foliage | `./src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/boss_lord_jaraxxus.cpp` |
| 38451 | `NPC_BALL_OF_INFERNO_FLAME` | Small Foliage | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 40668 | `NPC_LIVING_VINE` | Small Foliage | `./src/server/scripts/Kalimdor/HallsOfOrigination/halls_of_origination.h` |
| 40681 | `NPC_LIVING_INFERNO` | Small Foliage | `./src/server/scripts/Northrend/ChamberOfAspects/RubySanctum/ruby_sanctum.h` |
| 44262 | `NPC_MOSSHIDE_REP` | Small Foliage | `./src/server/scripts/EasternKingdoms/zone_loch_modan.cpp` |
| 44266 | `NPC_MOSSHIDE_CREDIT` | Small Foliage | `./src/server/scripts/EasternKingdoms/zone_loch_modan.cpp` |
| 47501 | `NPC_INFERNO_RUSH` | Small Foliage | `./src/server/scripts/EasternKingdoms/BastionOfTwilight/boss_council_of_ascendents.cpp` |
| 56036 | `NPC_INFERNAL` | Small Foliage | `./src/server/scripts/Kalimdor/CavernsOfTime/WellOfEternity/boss_mannoroth.cpp` |
| 81522 | `NPC_WITHERBARK` | Small Foliage | `./src/server/scripts/Draenor/Everbloom/the_everbloom.h` |
| 96159 | `NPC_COLOSSAL_INFERNAL` | Small Foliage | `./src/server/scripts/BrokenIsles/DemonHunterZones/zone_mardum.cpp` |
| 102398 | `NPC_BLAZING_INFERNAL` | Small Foliage | `./src/server/scripts/BrokenIsles/VioletHold/violet_hold_assault.h` |
| 103095 | `NPC_CORRUPTED_MUSHROOM_SMALL` | Small Foliage | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 103096 | `NPC_CORRUPTED_MUSHROOM_MEDIUM` | Small Foliage | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 103097 | `NPC_CORRUPTED_MUSHROOM_BIG` | Small Foliage | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 107969 | `NPC_VILE_AMBUSH_STALKER` | Small Foliage | `./src/server/scripts/BrokenIsles/EmeraldNightmare/emerald_nightmare.h` |
| 112668 | `NPC_INFERNAL_IMP` | Small Foliage | `./src/server/scripts/BrokenIsles/CourtOfStars/court_of_stars.h` |
| 116976 | `NPC_INFERNAL_SPIKE` | Small Foliage | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 119950 | `NPC_BRIMSTONE_INFERNAL` | Small Foliage | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
| 120014 | `NPC_CHOKING_VINES` | Small Foliage | `./src/server/scripts/BrokenIsles/CathedralEternalNight/cathedral_of_eternal_night.h` |
| 27273 | `NPC_FLAME_BRAZIER` | Small Lights | `./src/server/scripts/Northrend/UtgardeKeep/UtgardePinnacle/boss_svala.cpp` |
| 37184 | `NPC_ZAFOD_BOOMBOX` | Storage | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 52866 | `NPC_CRATER` | Storage | `./src/server/scripts/Kalimdor/Firelands/boss_lord_rhyolith.cpp` |
| 54546 | `NPC_BARREL` | Storage | `./src/server/scripts/DarkmoonIsland/DarkmoonWhackGnoll.cpp` |
| 56731 | `NPC_BARREL` | Storage | `./src/server/scripts/Pandaria/StormstoutBrewery/stormstout_brewery.cpp` |
| 72154 | `NPC_DESECRATED_WEAPON` | Storage | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/boss_garrosh_hellscream.cpp` |
| 72198 | `NPC_EMPOWERED_DESECRATED_WEAPON` | Storage | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/boss_garrosh_hellscream.cpp` |
| 129437 | `NPC_MUNITIONS_BARREL` | Storage | `./src/server/scripts/KulTiras/TolDagor/tol_dagor.h` |
| 147000 | `NPC_MUNITIONS_BARREL` | Storage | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 28166 | `NPC_UNSTABLE_SPHERE` | Tables and Desks | `./src/server/scripts/Northrend/Nexus/Oculus/boss_drakos.cpp` |
| 32823 | `NPC_BOUNTIFUL_TABLE` | Tables and Desks | `./src/server/scripts/Events/pilgrims_bounty.cpp` |
| 33050 | `NPC_UNSTABLE_SUN_BEAM` | Tables and Desks | `./src/server/scripts/Northrend/Ulduar/Ulduar/boss_freya.cpp` |
| 33487 | `NPC_IRONBAND_TABLET` | Tables and Desks | `./src/server/scripts/EasternKingdoms/zone_loch_modan.cpp` |
| 38107 | `NPC_UNSTABLE_EXPLOSION_STALKER` | Tables and Desks | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 46492 | `NPC_UNSTABLE_GROUNDING_FIELD` | Tables and Desks | `./src/server/scripts/Kalimdor/TheVortexPinnacle/the_vortex_pinnacle.h` |
| 53630 | `NPC_UNSTABLE_PYRELORD` | Tables and Desks | `./src/server/scripts/Kalimdor/Firelands/firelands.cpp` |
| 53901 | `NPC_UNSTABLE_MAGMA` | Tables and Desks | `./src/server/scripts/Kalimdor/Firelands/firelands.cpp` |
| 56917 | `NPC_STABLE_MUNITION` | Tables and Desks | `./src/server/scripts/Pandaria/GateSettingSun/gate_setting_sun.h` |
| 73104 | `NPC_UNSTABLE_SPARK` | Tables and Desks | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/boss_spoils_of_pandaria.cpp` |
| 99240 | `NPC_COUNTERMEASURES` | Tables and Desks | `./src/server/scripts/BrokenIsles/VaultOfTheWardens/vault_of_the_wardens.h` |
| 137123 | `NPC_CANNON_VIQ_ENCOUNTER` | Tables and Desks | `./src/server/scripts/KulTiras/SiegeOfBoralus/siege_of_boralus.h` |
| 143618 | `NPC_UNSTABLE_ORDNACE` | Tables and Desks | `./src/server/scripts/KulTiras/SiegeOfBoralus/siege_of_boralus.h` |
| 145644 | `NPC_BWONSAMDI_ENCOUNTER` | Tables and Desks | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 155354 | `NPC_AZSHARAS_INDOMITABLE` | Tables and Desks | `./src/server/scripts/Nazjatar/EternalPalace/eternal_palace.h` |
| 157235 | `NPC_UNSTABLE_VOID_STALKER` | Tables and Desks | `./src/server/scripts/Nyalotha/nyalotha.h` |
| 27979 | `NPC_FORGED_IRON_TROGG` | Utility | `./src/server/scripts/Northrend/Ulduar/HallsOfStone/boss_sjonnir.cpp` |
| 27982 | `NPC_FORGED_IRON_DWARF` | Utility | `./src/server/scripts/Northrend/Ulduar/HallsOfStone/boss_sjonnir.cpp` |
| 28525 | `NPC_NEW_AVALON_FORGE` | Utility | `./src/server/scripts/Spells/spell_quest.cpp` |
| 28823 | `NPC_VOLKHAN_ANVIL` | Utility | `./src/server/scripts/Northrend/Ulduar/HallsOfLightning/boss_volkhan.cpp` |
| 29240 | `NPC_STORMFORGED_LIEUTENANT` | Utility | `./src/server/scripts/Northrend/Ulduar/HallsOfLightning/boss_bjarngrim.cpp` |
| 29254 | `NPC_SILVER_COVENANT_GUARDIAN_MAGE` | Utility | `./src/server/scripts/Northrend/zone_dalaran.cpp` |
| 29861 | `NPC_STORMFORGED_ERADICTOR` | Utility | `./src/server/scripts/World/areatrigger_scripts.cpp` |
| 29862 | `NPC_STORMFORGED_MONITOR` | Utility | `./src/server/scripts/World/areatrigger_scripts.cpp` |
| 33236 | `NPC_STEELFORGED_DEFFENDER` | Utility | `./src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h` |
| 33316 | `NPC_IRONFORGE_RAM` | Utility | `./src/server/scripts/Spells/spell_generic.cpp` |
| 36495 | `NPC_FORGEMASTER_STALKER` | Utility | `./src/server/scripts/Northrend/FrozenHalls/PitOfSaron/pit_of_saron.h` |
| 74218 | `NPC_IRONFORGE_GUARD` | Utility | `./src/server/scripts/Pandaria/SiegeOfOrgrimmar/siege_of_orgrimmar.hpp` |
| 145903 | `NPC_DARKFORGED_CRUSADER` | Utility | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 447175 | `NPC_FORGE_OF_THE_GUARDIAN_447175` | Utility | `./src/server/scripts/BrokenIsles/Scenario/scenario_the_violet_hold.cpp` |
| 37014 | `NPC_ICE_WALL_TARGET` | Walls and Columns | `./src/server/scripts/Northrend/FrozenHalls/HallsOfReflection/halls_of_reflection.h` |
| 37918 | `NPC_COLUMN_OF_FROST` | Walls and Columns | `./src/server/scripts/Northrend/IcecrownCitadel/icecrown_citadel.h` |
| 40859 | `NPC_FIRE_WALL` | Walls and Columns | `./src/server/scripts/EasternKingdoms/Deadmines/deadmines.h` |
| 72020 | `NPC_JADEFIRE_WALL` | Walls and Columns | `./src/server/scripts/Pandaria/timeless_isle.h` |
| 135759 | `NPC_EARTHWALL_TOTEM` | Walls and Columns | `./src/server/scripts/Zandalar/King's Rest/kings_rest.h` |
| 146558 | `NPC_PTERRODAX_STORMWALL` | Walls and Columns | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 148890 | `NPC_JAINA_WALL_OF_ICE` | Walls and Columns | `./src/server/scripts/Zandalar/BattleOfDazarAlor/battle_of_dazaralor.h` |
| 999250 | `NPC_WALL_OF_ICE_1` | Walls and Columns | `./src/server/scripts/Pandaria/ThroneOfThunder/boss_durumu.cpp` |
| 999251 | `NPC_WALL_OF_ICE_2` | Walls and Columns | `./src/server/scripts/Pandaria/ThroneOfThunder/boss_durumu.cpp` |
| 999252 | `NPC_WALL_OF_ICE_3` | Walls and Columns | `./src/server/scripts/Pandaria/ThroneOfThunder/boss_durumu.cpp` |
| 121605 | `NPC_BREACH_IN_WINDOW` | Windows | `./src/server/scripts/BrokenIsles/TombOfSargeras/tomb_of_sargeras.h` |
