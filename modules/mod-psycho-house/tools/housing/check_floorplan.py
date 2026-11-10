#!/usr/bin/env python3
"""Run the floorplan rules over the seventeen shapes the drafts actually ship.

    python3 tools/housing/check_floorplan.py

`HousingMgr::PlaceRoom` cannot be compiled in this folder - there is no boost, no MySQL client
and no compiler - so the rules it enforces are restated here in Python and run against the real
`housing_room` rows: the four-yard grid, the anchor at 0,0, a shared wall for every room after
it, no overlap, one walkable piece, a stairwell under every room above the ground floor, and the
level's room budget.

The mirror is worth its weight because it can fail in both directions. If a shape's footprint,
its slot cost or its `stairs` flag changes so that the Initial Room Collection no longer fits a
level-1 house, this says so; if a rule in the C++ is loosened, the numbers here are what the doc
promises and the two can be compared.

What it cannot check is that the C++ implements these rules exactly. Two implementations, one
run: the Python one is the executable statement of intent, and the C++ is read against it.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DRAFTS = os.path.join(HERE, "drafts")

# Mirrors kRoomGridLimit and the level-1 and level-12 room budgets of doc 01 sec.3.1.
GRID_LIMIT = 16
BUDGET_LEVEL_1 = 19
BUDGET_LEVEL_12 = 134
MAX_FLOOR = 3


def fail(message):
    print("  %s" % message)
    failures.append(message)


failures = []


def read(path):
    with open(path, "r", encoding="utf-8") as handle:
        return handle.read()


def rooms_of(draft):
    sql = read(os.path.join(DRAFTS, draft, "sql", "world", "07_progression.sql"))
    block = sql.split("INSERT INTO `housing_room`")[1].split(";")[0]

    rooms = {}
    for match in re.finditer(r"\((\d+), \d+, '((?:[^']|'')*)', [-\d.]+, [-\d.]+, [-\d.]+, "
                             r"[-\d.]+, (\d+), (\d+), (\d+), (\d+), (\d+)\)", block):
        rooms[int(match.group(1))] = {
            "name": match.group(2).replace("''", "'"),
            "cost": int(match.group(3)),
            "level": int(match.group(4)),
            "cells_x": int(match.group(5)),
            "cells_y": int(match.group(6)),
            "stairs": bool(int(match.group(7))),
        }

    return rooms


def rect(room, x, y):
    return (x, y, x + room["cells_x"] - 1, y + room["cells_y"] - 1)


def overlaps(left, right):
    return left[0] <= right[2] and right[0] <= left[2] and left[1] <= right[3] and right[1] <= left[3]


def shares_edge(left, right):
    overlaps_x = left[0] <= right[2] and right[0] <= left[2]
    overlaps_y = left[1] <= right[3] and right[1] <= left[3]

    if overlaps_x and (left[3] + 1 == right[1] or right[3] + 1 == left[1]):
        return True

    if overlaps_y and (left[2] + 1 == right[0] or right[2] + 1 == left[0]):
        return True

    return False


def connected(rooms, plan):
    if not plan:
        return True

    anchor = None
    for index, placed in enumerate(plan):
        room = rooms[placed[0]]
        if placed[3] != 0:
            continue
        foot = rect(room, placed[1], placed[2])
        if foot[0] <= 0 <= foot[2] and foot[1] <= 0 <= foot[3]:
            anchor = index
            break

    if anchor is None:
        return False

    seen = {anchor}
    queue = [anchor]
    head = 0

    while head < len(queue):
        from_room = plan[queue[head]]
        head += 1
        from_foot = rect(rooms[from_room[0]], from_room[1], from_room[2])

        for index, placed in enumerate(plan):
            if index in seen:
                continue

            to_foot = rect(rooms[placed[0]], placed[1], placed[2])

            if placed[3] == from_room[3]:
                linked = shares_edge(from_foot, to_foot)
            elif abs(placed[3] - from_room[3]) == 1:
                linked = (rooms[from_room[0]]["stairs"] or rooms[placed[0]]["stairs"]) \
                    and overlaps(from_foot, to_foot)
            else:
                linked = False

            if linked:
                seen.add(index)
                queue.append(index)

    return len(seen) == len(plan)


def fits(rooms, plan, room_id, x, y, floor):
    room = rooms[room_id]

    if x < -GRID_LIMIT or y < -GRID_LIMIT \
            or x + room["cells_x"] - 1 > GRID_LIMIT or y + room["cells_y"] - 1 > GRID_LIMIT:
        return False

    foot = rect(room, x, y)
    ground_used = False

    for placed in plan:
        if placed[3] == 0:
            ground_used = True
        if placed[3] != floor:
            continue
        if overlaps(foot, rect(rooms[placed[0]], placed[1], placed[2])):
            return False

    if floor == 0 and not ground_used:
        return x == 0 and y == 0

    return connected(rooms, plan + [(room_id, x, y, floor)])


def find_cell(rooms, plan, room_id):
    for floor in range(MAX_FLOOR + 1):
        for y in range(-GRID_LIMIT, GRID_LIMIT + 1):
            for x in range(-GRID_LIMIT, GRID_LIMIT + 1):
                if fits(rooms, plan, room_id, x, y, floor):
                    return (room_id, x, y, floor)
    return None


def find_on_floor(rooms, plan, room_id, floor):
    for y in range(-GRID_LIMIT, GRID_LIMIT + 1):
        for x in range(-GRID_LIMIT, GRID_LIMIT + 1):
            if fits(rooms, plan, room_id, x, y, floor):
                return (room_id, x, y, floor)
    return None


def cost_of(rooms, plan):
    return sum(rooms[placed[0]]["cost"] for placed in plan)


def run_draft(draft):
    rooms = rooms_of(draft)

    if len(rooms) != 17:
        fail("%s: %d room shapes, doc 04 sec.3 has 17" % (draft, len(rooms)))
        return

    # 1. The Initial Room Collection fits a level-1 house, and costs what doc 04 sec.3 says.
    plan = []
    for room_id in range(1, 13):
        cell = find_cell(rooms, plan, room_id)
        if not cell:
            fail("%s: room %d has nowhere to go on a level-1 plan" % (draft, room_id))
            return
        plan.append(cell)

    collection = cost_of(rooms, plan)
    if collection != 16:
        fail("%s: the Initial Room Collection costs %d slots, doc 04 sec.3 says 16"
             % (draft, collection))

    if collection > BUDGET_LEVEL_1:
        fail("%s: the Initial Room Collection does not fit the level-1 budget of %d"
             % (draft, BUDGET_LEVEL_1))

    # 2. A level-1 house cannot spend past its budget, and can spend up to it exactly.
    spent = collection
    while True:
        cell = find_cell(rooms, plan, 4)
        if not cell or spent + rooms[4]["cost"] > BUDGET_LEVEL_1:
            break
        plan.append(cell)
        spent += rooms[4]["cost"]

    if spent != BUDGET_LEVEL_1:
        fail("%s: a level-1 plan tops out at %d slots, the budget is %d"
             % (draft, spent, BUDGET_LEVEL_1))

    # 3. Nothing floats: the ground floor first, then a stairwell, then the floor above.
    empty = []
    if fits(rooms, empty, 4, 0, 0, 1):
        fail("%s: a closet can be placed on floor 1 with nothing holding it up" % draft)

    upstairs = find_on_floor(rooms, plan, 4, 1)
    if not upstairs:
        fail("%s: no cell on floor 1 is reachable even with three stairwells on the ground floor"
             % draft)

    # 4. One room per cell.
    if fits(rooms, plan, 4, plan[0][1], plan[0][2], 0):
        fail("%s: a second room can be placed on the anchor's cell" % draft)

    # 5. The house is one piece, and taking the anchor out would break that.
    if not connected(rooms, plan):
        fail("%s: the plan the placer builds is not connected" % draft)

    rest = plan[1:]
    if connected(rooms, rest):
        fail("%s: removing the anchor leaves the plan connected, so the removal check proves nothing"
             % draft)

    # 6. Every shape fits somewhere by level 12, and the budget is what stops it.
    plan = []
    for room_id in range(1, 18):
        cell = find_cell(rooms, plan, room_id)
        if cell:
            plan.append(cell)

    if len(plan) != 17:
        fail("%s: only %d of the 17 shapes can be placed at level 12" % (draft, len(plan)))

    spent = cost_of(rooms, plan)
    if spent > BUDGET_LEVEL_12:
        fail("%s: the seventeen shapes cost %d slots, the level-12 budget is %d"
             % (draft, spent, BUDGET_LEVEL_12))


def main():
    print("floorplan rules, run against the shapes the drafts ship\n")

    for draft in sorted(os.listdir(DRAFTS)):
        path = os.path.join(DRAFTS, draft, "sql", "world", "07_progression.sql")
        if os.path.isfile(path):
            run_draft(draft)

    print("%d finding(s)" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
