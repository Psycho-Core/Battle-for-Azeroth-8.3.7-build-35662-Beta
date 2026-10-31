#!/usr/bin/env python3
"""Write the four introduction quests into both drafts.

    python3 tools/housing/build_quests.py

The rows are carriers: title, text, chain link, money reward. The objectives are not in
the DB because they are not objective types the world DB can express - "walk a plot",
"click a Cornerstone", "place three pieces", "reach house level 2" are all module events,
so the player script completes the quest when the event fires. That is stated in the SQL
header too, so nobody goes looking for a missing objective row.

Column names are not invented: every one of them is a column this build's own update files
write to (tools/housing/check_sql_columns.py harvests them, and checks this output).
"""

import os

HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DRAFTS = os.path.join(HERE, "drafts")

# Text is third person on purpose: the steward is talking, not the document.
QUESTS = [
    {
        "id": 900100,
        "title": "A House For You",
        "log": "Read the deed the steward hands over.",
        "description": "The steward has a deed with a name on it that is not the steward's. "
                       "$B$BEvery plot in the neighbourhood is standing empty and the "
                       "steward would rather they were not. The deed is the first one.",
        "completion": "The steward stamps the deed and points at the portal.",
        "money": 2000,
        "next": 900101,
    },
    {
        "id": 900101,
        "title": "My First Home",
        "log": "Walk the plot and claim it at the Cornerstone.",
        "description": "The deed is good for one plot. The Cornerstone in the middle of an "
                       "empty plot is what turns it into a home - once it is claimed the "
                       "walls go up around it.",
        "completion": "The Cornerstone is claimed and the plot belongs to somebody.",
        "money": 100000,
        "next": 900102,
    },
    {
        "id": 900102,
        "title": "Welcome Home",
        "log": "Place three pieces of decor, then return to the contractor.",
        "description": "An empty room with a roof is still an empty room. Three pieces of "
                       "decor is enough to make the point: the catalogue is open, the "
                       "budget is there, and the pieces go where they are put.",
        "completion": "The contractor looks the room over and nods.",
        "money": 0,
        "next": 900103,
    },
    {
        "id": 900103,
        "title": "Time to Decorate",
        "log": "Reach house level 2.",
        "description": "Level one is a shell. Level two is where the house starts paying "
                       "for itself in space - more interior budget, more rooms, and a "
                       "reason to keep decorating.",
        "completion": "The house reaches level two and the contractor moves on.",
        "money": 25000,
        "next": 0,
    },
]

INT_DEFAULT = 0
STR_DEFAULT = ""

# Columns the module sets; everything else takes the table default of 0 or an empty string.
SET = {
    "QuestType": 0,
    "QuestLevel": -1,
    "ScalingFactionGroup": 0,
    "MaxScalingLevel": 60,
    "MinLevel": 1,
    "QuestSortID": 0,
    "QuestInfoID": 0,
    "SuggestedGroupNum": 0,
    "RewardXPDifficulty": 0,
    "RewardXPMultiplier": 1,
    "RewardMoneyDifficulty": 0,
    "RewardMoneyMultiplier": 1,
    "RewardBonusMoney": 0,
    "Flags": 0,
    "FlagsEx": 0,
    "FlagsEx2": 0,
    "AllowableRaces": 0,
    "AcceptedSoundKitID": 890,
    "CompleteSoundKitID": 878,
    "VerifiedBuild": 35662,
}

TEXT = ("LogTitle", "LogDescription", "QuestDescription", "QuestCompletionLog",
        "AreaDescription", "PortraitGiverText", "PortraitGiverName", "PortraitTurnInText",
        "PortraitTurnInName")

COLUMNS = [
    "ID", "QuestType", "QuestLevel", "ScalingFactionGroup", "MaxScalingLevel",
    "QuestPackageID", "MinLevel", "QuestSortID", "QuestInfoID", "SuggestedGroupNum",
    "RewardNextQuest", "RewardXPDifficulty", "RewardXPMultiplier", "RewardMoney",
    "RewardMoneyDifficulty", "RewardMoneyMultiplier", "RewardBonusMoney",
    "RewardDisplaySpell1", "RewardDisplaySpell2", "RewardDisplaySpell3", "RewardSpell",
    "RewardHonor", "RewardKillHonor", "StartItem", "RewardArtifactXPDifficulty",
    "RewardArtifactXPMultiplier", "RewardArtifactCategoryID", "Flags", "FlagsEx",
    "FlagsEx2", "RewardItem1", "RewardAmount1", "RewardItem2", "RewardAmount2",
    "RewardItem3", "RewardAmount3", "RewardItem4", "RewardAmount4",
    "RewardChoiceItemID1", "RewardChoiceItemQuantity1", "RewardChoiceItemDisplayID1",
    "RewardChoiceItemID2", "RewardChoiceItemQuantity2", "RewardChoiceItemDisplayID2",
    "RewardChoiceItemID3", "RewardChoiceItemQuantity3", "RewardChoiceItemDisplayID3",
    "RewardChoiceItemID4", "RewardChoiceItemQuantity4", "RewardChoiceItemDisplayID4",
    "POIContinent", "POIx", "POIy", "POIPriority", "RewardTitle", "RewardArenaPoints",
    "RewardSkillLineID", "RewardNumSkillUps", "PortraitGiver", "PortraitGiverMount",
    "PortraitTurnIn", "RewardFactionID1", "RewardFactionValue1", "RewardFactionOverride1",
    "RewardFactionCapIn1", "RewardFactionFlags", "RewardCurrencyID1", "RewardCurrencyQty1",
    "AcceptedSoundKitID", "CompleteSoundKitID", "AreaGroupID", "TimeAllowed",
    "AllowableRaces", "TreasurePickerID", "Expansion", "ManagedWorldStateID",
    "QuestSessionBonus", "LogTitle", "LogDescription", "QuestDescription",
    "AreaDescription", "PortraitGiverText", "PortraitGiverName", "PortraitTurnInText",
    "PortraitTurnInName", "QuestCompletionLog", "VerifiedBuild",
]

HEADER = """-- Generated by tools/housing/build_quests.py - do not edit by hand.
--
-- The four introduction quests, ids 900100-900103, inside the module's reserved range.
--
-- There are no rows in `quest_objectives` for these on purpose. Their objectives are not
-- objective types the world database can express - walk a plot, click a Cornerstone,
-- place three pieces, reach house level 2 - so the player script completes the quest when
-- the module event fires. Do not go looking for a missing objective row.
--
-- The decor and shell rewards named in the content spec are module-side grants, not loot:
-- the pieces are `housing_decor` rows, so they are granted to the collection rather than
-- mailed. Only money is in this file.
--
-- Every column below is one the build's own update files write to; see
-- tools/housing/check_sql_columns.py.
"""


def quote(text):
    return "'" + text.replace("\\", "\\\\").replace("'", "''") + "'"


def rows():
    out = []
    for quest in QUESTS:
        values = []
        for column in COLUMNS:
            if column == "ID":
                values.append(str(quest["id"]))
            elif column == "RewardNextQuest":
                values.append(str(quest["next"]))
            elif column == "RewardMoney":
                values.append(str(quest["money"]))
            elif column == "LogTitle":
                values.append(quote(quest["title"]))
            elif column == "LogDescription":
                values.append(quote(quest["log"]))
            elif column == "QuestDescription":
                values.append(quote(quest["description"]))
            elif column == "QuestCompletionLog":
                values.append(quote(quest["completion"]))
            elif column in TEXT:
                values.append(quote(STR_DEFAULT))
            else:
                values.append(str(SET.get(column, INT_DEFAULT)))
        out.append("({})".format(", ".join(values)))
    return out


def build():
    # The separator is the statement: four tuples with nothing between them is a syntax
    # error 1064, not four rows. Every checker in this folder splits on parentheses, so a
    # missing comma here is invisible to all of them - which is why the MariaDB check parses
    # the statements now.
    body = ",\n".join(rows())
    sql = (HEADER
           + "\nDELETE FROM `quest_template` WHERE `ID` BETWEEN 900100 AND 900103;\n"
           + "INSERT INTO `quest_template` ({}) VALUES\n".format(
               ", ".join("`{}`".format(c) for c in COLUMNS))
           + body + ";\n"
           + "\nDELETE FROM `quest_template_addon` WHERE `ID` BETWEEN 900100 AND 900103;\n"
           + "INSERT INTO `quest_template_addon` (`ID`, `PrevQuestID`, `NextQuestID`) VALUES\n"
           + ",\n".join("({}, {}, {})".format(q["id"], prev["id"], q["id"])
                        for prev, q in zip([{"id": 0}] + QUESTS, QUESTS))
           + ";\n")
    for draft in ("midnight-port", "bfa-native"):
        path = os.path.join(DRAFTS, draft, "sql", "world", "06_quests.sql")
        with open(path, "w", encoding="utf-8") as handle:
            handle.write(sql)
        print("wrote {} ({} quests, {} columns)".format(
            os.path.relpath(path, HERE), len(QUESTS), len(COLUMNS)))


if __name__ == "__main__":
    build()
