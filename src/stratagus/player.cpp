//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name player.cpp - The players. */
//
//      (c) Copyright 1998-2007 by Lutz Sammer, Jimmy Salmon, Nehal Mistry
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//

//@{

/*----------------------------------------------------------------------------
-- Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "stratagus.h"
#include "video.h"
#include "sound.h"
#include "unitsound.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "map.h"
#include "ai.h"
#include "network.h"
#include "netconnect.h"
#include "interface.h"
#include "iolib.h"
#include "ui.h"
#include "actions.h"

/*----------------------------------------------------------------------------
--  Documentation
----------------------------------------------------------------------------*/

/**
**  @class CPlayer player.h
**
**  \#include "player.h"
**
**  This structure contains all informations about a player in game.
**
**  The player structure members:
**
**  CPlayer::Player
**
**    This is the unique slot number. It is not possible that two
**    players have the same slot number at the same time. The slot
**    numbers are reused in the future. This means if a player is
**    defeated, a new player can join using this slot. Currently
**    #PlayerMax (16) players are supported. This member is used to
**    access bit fields.
**    Slot #PlayerNumNeutral (15) is reserved for the neutral units
**    like gold-mines or critters.
**
**    @note Should call this member Slot?
**
**  CPlayer::Name
**
**    Name of the player used for displays and network game.
**    It is restricted to 15 characters plus final zero.
**
**  CPlayer::Type
**
**    Type of the player. This field is setup from the level (map).
**    We support currently #PlayerNeutral,
**    #PlayerNobody, #PlayerComputer, #PlayerPerson,
**    #PlayerRescuePassive and #PlayerRescueActive.
**    @see #PlayerTypes.
**
**  CPlayer::RaceName
**
**    Name of the race to which the player belongs, used to select
**    the user interface and the AI.
**    We have 'orc', 'human', 'alliance' or 'mythical'. Should
**    only be used during configuration and not during runtime.
**
**  CPlayer::Race
**
**    Race number of the player. This field is setup from the level
**    map. This number is mapped with #PlayerRaces to the symbolic
**    name CPlayer::RaceName.
**
**  CPlayer::AiName
**
**    AI name for computer. This field is setup
**    from the map. Used to select the AI for the computer
**    player.
**
**  CPlayer::Team
**
**    Team of player. Selected during network game setup. All players
**    of the same team are allied and enemy to all other teams.
**    @note It is planned to show the team on the map.
**
**  CPlayer::Enemy
**
**    A bit field which contains the enemies of this player.
**    If CPlayer::Enemy & (1<<CPlayer::Player) != 0 its an enemy.
**    Setup during startup using the CPlayer::Team, can later be
**    changed with diplomacy. CPlayer::Enemy and CPlayer::Allied
**    are combined, if none bit is set, the player is neutral.
**    @note You can be allied to a player, which sees you as enemy.
**
**  CPlayer::Allied
**
**    A bit field which contains the allies of this player.
**    If CPlayer::Allied & (1<<CPlayer::Player) != 0 its an allied.
**    Setup during startup using the Player:Team, can later be
**    changed with diplomacy. CPlayer::Enemy and CPlayer::Allied
**    are combined, if none bit is set, the player is neutral.
**    @note You can be allied to a player, which sees you as enemy.
**
**  CPlayer::SharedVision
**
**    A bit field which contains shared vision for this player.
**    Shared vision only works when it's activated both ways. Really.
**
**  CPlayer::StartX CPlayer::StartY
**
**    The tile map coordinates of the player start position. 0,0 is
**    the upper left on the map. This members are setup from the
**    map and only important for the game start.
**    Ignored if game starts with level settings. Used to place
**    the initial workers if you play with 1 or 3 workers.
**
**  CPlayer::Resources[::MaxCosts]
**
**    How many resources the player owns. Needed for building
**    units and structures.
**    @see _costs_, TimeCost, GoldCost, WoodCost, OilCost, MaxCosts.
**
**  CPlayer::MaxResources[::MaxCosts]
**
**    How many resources the player can store at the moment.
**
**  CPlayer::Incomes[::MaxCosts]
**
**    Income of the resources, when they are delivered at a store.
**    @see _costs_, TimeCost, GoldCost, WoodCost, OilCost, MaxCosts.
**
**  CPlayer::LastResources[::MaxCosts]
**
**    Keeps track of resources in time (used for calculating
**    CPlayer::Revenue, see below)
**
**  CPlayer::Revenue[::MaxCosts]
**
**    Production of resources per minute (or estimates)
**    Used just as information (statistics) for the player...
**
**  CPlayer::UnitTypesCount[::UnitTypeMax]
**
**    Total count for each different unit type. Used by the AI and
**    for dependencies checks. The addition of all counts should
**    be CPlayer::TotalNumUnits.
**    @note Should not use the maximum number of unit-types here,
**    only the real number of unit-types used.
**
**  CPlayer::AiEnabled
**
**    If the player is controlled by the computer and this flag is
**    true, than the player is handled by the AI on this local
**    computer.
**
**    @note Currently the AI is calculated parallel on all computers
**    in a network play. It is planned to change this.
**
**  CPlayer::Ai
**
**    AI structure pointer. Please look at #PlayerAi for more
**    informations.
**
**  CPlayer::Units
**
**    A table of all (CPlayer::TotalNumUnits) units of the player.
**
**  CPlayer::TotalNumUnits
**
**    Total number of units (incl. buildings) in the CPlayer::Units
**    table.
**
**  CPlayer::Demand
**
**    Total unit demand, used to demand limit.
**    A player can only build up to CPlayer::Food units and not more
**    than CPlayer::FoodUnitLimit units.
**
**    @note that CPlayer::NumFoodUnits > CPlayer::Food, when enough
**    farms are destroyed.
**
**  CPlayer::NumBuildings
**
**    Total number buildings, units that don't need food.
**
**  CPlayer::Food
**
**    Number of food available/produced. Player can't train more
**    CPlayer::NumFoodUnits than this.
**    @note that all limits are always checked.
**
**  CPlayer::FoodUnitLimit
**
**    Number of food units allowed. Player can't train more
**    CPlayer::NumFoodUnits than this.
**    @note that all limits are always checked.
**
**  CPlayer::BuildingLimit
**
**    Number of buildings allowed.  Player can't build more
**    CPlayer::NumBuildings than this.
**    @note that all limits are always checked.
**
**  CPlayer::TotalUnitLimit
**
**    Number of total units allowed. Player can't have more
**    CPlayer::NumFoodUnits+CPlayer::NumBuildings=CPlayer::TotalNumUnits
**    this.
**    @note that all limits are always checked.
**
**  CPlayer::Score
**
**    Total number of points. You can get points for killing units,
**    destroying buildings ...
**
**  CPlayer::TotalUnits
**
**    Total number of units made.
**
**  CPlayer::TotalBuildings
**
**    Total number of buildings made.
**
**  CPlayer::TotalResources[::MaxCosts]
**
**    Total number of resources collected.
**    @see _costs_, TimeCost, GoldCost, WoodCost, OilCost, MaxCosts.
**
**  CPlayer::TotalRazings
**
**    Total number of buildings destroyed.
**
**  CPlayer::TotalKills
**
**    Total number of kills.
**
**  CPlayer::Color
**
**    Color of units of this player on the minimap. Index number
**    into the global palette.
**
**  CPlayer::UnitColors
**
**    Unit colors of this player. Contains the hardware dependent
**    pixel values for the player colors (palette index #208-#211).
**    Setup from the global palette.
**    @note Index #208-#211 are various SHADES of the team color
**    (#208 is brightest shade, #211 is darkest shade) .... these
**    numbers are NOT red=#208, blue=#209, etc
**
**  CPlayer::Allow
**
**    Contains which unit-types and upgrades are allowed for the
**    player. Possible values are:
**    @li  `A' -- allowed,
**    @li  `F' -- forbidden,
**    @li  `R' -- acquired, perhaps other values
**    @li  `Q' -- acquired but forbidden (does it make sense?:))
**    @li  `E' -- enabled, allowed by level but currently forbidden
**    @see CAllow
**
**  CPlayer::UpgradeTimers
**
**    Timer for the upgrades. One timer for all possible upgrades.
**    Initial 0 counted up by the upgrade action, until it reaches
**    the upgrade time.
**    @see _upgrade_timers_
**    @note it is planned to combine research for faster upgrades.
*/

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

int NumPlayers;                  /// How many player slots used
CPlayer Players[PlayerMax];       /// All players in play
CPlayer *ThisPlayer;              /// Player on this computer
PlayerRace PlayerRaces;          /// Player races

bool NoRescueCheck;               /// Disable rescue check

/**
**  Colors used for minimap.
*/
SDL_Color *PlayerColorsRGB[PlayerMax];
Uint32 *PlayerColors[PlayerMax];

std::string PlayerColorNames[PlayerMax];

/**
**  Which indexes to replace with player color
*/
int PlayerColorIndexStart;
int PlayerColorIndexCount;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Init players.
*/
void InitPlayers()
{
	for (int p = 0; p < PlayerMax; ++p) {
		Players[p].Index = p;
		if (!Players[p].Type) {
			Players[p].Type = PlayerNobody;
		}
		for (int x = 0; x < PlayerColorIndexCount; ++x) {
			PlayerColors[p][x] = Video.MapRGB(TheScreen->format,
				PlayerColorsRGB[p][x].r,
				PlayerColorsRGB[p][x].g, PlayerColorsRGB[p][x].b);
		}
	}
}

/**
**  Clean up players.
*/
void CleanPlayers()
{
	ThisPlayer = NULL;
	for (unsigned int i = 0; i < PlayerMax; ++i) {
		Players[i].Clear();
	}
	NumPlayers = 0;
	NoRescueCheck = false;
}

#ifdef DEBUG
void FreePlayerColors()
{
	for (int i = 0; i < PlayerMax; ++i) {
		delete[] Players[i].UnitColors.Colors;
		delete[] PlayerColorsRGB[i];
		PlayerColorsRGB[i] = NULL;
		delete[] PlayerColors[i];
		PlayerColors[i] = NULL;
	}
}
#endif

/**
**  Clean up the PlayerRaces names.
*/
void CleanRaces()
{
	for (unsigned int i = 0; i < PlayerRaces.Count; ++i) {
		PlayerRaces.Name[i].clear();
		PlayerRaces.Display[i].clear();
	}
	PlayerRaces.Count = 0;
}

/**
**  Save state of players to file.
**
**  @param file  Output file.
**
**  @note FIXME: Not completely saved.
*/
void SavePlayers(CFile &file)
{
	Uint8 r, g, b;

	file.printf("\n--------------------------------------------\n");
	file.printf("--- MODULE: players\n\n");

	//  Dump all players
	for (int i = 0; i < NumPlayers; ++i) {
		const CPlayer &p = Players[i];
		file.printf("Player(%d,\n", i);
		file.printf("  \"name\", \"%s\",\n", p.Name.c_str());
		file.printf("  \"type\", ");
		switch (p.Type) {
			case PlayerNeutral:       file.printf("\"neutral\",");         break;
			case PlayerNobody:        file.printf("\"nobody\",");          break;
			case PlayerComputer:      file.printf("\"computer\",");        break;
			case PlayerPerson:        file.printf("\"person\",");          break;
			case PlayerRescuePassive: file.printf("\"rescue-passive\",");break;
			case PlayerRescueActive:  file.printf("\"rescue-active\","); break;
			default:                  file.printf("%d,", p.Type);break;
		}
		file.printf(" \"race\", \"%s\",", PlayerRaces.Name[p.Race].c_str());
		file.printf(" \"ai-name\", \"%s\",\n", p.AiName.c_str());
		file.printf("  \"team\", %d,", p.Team);

		file.printf(" \"enemy\", \"");
		for (int j = 0; j < PlayerMax; ++j) {
			file.printf("%c",(p.Enemy & (1 << j)) ? 'X' : '_');
		}
		file.printf("\", \"allied\", \"");
		for (int j = 0; j < PlayerMax; ++j) {
			file.printf("%c", (p.Allied & (1 << j)) ? 'X' : '_');
		}
		file.printf("\", \"shared-vision\", \"");
		for (int j = 0; j < PlayerMax; ++j) {
			file.printf("%c", (p.SharedVision & (1 << j)) ? 'X' : '_');
		}
		file.printf("\",\n  \"start\", {%d, %d},\n", p.StartPos.x, p.StartPos.y);

		// Resources
		file.printf("  \"resources\", {");
		for (int j = 0; j < MaxCosts; ++j) {
			file.printf("\"%s\", %d, ", DefaultResourceNames[j].c_str(), p.Resources[j]);
		}
		// Max Resources
		file.printf("},\n  \"max-resources\", {");
		for (int j = 0; j < MaxCosts; ++j) {
			file.printf("\"%s\", %d, ", DefaultResourceNames[j].c_str(), p.MaxResources[j]);
		}
		// Last Resources
		file.printf("},\n  \"last-resources\", {");
		for (int j = 0; j < MaxCosts; ++j) {
			file.printf("\"%s\", %d, ", DefaultResourceNames[j].c_str(), p.LastResources[j]);
		}
		// Incomes
		file.printf("},\n  \"incomes\", {");
		for (int j = 0; j < MaxCosts; ++j) {
			if (j) {
				if (j == MaxCosts / 2) {
					file.printf("\n ");
				} else {
					file.printf(" ");
				}
			}
			file.printf("\"%s\", %d,", DefaultResourceNames[j].c_str(), p.Incomes[j]);
		}
		// Revenue
		file.printf("},\n  \"revenue\", {");
		for (int j = 0; j < MaxCosts; ++j) {
			if (j) {
				if (j == MaxCosts / 2) {
					file.printf("\n ");
				} else {
					file.printf(" ");
				}
			}
			file.printf("\"%s\", %d,", DefaultResourceNames[j].c_str(), p.Revenue[j]);
		}

		// UnitTypesCount done by load units.

		file.printf("},\n  \"%s\",\n", p.AiEnabled ? "ai-enabled" : "ai-disabled");

		// Ai done by load ais.
		// Units done by load units.
		// TotalNumUnits done by load units.
		// NumBuildings done by load units.

		file.printf(" \"supply\", %d,", p.Supply);
		file.printf(" \"unit-limit\", %d,", p.UnitLimit);
		file.printf(" \"building-limit\", %d,", p.BuildingLimit);
		file.printf(" \"total-unit-limit\", %d,", p.TotalUnitLimit);

		file.printf("\n  \"score\", %d,", p.Score);
		file.printf("\n  \"total-units\", %d,", p.TotalUnits);
		file.printf("\n  \"total-buildings\", %d,", p.TotalBuildings);
		file.printf("\n  \"total-resources\", {");
		for (int j = 0; j < MaxCosts; ++j) {
			if (j) {
				file.printf(" ");
			}
			file.printf("%d,", p.TotalResources[j]);
		}
		file.printf("},");
		file.printf("\n  \"total-razings\", %d,", p.TotalRazings);
		file.printf("\n  \"total-kills\", %d,", p.TotalKills);

		SDL_GetRGB(p.Color, TheScreen->format, &r, &g, &b);
		file.printf("\n  \"color\", { %d, %d, %d },", r, g, b);

		// UnitColors done by init code.
		// Allow saved by allow.

		file.printf("\n  \"timers\", {");
		for (int j = 0; j < UpgradeMax; ++j) {
			if (j) {
				file.printf(" ,");
			}
			file.printf("%d", p.UpgradeTimers.Upgrades[j]);
		}
		file.printf("}");

		file.printf(")\n\n");
	}

	DebugPrint("FIXME: must save unit-stats?\n");

	//  Dump local variables
	file.printf("SetThisPlayer(%d)\n\n", ThisPlayer->Index);
}

/**
**  Create a new player.
**
**  @param type  Player type (Computer,Human,...).
*/
void CreatePlayer(int type)
{
	if (NumPlayers == PlayerMax) { // already done for bigmaps!
		return;
	}
	CPlayer &player = Players[NumPlayers];
	player.Index = NumPlayers;

	//  Allocate memory for the "list" of this player's units.
	//  FIXME: brutal way, as we won't need UnitMax for this player...
	//  FIXME: ARI: is this needed for 'PlayerNobody' ??
	//  FIXME: A: Johns: currently we need no init for the nobody player.
	memset(player.Units, 0, sizeof (player.Units));

	//  Take first slot for person on this computer,
	//  fill other with computer players.
	if (type == PlayerPerson && !NetPlayers) {
		if (!ThisPlayer) {
			ThisPlayer = &player;
		} else {
			type = PlayerComputer;
		}
	}
	if (NetPlayers && NumPlayers == NetLocalPlayerNumber) {
		ThisPlayer = &Players[NetLocalPlayerNumber];
	}

	if (NumPlayers == PlayerMax) {
		static int already_warned;

		if (!already_warned) {
			DebugPrint("Too many players\n");
			already_warned = 1;
		}
		return;
	}

	//  Make simple teams:
	//  All person players are enemies.
	int team;
	switch (type) {
		case PlayerNeutral:
		case PlayerNobody:
		default:
			team = 0;
			player.SetName("Neutral");
			break;
		case PlayerComputer:
			team = 1;
			player.SetName("Computer");
			break;
		case PlayerPerson:
			team = 2 + NumPlayers;
			player.SetName("Person");
			break;
		case PlayerRescuePassive:
		case PlayerRescueActive:
			// FIXME: correct for multiplayer games?
			player.SetName("Computer");
			team = 2 + NumPlayers;
			break;
	}
	DebugPrint("CreatePlayer name %s\n" _C_ player.Name.c_str());

	player.Type = type;
	player.Race = 0;
	player.Team = team;
	player.Enemy = 0;
	player.Allied = 0;
	player.AiName = "ai-passive";

	//  Calculate enemy/allied mask.
	for (int i = 0; i < NumPlayers; ++i) {
		switch (type) {
			case PlayerNeutral:
			case PlayerNobody:
			default:
				break;
			case PlayerComputer:
				// Computer allied with computer and enemy of all persons.
				if (Players[i].Type == PlayerComputer) {
					player.Allied |= (1 << i);
					Players[i].Allied |= (1 << NumPlayers);
				} else if (Players[i].Type == PlayerPerson ||
						Players[i].Type == PlayerRescueActive) {
					player.Enemy |= (1 << i);
					Players[i].Enemy |= (1 << NumPlayers);
				}
				break;
			case PlayerPerson:
				// Humans are enemy of all?
				if (Players[i].Type == PlayerComputer ||
						Players[i].Type == PlayerPerson) {
					player.Enemy |= (1 << i);
					Players[i].Enemy |= (1 << NumPlayers);
				} else if (Players[i].Type == PlayerRescueActive ||
						Players[i].Type == PlayerRescuePassive) {
					player.Allied |= (1 << i);
					Players[i].Allied |= (1 << NumPlayers);
				}
				break;
			case PlayerRescuePassive:
				// Rescue passive are allied with persons
				if (Players[i].Type == PlayerPerson) {
					player.Allied |= (1 << i);
					Players[i].Allied |= (1 << NumPlayers);
				}
				break;
			case PlayerRescueActive:
				// Rescue active are allied with persons and enemies of computer
				if (Players[i].Type == PlayerComputer) {
					player.Enemy |= (1 << i);
					Players[i].Enemy |= (1 << NumPlayers);
				} else if (Players[i].Type == PlayerPerson) {
					player.Allied |= (1 << i);
					Players[i].Allied |= (1 << NumPlayers);
				}
				break;
		}
	}

	//  Initial default incomes.
	for (int i = 0; i < MaxCosts; ++i) {
		player.Incomes[i] = DefaultIncomes[i];
	}

	//  Initial max resource amounts.
	for (int i = 0; i < MaxCosts; ++i) {
		player.MaxResources[i] = DefaultResourceMaxAmounts[i];
	}

	memset(player.UnitTypesCount, 0, sizeof (player.UnitTypesCount));

	player.Supply = 0;
	player.Demand = 0;
	player.NumBuildings = 0;
	player.TotalNumUnits = 0;
	player.Score = 0;

	player.Color = PlayerColors[NumPlayers][0];

	if (Players[NumPlayers].Type == PlayerComputer ||
			Players[NumPlayers].Type == PlayerRescueActive) {
		player.AiEnabled = 1;
	} else {
		player.AiEnabled = 0;
	}
	++NumPlayers;
}

/**
**  Change player name.
**
**  @param name    New name.
*/
void CPlayer::SetName(const std::string &name)
{
	Name = name;
}

/**
**  Clear all player data excepts members which don't change.
**
**  The fields that are not cleared are
**  UnitLimit, BuildingLimit, TotalUnitLimit and Allow.
*/
void CPlayer::Clear()
{
	Index = 0;
	Name.clear();
	Type = 0;
	Race = 0;
	AiName.clear();
	Team = 0;
	Enemy = 0;
	Allied = 0;
	SharedVision = 0;
	StartPos.x = 0;
	StartPos.y = 0;
	memset(Resources, 0, sizeof(Resources));
	memset(MaxResources, 0, sizeof(MaxResources));
	memset(LastResources, 0, sizeof(LastResources));
	memset(Incomes, 0, sizeof(Incomes));
	memset(Revenue, 0, sizeof(Revenue));
	memset(UnitTypesCount, 0, sizeof(UnitTypesCount));
	AiEnabled = 0;
	Ai = 0;
	memset(Units, 0, sizeof(Units));
	TotalNumUnits = 0;
	NumBuildings = 0;
	Supply = 0;
	Demand = 0;
	// FIXME: can't clear limits since it's initialized already
//	UnitLimit = 0;
//	BuildingLimit = 0;
//	TotalUnitLimit = 0;
	Score = 0;
	TotalUnits = 0;
	TotalBuildings = 0;
	memset(TotalResources, 0, sizeof(TotalResources));
	TotalRazings = 0;
	TotalKills = 0;
	Color = 0;
	UpgradeTimers.Clear();
}

/*----------------------------------------------------------------------------
--  Resource management
----------------------------------------------------------------------------*/

/**
**  Change the player resource.
**
**  @param resource  Resource to change.
**  @param value     How many of this resource.
*/
void CPlayer::SetResource(int resource, int value)
{
	if (this->MaxResources[resource] != -1) {
		this->Resources[resource] = std::min(value, this->MaxResources[resource]);
	} else {
		this->Resources[resource] = value;
	}
}

/**
**  Check if the unit-type didn't break any unit limits.
**
**  @param type    Type of unit.
**
**  @return        True if enough, negative on problem.
**
**  @note The return values of the PlayerCheck functions are inconsistent.
*/
int CPlayer::CheckLimits(const CUnitType &type) const
{
	//  Check game limits.
	if (NumUnits < UnitMax) {
		if (type.Building && NumBuildings >= BuildingLimit) {
			Notify("%s", _("Building Limit Reached"));
			return -1;
		}
		if (!type.Building && (TotalNumUnits - NumBuildings) >= UnitLimit) {
			Notify("%s", _("Unit Limit Reached"));
			return -2;
		}
		if (this->Demand + type.Demand > this->Supply && type.Demand) {
			Notify("%s", _("Insufficient Supply, increase Supply."));
			return -3;
		}
		if (TotalNumUnits >= TotalUnitLimit) {
			Notify("%s", _("Total Unit Limit Reached"));
			return -4;
		}
		if (UnitTypesCount[type.Slot] >=  Allow.Units[type.Slot]) {
			Notify(_("Limit of %d reached for this unit type"), Allow.Units[type.Slot]);
			return -6;
		}
		return 1;
	} else {
		Notify("%s", _("Cannot create more units."));
		if (AiEnabled) {
			// AiNoMoreUnits(player, type);
		}
		return -5;
	}
}

/**
**  Check if enough resources for are available.
**
**  @param costs   How many costs.
**
**  @return        False if all enough, otherwise a bit mask.
**
**  @note The return values of the PlayerCheck functions are inconsistent.
*/
int CPlayer::CheckCosts(const int *costs) const
{
	int err = 0;
	for (int i = 1; i < MaxCosts; ++i) {
		if (this->Resources[i] >= costs[i]) {
			continue;
		}
		const char *name = DefaultResourceNames[i].c_str();
		const char *actionName = DefaultActions[i].c_str();

		Notify(_("Not enough %s...%s more %s."), name, actionName, name);

		err |= 1 << i;
		if (i == 1 && GameSounds.NotEnough1[this->Race].Sound) {
			PlayGameSound(GameSounds.NotEnough1[this->Race].Sound, MaxSampleVolume);
		}
		if (i == 2 && GameSounds.NotEnough2[this->Race].Sound) {
			PlayGameSound(GameSounds.NotEnough2[this->Race].Sound, MaxSampleVolume);
		}
	}
	return err;
}

/**
**  Check if enough resources for new unit is available.
**
**  @param type    Type of unit.
**
**  @return        False if all enough, otherwise a bit mask.
*/
int CPlayer::CheckUnitType(const CUnitType &type) const
{
	return this->CheckCosts(type.Stats[this->Index].Costs);
}

/**
**  Add costs to the resources
**
**  @param costs   How many costs.
*/
void CPlayer::AddCosts(const int *costs)
{
	for (int i = 1; i < MaxCosts; ++i) {
		SetResource(i, Resources[i] + costs[i]);
	}
}

/**
**  Add the costs of an unit type to resources
**
**  @param type    Type of unit.
*/
void CPlayer::AddUnitType(const CUnitType &type)
{
	AddCosts(type.Stats[this->Index].Costs);
}

/**
**  Add a factor of costs to the resources
**
**  @param costs   How many costs.
**  @param factor  Factor of the costs to apply.
*/
void CPlayer::AddCostsFactor(const int *costs, int factor)
{
	for (int i = 1; i < MaxCosts; ++i) {
		SetResource(i, this->Resources[i] + costs[i] * factor / 100);
	}
}

/**
**  Subtract costs from the resources
**
**  @param costs   How many costs.
*/
void CPlayer::SubCosts(const int *costs)
{
	for (int i = 1; i < MaxCosts; ++i) {
		SetResource(i, this->Resources[i] - costs[i]);
	}
}

/**
**  Substract the costs of new unit from resources
**
**  @param type    Type of unit.
*/
void CPlayer::SubUnitType(const CUnitType &type)
{
	this->SubCosts(type.Stats[this->Index].Costs);
}

/**
**  Substract a factor of costs from the resources
**
**  @param costs   How many costs.
**  @param factor  Factor of the costs to apply.
*/
void CPlayer::SubCostsFactor(const int *costs, int factor)
{
	for (int i = 1; i < MaxCosts; ++i) {
		SetResource(i, this->Resources[i] - costs[i] * 100 / factor);
	}
}

/**
**  Have unit of type.
**
**  @param type    Type of unit.
**
**  @return        How many exists, false otherwise.
*/
int CPlayer::HaveUnitTypeByType(const CUnitType &type) const
{
	return UnitTypesCount[type.Slot];
}

/**
**  Have unit of type.
**
**  @param ident   Identifier of unit-type that should be lookuped.
**
**  @return        How many exists, false otherwise.
**
**  @note This function should not be used during run time.
*/
int CPlayer::HaveUnitTypeByIdent(const std::string &ident) const
{
	return UnitTypesCount[UnitTypeByIdent(ident)->Slot];
}

/**
**  Initialize the Ai for all players.
*/
void PlayersInitAi()
{
	for (int player = 0; player < NumPlayers; ++player) {
		if (Players[player].AiEnabled) {
			AiInit(Players[player]);
		}
	}
}

/**
**  Handle AI of all players each game cycle.
*/
void PlayersEachCycle()
{
	for (int player = 0; player < NumPlayers; ++player) {
		CPlayer &p = Players[player];

		if (p.AiEnabled) {
			AiEachCycle(p);
		}
	}
}

/**
**  Handle AI of a player each second.
**
**  @param playerIdx  the player to update AI
*/
void PlayersEachSecond(int playerIdx)
{
	CPlayer &player = Players[playerIdx];

	if ((GameCycle / CYCLES_PER_SECOND) % 10 == 0) {
		for (int res = 0; res < MaxCosts; ++res) {
			player.Revenue[res] = player.Resources[res] - player.LastResources[res];
			player.Revenue[res] *= 6;  // estimate per minute
			player.LastResources[res] = player.Resources[res];
		}
	}
	if (player.AiEnabled) {
		AiEachSecond(player);
	}
}

/**
**  Change current color set to new player.
**
**  FIXME: use function pointer here.
**
**  @param player  Pointer to player.
**  @param sprite  The sprite in which the colors should be changed.
*/
void GraphicPlayerPixels(CPlayer &player, const CGraphic &sprite)
{
	Assert(PlayerColorIndexCount);

	SDL_LockSurface(sprite.Surface);
	SDL_SetColors(sprite.Surface, player.UnitColors.Colors, PlayerColorIndexStart, PlayerColorIndexCount);
	if (sprite.SurfaceFlip) {
		SDL_SetColors(sprite.SurfaceFlip, player.UnitColors.Colors, PlayerColorIndexStart, PlayerColorIndexCount);
	}
	SDL_UnlockSurface(sprite.Surface);
}

/**
**  Setup the player colors for the current palette.
**
**  @todo  FIXME: could be called before PixelsXX is setup.
*/
void SetPlayersPalette()
{
	for (int i = 0; i < PlayerMax; ++i) {
		delete[] Players[i].UnitColors.Colors;
		Players[i].UnitColors.Colors = new SDL_Color[PlayerColorIndexCount];
		memcpy(Players[i].UnitColors.Colors, PlayerColorsRGB[i],
			sizeof(SDL_Color) * PlayerColorIndexCount);
	}
}

/**
**  Output debug informations for players.
*/
void DebugPlayers()
{
#ifdef DEBUG
	DebugPrint("Nr   Color   I Name     Type         Race    Ai\n");
	DebugPrint("--  -------- - -------- ------------ ------- -----\n");
	for (int i = 0; i < PlayerMax; ++i) {
		if (Players[i].Type == PlayerNobody) {
			continue;
		}
		const char *playertype;

		switch (Players[i].Type) {
			case 0: playertype = "Don't know 0"; break;
			case 1: playertype = "Don't know 1"; break;
			case 2: playertype = "neutral     "; break;
			case 3: playertype = "nobody      "; break;
			case 4: playertype = "computer    "; break;
			case 5: playertype = "person      "; break;
			case 6: playertype = "rescue pas. "; break;
			case 7: playertype = "rescue akt. "; break;
			default : playertype = "?unknown?   "; break;
		}
		DebugPrint("%2d: %8.8s %c %-8.8s %s %7s %s\n" _C_ i _C_ PlayerColorNames[i].c_str() _C_
			ThisPlayer == &Players[i] ? '*' :
				Players[i].AiEnabled ? '+' : ' ' _C_
			Players[i].Name.c_str() _C_ playertype _C_
			PlayerRaces.Name[Players[i].Race].c_str() _C_
			Players[i].AiName.c_str());
	}
#endif
}

/**
**  Notify player about a problem.
**
**  @param type    Problem type
**  @param pos     Map tile position
**  @param fmt     Message format
**  @param ...     Message varargs
**
**  @todo FIXME: We must also notfiy allied players.
*/
void CPlayer::Notify(int type, const Vec2i &pos, const char *fmt, ...) const
{
	Assert(Map.Info.IsPointOnMap(pos));
	char temp[128];
	Uint32 color;
	va_list va;

	// Notify me, and my TEAM members
	if (this != ThisPlayer && !IsTeamed(*ThisPlayer)) {
		return;
	}

	va_start(va, fmt);
	temp[sizeof(temp) - 1] = '\0';
	vsnprintf(temp, sizeof(temp) - 1, fmt, va);
	va_end(va);
	switch (type)
	{
		case NotifyRed:
			color = ColorRed;
			break;
		case NotifyYellow:
			color = ColorYellow;
			break;
		case NotifyGreen:
			color = ColorGreen;
			break;
		default: color = ColorWhite;
	}
	UI.Minimap.AddEvent(pos.x, pos.y, color);
	if (this == ThisPlayer) {
		SetMessageEvent(pos, "%s", temp);
	} else {
		SetMessageEvent(pos, "(%s): %s", Name.c_str(), temp);
	}
}

/**
**  Notify player about a problem.
**
**  @param type    Problem type
**  @param pos     Map tile position
**  @param fmt     Message format
**  @param ...     Message varargs
**
**  @todo FIXME: We must also notfiy allied players.
*/
void CPlayer::Notify(const char *fmt, ...) const
{
	// Notify me, and my TEAM members
	if (this != ThisPlayer && !IsTeamed(*ThisPlayer)) {
		return;
	}
	char temp[128];
	va_list va;

	va_start(va, fmt);
	temp[sizeof(temp) - 1] = '\0';
	vsnprintf(temp, sizeof(temp) - 1, fmt, va);
	va_end(va);
	if (this == ThisPlayer) {
		SetMessage("%s", temp);
	} else {
		SetMessage("(%s): %s", Name.c_str(), temp);
	}
}


/**
**  Check if the player is an enemy
*/
bool CPlayer::IsEnemy(const CPlayer &player) const
{
	return IsEnemy(player.Index);
}

/**
**  Check if the unit is an enemy
*/
bool CPlayer::IsEnemy(const CUnit &unit) const
{
	return IsEnemy(*unit.Player);
}

/**
**  Check if the player is an ally
*/
bool CPlayer::IsAllied(const CPlayer &player) const
{
	return (Allied & (1 << player.Index)) != 0;
}

/**
**  Check if the unit is an ally
*/
bool CPlayer::IsAllied(const CUnit &unit) const
{
	return IsAllied(*unit.Player);
}

/**
**  Check if the player shares vision with the player
*/
bool CPlayer::IsSharedVision(const CPlayer &player) const
{
	return (SharedVision & (1 << player.Index)) != 0;
}

/**
**  Check if the player shares vision with the unit
*/
bool CPlayer::IsSharedVision(const CUnit &unit) const
{
	return IsSharedVision(*unit.Player);
}

/**
**  Check if the both players share vision
*/
bool CPlayer::IsBothSharedVision(const CPlayer &player) const
{
	return (SharedVision & (1 << player.Index)) != 0 &&
		(player.SharedVision & (1 << Index)) != 0;
}

/**
**  Check if the player and the unit share vision
*/
bool CPlayer::IsBothSharedVision(const CUnit &unit) const
{
	return IsBothSharedVision(*unit.Player);
}

/**
**  Check if the player is teamed
*/
bool CPlayer::IsTeamed(const CPlayer &player) const
{
	return Team == player.Team;
}

/**
**  Check if the unit is teamed
*/
bool CPlayer::IsTeamed(const CUnit &unit) const
{
	return IsTeamed(*unit.Player);
}

//@}
