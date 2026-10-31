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

// Script module loader for the Housing directory.
// The function name is generated from the directory name, so this file must stay named
// housing_script_loader.cpp and export AddHousingScripts().

void AddSC_housing_world_script();
void AddSC_housing_player_script();
void AddSC_housing_commands();

void AddHousingScripts()
{
    AddSC_housing_world_script();
    AddSC_housing_player_script();
    AddSC_housing_commands();
}
