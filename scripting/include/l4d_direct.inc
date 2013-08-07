/**
 * vim: set ts=4 :
 * =============================================================================
 * L4D_Direct
 * Copyright (C) 2012 Michael "ProdigySim" Busby
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */
#if defined _l4d_direct_included
 #endinput
#endif
#define _l4d_direct_included

#include <sourcemod>
#include <l4dd_internals>

/* --- CDirector Variable access --- */

/**
 * Gets the number of times the tank has passed to a player.
 *
 * @note When this variable is >1 the tank will be replaced with a bot when the his frustration reaches 0.
 * @note The initial pass from AI to a player counts as a pass.
 * @note As this is global on the director weird things could potentially happen if more than one tank is alive at a time with z_frustration 1.
 *
 * @return          The number of passes.
 */
stock L4DDirect_GetTankPassedCount()
{
	return LoadFromAddress(L4DDirect_GetTankPassedCountAddr(), NumberType_Int32);
}

/**
 * Sets the number of times the tank has passed to a player.
 *
 * @note When this variable is >1 the tank will be replaced with a bot when the his frustration reaches 0.
 * @note The initial pass from AI to a player counts as a pass.
 * @note As this is global on the director weird things could potentially happen if more than one tank is alive at a time with z_frustration 1.
 *
 * @param           New number of passes value
 * @noreturn
 */
stock L4DDirect_SetTankPassedCount(passes)
{
	StoreToAddress(L4DDirect_GetTankPassedCountAddr(), passes, NumberType_Int32);
}

/**
 * Reads the tank flow percent for a given round for versus mode
 *
 * @note When the survivors reach this flow percent minus versus_boss_buffer converted to flow percent, a tank will spawn.
 *
 * @param roundNumber	Round number to read tank spawn flow percent of
 * @return				Tank spawn flow percent for the given round
 * @error				Director address not found.
 */
stock Float:L4DDirect_GetVSTankFlowPercent(roundNumber)
{
	if (roundNumber < 0 || roundNumber > 1) return -1.0;

	return Float:LoadFromAddress(L4DDirect_GetVSTankFlowAddr() + Address:(roundNumber * 4), NumberType_Int32);
}

/**
 * Sets the tank flow percent for a given round for versus mode
 *
 * @note When the survivors reach this flow percent minus versus_boss_buffer converted to flow percent, a tank will spawn.
 *
 * @param roundNumber	Round number to set tank spawn flow percent of
 * @param flow			Floating point percent of flow distance.
 * @noreturn
 * @error				Director address not found.
 */
stock L4DDirect_SetVSTankFlowPercent(roundNumber, Float:flow)
{
	if (roundNumber < 0 || roundNumber > 1) return;

	StoreToAddress(L4DDirect_GetVSTankFlowAddr() + Address:(roundNumber * 4), _:flow, NumberType_Int32);
}

/* --- TerrorNavMesh Variable access --- */

/**
 * Get the max flow distance (in flow units) for the current map.
 *
 * @note The flow distance for each map is generated as it is loaded, and it can change slightly (a few hundred units) with each load.
 * @note You can use this value to convert a flow distance to a flow percent, and vice versa.
 *
 * @return				Max flow distance for the current loaded map.
 * @error				TerrorNavMesh address not found.
 */
stock Float:L4DDirect_GetMapMaxFlowDistance()
{
	return Float:LoadFromAddress(L4DDirect_GetMapMaxFlowDistanceAddr(), NumberType_Int32);
}

/* --- CTerrorPlayer Variable access --- */

/**
 * Get the clients health bonus.
 *
 * @note Survivors health bonuses are 0 until CTerrorPlayer:RecalculateVersusScore(void) calculates it.
 *
 * @param client		Index of the client whose health bonus is to be returned.
 * @return				Int value of the survivors health bonus.
*/
stock L4DDirect_GetSurvivorHealthBonus(client)
{
	if (!IsClient(client))
		return -1;

	decl value;
	if (GetEntityValueFromOffset(client, "CTerrorPlayer::m_iSurvivorHealthBonus", NumberType_Int32, value))
		return value;

	return -1;
}

/**
 * Looks up the number of tickets a client has for entry into the tank lottery.
 *
 * @note The number of tickets you have is equal to your damage done as an SI and will still increase as you do damage with the Tank.
 * @note When the tank is passed away from you your tickets are set back to zero.
 *
 * @param client		Client id to get the tickets for
 * @return				Number of tickets.
 * @error				Invalid client.
 */
stock L4DDirect_GetTankTickets(client)
{
	if (!IsClient(client))
		return -1;

	decl value;
	if (GetEntityValueFromOffset(client, "CTerrorPlayer::m_iTankTickets", NumberType_Int32, value))
		return value;

	return -1;
}

/**
 * Sets the number of tickets a player has for entry into the tank lottery.
 *
 * @param client		Client id to set the tickets for
 * @param tickets		New value for the client's tank lottery tickets
 * @noreturn
 * @error				Invalid client.
 */
stock L4DDirect_SetTankTickets(client, tickets)
{
	if (!IsClient(client))
		return;

	SetEntityValueFromOffset(client, "CTerrorPlayer::m_iTankTickets", NumberType_Int32, tickets);
}

/* --- Functions Calls --- */

/**
 * Force the director to pass the tank.
 *
 * @param entity			Entity index of the tank
 * @param bEnterStasis		Should the tank be put in statis
 * @return					False on error otherwise true
 * @error					SDK call preparation failed
 */
stock bool:L4DDirect_TryOfferingTankBot(entity, bEnterStasis)
{
	static Handle:TryOfferingTankBotSDKCall = INVALID_HANDLE;

	if (TryOfferingTankBotSDKCall == INVALID_HANDLE)
	{
		StartPrepSDKCall(SDKCall_Raw);

		if (!PrepSDKCall_SetFromConf(L4DDirect_GetGameConf(), SDKConf_Signature, "CDirector::TryOfferingTankBot"))
		{
			return false;
		}

		PrepSDKCall_AddParameter(SDKType_CBaseEntity, SDKPass_Pointer);
		PrepSDKCall_AddParameter(SDKType_Bool, SDKPass_Plain);
		TryOfferingTankBotSDKCall = EndPrepSDKCall();

		if (TryOfferingTankBotSDKCall == INVALID_HANDLE)
		{
			return false;
		}
	}

	SDKCall(TryOfferingTankBotSDKCall, L4DDirect_GetCDirector(), entity, bEnterStasis);
	return true;
}

/**
 * Force to pass the tank to another player.
 *
 * @param entity			Entity index of the tank
 * @param target			Entity index of infected player
 * @return					False on error otherwise true
 * @error					SDK call preparation failed
 */
stock bool:L4DDirect_ReplaceTank(entity, target)
{
	static Handle:hReplaceTank;

	if (hReplaceTank == INVALID_HANDLE){

		StartPrepSDKCall(SDKCall_Raw);

		if (PrepSDKCall_SetFromConf(L4DDirect_GetGameConf(), SDKConf_Signature, "ZombieManager::ReplaceTank")){

			PrepSDKCall_AddParameter(SDKType_CBasePlayer, SDKPass_Pointer);
			PrepSDKCall_AddParameter(SDKType_CBasePlayer, SDKPass_Pointer);
			hReplaceTank = EndPrepSDKCall();
		}

		if (hReplaceTank == INVALID_HANDLE)
			return false;
	}

	SDKCall(hReplaceTank, L4DDirect_GetZombieManager(), entity, target);
	return true;
}

static bool:IsClient(index)
{
	return index > 0 && index <= MaxClients;
}
