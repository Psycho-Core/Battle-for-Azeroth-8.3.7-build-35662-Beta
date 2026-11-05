#!/usr/bin/env python3
"""Build the progression content every draft shares.

    from build_progression import progression_sql
    sql = progression_sql(pet_pieces)

This is the part of the module that is the same in both drafts: the twelve-step level
ladder, the exterior shells, the rooms inside them, the portal destination list, and the
four Endeavors with their tasks and milestone rewards.

Two rules run through it, both learned the hard way in this folder:

  * every id that belongs to the realm's own content (a shell object, a portal destination,
    an Endeavor target) is left at 0 here and carries the *name* the bind statement
    searches for. A number typed from memory is a wrong number, and a wrong target is worse
    than a missing one because it completes on the wrong kill;
  * anything I could not verify is commented as such, in the row, not in a changelog.
"""

# Retail's ladder. XP thresholds are retail's. The three budgets are retail's endpoints
# (910 to 5975 indoor, 19 to 134 room, 200 to 350 exterior) with the eleven steps between
# them filled in as a straight line - the guide publishes the ends, not the middle.
LEVELS = [
    # level, xp, indoor, room, outdoor, rooms
    (1, 0, 910, 19, 200, 2),
    (2, 10, 1370, 29, 214, 3),
    (3, 1200, 1831, 40, 227, 4),
    (4, 2400, 2291, 50, 241, 5),
    (5, 3700, 2752, 61, 255, 6),
    (6, 5700, 3212, 71, 268, 7),
    (7, 7900, 3672, 82, 282, 8),
    (8, 10300, 4133, 92, 295, 9),
    (9, 12900, 4593, 103, 309, 10),
    (10, 15750, 5054, 113, 323, 12),
    (11, 18850, 5514, 124, 336, 15),
    (12, 22200, 5975, 134, 350, 18),
]

# shell id, name, search stem, size class, faction mask, level, flags
SHELLS = [
    (1, "Cottage", "House", 0, 1, 1, 0),
    (2, "Townhouse", "House", 1, 1, 1, 0),
    (3, "Manor", "House", 2, 1, 8, 0),
    (4, "Cottage", "House", 0, 2, 1, 0),
    (5, "Townhouse", "House", 1, 2, 1, 0),
    (6, "Manor", "House", 2, 2, 8, 0),
]

# Rooms are areas of the interior, not separate geometry: a room is a circle around an
# offset from the interior anchor, it costs room budget, and it is the area a player has to
# stand in to decorate. shell_id 0 = every shell has this room.
ROOMS = [
    ("Entry Hall", 0.0, 0.0, 0.0, 14.0, 1, 1),
    ("Great Room", 0.0, 22.0, 0.0, 18.0, 3, 1),
    ("Kitchen", -22.0, 0.0, 0.0, 12.0, 3, 2),
    ("Bedroom", 22.0, 0.0, 0.0, 12.0, 3, 2),
    ("Bath", 22.0, 22.0, 0.0, 9.0, 2, 3),
    ("Study", -22.0, 22.0, 0.0, 11.0, 4, 3),
    ("Cellar", 0.0, -22.0, -4.0, 12.0, 4, 4),
    ("Armory", -44.0, 0.0, 0.0, 13.0, 6, 4),
    ("Workshop", 44.0, 0.0, 0.0, 13.0, 6, 5),
    ("Trophy Room", 0.0, 44.0, 0.0, 14.0, 7, 5),
    ("Library", -44.0, 22.0, 0.0, 13.0, 6, 6),
    ("Gallery", 44.0, 22.0, 0.0, 13.0, 6, 6),
    ("Conservatory", -44.0, -22.0, 0.0, 15.0, 8, 7),
    ("Barracks", 44.0, -22.0, 0.0, 15.0, 8, 8),
    ("Vault", 0.0, -44.0, 0.0, 10.0, 8, 9),
    ("Chapel", 22.0, 44.0, 0.0, 12.0, 7, 9),
    ("Ballroom", -22.0, 44.0, 0.0, 20.0, 10, 10),
    ("Terrace", 0.0, 66.0, 0.0, 24.0, 12, 11),
]

# The portal amenity's destinations. Position comes from the realm's own `game_tele` table
# by name - game_tele is exactly "a named place with a map and coordinates", which is what a
# portal is. A name the realm does not have stays at 0 and the destination is not offered.
PORTALS = [
    ("Stormwind", "Stormwind", 1, 1),
    ("Ironforge", "Ironforge", 1, 1),
    ("Darnassus", "Darnassus", 1, 1),
    ("Orgrimmar", "Orgrimmar", 2, 1),
    ("Thunder Bluff", "Thunder Bluff", 2, 1),
    ("Undercity", "Undercity", 2, 1),
    ("Shattrath", "Shattrath", 3, 1),
    ("Dalaran", "Dalaran", 3, 1),
    ("Boralus", "Boralus", 1, 1),
    ("Dazar'alor", "Dazar'alor", 2, 1),
]

# Endeavors. `target` is 0 and `target_name` carries what to look for: the bind statement
# fills it from creature_template / quest_template by name.
# (type, target_name, required, text)
#
# Only four of the six task types are used: this core has hooks for a creature kill, a
# quest turn-in and a pet battle, and a delivery is turned in by hand. There is no
# item-gained hook, so TaskLootItem and TaskCraftItem are defined and never emitted -
# counting them would mean touching the core, which this module does not do.
ENDEAVORS = [
    (1, "Harvest Home", "The neighbourhood fills its larder for the season.", 30, 1, [
        (0, "Kobold Vermin", 10, "Kill 10 Kobold Vermin"),
        (0, "Harvest Golem", 5, "Kill 5 Harvest Golems"),
        (0, "Boar", 10, "Kill 10 Boars"),
        (2, "A House For You", 1, "Complete A House For You"),
        (5, "", 3, "Win 3 pet battles"),
        (6, "", 5, "Deliver 5 loads at the Coffer"),
    ]),
    (2, "The Hunters' Gate", "Something is taking the livestock at night.", 30, 1, [
        (0, "Wolf", 10, "Kill 10 Wolves"),
        (0, "Hogger", 1, "Kill Hogger"),
        (1, "Edwin VanCleef", 1, "Defeat Edwin VanCleef"),
        (0, "Defias Pillager", 10, "Kill 10 Defias Pillagers"),
        (2, "First Home", 1, "Complete First Home"),
        (2, "Welcome Home", 1, "Complete Welcome Home"),
    ]),
    (3, "Winter Veil Warmth", "The neighbourhood decorates for the holiday.", 30, 1, [
        (1, "The Abominable Greench", 1, "Defeat the Abominable Greench"),
        (2, "Time to Decorate", 1, "Complete Time to Decorate"),
        (5, "", 5, "Win 5 pet battles"),
        (6, "", 10, "Deliver 10 loads at the Coffer"),
        (0, "Winterfall Den Watcher", 10, "Kill 10 Winterfall Den Watchers"),
        (0, "Frostsaber", 10, "Kill 10 Frostsabers"),
    ]),
    (4, "Deep Places", "The old things under the rock are stirring again.", 30, 1, [
        (1, "Onyxia", 1, "Defeat Onyxia"),
        (1, "N'Zoth the Corruptor", 1, "Defeat N'Zoth the Corruptor"),
        (0, "Faceless One", 10, "Kill 10 Faceless Ones"),
        (1, "C'Thun", 1, "Kill C'Thun"),
        (5, "", 8, "Win 8 pet battles"),
        (6, "", 5, "Deliver 5 loads at the Coffer"),
    ]),
]

# Milestone rewards: XP, Community Coupons, and the milestone that unlocks stock up to it.
REWARDS = [
    (1, 1, 500, 100, 3363, 25),
    (1, 2, 1000, 200, 3363, 50),
    (1, 3, 2000, 350, 3363, 100),
    (1, 4, 4000, 500, 3363, 200),
    (2, 1, 500, 100, 3363, 25),
    (2, 2, 1000, 200, 3363, 50),
    (2, 3, 2000, 350, 3363, 100),
    (2, 4, 4000, 500, 3363, 200),
    (3, 1, 500, 100, 3363, 25),
    (3, 2, 1000, 200, 3363, 50),
    (3, 3, 2000, 350, 3363, 100),
    (3, 4, 4000, 500, 3363, 200),
    (4, 1, 500, 100, 3363, 25),
    (4, 2, 1000, 200, 3363, 50),
    (4, 3, 2000, 350, 3363, 100),
    (4, 4, 4000, 500, 3363, 200),
]

# Creature search names for the decor that owns a creature. A piece is listed here by the
# name of the creature it should hold, not by id: the bind statement decides.
PET_DECOR_NAMES = ("Pepe", "Cat", "Dog", "Cockroach", "Whelpling", "Puppy")


def esc(text):
    return text.replace("'", "''")


def progression_sql(pet_pieces):
    """Return the whole progression file as one string.

    pet_pieces: iterable of (decor_id, decor_name) for decor rows that are a pet bed.
    """
    out = []
    add = out.append

    add("-- ---------------------------------------------------------------------------")
    add("-- Progression content: the level ladder, shells, rooms, portals and Endeavors.")
    add("--")
    add("-- Every realm-owned id in this file is 0 and carries the name to search for.")
    add("-- tools/sql/bind_housing_content.sql fills them from the realm's own tables and")
    add("-- reports what it could not find; nothing here is a number typed from memory.")
    add("-- ---------------------------------------------------------------------------")
    add("")

    add("DELETE FROM `housing_house_level`;")
    add("INSERT INTO `housing_house_level` (`level`, `xp`, `budget_indoor`, `budget_room`, "
        "`budget_outdoor`, `rooms`) VALUES")
    rows = []
    for level, xp, indoor, room, outdoor, rooms in LEVELS:
        rows.append("({}, {}, {}, {}, {}, {})".format(level, xp, indoor, room, outdoor, rooms))
    add(",\n".join(rows) + ";")
    add("")

    add("DELETE FROM `housing_shell` WHERE `id` BETWEEN 1 AND 12;")
    add("INSERT INTO `housing_shell` (`id`, `name`, `gameobject_entry`, `go_name`, "
        "`size_class`, `faction_mask`, `level_required`, `flags`) VALUES")
    rows = []
    for sid, name, stem, size, mask, level, flags in SHELLS:
        rows.append("({}, '{}', 0, '{}', {}, {}, {}, {})".format(
            sid, esc(name), esc(stem), size, mask, level, flags))
    add(",\n".join(rows) + ";")
    add("")

    add("DELETE FROM `housing_room` WHERE `id` BETWEEN 1 AND 32;")
    add("INSERT INTO `housing_room` (`id`, `shell_id`, `name`, `x`, `y`, `z`, `radius`, "
        "`budget_cost`, `level_required`) VALUES")
    rows = []
    for i, (name, x, y, z, radius, cost, level) in enumerate(ROOMS):
        rows.append("({}, 0, '{}', {}, {}, {}, {}, {}, {})".format(
            i + 1, esc(name), x, y, z, radius, cost, level))
    add(",\n".join(rows) + ";")
    add("")
    add("-- shell_id 0 on a room means every shell has it. Rooms are areas of the interior,")
    add("-- not separate geometry: the room a player stands in is the room they may decorate,")
    add("-- and the room budget is what pays for it.")
    add("")

    add("DELETE FROM `housing_portal` WHERE `id` BETWEEN 1 AND 32;")
    add("INSERT INTO `housing_portal` (`id`, `amenity_id`, `name`, `map`, `x`, `y`, `z`, `o`, "
        "`faction_mask`, `level_required`, `tele_name`) VALUES")
    rows = []
    for i, (name, tele, mask, level) in enumerate(PORTALS):
        rows.append("({}, 0, '{}', 0, 0, 0, 0, 0, {}, {}, '{}')".format(
            i + 1, esc(name), mask, level, esc(tele)))
    add(",\n".join(rows) + ";")
    add("")

    add("DELETE FROM `housing_endeavor` WHERE `id` BETWEEN 1 AND 12;")
    add("INSERT INTO `housing_endeavor` (`id`, `name`, `description`, `duration_days`, "
        "`base_required`, `flags`) VALUES")
    rows = []
    for eid, name, desc, days, base, _tasks in ENDEAVORS:
        rows.append("({}, '{}', '{}', {}, {}, 0)".format(eid, esc(name), esc(desc), days, base))
    add(",\n".join(rows) + ";")
    add("")

    add("DELETE FROM `housing_endeavor_task` WHERE `id` BETWEEN 1 AND 128;")
    add("INSERT INTO `housing_endeavor_task` (`id`, `endeavor_id`, `type`, `target`, "
        "`target_name`, `required`, `text`) VALUES")
    rows = []
    task_id = 1
    for eid, name, desc, days, base, tasks in ENDEAVORS:
        for ttype, target_name, required, text in tasks:
            rows.append("({}, {}, {}, 0, '{}', {}, '{}')".format(
                task_id, eid, ttype, esc(target_name), required, esc(text)))
            task_id += 1
    add(",\n".join(rows) + ";")
    add("")

    add("DELETE FROM `housing_endeavor_reward` WHERE `endeavor_id` BETWEEN 1 AND 12;")
    add("INSERT INTO `housing_endeavor_reward` (`endeavor_id`, `milestone`, `item_entry`, "
        "`item_count`, `currency_id`, `currency_amount`, `xp`) VALUES")
    rows = []
    for eid, milestone, xp, amount, currency, _order in REWARDS:
        rows.append("({}, {}, 0, 1, {}, {}, {})".format(eid, milestone, currency, amount, xp))
    add(",\n".join(rows) + ";")
    add("")
    add("-- Rewards pay XP and Community Coupons (3363, a real 8.3.7 currency). No item is")
    add("-- granted: an item id belongs to the realm's item_template, and no decor piece needs")
    add("-- one when the catalogue is the reward. The milestone is what unlocks vendor stock")
    add("-- up to that step (housing_vendor_stock.milestone).")
    add("")

    # The dye palette. A dye is a swap to another game object, so the palette is the set of
    # colour words the realm's gameobject_template actually uses in front of a furniture name.
    # Six is what 8.3.7 has; adding a seventh row here is all a seventh colour needs.
    add("DELETE FROM `housing_dye`;")
    add("INSERT INTO `housing_dye` (`id`, `name`) VALUES")
    add("(1, 'Red'), (2, 'Blue'), (3, 'Green'), (4, 'Yellow'), (5, 'White'), (6, 'Black');")
    add("")

    add("DELETE FROM `housing_decor_creature` WHERE `decor_id` >= 1000;")
    add("INSERT INTO `housing_decor_creature` (`decor_id`, `kind`, `creature_entry`, "
        "`creature_name`, `roaming`, `radius`) VALUES")
    # The creature is named, not numbered: the entry differs between realms, so the bind
    # statement resolves 'Cat' against the realm's own creature_template. roaming 0 puts the
    # animal on the bed instead of walking it round the room, which is what a pet bed is.
    rows = []
    for decor_id, _decor_name in pet_pieces:
        rows.append("({}, 0, 0, 'Cat', 0, 0)".format(decor_id))
    if rows:
        add(",\n".join(rows) + ";")
    else:
        add("(0, 0, 0, '', 0, 0);")
    add("")
    add("-- kind 0 pet bed, 1 mount stand, 2 mannequin. A bed whose creature_entry stays 0")
    add("-- after the bind is furniture: the piece places and the creature is simply absent,")
    add("-- which is visible in the audit report rather than silent.")
    add("")
    return "\n".join(out) + "\n"
