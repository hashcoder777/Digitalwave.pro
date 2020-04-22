#include "ResolverRecode.h"
#include "..\Aimbot\Aimbot.h"
#include "..\Aimbot\Autowall.h"
#include "..\Aimbot\LagComp.h"
#include "..\..\Utils\Utils.h"
#include "..\..\SDK\IVEngineClient.h"
#include "..\..\SDK\Hitboxes.h"
#include "..\..\SDK\PlayerInfo.h"
#include "..\..\Utils\Math.h"
#include "..\..\SDK\IBaseClientDll.h"
#include "../../SDK/ICvar.h"
#include "..\..\Menu\Menu.h"

ResolverRecode g_ResolverRecode;

void ResolverRecode::HandleHits(C_BaseEntity* pEnt) //def handle
{
	auto NetChannel = g_pEngine->GetNetChannelInfo();

	if (!NetChannel)
		return;

	static float predTime[65];
	static bool init[65];

	if (g::Shot[pEnt->EntIndex()])
	{
		if (init[pEnt->EntIndex()])
		{
			g_ResolverRecode.pitchHit[pEnt->EntIndex()] = pEnt->GetEyeAngles().x;
			predTime[pEnt->EntIndex()] = g_pGlobalVars->curtime + NetChannel->GetAvgLatency(FLOW_INCOMING) + NetChannel->GetAvgLatency(FLOW_OUTGOING) + TICKS_TO_TIME(1) + TICKS_TO_TIME(g_pEngine->GetNetChannel()->m_nChokedPackets);
			init[pEnt->EntIndex()] = false;
		}

		if (g_pGlobalVars->curtime > predTime[pEnt->EntIndex()] && !g::Hit[pEnt->EntIndex()])
		{
			g::MissedShots[pEnt->EntIndex()] += 1;
			g::Shot[pEnt->EntIndex()] = false;
		}
		else if (g_pGlobalVars->curtime <= predTime[pEnt->EntIndex()] && g::Hit[pEnt->EntIndex()])
			g::Shot[pEnt->EntIndex()] = false;

	}
	else
		init[pEnt->EntIndex()] = true;

	g::Hit[pEnt->EntIndex()] = false;
}

void ResolverRecode::fix_local_player_animations()
{
	auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());

	if (!local_player)
		return;

	static float sim_time;
	if (sim_time != local_player->GetSimulationTime())
	{
		auto state = local_player->AnimState(); if (!state) return;

		const float curtime = g_pGlobalVars->curtime;
		const float frametime = g_pGlobalVars->frametime;
		const float realtime = g_pGlobalVars->realtime;
		const float absoluteframetime = g_pGlobalVars->absoluteframetime;
		const float absoluteframestarttimestddev = g_pGlobalVars->absoluteframestarttimestddev;
		const float interpolation_amount = g_pGlobalVars->interpolationAmount;
		const float framecount = g_pGlobalVars->framecount;
		const float tickcount = g_pGlobalVars->tickcount;

		static auto host_timescale = g_pCvar->FindVar(("host_timescale"));

		g_pGlobalVars->curtime = local_player->GetSimulationTime();
		g_pGlobalVars->realtime = local_player->GetSimulationTime();
		g_pGlobalVars->frametime = g_pGlobalVars->intervalPerTick * host_timescale->GetFloat();
		g_pGlobalVars->absoluteframetime = g_pGlobalVars->intervalPerTick * host_timescale->GetFloat();
		g_pGlobalVars->absoluteframestarttimestddev = local_player->GetSimulationTime() - g_pGlobalVars->intervalPerTick * host_timescale->GetFloat();
		g_pGlobalVars->interpolationAmount = 0;
		g_pGlobalVars->framecount = TICKS_TO_TIME(local_player->GetSimulationTime());
		g_pGlobalVars->tickcount = TICKS_TO_TIME(local_player->GetSimulationTime());
		int backup_flags = local_player->GetFlags();

		AnimationLayer backup_layers[15];
		std::memcpy(backup_layers, local_player->GetAnimOverlays(), (sizeof(AnimationLayer) * 15));

		if (state->m_iLastClientSideAnimationUpdateFramecount == g_pGlobalVars->framecount)
			state->m_iLastClientSideAnimationUpdateFramecount = g_pGlobalVars->framecount - 1;

		std::memcpy(local_player->GetAnimOverlays(), backup_layers, (sizeof(AnimationLayer) * 15));


		g_pGlobalVars->curtime = curtime;
		g_pGlobalVars->realtime = realtime;
		g_pGlobalVars->frametime = frametime;
		g_pGlobalVars->absoluteframetime = absoluteframetime;
		g_pGlobalVars->absoluteframestarttimestddev = absoluteframestarttimestddev;
		g_pGlobalVars->interpolationAmount = interpolation_amount;
		g_pGlobalVars->framecount = framecount;
		g_pGlobalVars->tickcount = tickcount;
		sim_time = local_player->GetSimulationTime();
	}
	local_player->invalidate_bone_cache();
	local_player->SetupBones(nullptr, -1, 0x7FF00, g_pGlobalVars->curtime);
}

float ResolverRecode::AngleNormalize(float angle)
{
	angle = fmodf(angle, 360.0f);
	if (angle > 180)
	{
		angle -= 360;
	}
	if (angle < -180)
	{
		angle += 360;
	}
	return angle;
}

float ResolverRecode::NormalizeYaw180(float yaw)
{
	if (yaw > 180)
		yaw -= (round(yaw / 360) * 360.f);
	else if (yaw < -180)
		yaw += (round(yaw / 360) * -360.f);

	return yaw;
}

float ResolverRecode::angle_difference(float a, float b) {
	auto diff = NormalizeYaw180(a - b);

	if (diff < 180)
		return diff;
	return diff - 360;
}

float ResolverRecode::approach(float cur, float target, float inc) {
	inc = abs(inc);

	if (cur < target)
		return min(cur + inc, target);
	if (cur > target)
		return max(cur - inc, target);

	return target;
}

float ResolverRecode::approach_angle(float cur, float target, float inc) {
	auto diff = angle_difference(target, cur);
	return approach(cur, cur + diff, inc);
}

void SanaPidaras(C_BaseEntity* entity)
{
	if (g_Menu.Config.ResolverType == 3)
		return;
	// PRESS F RESOLVER ( ÃÅÉÑÅÍÑ ÏÎ ÝÒÎÌÓ ÝÒÀ ÑÈËÜÍÀ )
	auto local_player = g::pLocalEntity;
	auto animstate = entity->AnimState();
	if (animstate)
	{
		//auto v9 = absolute_mask(COERCE_INT(animstate->last_client_side_animation_update_framecount - animstate->last_client_side_animation_update_time));
		float speedfraction = max(0, min(*reinterpret_cast<float*>(animstate + 0xF8), 1));
		speedfraction = 0.0;
		if (animstate->m_flFeetSpeedUnknownForwardOrSideways < 0.0)
			speedfraction = 0.0;
		else
			speedfraction = fminf(DWORD(animstate->m_flFeetSpeedUnknownForwardOrSideways), 0x3F800000);
		float unk1 = ((*reinterpret_cast<float*> (animstate + 0x11C) * -0.30000001) - 0.19999999) * speedfraction;
		float unk2 = unk1 + 1.f;
		float unk3;
		if (animstate->m_fDuckAmount > 0.0)
		{
			int v29 = 0.0;
			if (animstate->m_flFeetSpeedUnknownForwardOrSideways < 0.0)
				v29 = 0.0;
			else
				v29 = fminf(DWORD(animstate->m_flFeetSpeedUnknownForwardOrSideways), 0x3F800000);
		}
		//GetClientEnt = *(*g_EntityList + 12);// GetLocalPlayer
		//localplayer = GetClientEnt(g_EntityList, loc);
		if (local_player)
		{
			for (int i = 0; i <= 64; ++i)
			{
				auto animation_state = entity->AnimState();

				//player = GetClientEntity(g_EntityList, i);
				if (local_player->IsAlive() && local_player->IsDormant())// dormant
				{
					auto v28 = animation_state->m_flEyeYaw == 0.0 ? -58 : 58;
					if (v28)
						return;
					auto v27 = animation_state->m_flEyeYaw == 0.0 ? -89 : 89;
					if (v27)
						return;
					auto v26 = animation_state->m_flEyeYaw == 0.0 ? -79 : 79;
					if (v26)
						return;
					auto v25 = animation_state->m_flEyeYaw == 0.0 ? -125 : 125;
					if (v25)
						return;
					auto v24 = animation_state->m_flEyeYaw == 0.0 ? -78 : 78;
					if (v24)
						return;
				}
			}
			int v8 = 0;
			int v7 = 0;
			for (int a2a = 0; a2a < 64; ++a2a)
			{
				auto v32 = local_player->GetAnimOverlay(a2a);
				//if (v32 && a== 979 && *(v32 + 44) != *(v32 + 28))
				//	auto v6 = entity->GetLowerBodyYaw();

				auto v20 = BYTE(animstate->speed_2d) * unk2;
				auto a1 = BYTE(animstate->speed_2d) * unk2;
				int v30 = 0.0;
				auto eye_angles_y = animstate->m_flEyeYaw;
				auto goal_feet_yaw = animstate->m_flGoalFeetYaw;
				auto v22 = eye_angles_y - goal_feet_yaw;
				if (v20 < v22)
				{
					auto v11 = v20;
					v30 = eye_angles_y - v11;
				}
				else if (a1 > v22)
				{
					auto v12 = a1;
					v30 = v12 + eye_angles_y;
				}
				auto v36 = std::fmodf(v30, 360.0);
				if (v36 > 180.0)
					v36 = v36 - 360.0;
				if (v36 < 180.0)
					v36 = v36 + 360.0;
				animstate->m_flGoalFeetYaw = v36;
				if (g::MissedShots[entity->EntIndex()] > 2)//(global::missedshots > 2)
				{
					switch (g::MissedShots[entity->EntIndex()])
					{
					case 3:
						animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw + 45.0;
						break;
					case 4:
						animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw - 45.0;
						break;
					case 5:
						animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw - 30.0;
						break;
					case 6:
						animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw + 30.0;
						break;
						//case 7:
							//animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw + animstate->m_flCurrentFeetYaw + 25.0;
						//case 8:
							//animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw - animstate->m_flCurrentFeetYaw - 25.0;
					default:
						return;
					}
				}
			}
		}
	}
}


/*void ResolverRecode::resolve(C_BaseEntity* ent)
{
	if (g_Menu.Config.ResolverType == 1)
		return;

	if (!g::pLocalEntity->IsAlive())
		return;
	auto animState = ent->AnimState();

	static auto GetSmoothedVelocity = [](float min_delta, Vector a, Vector b) {
		Vector delta = a - b;
		float delta_length = delta.Length();

		if (delta_length <= min_delta) {
			Vector result;
			if (-min_delta <= delta_length) {
				return a;
			}
			else {
				float iradius = 1.0f / (delta_length + FLT_EPSILON);
				return b - ((delta * iradius) * min_delta);
			}
		}
		else {
			float iradius = 1.0f / (delta_length + FLT_EPSILON);
			return b + ((delta * iradius) * min_delta);
		}
	};
	float v25;
	v25 = std::clamp(animState->m_fLandingDuckAdditiveSomething + ent->m_flDuckAmount(), 1.0f, 0.0f);
	float v26 = animState->m_fDuckAmount;
	float v27 = g_pClientState->m_nChokedCommands * 6.0f;
	float v28;

	// clamp 
	if ((v25 - v26) <= v27) {
		if (-v27 <= (v25 - v26))
			v28 = v25;
		else
			v28 = v26 - v27;
	}
	else {
		v28 = v26 + v27;
	}

	Vector velocity = ent->GetVelocity();
	float flDuckAmount = std::clamp(v28, 1.0f, 0.0f);

	Vector animationVelocity = GetSmoothedVelocity(g_pClientState->m_nChokedCommands * 2000.0f, velocity, animState->m_flVelocity());
	float speed = std::fminf(animationVelocity.Length(), 260.0f);

	auto weapon = (WeaponInfo_t*)g::pLocalEntity->GetActiveWeapon();

	float flMaxMovementSpeed = 260.0f;
	if (weapon) {
		flMaxMovementSpeed = std::fmaxf(weapon->max_speed, 0.001f);
	}

	float flRunningSpeed = speed / (flMaxMovementSpeed * 0.520f);
	float flDuckingSpeed = speed / (flMaxMovementSpeed * 0.340f);

	flRunningSpeed = std::clamp(flRunningSpeed, 0.0f, 1.0f);

	float flYawModifier = (((animState->m_bOnGround * -0.3f) - 0.2f) * flRunningSpeed) + 1.0f;
	if (flDuckAmount > 0.0f) {
		float flDuckingSpeed = std::clamp(flDuckingSpeed, 0.0f, 1.0f);
		flYawModifier += (flDuckAmount * flDuckingSpeed) * (0.5f - flYawModifier);
	}

	float m_flMaxBodyYaw = *(float*)(uintptr_t(animState) + 0x334) * flYawModifier;
	float m_flMinBodyYaw = *(float*)(uintptr_t(animState) + 0x330) * flYawModifier;

	float flEyeYaw = ent->GetEyeAngles().y;
	float flEyeDiff = std::remainderf(flEyeYaw - g_ResolverRecode.resolverinfoo.fakegoalfeetyaw, 360.f);

	if (flEyeDiff <= m_flMaxBodyYaw) {
		if (m_flMinBodyYaw > flEyeDiff)
			g_ResolverRecode.resolverinfoo.fakegoalfeetyaw = fabs(m_flMinBodyYaw) + flEyeYaw;
	}
	else {
		g_ResolverRecode.resolverinfoo.fakegoalfeetyaw = flEyeYaw - fabs(m_flMaxBodyYaw);
	}

	g_ResolverRecode.resolverinfoo.fakegoalfeetyaw = std::remainderf(g_ResolverRecode.resolverinfoo.fakegoalfeetyaw, 360.f);

	if (speed > 0.1f || fabs(velocity.z) > 100.0f) {
		g_ResolverRecode.resolverinfoo.fakegoalfeetyaw = g_ResolverRecode.approach_angle(
			flEyeYaw,
			g_ResolverRecode.resolverinfoo.fakegoalfeetyaw,
			((animState->m_bOnGround * 20.0f) + 30.0f)
			* g_pClientState->m_nChokedCommands);
	}
	else {
		g_ResolverRecode.resolverinfoo.fakegoalfeetyaw = g_ResolverRecode.approach_angle(
			ent->GetLowerBodyYaw(),
			g_ResolverRecode.resolverinfoo.fakegoalfeetyaw,
			g_pClientState->m_nChokedCommands * 100.0f);
	}

	float Left = flEyeYaw - m_flMinBodyYaw;
	float Right = flEyeYaw + m_flMaxBodyYaw;

	float resolveYaw;
	switch (g::MissedShots[ent->EntIndex()] % 3)
	{
	case 0: resolveYaw = Left; break;
	case 1: resolveYaw = g_ResolverRecode.resolverinfoo.fakegoalfeetyaw; break;
	case 2: resolveYaw = Right; break;
	}

	animState->m_flGoalFeetYaw = resolveYaw;
}*/

void ResolverRecode::FrameStage(ClientFrameStage_t stage)
{
	if (!g::pLocalEntity || !g_pEngine->IsInGame())
		return;

	static bool  wasDormant[65];

	for (int i = 1; i < g_pEngine->GetMaxClients(); ++i)
	{
		C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(i);

		if (!pPlayerEntity
			|| !pPlayerEntity->IsAlive())
			continue;
		if (pPlayerEntity->IsDormant())
		{
			wasDormant[i] = true;
			continue;
		}

		if (stage == FRAME_RENDER_START)
		{
			if (pPlayerEntity == g::pLocalEntity) {
				pPlayerEntity->ClientAnimations(true);
				fix_local_player_animations();
				pPlayerEntity->UpdateClientAnimation();
				pPlayerEntity->ClientAnimations(false);
			}
		}
		if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START)
			if (pPlayerEntity != g::pLocalEntity)
			{
				if (g_Menu.Config.Resolver) {
					if (g_Menu.Config.ResolverType == 3)
						SanaPidaras(pPlayerEntity);
				}
			}
		if (stage == FRAME_NET_UPDATE_END && pPlayerEntity != g::pLocalEntity)
		{
			auto VarMap = reinterpret_cast<uintptr_t>(pPlayerEntity) + 36;
			auto VarMapSize = *reinterpret_cast<int*>(VarMap + 20);

			for (auto index = 0; index < VarMapSize; index++)
				* reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(VarMap) + index * 12) = 0;
		}

		wasDormant[i] = false;
	}
}