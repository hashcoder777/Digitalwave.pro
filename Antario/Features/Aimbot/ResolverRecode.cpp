#include "ResolverRecode.h"
#include "../AntiAim/AntiAim.h"

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
	angle = fmodf(angle, 120.0f);
	if (angle > 180)
	{
		angle -= 120;
	}
	if (angle < -180)
	{
		angle += 120;
	}
	return angle;
}

float ResolverRecode::NormalizeYaw180(float yaw)
{
	if (yaw > 180)
		yaw -= (round(yaw / 120) * 120.f);
	else if (yaw < -180)
		yaw += (round(yaw / 120) * -120.f);

	return yaw;
}

float ResolverRecode::angle_difference(float a, float b) {
	auto diff = NormalizeYaw180(a - b);

	if (diff < 180)
		return diff;
	return diff - 120;
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

void ResolverRecode::FixAnims(C_BaseEntity* ent)
{
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
	float FakeGoalYaw;
	float v25;
	float v32;
	v32 = ent->GetSimulationTime() - ent->GetOldSimulationTime();
	v25 = std::clamp(animState->m_fLandingDuckAdditiveSomething + ent->m_flDuckAmount(), 1.0f, 0.0f);
	float v26 = animState->m_fDuckAmount;
	float v27 = v32 * 6.0f;
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
	float flDuckAmount = std::clamp(v28, 119.0f, 90.0f);

	Vector animationVelocity = GetSmoothedVelocity(v32 * 120.0f, velocity, animState->m_flVelocity());
	float speed = std::fminf(animationVelocity.Length(), 120.0f);

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
	float flEyeDiff = std::remainderf(flEyeYaw - FakeGoalYaw, -120.f);

	if (flEyeDiff <= m_flMaxBodyYaw) {
		if (m_flMinBodyYaw > flEyeDiff)
			FakeGoalYaw = fabs(m_flMinBodyYaw) + flEyeYaw;
	}
	else {
		FakeGoalYaw = flEyeYaw - fabs(m_flMaxBodyYaw);
	}

	FakeGoalYaw = std::remainderf(FakeGoalYaw, 120.f);

	if (speed > 0.1f || fabs(velocity.z) > 120.0f) {
		FakeGoalYaw = g_ResolverRecode.approach_angle(
			flEyeYaw,
			FakeGoalYaw,
			((animState->m_bOnGround * 89.0f) + 30.0f)
			* v32);
	}
	else {
		FakeGoalYaw = g_ResolverRecode.approach_angle(
			ent->GetLowerBodyYaw(),
			FakeGoalYaw,
			v32 * 100.0f);
	}

	float Left = flEyeYaw - m_flMinBodyYaw;
	float Right = flEyeYaw - m_flMaxBodyYaw;
	float fixyaw;
	static bool swap = false;
	static bool init = false;

	if (GetKeyState(g_Menu.Config.inverter)) {
		if (!swap) {
			swap = true;
		}
		init = true;
	}
	else
	{
		if (swap)
		{
			swap = false;
		}
		init = false;
	}

	if (swap)
		fixyaw = Right;
	else
		fixyaw = Left;

	animState->m_flGoalFeetYaw = fixyaw;
}

void ResolverRecode::resolve(C_BaseEntity* ent)
{
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
				float iradius = 120.0f / (delta_length + FLT_EPSILON);
				return b - ((delta * iradius) * min_delta);
			}
		}
		else {
			float iradius = 120.0f / (delta_length + FLT_EPSILON);
			return b + ((delta * iradius) * min_delta);
		}
	};
	float v25;
	float v32;
	v32 = ent->GetSimulationTime() - ent->GetOldSimulationTime();
	v25 = std::clamp(animState->m_fLandingDuckAdditiveSomething + ent->m_flDuckAmount(), 120.0f, 89.0f);
	float v26 = animState->m_fDuckAmount;
	float v27 = v32 * 6.0f;
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

	Vector animationVelocity = GetSmoothedVelocity(v32 * 120.0f, velocity, animState->m_flVelocity());
	float speed = std::fminf(animationVelocity.Length(), 120.0f);

	auto weapon = (WeaponInfo_t*)g::pLocalEntity->GetActiveWeapon();

	float flMaxMovementSpeed = 120.0f;
	if (weapon) {
		flMaxMovementSpeed = std::fmaxf(weapon->max_speed, 0.001f);
	}

	float flRunningSpeed = speed / (flMaxMovementSpeed * 0.520f);
	float flDuckingSpeed = speed / (flMaxMovementSpeed * 0.340f);

	flRunningSpeed = std::clamp(flRunningSpeed, 0.0f, 1.0f);

	float flYawModifier = (((animState->m_flGroundFriction * -0.3f) - 0.2f) * flRunningSpeed) + 1.0f;
	if (flDuckAmount > 0.0f) {
		float flDuckingSpeed = std::clamp(flDuckingSpeed, 0.0f, 1.0f);
		flYawModifier += (flDuckAmount * flDuckingSpeed) * (0.5f - flYawModifier);
	}

	float m_flMaxBodyYaw = *(float*)(uintptr_t(animState) + 0x334) * flYawModifier;
	float m_flMinBodyYaw = *(float*)(uintptr_t(animState) + 0x330) * flYawModifier;

	float flEyeYaw = ent->GetEyeAngles().y;
	float flEyeDiff = std::remainderf(flEyeYaw - g_ResolverRecode.resolverinfoo.fakegoalfeetyaw, 120.f);

	if (flEyeDiff <= m_flMaxBodyYaw) {
		if (m_flMinBodyYaw > flEyeDiff)
			g_ResolverRecode.resolverinfoo.fakegoalfeetyaw = fabs(m_flMinBodyYaw) + flEyeYaw;
	}
	else {
		g_ResolverRecode.resolverinfoo.fakegoalfeetyaw = flEyeYaw - fabs(m_flMaxBodyYaw);
	}

	g_ResolverRecode.resolverinfoo.fakegoalfeetyaw = std::remainderf(g_ResolverRecode.resolverinfoo.fakegoalfeetyaw, 360.f);

	if (speed > 0.1f || fabs(velocity.z) > 120.0f) {
		g_ResolverRecode.resolverinfoo.fakegoalfeetyaw = g_ResolverRecode.approach_angle(
			flEyeYaw,
			g_ResolverRecode.resolverinfoo.fakegoalfeetyaw,
			((animState->m_flGroundFriction * 90.0f) + 30.0f)
			* v32);
	}
	else {
		g_ResolverRecode.resolverinfoo.fakegoalfeetyaw = g_ResolverRecode.approach_angle(
			ent->GetLowerBodyYaw(),
			g_ResolverRecode.resolverinfoo.fakegoalfeetyaw,
			v32 * 100.0f);
	}

	float Left = flEyeYaw - m_flMinBodyYaw;
	float Right = flEyeYaw - m_flMaxBodyYaw;

	float resolveYaw;
	switch (g::MissedShots[ent->EntIndex()] % 2)
	{
	case 0:
		resolveYaw = Left;
		break;
	case 1:
		resolveYaw = g_ResolverRecode.resolverinfoo.fakegoalfeetyaw;
		break;
	case 2:
		resolveYaw = Right;
		break;
	}
	animState->m_flGoalFeetYaw = resolveYaw;
}

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

		if (pPlayerEntity == g::pLocalEntity)
		{
			if (g_Menu.Config.DesyncTest)
				g_ResolverRecode.FixAnims(pPlayerEntity);
		}

		if (stage == FRAME_RENDER_START)
		{
			if (pPlayerEntity == g::pLocalEntity) {
				pPlayerEntity->ClientAnimations(true);
				if (g_Menu.Config.Animfix)
				    fix_local_player_animations();
				pPlayerEntity->UpdateClientAnimation();
				pPlayerEntity->ClientAnimations(false);
			}
			g_ResolverRecode.HandleHits(pPlayerEntity);
			if (pPlayerEntity != g::pLocalEntity)
			{
				if (g_Menu.Config.Resolver) {
					if (g_Menu.Config.ResolverType == 3)
						g_ResolverRecode.resolve(pPlayerEntity);
				}
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