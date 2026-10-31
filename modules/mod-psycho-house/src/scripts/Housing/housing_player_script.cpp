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
};

void AddSC_housing_player_script()
{
    new HousingPlayerScript();
}
