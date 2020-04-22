#pragma once

#include "..\Aimbot\Autowall.h"
#include "..\Aimbot\Aimbot.h"
#include "..\Aimbot\LagComp.h"
#include "..\..\Utils\GlobalVars.h"
#include "..\..\Utils\Math.h"
#include "..\..\SDK\ICvar.h"
#include "..\..\SDK\CPrediction.h"
#include "..\..\Menu\Menu.h"
#include <iostream>
#include <algorithm>
#include "../../SDK/texture_group_names.h"
#include "../../SDK/IClientMode.h"
#include "../../SDK/IMaterial.h"
#include "../../SDK/IBaseClientDll.h"
#include <unordered_map>

// kinda just throw shit in here

#define _SOLVEY(a, b, c, d, e, f) ((c * b - d * a) / (c * f - d * e))
#define SOLVEY(...) _SOLVEY(?, ?, ?, ?, ?, ?)
#define SOLVEX(y, world, forward, right) ((world.x - right.x * y) / forward.x)


class Misc
{
public:
	void OnCreateMove()
	{
		this->pCmd = g::pCmd;
		this->pLocal = g::pLocalEntity;

		this->DoAutostrafe();
		this->DoBhop();
		if (!g_pEngine->IsVoiceRecording()) {
			this->DoFakeLag();
		}
		this->AutoRevolver();
	};

	void AutoRevolver() // xy0
	{
		if (!g_Menu.Config.autorevolver)
			return;
		auto me = g::pLocalEntity;
		auto cmd = g::pCmd;
		auto weapon = me->GetActiveWeapon();

		if (!g_Menu.Config.Aimbot)
			return;

		if (!me || !me->IsAlive() || !weapon)
			return;

		if (weapon->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_REVOLVER)
		{
			static int delay = 0; 
			delay++;

			if (delay <= 15)
				g::pCmd->buttons |= IN_ATTACK;
			else {
				delay = 0;
				g::revolvertime = g_pGlobalVars->curtime;

			}
		}
	}

	void MovementFix(Vector& oldang)
	{
		Vector vMovements(g::pCmd->forwardmove, g::pCmd->sidemove, 0.f);

		if (vMovements.Length2D() == 0)
			return;

		Vector vRealF, vRealR;
		Vector aRealDir = g::pCmd->viewangles;
		aRealDir.Clamp();

		g_Math.AngleVectors(aRealDir, &vRealF, &vRealR, nullptr);
		vRealF[2] = 0;
		vRealR[2] = 0;

		VectorNormalize(vRealF);
		VectorNormalize(vRealR);

		Vector aWishDir = oldang;
		aWishDir.Clamp();

		Vector vWishF, vWishR;
		g_Math.AngleVectors(aWishDir, &vWishF, &vWishR, nullptr);

		vWishF[2] = 0;
		vWishR[2] = 0;

		VectorNormalize(vWishF);
		VectorNormalize(vWishR);

		Vector vWishVel;
		vWishVel[0] = vWishF[0] * g::pCmd->forwardmove + vWishR[0] * g::pCmd->sidemove;
		vWishVel[1] = vWishF[1] * g::pCmd->forwardmove + vWishR[1] * g::pCmd->sidemove;
		vWishVel[2] = 0;

		float a = vRealF[0], b = vRealR[0], c = vRealF[1], d = vRealR[1];
		float v = vWishVel[0], w = vWishVel[1];

		float flDivide = (a * d - b * c);
		float x = (d * v - b * w) / flDivide;
		float y = (a * w - c * v) / flDivide;

		g::pCmd->forwardmove = x;
		g::pCmd->sidemove = y;
	}

	void NoRecoil(CUserCmd* cmd)
	{
		if (!g_Menu.Config.aimbot_norecoil)
			return;

		auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
		if (!local_player)
			return;

		auto weapon = local_player->GetActiveWeapon();
		if (weapon)
			weapon->GetAccuracyPenalty();

		cmd->viewangles -= local_player->GetAimPunchAngle() * 2;
	}

	void BootlegNightMode()
	{
		if (g_Menu.Config.PostProcess)
		{
			ConVar* mat_postprocess_enable = g_pCvar->FindVar("mat_postprocess_enable");
			*(int*)((DWORD)& mat_postprocess_enable->fnChangeCallback + 0xC) = 0; // ew
			mat_postprocess_enable->SetValue(0);
		}
		else
		{
			ConVar* mat_postprocess_enable = g_pCvar->FindVar("mat_postprocess_enable");
			*(int*)((DWORD)& mat_postprocess_enable->fnChangeCallback + 0xC) = 0; // ew
			mat_postprocess_enable->SetValue(1);

		}
		float ratio = (g_Menu.Config.AspectRatioSlider * 0.1) / 2;
		static auto bruh = g_pCvar->FindVar("r_aspectratio");
		if (g_Menu.Config.AspectRatio) {
			if (ratio > 0.001) {
				g_pCvar->FindVar("r_aspectratio")->SetValue(ratio);
			}
			else {
				g_pCvar->FindVar("r_aspectratio")->SetValue((35 * 0.1f) / 2);
			}
		}
		else {
			g_pCvar->FindVar("r_aspectratio")->SetValue((35 * 0.1f) / 2);
		}
	}

	void ShowImpacts()
	{
		if (g_Menu.Config.Impacts)
		{
			ConVar* sv_showimpacts = g_pCvar->FindVar("sv_showimpacts");
			*(int*)((DWORD)& sv_showimpacts->fnChangeCallback + 0xC) = 0; // ew
			sv_showimpacts->SetValue(1);

		}
		else {
			ConVar* sv_showimpacts = g_pCvar->FindVar("sv_showimpacts");
			*(int*)((DWORD)& sv_showimpacts->fnChangeCallback + 0xC) = 0; // ew
			sv_showimpacts->SetValue(0);
		}

	}


	IMaterial* sMaterial = nullptr;

	std::vector<const char*> SmokeMaterials =
	{
			"particle/vistasmokev1/vistasmokev1_fire",
			"particle/vistasmokev1/vistasmokev1_smokegrenade",
			"particle/vistasmokev1/vistasmokev1_emods",
			"particle/vistasmokev1/vistasmokev1_emods_impactdust",
	};

	void DoNoSmoke(ClientFrameStage_t curStage)
	{
		static auto linegoesthrusmoke = Utils::FindPattern("client_panorama.dll", (BYTE*)"\x55\x8B\xEC\x83\xEC\x08\x8B\x15\x00\x00\x00\x00\x0F\x57\xC0", "xxxxxxxx????xxx");

		for (auto mat : SmokeMaterials)
		{
			sMaterial = g_pMaterialSys->FindMaterial(mat, TEXTURE_GROUP_OTHER);



			if (g_Menu.Config.NoSmoke)
			{
				sMaterial->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, true);
				static auto smokecout = *(DWORD*)(linegoesthrusmoke + 0x8);
				*(int*)(smokecout) = 0;
			}
			else
			{
				sMaterial->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, false);


			}
		}
	}


	void clan_tag() { // pasted from subliminal br0 ez
		auto apply = [](const char* name) -> void {
			using Fn = int(__fastcall*)(const char*, const char*);
			static auto fn = reinterpret_cast<Fn>(Utils::FindSignature("engine.dll", "53 56 57 8B DA 8B F9 FF 15"));
			fn(name, name);
		};

		static std::string tag = "digitalwave            ";

		static float last_time = 0;

		if (g_pGlobalVars->curtime > last_time) {
			tag += tag.at(0);
			tag.erase(0, 1);
			std::string dollarsign = u8"💀   ";
			dollarsign += tag;
			apply(dollarsign.c_str());

			last_time = g_pGlobalVars->curtime + 0.5f;
		}

		if (fabs(last_time - g_pGlobalVars->curtime) > 1.f)
			last_time = g_pGlobalVars->curtime;

	}

	bool isthirdperson;

	void ThirdPerson(ClientFrameStage_t curStage)
	{
		if (!g_pEngine->IsInGame() || !g_pEngine->IsConnected() || !g::pLocalEntity || !g_Menu.Config.Thirdperson)
			return;

		static bool init = false;

		if (GetKeyState(g_Menu.Config.ThirdpersonKey) && g::pLocalEntity->IsAlive())
		{
			if (init)
			{
				ConVar* sv_cheats = g_pCvar->FindVar("sv_cheats");
				*(int*)((DWORD)& sv_cheats->fnChangeCallback + 0xC) = 0; // ew
				sv_cheats->SetValue(1);
				g_pEngine->ExecuteClientCmd("thirdperson");
				isthirdperson = true;
			}
			init = false;
		}
		else
		{
			if (!init)
			{
				ConVar* sv_cheats = g_pCvar->FindVar("sv_cheats");
				*(int*)((DWORD)& sv_cheats->fnChangeCallback + 0xC) = 0; // ew
				sv_cheats->SetValue(1);
				g_pEngine->ExecuteClientCmd("firstperson");
				isthirdperson = false;
			}
			init = true;
		}

		if (curStage == FRAME_RENDER_START && GetKeyState(g_Menu.Config.ThirdpersonKey) && g::pLocalEntity->IsAlive())
		{
			g_pPrediction->SetLocalViewAngles(Vector(g::FakeAngle.x, g::FakeAngle.y, 0)); // lol
		}
	}
	void Crosshair()
	{
		if (!g::pLocalEntity)
			return;

		if (!g::pLocalEntity->IsAlive())
			return;

		if (!g_pEngine->IsInGame() && g_pEngine->IsConnected())
			return;

		if (g::pLocalEntity->IsScoped() && g_Menu.Config.NoScope && g::pLocalEntity->IsAlive())
		{
			int Height, Width;
			g_pEngine->GetScreenSize(Width, Height);

			Vector punchAngle = g::pLocalEntity->GetAimPunchAngle();

			float x = Width / 2;
			float y = Height / 2;
			int dy = Height / 90;
			int dx = Width / 90;
			x -= (dx * (punchAngle.y));
			y += (dy * (punchAngle.x));

			Vector2D screenPunch = { x, y };

			g_pSurface->Line(0, screenPunch.y, Width, screenPunch.y, Color(0, 0, 0, 255));
			g_pSurface->Line(screenPunch.x, 0, screenPunch.x, Height, Color(0, 0, 0, 255));
		}

		static bool init = false;
		static bool init2 = false;

		if (g_Menu.Config.Crosshair[0])
		{
			if (g::pLocalEntity->IsScoped())
			{
				if (init2)
				{
					ConVar* sv_cheats = g_pCvar->FindVar("sv_cheats");
					*(int*)((DWORD)& sv_cheats->fnChangeCallback + 0xC) = 0; // ew
					sv_cheats->SetValue(1);

					g_pEngine->ExecuteClientCmd("weapon_debug_spread_show 0");
					g_pEngine->ExecuteClientCmd("crosshair 0");
				}
				init2 = false;
			}
			else
			{
				if (!init2)
				{
					ConVar* sv_cheats = g_pCvar->FindVar("sv_cheats");
					*(int*)((DWORD)& sv_cheats->fnChangeCallback + 0xC) = 0; // ew
					sv_cheats->SetValue(1);

					g_pEngine->ExecuteClientCmd("weapon_debug_spread_show 3");
					g_pEngine->ExecuteClientCmd("crosshair 1");
				}
				init2 = true;
			}

			init = false;
		}
		else
		{
			if (!init)
			{
				ConVar* sv_cheats = g_pCvar->FindVar("sv_cheats");
				*(int*)((DWORD)& sv_cheats->fnChangeCallback + 0xC) = 0; // ew
				sv_cheats->SetValue(1);

				g_pEngine->ExecuteClientCmd("weapon_debug_spread_show 0");
				g_pEngine->ExecuteClientCmd("crosshair 1");
			}
			init = true;
		}
	}

	bool fakelag;

	void NormalWalk() // heh
	{
		g::pCmd->buttons &= ~IN_MOVERIGHT;
		g::pCmd->buttons &= ~IN_MOVELEFT;
		g::pCmd->buttons &= ~IN_FORWARD;
		g::pCmd->buttons &= ~IN_BACK;

		if (g::pCmd->forwardmove > 0.f)
			g::pCmd->buttons |= IN_FORWARD;
		else if (g::pCmd->forwardmove < 0.f)
			g::pCmd->buttons |= IN_BACK;
		if (g::pCmd->sidemove > 0.f)
		{
			g::pCmd->buttons |= IN_MOVERIGHT;
		}
		else if (g::pCmd->sidemove < 0.f)
		{
			g::pCmd->buttons |= IN_MOVELEFT;
		}
	}

private:
	CUserCmd* pCmd;
	C_BaseEntity* pLocal;

	void DoBhop() const
	{
		if (!g_Menu.Config.Bhop)
			return;

		if (!g::pLocalEntity->IsAlive())
			return;

		static bool bLastJumped = false;
		static bool bShouldFake = false;

		if (!bLastJumped && bShouldFake)
		{
			bShouldFake = false;
			pCmd->buttons |= IN_JUMP;
		}
		else if (pCmd->buttons & IN_JUMP)
		{
			if (pLocal->GetFlags() & FL_ONGROUND)
				bShouldFake = bLastJumped = true;
			else
			{
				pCmd->buttons &= ~IN_JUMP;
				bLastJumped = false;
			}
		}
		else
			bShouldFake = bLastJumped = false;
	}

	template<class T, class U>
	static T clamp(T in, U low, U high) {
		if (in <= low)
			return low;

		if (in >= high)
			return high;

		return in;
	}


	void DoAutostrafe() const
	{
		if (!g_Menu.Config.AutoStrafe)
			return;

		static bool is_bhopping;
		static float calculated_direction;
		static bool in_transition;
		static float true_direction;
		static float wish_direction;
		static float step;
		static float rough_direction;

		auto local_player = g::pLocalEntity;
		if (!local_player) return;


		enum directions
		{
			FORWARDS = 0,
			BACKWARDS = 180,
			LEFT = 90,
			RIGHT = -90
		};

		// Reset direction when player is not strafing
		is_bhopping = pCmd->buttons & IN_JUMP;
		if (!is_bhopping && local_player->GetFlags() & FL_ONGROUND)
		{
			calculated_direction = directions::FORWARDS;
			in_transition = false;
			return;
		}

		// Get true view angles
		Vector base{ };
		g_pEngine->GetViewAngles(base);

		// Calculate the rough direction closest to the player's true direction
		auto get_rough_direction = [&](float true_direction) -> float
		{
			// Make array with our four rough directions
			std::array< float, 4 > minimum = { directions::FORWARDS, directions::BACKWARDS, directions::LEFT, directions::RIGHT };
			float best_angle, best_delta = 181.f;

			// Loop through our rough directions and find which one is closest to our true direction
			for (size_t i = 0; i < minimum.size(); ++i)
			{
				float rough_direction = base.y + minimum.at(i);
				float delta = fabsf(g_Math.NormalizeYaw(true_direction - rough_direction));

				// Only the smallest delta wins out
				if (delta < best_delta)
				{
					best_angle = rough_direction;
					best_delta = delta;
				}
			}

			return best_angle;
		};

		// Get true direction based on player velocity
		true_direction = local_player->GetVelocity().Angle().y;

		// Detect wish direction based on movement keypresses
		if (pCmd->buttons & IN_FORWARD)
		{
			wish_direction = base.y + directions::FORWARDS;
		}
		else if (pCmd->buttons & IN_BACK)
		{
			wish_direction = base.y + directions::BACKWARDS;
		}
		else if (pCmd->buttons & IN_MOVELEFT)
		{
			wish_direction = base.y + directions::LEFT;
		}
		else if (pCmd->buttons & IN_MOVERIGHT)
		{
			wish_direction = base.y + directions::RIGHT;
		}
		else
		{
			// Reset to forward when not pressing a movement key, then fix anti-aim strafing by setting IN_FORWARD
			pCmd->buttons |= IN_FORWARD;
			wish_direction = base.y + directions::FORWARDS;
		}
		// Calculate the ideal strafe rotation based on player speed (c) navewindre
		float speed_rotation = min(RAD2DEG(std::asinf(30.f / local_player->GetVelocity().Length2D())) * 0.5f, 45.f);
		if (in_transition)
		{
			// Get value to rotate by via calculated speed rotation
			float ideal_step = speed_rotation + calculated_direction;
			step = fabsf(g_Math.NormalizeYaw(calculated_direction - ideal_step)); // 15.f is a good alternative, but here is where you would do your "speed" slider value for the autostrafer

			// Check when the calculated direction arrives close to the wish direction
			if (fabsf(g_Math.NormalizeYaw(wish_direction - calculated_direction)) > step)
			{
				float add = g_Math.NormalizeYaw(calculated_direction + step);
				float sub = g_Math.NormalizeYaw(calculated_direction - step);

				// Step in direction that gets us closer to our wish direction
				if (fabsf(g_Math.NormalizeYaw(wish_direction - add)) >= fabsf(g_Math.NormalizeYaw(wish_direction - sub)))
				{
					calculated_direction -= step;
				}
				else
				{
					calculated_direction += step;
				}
			}
			else
			{
				// Stop transitioning when we meet our wish direction
				in_transition = false;
			}
		}
		else
		{
			// Get rough direction and setup calculated direction only when not transitioning
			rough_direction = get_rough_direction(true_direction);
			calculated_direction = rough_direction;

			// When we have a difference between our current (rough) direction and our wish direction, then transition
			if (rough_direction != wish_direction)
			{
				in_transition = true;
			}
		}

		// Set movement up to be rotated
		pCmd->forwardmove = 0.f;
		pCmd->sidemove = pCmd->command_number % 2 ? 450.f : -450.f;

		// Calculate ideal rotation based on our newly calculated direction
		float direction = (pCmd->command_number % 2 ? speed_rotation : -speed_rotation) + calculated_direction;

		// Rotate our direction based on our new, defininite direction
		float rotation = DEG2RAD(base.y - direction);

		float cos_rot = cos(rotation);
		float sin_rot = sin(rotation);

		float forwardmove = (cos_rot * pCmd->forwardmove) - (sin_rot * pCmd->sidemove);
		float sidemove = (sin_rot * pCmd->forwardmove) + (cos_rot * pCmd->sidemove);

		// Apply newly rotated movement
		pCmd->forwardmove = forwardmove;
		pCmd->sidemove = sidemove;
	}


	bool SendPacket;
	void DoFakeLag() const
	{
		g::bSendPacket = true;
		int choke_amount = 1;

		auto local_player = g::pLocalEntity;
		if (!local_player || local_player->GetHealth() <= 0)
			return;



		auto net_channel = g_pEngine->GetNetChannel();
		if (!net_channel)
			return;

		static Vector originrecords[2];

		Vector curpos = local_player->GetOrigin();


		if (SendPacket) {
			static bool flip = false;
			originrecords[flip ? 0 : 1] = curpos;
			flip = !flip;
		}

		if ((originrecords[0] - originrecords[1]).LengthSqr() > 4096.f)
			g::lcBroken = true;
		else
			g::lcBroken = false;

		if (g_Menu.Config.FakeLagEnable)
		{
			if (g::fakeduck)
			{
				g::bSendPacket = (14 <= *(int*)(uintptr_t(g_pClientState) + 0x4D28));
			}
			else if (g_pEngine->IsVoiceRecording())
			{
				choke_amount = 4;
			}
			else if (g_Menu.Config.FakeLagEnable)
			{
				if (g_Menu.Config.FakelagType == 0)
				{
					float speed = local_player->GetVelocity().Length2D();
					if (speed > 90)
						choke_amount = 8;
					else if (speed > 170)
						choke_amount = 7;
					else if (speed > 220)
						choke_amount = 6;
					else
						choke_amount = 10;
				}
				else if (g_Menu.Config.FakelagType == 1)
				{

					if (local_player->GetVelocity().Length2D() > 0.1f) {
						choke_amount = static_cast<int>((64.0f / g_pGlobalVars->intervalPerTick) /
							local_player->GetVelocity().Length2D());
					}
					else {
						choke_amount = 16;
					}

					if (choke_amount > 16) {
						choke_amount = 16;
					}

				}
				else if (g_Menu.Config.FakelagType == 2)
				{

					if (g::pLocalEntity->GetFlags() & FL_ONGROUND)
						choke_amount = g_Menu.Config.FakelagThreshold;
					else
						choke_amount = g_Menu.Config.FakeLagValue;

				}
				else
					choke_amount = 1;
			}
		}
		else
			choke_amount = 1;


		if (g_pEngine->GetNetChannel()->m_nChokedPackets >= min(16, choke_amount))
		{
			g::bSendPacket = true;
		}
		else
			g::bSendPacket = false;
	}
};
extern Misc g_Misc;