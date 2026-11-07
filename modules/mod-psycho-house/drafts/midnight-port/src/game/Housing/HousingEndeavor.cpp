/*
 * Copyright (C) 2022 BfaCore Reforged
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "HousingEndeavor.h"

#include "DatabaseEnv.h"
#include "HousingMgr.h"
#include "Log.h"
#include "Map.h"
#include "MapManager.h"
#include "Player.h"
#include "Util.h"

#include <algorithm>

namespace Housing
{
    EndeavorMgr* EndeavorMgr::instance()
    {
        static EndeavorMgr instance;
        return &instance;
    }

    void EndeavorMgr::Unload()
    {
        _states.clear();
        _loaded.clear();
        _list.clear();
        _byId.clear();
        _tasks.clear();
        _rewards.clear();
    }

    void EndeavorMgr::Load()
    {
        Unload();

        QueryResult result = WorldDatabase.Query(
            "SELECT `id`, `name`, `description`, `duration_days`, `base_required` FROM `housing_endeavor`");
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                EndeavorInfo info;
                info.Id = fields[0].GetUInt32();
                info.Name = fields[1].GetString();
                info.Description = fields[2].IsNull() ? "" : fields[2].GetString();
                info.DurationDays = fields[3].GetUInt32();
                info.BaseRequired = fields[4].GetUInt32();
                _list.push_back(info);
            }
            while (result->NextRow());
        }

        // Tasks. A task with target 0 has not been bound to realm content yet and never
        // completes - which is the safe failure: it cannot complete on the wrong kill.
        result = WorldDatabase.Query(
            "SELECT `id`, `endeavor_id`, `type`, `target`, `required`, `text` FROM `housing_endeavor_task` "
            "ORDER BY `endeavor_id`, `id`");
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                EndeavorTask task;
                task.Id = fields[0].GetUInt32();
                task.EndeavorId = fields[1].GetUInt32();
                task.Type = fields[2].GetUInt8();
                task.Target = fields[3].GetUInt32();
                task.Required = std::max(1u, fields[4].GetUInt32());
                task.Text = fields[5].GetString();
                _tasks[task.EndeavorId].push_back(task);
            }
            while (result->NextRow());
        }

        result = WorldDatabase.Query(
            "SELECT `endeavor_id`, `milestone`, `item_entry`, `item_count`, `currency_id`, "
            "`currency_amount`, `xp` FROM `housing_endeavor_reward` ORDER BY `endeavor_id`, `milestone`");
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                EndeavorReward reward;
                reward.EndeavorId = fields[0].GetUInt32();
                reward.Milestone = fields[1].GetUInt8();
                reward.ItemId = fields[2].GetUInt32();
                reward.Count = fields[3].GetUInt32();
                reward.CurrencyId = fields[4].GetUInt32();
                reward.CurrencyAmount = fields[5].GetUInt32();
                reward.Xp = fields[6].GetUInt32();
                _rewards[reward.EndeavorId].push_back(reward);
            }
            while (result->NextRow());
        }

        for (auto& info : _list)
        {
            auto taskItr = _tasks.find(info.Id);
            if (taskItr != _tasks.end())
                info.Tasks = taskItr->second;

            auto rewardItr = _rewards.find(info.Id);
            if (rewardItr != _rewards.end())
                info.Rewards = rewardItr->second;

            _byId[info.Id] = &info;
        }

        TC_LOG_INFO("housing", "Loaded {} endeavor(s), {} task(s), {} reward row(s)",
            uint32(_list.size()), uint32(_tasks.size()), uint32(_rewards.size()));
    }

    EndeavorInfo const* EndeavorMgr::GetInfo(uint32 id) const
    {
        auto const itr = _byId.find(id);
        return itr != _byId.end() ? itr->second : nullptr;
    }

    std::vector<EndeavorTask> const* EndeavorMgr::TasksFor(uint32 endeavorId) const
    {
        auto const itr = _tasks.find(endeavorId);
        return itr != _tasks.end() ? &itr->second : nullptr;
    }

    void EndeavorMgr::LoadNeighborhood(uint32 neighborhoodId)
    {
        if (!neighborhoodId || _loaded.count(neighborhoodId))
            return;

        _loaded.insert(neighborhoodId);

        State state;
        if (QueryResult result = CharacterDatabase.Query(
            "SELECT `endeavorId`, `milestone`, `started`, `ends` FROM `character_housing_endeavor` "
            "WHERE `neighborhoodId` = {}", neighborhoodId))
        {
            Field* fields = result->Fetch();
            state.EndeavorId = fields[0].GetUInt32();
            state.Milestone = fields[1].GetUInt8();
            state.Started = fields[2].GetUInt32();
            state.Ends = fields[3].GetUInt32();

            if (QueryResult rows = CharacterDatabase.Query(
                "SELECT `taskId`, `progress` FROM `character_housing_endeavor_task` WHERE `neighborhoodId` = {}",
                neighborhoodId))
            {
                do
                {
                    Field* taskFields = rows->Fetch();
                    state.Progress[taskFields[0].GetUInt32()] = taskFields[1].GetUInt32();
                }
                while (rows->NextRow());
            }
        }

        if (state.EndeavorId)
            _states[neighborhoodId] = state;
    }

    void EndeavorMgr::SaveNeighborhood(uint32 neighborhoodId)
    {
        auto itr = _states.find(neighborhoodId);
        if (itr == _states.end())
        {
            CharacterDatabase.DirectPExecute(
                "DELETE FROM `character_housing_endeavor` WHERE `neighborhoodId` = {}", neighborhoodId);
            CharacterDatabase.DirectPExecute(
                "DELETE FROM `character_housing_endeavor_task` WHERE `neighborhoodId` = {}", neighborhoodId);
            return;
        }

        State const& state = itr->second;

        CharacterDatabase.DirectPExecute(
            "REPLACE INTO `character_housing_endeavor` (`neighborhoodId`, `endeavorId`, `milestone`, "
            "`started`, `ends`) VALUES ({}, {}, {}, {}, {})",
            neighborhoodId, state.EndeavorId, uint32(state.Milestone), state.Started, state.Ends);

        for (auto const& pair : state.Progress)
            CharacterDatabase.DirectPExecute(
                "REPLACE INTO `character_housing_endeavor_task` (`neighborhoodId`, `taskId`, `progress`) "
                "VALUES ({}, {}, {})", neighborhoodId, pair.first, pair.second);
    }

    bool EndeavorMgr::HasActive(uint32 neighborhoodId) const
    {
        auto const itr = _states.find(neighborhoodId);
        return itr != _states.end() && itr->second.EndeavorId != 0;
    }

    uint32 EndeavorMgr::GetActiveId(uint32 neighborhoodId) const
    {
        auto const itr = _states.find(neighborhoodId);
        return itr != _states.end() ? itr->second.EndeavorId : 0;
    }

    uint8 EndeavorMgr::GetMilestone(uint32 neighborhoodId) const
    {
        auto const itr = _states.find(neighborhoodId);
        return itr != _states.end() ? itr->second.Milestone : 0;
    }

    uint32 EndeavorMgr::GetTaskProgress(uint32 neighborhoodId, uint32 taskId) const
    {
        auto const itr = _states.find(neighborhoodId);
        if (itr == _states.end())
            return 0;

        auto const taskItr = itr->second.Progress.find(taskId);
        return taskItr != itr->second.Progress.end() ? taskItr->second : 0;
    }

    uint32 EndeavorMgr::GetCompletedTasks(uint32 neighborhoodId) const
    {
        auto const id = GetActiveId(neighborhoodId);
        std::vector<EndeavorTask> const* tasks = TasksFor(id);
        if (!tasks)
            return 0;

        uint32 done = 0;
        for (auto const& task : *tasks)
            if (GetTaskProgress(neighborhoodId, task.Id) >= task.Required)
                ++done;

        return done;
    }

    void EndeavorMgr::Start(uint32 neighborhoodId, uint32 endeavorId)
    {
        if (!neighborhoodId || HasActive(neighborhoodId))
            return;

        EndeavorInfo const* info = GetInfo(endeavorId);
        if (!info)
            return;

        State state;
        state.EndeavorId = endeavorId;
        state.Milestone = 0;
        state.Started = uint32(time(nullptr));
        state.Ends = state.Started + info->DurationDays * 86400;

        _states[neighborhoodId] = state;
        _loaded.insert(neighborhoodId);
        SaveNeighborhood(neighborhoodId);

        if (HousingNeighborhood* neighborhood = sHousingMgr->GetNeighborhood(neighborhoodId))
            TC_LOG_INFO("housing", "neighborhood {} started endeavor {} ({})",
                neighborhoodId, endeavorId, neighborhood->GetName());
    }

    void EndeavorMgr::Update(uint32 /*diff*/)
    {
        if (_list.empty())
            return;

        uint32 const now = uint32(time(nullptr));

        for (auto& pair : _states)
        {
            State& state = pair.second;
            if (!state.EndeavorId || !state.Ends || state.Ends > now)
                continue;

            // Rotation: the finished endeavour is closed, progress is cleared, and the next
            // definition in the list starts. Deliberately not random: a neighbourhood that
            // rolls the same job twice in a row looks broken.
            uint32 const next = (state.EndeavorId % uint32(_list.size())) + 1;
            TC_LOG_INFO("housing", "neighborhood {} finished endeavor {}, starting {}",
                pair.first, state.EndeavorId, next);

            CharacterDatabase.DirectPExecute(
                "DELETE FROM `character_housing_endeavor_task` WHERE `neighborhoodId` = {}", pair.first);
            CharacterDatabase.DirectPExecute(
                "DELETE FROM `character_housing_endeavor_claim` WHERE `neighborhoodId` = {}", pair.first);

            state = State();
            _states.erase(pair.first);
            _loaded.erase(pair.first);

            Start(pair.first, next);
            break;   // one rotation per tick; the states map is being modified
        }
    }

    uint32 EndeavorMgr::GetEnds(uint32 neighborhoodId) const
    {
        auto const state = _states.find(neighborhoodId);
        return state != _states.end() ? state->second.Ends : 0;
    }

    uint32 EndeavorMgr::GetRequired(uint32 neighborhoodId, EndeavorTask const& task) const
    {
        // A street of forty has to work four times as hard for the same milestone as a street
        // of ten, or an Endeavor is over the moment a big neighbourhood looks at it.
        return sHousingMgr->ScaledRequirement(task.Required, neighborhoodId);
    }

    void EndeavorMgr::OnEvent(Player* player, uint8 type, uint32 target, uint32 count)
    {
        if (!player || !count || _states.empty())
            return;

        uint32 const neighborhoodId = sHousingMgr->GetPlayerNeighborhoodId(player);
        if (!neighborhoodId)
            return;

        auto itr = _states.find(neighborhoodId);
        if (itr == _states.end())
            return;

        std::vector<EndeavorTask> const* tasks = TasksFor(itr->second.EndeavorId);
        if (!tasks)
            return;

        for (auto const& task : *tasks)
        {
            if (task.Type != type || !task.Target || task.Target != target)
                continue;

            uint32 const required = GetRequired(neighborhoodId, task);

            uint32& progress = itr->second.Progress[task.Id];
            if (progress >= required)
                continue;

            progress = std::min(required, progress + count);
            if (progress >= required)
                CompleteTask(neighborhoodId, task.Id);
            else
                SaveNeighborhood(neighborhoodId);
        }
    }

    void EndeavorMgr::CompleteTask(uint32 neighborhoodId, uint32 /*taskId*/)
    {
        SaveNeighborhood(neighborhoodId);
        RecomputeMilestone(neighborhoodId);

        for (auto const& entry : sHousingMgr->GetNeighborhoods())
        {
            if (entry.first != neighborhoodId)
                continue;

            if (Player* owner = entry.second->GetAnyResident())
                sHousingMgr->GrantHouseXp(owner, sHousingMgr->GetXpPerEndeavorTask());
            break;
        }
    }

    bool EndeavorMgr::Deliver(Player* player)
    {
        if (!player)
            return false;

        uint32 const neighborhoodId = sHousingMgr->GetPlayerNeighborhoodId(player);
        auto itr = _states.find(neighborhoodId);
        if (itr == _states.end())
            return false;

        std::vector<EndeavorTask> const* tasks = TasksFor(itr->second.EndeavorId);
        if (!tasks)
            return false;

        bool counted = false;
        for (auto const& task : *tasks)
        {
            if (task.Type != TaskDeliver)
                continue;

            uint32& progress = itr->second.Progress[task.Id];
            if (progress >= task.Required)
                continue;

            ++progress;
            counted = true;
            if (progress >= task.Required)
                CompleteTask(neighborhoodId, task.Id);
            break;
        }

        if (counted)
            SaveNeighborhood(neighborhoodId);

        return counted;
    }

    void EndeavorMgr::RecomputeMilestone(uint32 neighborhoodId)
    {
        auto itr = _states.find(neighborhoodId);
        if (itr == _states.end())
            return;

        uint32 const done = GetCompletedTasks(neighborhoodId);
        uint8 const milestone = uint8(std::min<uint32>(4, done));
        if (milestone == itr->second.Milestone)
            return;

        itr->second.Milestone = milestone;
        SaveNeighborhood(neighborhoodId);

        // The milestone is what the hub is themed by, so the props go up as it moves, and
        // everybody standing in the street is told rather than having to read a command.
        for (auto const& entry : sHousingMgr->GetNeighborhoods())
        {
            if (entry.first != neighborhoodId)
                continue;

            sHousingMgr->BroadcastEndeavorMilestone(entry.second.get(), milestone);

            if (Map* map = sMapMgr->FindMap(entry.second->GetMapId(), 0))
                sHousingMgr->SpawnEndeavorTheming(entry.second.get(), map,
                    itr->second.EndeavorId, milestone);

            break;
        }

        if (milestone >= 4)
        {
            // A finished endeavour pays its XP once, to the neighbourhood, not per resident.
            for (auto const& entry : sHousingMgr->GetNeighborhoods())
            {
                if (entry.first != neighborhoodId)
                    continue;

                if (Player* owner = entry.second->GetAnyResident())
                    sHousingMgr->GrantHouseXp(owner, kXpPerEndeavor);
                break;
            }
        }
    }

    uint8 EndeavorMgr::GetMilestoneFor(Player* player)
    {
        if (!player)
            return 0;

        return GetMilestone(sHousingMgr->GetPlayerNeighborhoodId(player));
    }

    bool EndeavorMgr::CanClaim(Player* player, uint8 milestone)
    {
        if (!player || !milestone)
            return false;

        uint32 const neighborhoodId = sHousingMgr->GetPlayerNeighborhoodId(player);
        if (!neighborhoodId || GetMilestone(neighborhoodId) < milestone)
            return false;

        uint32 const accountId = player->GetSession()->GetAccountId();
        QueryResult result = CharacterDatabase.Query(
            "SELECT 1 FROM `character_housing_endeavor_claim` WHERE `neighborhoodId` = {} "
            "AND `milestone` = {} AND `accountId` = {}", neighborhoodId, uint32(milestone), accountId);

        return !result;
    }

    bool EndeavorMgr::Claim(Player* player, uint8 milestone)
    {
        if (!CanClaim(player, milestone))
            return false;

        uint32 const neighborhoodId = sHousingMgr->GetPlayerNeighborhoodId(player);
        uint32 const accountId = player->GetSession()->GetAccountId();
        uint32 const endeavorId = GetActiveId(neighborhoodId);

        CharacterDatabase.DirectPExecute(
            "REPLACE INTO `character_housing_endeavor_claim` (`neighborhoodId`, `milestone`, `accountId`, "
            "`claimed`) VALUES ({}, {}, {}, UNIX_TIMESTAMP())", neighborhoodId, uint32(milestone), accountId);

        for (auto const& reward : _rewards[endeavorId])
        {
            if (reward.Milestone != milestone)
                continue;

            if (reward.CurrencyId && reward.CurrencyAmount)
                player->ModifyCurrency(reward.CurrencyId, int32(reward.CurrencyAmount));

            // A reward row may name an item instead of, or as well as, currency. The shipped
            // rows do not - the catalogue is the reward and the milestone gate is the prize -
            // but a row that names one is paid rather than silently skipped.
            if (reward.ItemId && reward.Count)
            {
                if (!player->AddItem(reward.ItemId, reward.Count))
                    player->GetSession()->SendNotification(
                        "A reward could not be delivered - free a bag slot and ask the Coffer again.");
            }

            if (reward.Xp)
                sHousingMgr->GrantHouseXp(player, reward.Xp);
        }

        return true;
    }

    std::string EndeavorMgr::BuildReport() const
    {
        std::string report = "Endeavors: " + std::to_string(_list.size()) + " definition(s)\n";

        for (auto const& pair : _states)
        {
            EndeavorInfo const* info = GetInfo(pair.second.EndeavorId);
            report += Trinity::StringFormat("  neighborhood {}: {} milestone {}/4, {} of {} tasks, ends {}",
                pair.first, info ? info->Name : "?", uint32(pair.second.Milestone),
                GetCompletedTasks(pair.first),
                uint32(info ? info->Tasks.size() : 0), pair.second.Ends) + "\n";
        }

        return report;
    }
} // namespace Housing
