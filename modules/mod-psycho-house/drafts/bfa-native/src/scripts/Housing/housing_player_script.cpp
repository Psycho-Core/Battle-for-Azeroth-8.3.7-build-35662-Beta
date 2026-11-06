/*
 * Copyright (C) 2022 BfaCore Reforged
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "HousingMgr.h"
#include "Player.h"
#include "ScriptMgr.h"

#include "HousingEndeavor.h"
#include "HousingMgr.h"
#include "Player.h"

using namespace Housing;

// Player lifecycle for the housing module. The update hook is intentionally cheap: it
// only does real work when the player is standing on a housing map, and even then it is
// throttled inside HousingMgr (500 ms).
class HousingPlayerScript : public PlayerScript
{
public:
    HousingPlayerScript() : PlayerScript("HousingPlayerScript") { }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        sHousingMgr->OnPlayerLogin(player);
    }

    void OnLogout(Player* player) override
    {
        sHousingMgr->OnPlayerLogout(player);
    }

    void OnUpdate(Player* player, uint32 diff) override
    {
        sHousingMgr->OnPlayerUpdate(player, diff);
    }

    void OnUpdateZone(Player* player, uint32 /*newZone*/, uint32 /*oldZone*/, uint32 /*newArea*/) override
    {
        sHousingMgr->OnPlayerZoneChange(player);
    }

    void OnMapChanged(Player* player) override
    {
        sHousingMgr->OnPlayerZoneChange(player);
    }

    // ---- Endeavor tasks ---------------------------------------------------------
    //
    // Three hooks cover the task types this core can actually see: a kill counts for
    // TaskKillCreature and, for a boss, for TaskKillBoss; then a quest turn-in and a pet
    // battle. There is no item-gained hook in this core, so loot and craft tasks are not
    // emitted at all rather than silently never completing. Deliveries are turned in at the
    // Coffer by hand and arrive through the addon opcode.
    void OnCreatureKill(Player* killer, Creature* killed) override
    {
        if (!killer || !killed)
            return;

        uint32 const entry = killed->GetEntry();
        sHousingEndeavor->OnEvent(killer, TaskKillCreature, entry, 1);

        if (killed->IsDungeonBoss() || killed->isWorldBoss())
            sHousingEndeavor->OnEvent(killer, TaskKillBoss, entry, 1);
    }

    void OnQuestComplete(Player* player, Quest const* quest) override
    {
        if (!player || !quest)
            return;

        uint32 const questId = quest->GetQuestId();
        sHousingEndeavor->OnEvent(player, TaskCompleteQuest, questId, 1);

        // A quest can be the source of a piece of decor. This is the only non-vendor source
        // with a hook of its own: there is no item-gained hook for drops, and achievements
        // are reconciled at login by HousingMgr::SyncAchievementDecor.
        sHousingMgr->GrantSourceDecor(player, SourceQuest, questId);
    }

    void OnPetBattleFinish(Player* player) override
    {
        sHousingEndeavor->OnEvent(player, TaskPetBattle, 0, 1);
    }
};

void AddSC_housing_player_script()
{
    new HousingPlayerScript();
}
