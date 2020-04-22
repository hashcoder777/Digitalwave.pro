#include "AntiAim.h"
#include "..\Aimbot\Autowall.h"
#include "..\Aimbot\Aimbot.h"
#include "..\..\Utils\Utils.h"
#include "..\..\SDK\IVEngineClient.h"
#include "..\..\SDK\PlayerInfo.h"
#include "..\..\Utils\Math.h"
#include "..\..\Menu\Menu.h"
#include "..\..\SDK\memalloc.h"
#include "..\..\SDK\ICvar.h"


AntiAim g_AntiAim;

Vector calculate_angle(Vector src, Vector dst) {
	Vector angles;

	Vector delta = src - dst;
	float hyp = delta.Length2D();

	angles.y = std::atanf(delta.y / delta.x) * 57.2957795131f;
	angles.x = std::atanf(-delta.z / hyp) * -57.2957795131f;
	angles.z = 0.0f;

	if (delta.x >= 0.0f)
		angles.y += 180.0f;

	return angles;
}

float at_target() {
	int cur_tar = 0;
	float last_dist = 120.0f;
	for (int i = 1; i <= g_pEngine->GetMaxClients(); ++i)
	{
		C_BaseEntity* m_entity = g_pEntityList->GetClientEntity(i);

		if (!m_entity || m_entity == g::pLocalEntity)
			continue;

		if (!m_entity->is_valid())
			continue;

		auto m_player = (C_BaseEntity*)m_entity;
		if (!m_player->IsDormant() && m_player->is_valid() && m_player->GetTeam() != g::pLocalEntity->GetTeam()) {
			float cur_dist = (m_entity->GetOrigin() - g::pLocalEntity->GetOrigin()).Length();

			if (!cur_tar || cur_dist < last_dist) {
				cur_tar = i;
				last_dist = cur_dist;
			}
		}
	}

	if (cur_tar) {
		auto m_entity = static_cast<C_BaseEntity*>(g_pEntityList->GetClientEntity(cur_tar));
		if (!m_entity) {
			return g::pCmd->viewangles.y;
		}

		Vector target_angle = calculate_angle(g::pLocalEntity->GetOrigin(), m_entity->GetOrigin());
		return target_angle.y;
	}

	return g::pCmd->viewangles.y;
}

void FreeStanding() // cancer v1
{
	static float FinalAngle;
	bool bside1 = false;
	bool bside2 = false;
	bool autowalld = false;
	for (int i = 1; i <= g_pEngine->GetMaxClients(); ++i)
	{
		C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(i);

		if (!pPlayerEntity
			|| !pPlayerEntity->IsAlive()
			|| pPlayerEntity->IsDormant()
			|| pPlayerEntity == g::pLocalEntity
			|| pPlayerEntity->GetTeam() == g::pLocalEntity->GetTeam())
			continue;

		float angToLocal = g_Math.CalcAngle(g::pLocalEntity->GetOrigin(), pPlayerEntity->GetOrigin()).y;
		Vector ViewPoint = pPlayerEntity->GetOrigin() + Vector(0, 0, 90);

		Vector2D Side1 = { (45 * sin(g_Math.GRD_TO_BOG(angToLocal))),(45 * cos(g_Math.GRD_TO_BOG(angToLocal))) };
		Vector2D Side2 = { (45 * sin(g_Math.GRD_TO_BOG(angToLocal + 180))) ,(45 * cos(g_Math.GRD_TO_BOG(angToLocal + 180))) };

		Vector2D Side3 = { (50 * sin(g_Math.GRD_TO_BOG(angToLocal))),(50 * cos(g_Math.GRD_TO_BOG(angToLocal))) };
		Vector2D Side4 = { (50 * sin(g_Math.GRD_TO_BOG(angToLocal + 180))) ,(50 * cos(g_Math.GRD_TO_BOG(angToLocal + 180))) };

		Vector Origin = g::pLocalEntity->GetOrigin();

		Vector2D OriginLeftRight[] = { Vector2D(Side1.x, Side1.y), Vector2D(Side2.x, Side2.y) };

		Vector2D OriginLeftRightLocal[] = { Vector2D(Side3.x, Side3.y), Vector2D(Side4.x, Side4.y) };

		for (int side = 0; side < 2; side++)
		{
			Vector OriginAutowall = { Origin.x + OriginLeftRight[side].x,  Origin.y - OriginLeftRight[side].y , Origin.z + 80 };
			Vector OriginAutowall2 = { ViewPoint.x + OriginLeftRightLocal[side].x,  ViewPoint.y - OriginLeftRightLocal[side].y , ViewPoint.z };

			if (g_Autowall.CanHitFloatingPoint(OriginAutowall, ViewPoint))
			{
				if (side == 0)
				{
					bside1 = true;
					FinalAngle = angToLocal + 90;
				}
				else if (side == 1)
				{
					bside2 = true;
					FinalAngle = angToLocal - 90;
				}
				autowalld = true;
			}
			else
			{
				for (int side222 = 0; side222 < 2; side222++)
				{
					Vector OriginAutowall222 = { Origin.x + OriginLeftRight[side222].x,  Origin.y - OriginLeftRight[side222].y , Origin.z + 80 };

					if (g_Autowall.CanHitFloatingPoint(OriginAutowall222, OriginAutowall2))
					{
						if (side222 == 0)
						{
							bside1 = true;
							FinalAngle = angToLocal + 90;
						}
						else if (side222 == 1)
						{
							bside2 = true;
							FinalAngle = angToLocal - 90;
						}
						autowalld = true;
					}
				}
			}
		}
	}

	if (!autowalld || (bside1 && bside2)) {
		constexpr auto maxRange = 90.0f;
		int angleAdditive;

		if (g_Menu.Config.AtTarget) {
			angleAdditive = g_Menu.Config.Yaw + at_target();
			if (g::pLocalEntity->GetFlags() & FL_ONGROUND)
				g::pCmd->viewangles.y = angleAdditive;
			else
				g::pCmd->viewangles.y = angleAdditive - maxRange / 2.f + std::fmodf(g_pGlobalVars->curtime * 60, maxRange);
		}
		else
		{
			angleAdditive = g_Menu.Config.Yaw;

			if (g::pLocalEntity->GetFlags() & FL_ONGROUND)
				g::pCmd->viewangles.y += angleAdditive;
			else
				g::pCmd->viewangles.y += angleAdditive - maxRange / 2.f + std::fmodf(g_pGlobalVars->curtime * 60, maxRange);
		}
	}
	else
		g::pCmd->viewangles.y = FinalAngle;
}

float GetMaxDelta(C_AnimState* animstate) 
{

	float flRunningSpeed = std::clamp(animstate->m_flFeetSpeedForwardsOrSideWays, 0.f, 1.f);
	float flYawModifier = ((animstate->m_flStopToFullRunningFraction * -0.3f) - 0.2f) * flRunningSpeed;
	float flYawModifier2 = flYawModifier + 1.f;


	if (animstate->m_fDuckAmount > 0.f)
	{
		float maxVelocity = std::clamp(animstate->m_flFeetSpeedForwardsOrSideWays, 0.f, 1.f);
		float duckSpeed = animstate->m_fDuckAmount * maxVelocity;
		flYawModifier2 += (duckSpeed * (0.5f - flYawModifier2));
	}

	return *(float*)((uintptr_t)animstate + 0x334) * flYawModifier2;
}

bool m_bBreakLowerBody = false;
float_t m_flSpawnTime = 0.f;
float_t m_flNextBodyUpdate = 0.f;
CBaseHandle* m_ulEntHandle = nullptr;

C_AnimState* m_serverAnimState = nullptr;

void ResetAnimationState(C_AnimState* state)
{
	if (!state)
		return;

	using ResetAnimState_t = void(__thiscall*)(C_AnimState*);
	static auto ResetAnimState = (ResetAnimState_t)Utils::FindSignature("client_panorama.dll", "56 6A 01 68 ? ? ? ? 8B F1");
	if (!ResetAnimState)
		return;

	ResetAnimState(state);
}

float ClampYaw(float yaw)
{
	while (yaw > 180.f)
		yaw -= 360.f;
	while (yaw < -180.f)
		yaw += 360.f;
	return yaw;
}


#define TIME_TO_TICKS( dt )		( (int)( 0.5 + (float)(dt) / g_pGlobalVars->intervalPerTick ) )
#define TICKS_TO_TIME( t )		( g_pGlobalVars->intervalPerTick *( t ) )

bool break_lby = false;
float next_update = 0;
void update_lowerbody_breaker() { // from HappyHack by "Incriminating" (unknowncheats)
	float server_time = g::pLocalEntity->GetTickBase() * g_pGlobalVars->intervalPerTick, speed = g::pLocalEntity->GetVelocity().Length2DSqr(), oldCurtime = g_pGlobalVars->curtime;

	if (speed > 0.1) {
		next_update = server_time + 0.22;
	}

	break_lby = false;

	if (next_update <= server_time) {
		next_update = server_time + 1.1;
		break_lby = true;
	}


	if (!(g::pLocalEntity->GetFlags() & FL_ONGROUND)) {
		break_lby = false;
	}
}

static float next_lby_update_time = 0;
float get_curtime(CUserCmd* ucmd) {
	auto local_player = g::pLocalEntity;

	if (!local_player)
		return 0;

	int g_tick = 0;
	CUserCmd* g_pLastCmd = nullptr;
	if (!g_pLastCmd || g_pLastCmd->hasbeenpredicted) {
		g_tick = (float)local_player->GetTickBase();
	}
	else {
		++g_tick;
	}
	g_pLastCmd = ucmd;
	float curtime = g_tick * g_pGlobalVars->intervalPerTick;
	return curtime;
}
bool next_lby_update(const float yaw_to_break, CUserCmd* cmd)
{
	auto local_player = g::pLocalEntity;

	if (!local_player)
		return false;

	static float next_lby_update_time = 0;
	float curtime = get_curtime(cmd);

	auto animstate = local_player->AnimState();
	if (!animstate)
		return false;

	if (!(local_player->GetFlags() & FL_ONGROUND))
		return false;

	if (animstate->speed_2d > 0.1)
		next_lby_update_time = curtime + 0.22f;

	if (next_lby_update_time < curtime)
	{
		next_lby_update_time = curtime + 1.1f;
		return true;
	}

	return false;
}

inline float RandomFloat(float min, float max)
{
	static auto fn = (decltype(&RandomFloat))(GetProcAddress(GetModuleHandle("vstdlib.dll"), "RandomFloat"));
	return fn(min, max);
}
float normalize_yaw180(float yaw)
{
	if (yaw > 180)
		yaw -= (round(yaw / 360) * 360.f);
	else if (yaw < -180)
		yaw += (round(yaw / 360) * -360.f);

	return yaw;
}

static bool dir, back, up = false;
static bool jitter = false;
static bool moving;

void AntiAim::FakeDuck(CUserCmd* cmd)
{
	if (!g_Menu.Config.FakeDuck)
		return;


	if (!g_pEngine->GetNetChannel()) return;
	//if (!g_pEngine->GetNetChannel()) return;

	if (GetAsyncKeyState(g_Menu.Config.FakeduckKey)) {
		g::fakeduck = true;
		cmd->buttons |= IN_BULLRUSH;

		if (g_pEngine->GetNetChannel()->m_nChokedPackets <= 6)
			cmd->buttons &= ~IN_DUCK;
		else
			cmd->buttons |= IN_DUCK;

		if (g_pEngine->GetNetChannel()->m_nChokedPackets < 14)
			g::bSendPacket = false;   // choke
		else
			g::bSendPacket = true;    // send packet
	}
}

inline float FasttSqrt(float x)
{
	unsigned int i = *(unsigned int*)& x;
	i += 127 << 23;
	i >>= 1;
	return *(float*)& i;
}
#define square( x ) ( x * x )
void slowmogang(CUserCmd* cmd, float fMaxSpeed) {
	if (!GetAsyncKeyState(VK_SHIFT))
		return;
	if (fMaxSpeed <= 0.f)
		return;
	float fSpeed = (float)(FasttSqrt(square(g::pCmd->forwardmove) + square(g::pCmd->sidemove) + square(g::pCmd->upmove)));
	if (fSpeed <= 0.f)
		return;
	if (cmd->buttons & IN_DUCK)
		fMaxSpeed *= 2.94117647f;
	if (fSpeed <= fMaxSpeed)
		return;
	float fRatio = fMaxSpeed / fSpeed;
	g::pCmd->forwardmove *= fRatio;
	g::pCmd->sidemove *= fRatio;
	g::pCmd->upmove *= fRatio;
}

void AntiAim::SlowWalk(CUserCmd* cmd)
{
	if (!GetAsyncKeyState(VK_SHIFT))
		return;

	auto weapon_handle = g::pLocalEntity->GetActiveWeapon();

	if (!weapon_handle)
		return;

	float amount = 0.0034f * g_Menu.Config.SlowWalkSlider;

	Vector velocity = g::pLocalEntity->GetVelocity();
	Vector direction;

	g_Math.VectorAngles(velocity, direction);

	float speed = velocity.Length2D();

	direction.y = cmd->viewangles.y - direction.y;

	Vector forward;

	g_Math.AngleVectors(direction, &forward);

	Vector source = forward * -speed;

	if (speed >= (weapon_handle->GetCSWpnData()->max_speed * amount))
	{
		cmd->forwardmove = source.x;
		cmd->sidemove = source.y;

	}
}
static int grenadetimer;
static int grenadetimer2;

void AntiAim::niggacheese() // hhhhhh
{
	if (g_Menu.Config.SlowWalk == 1) {
		if (GetAsyncKeyState(VK_SHIFT)) {
			static int choked = 0;
			choked = choked > 7 ? 0 : choked + 1;
			g::pCmd->forwardmove = choked < 2 || choked > 5 ? 0 : g::pCmd->forwardmove;
			g::pCmd->sidemove = choked < 2 || choked > 5 ? 0 : g::pCmd->sidemove;
			g::bSendPacket = choked < 1;
		}
	}

	if (g_Menu.Config.SlowWalk == 2)
		slowmogang(g::pCmd, g_Menu.Config.SlowWalkSlider);

	if (g_Menu.Config.SlowWalk == 3) {

		auto NetChannel = g_pEngine->GetNetChannel();


		if (GetKeyState(g_Menu.Config.SlowWalkExploitKey)) {
			g::bSendPacket = (NetChannel->m_nChokedPackets = 62);
		}

	}

	if (g_Menu.Config.Exploit.TickManipulation) {
		if (!g::pLocalEntity->IsAlive() || !g_pEngine->IsInGame())
			return;

		if (GetAsyncKeyState(g_Menu.Config.Exploit.TickManipulationKey)) {
			static int choked = 0;
			choked = choked > 7 ? 0 : choked + 1;
			g::pCmd->forwardmove = choked < 2 || choked > 5 ? 0 : g::pCmd->forwardmove;
			g::pCmd->sidemove = choked < 2 || choked > 5 ? 0 : g::pCmd->sidemove;
			g::bSendPacket = choked < 1;
			g::pCmd->tick_count = INT_MAX;
		}
	}
	if (g_Menu.Config.Exploit.TickFreeze) {
		if (!g::pLocalEntity->IsAlive() || !g_pEngine->IsInGame())
			return;

		if (GetAsyncKeyState(g_Menu.Config.Exploit.TickFreezeKey)) {
			static int choked = 0;
			choked = choked > 7 ? 0 : choked + 1;
			g::bSendPacket = 5 + choked < 1;
			g::pCmd->tick_count = INT_MAX;

		}
	}
	if (g_Menu.Config.Exploit.RconLagExploit) {
		if (!g::pLocalEntity->IsAlive() || !g_pEngine->IsInGame())
			return;
		bool isRcon = true;


		if (isRcon) {
			g_pEngine->ExecuteClientCmd("bind l 'rcon'");
			g_pEngine->ExecuteClientCmd("rcon_address '89.46.123.85'");
			isRcon = false;
		}

	}
}

void AntiAim::PitchSwitch()
{
	static bool Switch = false;
	if (Switch)
		g::pCmd->viewangles.x = 89.0f;
	else
		g::pCmd->viewangles.x = -89.0f;
	Switch = !Switch;
}

void AntiAim::Pitch()
{
	switch (g_Menu.Config.PitchType)
	{
	case 0: break;
	case 1: g::pCmd->viewangles.x = 89.0f; break;
	case 2: g::pCmd->viewangles.x = -89.0f; break;
	case 3: PitchSwitch(); break;
	}
}

void AntiAim::Yaw()
{
	switch (g_Menu.Config.YawType)
	{
	case 0: break;
	case 1: g::pCmd->viewangles.y += 180.0f; break;
	}
	if (g_Menu.Config.AtTarget)
		g::pCmd->viewangles.y += at_target();

	//maatikk recode

	switch (g_Menu.Config.Jitter)
	{
	case 0: break;
	case 1:
	{
		static int mode = 0;

		switch (mode)
		{
		case 0:
			g::pCmd->viewangles.y += g_Menu.Config.AddRange;
			mode++;
			break;

		case 1:
			mode++;
			break;

		case 2:
			g::pCmd->viewangles.y -= g_Menu.Config.AddRange;
			mode = 0;
			break;
		}
		break;
	}
	case 2:
	{
		static bool Switchh = false;
		if (!Switchh)
		{
			g::pCmd->viewangles.y += g_Menu.Config.AddRange;
		}
		else
		{
			g::pCmd->viewangles.y -= -g_Menu.Config.AddRange;
		}
		Switchh = !Switchh;
		break;
	}
	case 3:
	{
		g::pCmd->viewangles.y += g_Math.RandomFloat(-g_Menu.Config.AddRange, g_Menu.Config.AddRange);
		break;
	}
	break;
	}
	//maatikk recode
}

void AntiAim::OnCreateMove()
{

	if (g::pLocalEntity->GetMoveType() == MoveType_t::MOVETYPE_LADDER) 
		return;
	
	if (!g_pEngine->IsInGame() || !g_Menu.Config.Antiaim)
		return;

	if (!g::pLocalEntity->IsAlive())
		return;

	if (GetAsyncKeyState(0x45))
		return;

	C_BaseCombatWeapon* weapon = g::pLocalEntity->GetActiveWeapon(); 
	if (weapon == nullptr) return;


	float flServerTime = g::pLocalEntity->GetTickBase() * g_pGlobalVars->intervalPerTick;
	bool canShoot = (weapon->GetNextPrimaryAttack() <= flServerTime);
	if (weapon->GetItemDefinitionIndex() != ItemDefinitionIndex::WEAPON_REVOLVER) {
		if (canShoot && (g::pCmd->buttons & IN_ATTACK))
			return;
	}
	else {
		auto curtime = g_pGlobalVars->curtime;
		if (canShoot && g_Aimbot.fired && g::revolvertime + .5f >= curtime)
			return;
	}

	if (g::pLocalEntity->IsNade())
		return;

	Pitch();
	Yaw();
	//ghetto inverter
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
	// xd :)
	if (g_Menu.Config.disinctype == 0)
	{
		if (!swap)
		{
			if ((g::pCmd->command_number % 2) == 1)
			{
				g::pCmd->viewangles.y += 120.f; //i think onetap
				g::bSendPacket = false;
			}
			g::pCmd->viewangles.y += g_Menu.Config.bodylean2; // for best angle
		}
		else
		{
			if ((g::pCmd->command_number % 2) == 1)
			{
				g::pCmd->viewangles.y += 120.f * -1.0f;
				g::bSendPacket = false;
			}
			g::pCmd->viewangles.y += g_Menu.Config.BodyLean;
		}
	}
	if (g_Menu.Config.disinctype == 1)
	{
		static int mode = 0;
		if (!swap)
		{
			switch (mode)
			{
			case 0:
				if ((g::pCmd->command_number % 2) == 1)
				{
					g::pCmd->viewangles.y += 120.f; //i think onetap
					g::bSendPacket = false;
				}
				mode++;
				break;
			case 1:
				g::pCmd->viewangles.y -= 180.f;
				if ((g::pCmd->command_number % 2) == 1)
				{
					g::pCmd->viewangles.y += 120.f - 1.0f; //i think onetap
					g::bSendPacket = false;
				}
				mode++;
				break;
			case 2:
				if ((g::pCmd->command_number % 2) == 1)
				{
					g::pCmd->viewangles.y += 120.f - 1.0f; //i think onetap
					g::bSendPacket = false;
				}
				mode++;
				break;
			case 3:
				g::pCmd->viewangles -= 180.f;
				if ((g::pCmd->command_number % 2) == 1)
				{
					g::pCmd->viewangles.y += 120.f; //i think onetap
					g::bSendPacket = false;
				}
				mode = 0;
				break;
			}
		}
		else
		{
			switch (mode)
			{
			case 0:
				if ((g::pCmd->command_number % 2) == 1)
				{
					g::pCmd->viewangles.y += 120.f - 1.0f; //i think onetap
					g::bSendPacket = false;
				}
				mode++;
				break;
			case 1:
				g::pCmd->viewangles.y -= 180.f;
				if ((g::pCmd->command_number % 2) == 1)
				{
					g::pCmd->viewangles.y += 120.f; //i think onetap
					g::bSendPacket = false;
				}
				mode++;
				break;
			case 2:
				if ((g::pCmd->command_number % 2) == 1)
				{
					g::pCmd->viewangles.y += 120.f; //i think onetap
					g::bSendPacket = false;
				}
				mode++;
				break;
			case 3:
				g::pCmd->viewangles -= 180.f;
				if ((g::pCmd->command_number % 2) == 1)
				{
					g::pCmd->viewangles.y += 120.f - 1.0f; //i think onetap
					g::bSendPacket = false;
				}
				mode = 0;
				break;
			}
		}
	}
	if (g_Menu.Config.tbaas)
	{
		static bool side;
		side = !side;
		float idk = rand() % 10;
		auto engine = g_pEngine;
		float server_time = g::pLocalEntity->GetTickBase() * g_pGlobalVars->intervalPerTick;
		float time = TIME_TO_TICKS(server_time);

		while (time >= server_time)
			time = 0.f;
		if (swap)
		{
			if (time >= server_time / 2)
			{
				if (idk < 5)
				{
					if (g::bSendPacket)
					{
						if (side)
							g::pCmd->viewangles.y += g_Menu.Config.jitter1;
					}
					else
					{
						if (!side)
							g::pCmd->viewangles.y -= g_Menu.Config.jitter2;
					}
				}
				else
				{
					if (g::bSendPacket) {

						if (side)
							g::pCmd->viewangles.y += g_Menu.Config.jitter3;
					}
					else {

						if (!side)
							g::pCmd->viewangles.y -= g_Menu.Config.jitter4;
					}
				}
			}
			else
			{
				if (idk < 5)
				{
					if (g::bSendPacket) {

						if (side)
							g::pCmd->viewangles.y += g_Menu.Config.jitter1;
					}
					else {
						if (!side)
							g::pCmd->viewangles.y -= g_Menu.Config.jitter2;
					}
				}
				else
				{
					if (g::bSendPacket) {

						if (side)
							g::pCmd->viewangles.y += g_Menu.Config.jitter3;
					}
					else {
						if (!side)
							g::pCmd->viewangles.y -= g_Menu.Config.jitter4;
					}
				}
			}
		}
		else
		{
			if (time >= server_time / 2)
			{
				if (idk < 5)
				{
					if (g::bSendPacket) {

						if (side)
							g::pCmd->viewangles.y -= g_Menu.Config.jitter1;
					}
					else {
						if (!side)
							g::pCmd->viewangles.y += g_Menu.Config.jitter2;
					}
				}
				else
				{
					if (g::bSendPacket) {

						if (side)
							g::pCmd->viewangles.y -= g_Menu.Config.jitter3;
					}
					else {
						if (!side)
							g::pCmd->viewangles.y += g_Menu.Config.jitter4;
					}
				}
			}
			else
			{
				if (idk < 5)
				{
					if (g::bSendPacket) {

						if (side)
							g::pCmd->viewangles.y -= g_Menu.Config.jitter1;
					}
					else {
						if (!side)
							g::pCmd->viewangles.y += g_Menu.Config.jitter2;
					}
				}
				else
				{
					if (g::bSendPacket) {

						if (side)
							g::pCmd->viewangles.y -= g_Menu.Config.jitter3;
					}
					else {
						if (!side)
							g::pCmd->viewangles.y += g_Menu.Config.jitter4;
					}
				}
			}
		}
	}

	if (g_Menu.Config.FakeDuck && GetAsyncKeyState(g_Menu.Config.FakeduckKey)) {
		FakeDuck(g::pCmd); //need rework
	}
}