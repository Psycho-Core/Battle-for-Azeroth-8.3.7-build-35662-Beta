/*
 * Copyright (C) 2022 BfaCore Reforged
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef HousingEndeavor_h__
#define HousingEndeavor_h__

#include "HousingDefines.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class Player;

namespace Housing
{
    /// Endeavors: the monthly job the whole neighbourhood works on.
    ///
    /// One is active per neighbourhood. Every task counts something players already do in
    /// 8.3.7 - a kill, a quest, a gathered item, a pet battle - and the tasks that are
    /// finished are what move the milestone, which is what unlocks vendor stock.
    ///
    /// Threading: world thread only, like the rest of the module. The two `On*` handlers
    /// are called from script hooks that already run there.
    class TC_GAME_API EndeavorMgr
    {
    public:
        static EndeavorMgr* instance();

        /// World data: definitions, tasks and rewards.
        void Load();
        void Unload();

        EndeavorInfo const* GetInfo(uint32 id) const;
        std::vector<EndeavorInfo> const& GetInfos() const { return _list; }

        // ---- per-neighbourhood state -----------------------------------------

        void LoadNeighborhood(uint32 neighborhoodId);
        void SaveNeighborhood(uint32 neighborhoodId);

        bool HasActive(uint32 neighborhoodId) const;
        uint32 GetActiveId(uint32 neighborhoodId) const;
        uint8 GetMilestone(uint32 neighborhoodId) const;
        uint32 GetTaskProgress(uint32 neighborhoodId, uint32 taskId) const;
        uint32 GetCompletedTasks(uint32 neighborhoodId) const;

        /// What a task costs this neighbourhood, which is not what the row says: an Endeavor
        /// scales with how many people live in the street.
        uint32 GetRequired(uint32 neighborhoodId, EndeavorTask const& task) const;

        /// Start `endeavorId` in this neighbourhood. A no-op if one is already running.
        void Start(uint32 neighborhoodId, uint32 endeavorId);

        /// The rotation: finish an expired one and pick the next for the neighbourhood.
        void Update(uint32 diff);

        // ---- progress --------------------------------------------------------

        /// Count one event of `type` against `target` in the player's neighbourhood.
        void OnEvent(Player* player, uint8 type, uint32 target, uint32 count = 1);

        /// Turn in a delivery at the Coffer: the only task a player completes by hand.
        bool Deliver(Player* player);

        // ---- rewards ---------------------------------------------------------

        bool CanClaim(Player* player, uint8 milestone);
        bool Claim(Player* player, uint8 milestone);

        /// What the player's neighbourhood has unlocked so far, for the stock filter.
        uint8 GetMilestoneFor(Player* player);

        std::string BuildReport() const;

    private:
        EndeavorMgr() = default;

        struct State
        {
            uint32 EndeavorId = 0;
            uint8 Milestone = 0;
            uint32 Started = 0;
            uint32 Ends = 0;
            std::map<uint32, uint32> Progress;   // taskId -> count
            std::set<uint8> Claimed;            // milestones claimed by this account cache
        };

        // Keyed by neighbourhood id. Loaded lazily and written back on change.
        std::map<uint32, State> _states;
        std::set<uint32> _loaded;
        std::vector<EndeavorInfo> _list;
        std::map<uint32, EndeavorInfo const*> _byId;
        std::map<uint32, std::vector<EndeavorTask>> _tasks;   // endeavorId -> tasks
        std::map<uint32, std::vector<EndeavorReward>> _rewards;

        void RecomputeMilestone(uint32 neighborhoodId);
        void CompleteTask(uint32 neighborhoodId, uint32 taskId);
        std::vector<EndeavorTask> const* TasksFor(uint32 endeavorId) const;
    };
} // namespace Housing

#define sHousingEndeavor Housing::EndeavorMgr::instance()

#endif // HousingEndeavor_h__
