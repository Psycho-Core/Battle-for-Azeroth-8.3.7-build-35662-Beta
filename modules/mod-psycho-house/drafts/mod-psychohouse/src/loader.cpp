/*
 * Copyright (C) 2022 BfaCore Reforged
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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

// Script module loader.
//
// The build generates the export name from the folder it links: the module installs into
// src/server/scripts/PsychoHouse, so the function below has to be AddPsychoHouseScripts()
// and nothing else. A folder called mod-psychohouse would generate a name with a hyphen in
// it and would not compile - that is why the install step renames the folder.

void AddSC_psychohouse_gossip();
void AddSC_psychohouse_hooks();

void AddPsychoHouseScripts()
{
    AddSC_psychohouse_gossip();
    AddSC_psychohouse_hooks();
}
