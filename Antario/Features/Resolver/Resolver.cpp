#include "Resolver.h"
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
#include "../rankchanger/IMemAlloc.h""
#include <array>

#define clamp(val, min, max) (((val) > (max)) ? (max) : (((val) < (min)) ? (min) : (val)))

Resolver g_Resolver;
IMemAlloc* g_pMemalloc;

Vector TraceToEnd(Vector start, Vector end)
{
	C_Trace trace;
	CTraceWorldOnly* filter;

	g_pTrace->TraceRay(C_Ray(start, end), mask_all, filter, &trace);

	return trace.end;
}

inline float NormalizeYaw(float yaw)
{
	if (yaw > 180)
		yaw -= (round(yaw / 120) * 120.f);
	else if (yaw < -180)
		yaw += (round(yaw / 120) * -120.f);
	return yaw;
}

float GetLBYRotatedYaw(float lby, float yaw)
{
	float delta = g_Math.NormalizeYaw(yaw - lby);
	if (fabs(delta) < 25.f)
		return lby;

	if (delta > 0.f)
		return yaw + 25.f;

	return yaw;
}


inline float NormalizePitch(float pitch)
{
	while (pitch > 90.f)
		pitch -= 180.f;
	while (pitch < -90.f)
		pitch += 180.f;
	return pitch;
}

float __fastcall ang_dif(float a1, float a2)
{
	float val = fmodf(a1 - a2, 360.0);
	while (val < -180.0f) val += 360.0f;
	while (val > 180.0f) val -= 360.0f;
	return val;
}

Vector CalcAngle69(Vector dst, Vector src)
{
	Vector angles;
	double delta[3] = { (src.x - dst.x), (src.y - dst.y), (src.z - dst.z) };
	double hyp = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
	angles.x = (float)(atan(delta[2] / hyp) * 180.0 / 3.14159265);
	angles.y = (float)(atanf(delta[1] / delta[0]) * 57.295779513082f);
	angles.z = 0.0f;

	if (delta[0] >= 0.0)
	{
		angles.y += 180.0f;
	}
	return angles;
}
template <class T>
constexpr const T& Max(const T& x, const T& y)
{
	return (x < y) ? y : x;
}

float NormalizeYaw180(float yaw)
{
	if (yaw > 120)
		yaw -= (round(yaw / 120) * 120.f);
	else if (yaw < -120)
		yaw += (round(yaw / 120) * -120.f);

	return yaw;
}

float angle_difference(float a, float b) {
	auto diff = NormalizeYaw180(a - b);

	if (diff < 180)
		return diff;
	return diff - 360;
}

bool delta_58(float first, float second)
{
	if (first - second < 58.f && first - second > -58.f)
	{
		return true;
	}
	return false;
}

void AngleVectors(const Vector& angles, Vector* forward)
{
	Assert(s_bMathlibInitialized);
	Assert(forward);
	float	sp, sy, cp, cy;
	sy = sin(DEG2RAD(angles[1]));
	cy = cos(DEG2RAD(angles[1]));
	sp = sin(DEG2RAD(angles[0]));
	cp = cos(DEG2RAD(angles[0]));
	forward->x = cp * cy;
	forward->y = cp * sy;
	forward->z = -sp;
}

void NormalizeNumX(Vector& vIn, Vector& vOut)
{
	float flLen = vIn.Length();
	if (flLen == 0) {
		vOut.Init(0, 0, 1);
		return;
	}
	flLen = 1 / flLen;
	vOut.Init(vIn.x * flLen, vIn.y * flLen, vIn.z * flLen);
}

float flAngleMod(float flAngle)
{
	return((360.0f / 65536.0f) * ((int32_t)(flAngle * (65536.0f / 360.0f)) & 65535));
}

float approach(float cur, float target, float inc) {
	inc = abs(inc);

	if (cur < target)
		return min(cur + inc, target);
	if (cur > target)
		return max(cur - inc, target);

	return target;
}

float approach_angle(float cur, float target, float inc) {
	auto diff = angle_difference(target, cur);
	return approach(cur, cur + diff, inc);
}

float ApproachAngle(float target, float value, float speed)
{
	target = flAngleMod(target);
	value = flAngleMod(value);
	float delta = target - value;
	if (speed < 0)
		speed = -speed;
	if (delta < -180)
		delta += 360;
	else if (delta > 180)
		delta -= 360;
	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;
	return value;
}

bool solve_desync_simple(C_BaseEntity * e) // 979
{
	if (!e || e->IsDormant() || !e->IsAlive())
		return false;

	for (size_t i = 0; i < e->NumOverlays(); i++)
	{
		auto layer = e->GetAnimOverlay(i);
		if (!layer)
			continue;

		if (e->GetSequenceActivity(layer->m_nSequence) == 979)
		{
			if (layer->m_flWeight == 0.0f && (layer->m_flCycle == 0.0f || layer->m_flCycle != layer->m_flPrevCycle))
				return true;
		}
	}
	return false;
}

bool Resolver::AntiPener(C_BaseEntity* entity, float &yaw, int damage_tolerance)
{

	Autowall* wall = nullptr;


	if (!(entity->GetFlags() & FL_ONGROUND))
		return false;

	auto local_player = g::pLocalEntity;
	if (!local_player || local_player->GetLifeState() != 0)
		return false;


	if (!wall) return false;

	std::vector<C_BaseEntity*> enemies;

	const float height = 64;

	float leftdamage = 0.f, rightdamage = 0.f, backdamage = 0.f;


	std::vector<Vector> last_eye_positions;

	last_eye_positions.insert(last_eye_positions.begin(), local_player->GetOrigin() + local_player->GetViewOffset());
	if (last_eye_positions.size() > 128)
		last_eye_positions.pop_back();

	auto nci = g_pEngine->GetNetChannelInfo();
	if (!nci)
		return false;

	const int latency_ticks = TIME_TO_TICKS(nci->GetLatency(FLOW_OUTGOING));
	const auto latency_based_eye_pos = last_eye_positions.size() <= latency_ticks ? last_eye_positions.back() : last_eye_positions[latency_ticks];
	float lpos = 0.f;

	Vector direction_1, direction_2, direction_3;
	g_Math.AngleVectors(Vector(0.f, g_Math.CalcAngle(local_player->GetOrigin(), entity->GetOrigin()).y + 90.f, 0.f), &direction_1);
	g_Math.AngleVectors(Vector(0.f, g_Math.CalcAngle(local_player->GetOrigin(), entity->GetOrigin()).y - 90.f, 0.f), &direction_2);
	g_Math.AngleVectors(Vector(0.f, g_Math.CalcAngle(local_player->GetOrigin(), entity->GetOrigin()).y + 180.f, 0.f), &direction_3);

	const auto left_eye_pos = entity->GetOrigin() + Vector(0, 0, height) + (direction_1 * 16.f);
	const auto right_eye_pos = entity->GetOrigin() + Vector(0, 0, height) + (direction_2 * 16.f);
	const auto back_eye_pos = entity->GetOrigin() + Vector(0, 0, height) + (direction_3 * 16.f);

	leftdamage = wall->CanHitFloatingPoint(latency_based_eye_pos, left_eye_pos);
	rightdamage = wall->CanHitFloatingPoint(latency_based_eye_pos, right_eye_pos);
	backdamage = wall->CanHitFloatingPoint(latency_based_eye_pos, back_eye_pos);



	int right_damage = rightdamage;
	int left_damage = leftdamage;
	int back_damage = backdamage;

	float at_target_yaw = g_Math.CalcAngle(local_player->GetOrigin(), entity->GetOrigin()).y;
	const float right_yaw = at_target_yaw - 90.f;
	const float left_yaw = at_target_yaw + 90.f;
	auto head_position = entity->GetOrigin() + Vector(0, 0, 64.f);

	float calculated_yaw;

	/// Find the lowest fov enemy
	C_BaseEntity* closest_enemy = nullptr;
	float lowest_fov = 360.f;
	for (int i = 0; i <= 64; i++)
	{
		auto ent = g_pEntityList->GetClientEntity(i);
		if (!ent || ent->IsDormant() || ent->GetTeam() == entity->GetTeam() || ent->GetHealth() <= 0 || !ent->IsAlive())
			continue;

		const float current_fov = fabs(g_Math.NormalizeYaw(g_Math.CalcAngle(entity->GetOrigin(), ent->GetOrigin()).y - at_target_yaw));
		if (current_fov < lowest_fov)
		{
			lowest_fov = current_fov;
			closest_enemy = ent;
		}

		enemies.push_back(ent);
	}

	if (closest_enemy == nullptr)
		return false;

	auto RotateAndExtendPosition = [](Vector position, float yaw, float distance) -> Vector
	{
		Vector direction;
		g_Math.AngleVectors(Vector(0, yaw, 0), &direction);

		return position + (direction * distance);
	};

	auto right_head_position = RotateAndExtendPosition(head_position, right_yaw, 16.f);
	auto left_head_position = RotateAndExtendPosition(head_position, left_yaw, 16.f);

	auto CalcDamage = [wall, entity, enemies](Vector point) -> int
	{
		int damage = 0;
		for (auto& enemy : enemies)
		{
			damage += Max<int>(wall->CanHitFloatingPoint(enemy->GetOrigin() + Vector(0, 0, 64.f), point),
				wall->CanHitFloatingPoint(enemy->GetOrigin() + Vector(0, 0, 64.f), point));
		}

		return damage;
	};


	auto RotateLBYAndYaw = [right_yaw, left_yaw, entity](int right_damage, int left_damage, float& yaw, bool prefect_angle = false) -> bool
	{
		bool prefer_right = right_damage < left_damage;

		yaw = prefer_right ? right_yaw : left_yaw;

		/// If not moving
		if (prefect_angle)
			yaw = g_Math.GetLBYRotatedYaw(entity->GetLowerBodyYaw(), yaw);

		return true;
	};

	if (left_damage >= damage_tolerance && right_damage >= damage_tolerance && back_damage >= damage_tolerance)
		return false;

	if (left_damage >= damage_tolerance && right_damage >= damage_tolerance && back_damage < damage_tolerance)
		calculated_yaw = at_target_yaw + 180.f;
	else if (right_damage == left_damage)
	{
		if (g_Math.NormalizePitch(g_Math.CalcAngle(entity->GetOrigin(), local_player->GetOrigin()).x) > 15.f && back_damage < damage_tolerance)
			calculated_yaw = at_target_yaw + 180.f;
		else
		{
			right_head_position = RotateAndExtendPosition(head_position, right_yaw, 16.f);
			left_head_position = RotateAndExtendPosition(head_position, left_yaw, 16.f);

			right_damage = CalcDamage(right_head_position), left_damage = CalcDamage(left_head_position);

			if (right_damage == left_damage)
			{
				/// just return the side closest to a wall
				right_head_position = TraceToEnd(head_position, RotateAndExtendPosition(head_position, right_yaw, 16.f));
				left_head_position = TraceToEnd(head_position, RotateAndExtendPosition(head_position, left_yaw, 16.f));

				float distance_1, distance_2;
				C_Trace trace;
				CTraceWorldOnly* filter;

				auto end_pos = local_player->GetOrigin() + Vector(0, 0, 64.f);

				/// right position

				g_pTrace->TraceRay(C_Ray(right_head_position, end_pos), mask_all, filter, &trace);
				distance_1 = (right_head_position - trace.end).Length();

				/// left position
				g_pTrace->TraceRay(C_Ray(left_head_position, end_pos), mask_all, filter, &trace);
				distance_2 = (left_head_position - trace.end).Length();

				if (fabs(distance_1 - distance_2) > 15.f)
					RotateLBYAndYaw(distance_1, distance_2, calculated_yaw);
				else
					calculated_yaw = at_target_yaw + 180;
			}
			else
				RotateLBYAndYaw(right_damage, left_damage, calculated_yaw);
		}
	}
	else
	{
		if (g_Math.NormalizePitch(g_Math.CalcAngle(entity->GetOrigin(), local_player->GetOrigin()).x) > 15.f && back_damage < damage_tolerance)
			calculated_yaw = at_target_yaw + 180.f;
		else
		{
			bool prefer_right = (right_damage < left_damage);
			calculated_yaw = prefer_right ? right_yaw : left_yaw;
		}
	}



	yaw = calculated_yaw;



	return true;
}


void Resolver::AnimationFix(C_BaseEntity* pEnt)
{
	//who needs structs or classes not me lol
	static float oldSimtime[65];
	static float storedSimtime[65];
	static float ShotTime[65];
	static float SideTime[65][3];
	static int LastDesyncSide[65];
	static bool Delaying[65];
	static AnimationLayer StoredLayers[64][15];
	static C_AnimState * StoredAnimState[65];
	static float StoredPosParams[65][24];
	static Vector oldEyeAngles[65];
	static float oldGoalfeetYaw[65];
	float* PosParams = (float*)((uintptr_t)pEnt + 0x2774);
	bool update = false;
	bool shot = false;

	static bool jittering[65];

	auto* AnimState = pEnt->AnimState();
	if (!AnimState || !pEnt->AnimOverlays() || !PosParams)
		return;

	auto RemapVal = [](float val, float A, float B, float C, float D) -> float
	{
		if (A == B)
			return val >= B ? D : C;
		return C + (D - C) * (val - A) / (B - A);
	};

	if (storedSimtime[pEnt->EntIndex()] != pEnt->GetSimulationTime())
	{
		jittering[pEnt->EntIndex()] = false;
		pEnt->ClientAnimations(true);
		pEnt->UpdateClientAnimation();

		memcpy(StoredPosParams[pEnt->EntIndex()], PosParams, sizeof(float) * 24);
		memcpy(StoredLayers[pEnt->EntIndex()], pEnt->AnimOverlays(), (sizeof(AnimationLayer) * pEnt->NumOverlays()));

		oldGoalfeetYaw[pEnt->EntIndex()] = AnimState->m_flGoalFeetYaw;

		if (pEnt->GetActiveWeapon() && !pEnt->IsKnifeorNade())
		{
			if (ShotTime[pEnt->EntIndex()] != pEnt->GetActiveWeapon()->GetLastShotTime())
			{
				shot = true;
				ShotTime[pEnt->EntIndex()] = pEnt->GetActiveWeapon()->GetLastShotTime();
			}
			else
				shot = false;
		}
		else
		{
			shot = false;
			ShotTime[pEnt->EntIndex()] = 0.f;
		}

		float angToLocal = g_Math.NormalizeYaw(g_Math.CalcAngle(g::pLocalEntity->GetOrigin(), pEnt->GetOrigin()).y);

		float Back = g_Math.NormalizeYaw(angToLocal);
		float DesyncFix = 0;
		float Resim = g_Math.NormalizeYaw((0.24f / (pEnt->GetSimulationTime() - oldSimtime[pEnt->EntIndex()]))*(oldEyeAngles[pEnt->EntIndex()].y - pEnt->GetEyeAngles().y));

		if (Resim > -119.f)
			Resim = 120.f;
		if (Resim < -98.f)
			Resim = -88.f;
		if (Resim > 89.f)
			Resim = 89.f;
		if (Resim < -120.f)
			Resim = -120.f;

		if (pEnt->GetVelocity().Length2D() > 0.5f && !shot)
		{
			float Delta = g_Math.NormalizeYaw(g_Math.NormalizeYaw(g_Math.CalcAngle(Vector(0, 0, 0), pEnt->GetVelocity()).y) - g_Math.NormalizeYaw(g_Math.NormalizeYaw(AnimState->m_flGoalFeetYaw + RemapVal(PosParams[11], 0, 1, -60, 60)) + Resim));

			int CurrentSide = 0;

			if (Delta < 0)
			{
				CurrentSide = 1;
				SideTime[pEnt->EntIndex()][1] = g_pGlobalVars->curtime;
			}
			else if (Delta > 0)
			{
				CurrentSide = 2;
				SideTime[pEnt->EntIndex()][2] = g_pGlobalVars->curtime;
			}

			if (LastDesyncSide[pEnt->EntIndex()] == 1)
			{
				Resim += (58.f - Resim);
				DesyncFix += (58.f - Resim);
				Resim += (89.f - Resim);
				DesyncFix += (89.f - Resim);
			}
			if (LastDesyncSide[pEnt->EntIndex()] == 2)
			{
				Resim += (-58.f - Resim);
				DesyncFix += (-58.f - Resim);
				Resim += (89.f - Resim);
				DesyncFix += (89.f - Resim);
			}

			if (LastDesyncSide[pEnt->EntIndex()] != CurrentSide)
			{
				Delaying[pEnt->EntIndex()] = true;

				if (.5f < (g_pGlobalVars->curtime - SideTime[pEnt->EntIndex()][LastDesyncSide[pEnt->EntIndex()]]))
				{
					LastDesyncSide[pEnt->EntIndex()] = CurrentSide;
					Delaying[pEnt->EntIndex()] = false;
				}
			}

			if (!Delaying[pEnt->EntIndex()])
				LastDesyncSide[pEnt->EntIndex()] = CurrentSide;
		}
		else if (!shot)
		{
			float Brute = UseFreestandAngle[pEnt->EntIndex()] ? g_Math.NormalizeYaw(Back + FreestandAngle[pEnt->EntIndex()]) : pEnt->GetLowerBodyYaw();

			float Delta = g_Math.NormalizeYaw(g_Math.NormalizeYaw(Brute - g_Math.NormalizeYaw(g_Math.NormalizeYaw(AnimState->m_flGoalFeetYaw + RemapVal(PosParams[11], 0, 1, -60, 60))) + Resim));

			if (Delta > 58.f)
				Delta = 58.f;
			if (Delta < -58.f)
				Delta = -120.f;
			if (Delta > 89.f)
				Delta = 89.f;
			if (Delta < -120.f)
				Delta = -120.f;

			Resim += Delta;
			DesyncFix += Delta;

			if (Resim > 180.f)
				Resim = 179.f;
			if (Resim < -179.f)
				Resim = -120.f;
			if (Resim > 89.f)
				Resim = 89.f;
			if (Resim < -120.f)
				Resim = -120.f;
			
				
		}

		float Equalized = g_Math.NormalizeYaw(g_Math.NormalizeYaw(AnimState->m_flGoalFeetYaw + RemapVal(PosParams[11], 0, 1, -60, 60)) + Resim);

		float JitterDelta = fabs(g_Math.NormalizeYaw(oldEyeAngles[pEnt->EntIndex()].y - pEnt->GetEyeAngles().y));

		if (JitterDelta >= 70.f && !shot)
			jittering[pEnt->EntIndex()] = true;

		if (pEnt != g::pLocalEntity && pEnt->GetTeam() != g::pLocalEntity->GetTeam() && (pEnt->GetFlags() & FL_ONGROUND) && g_Menu.Config.Resolver)
		{
			if (jittering[pEnt->EntIndex()])
				AnimState->m_flGoalFeetYaw = g_Math.NormalizeYaw(pEnt->GetEyeAngles().y + DesyncFix);
			else
				AnimState->m_flGoalFeetYaw = Equalized;

			pEnt->SetLowerBodyYaw(AnimState->m_flGoalFeetYaw);
		}

		StoredAnimState[pEnt->EntIndex()] = AnimState;

		oldEyeAngles[pEnt->EntIndex()] = pEnt->GetEyeAngles();

		oldSimtime[pEnt->EntIndex()] = storedSimtime[pEnt->EntIndex()];

		storedSimtime[pEnt->EntIndex()] = pEnt->GetSimulationTime();

		update = true;
	}

	pEnt->ClientAnimations(false);

	if (pEnt != g::pLocalEntity && pEnt->GetTeam() != g::pLocalEntity->GetTeam() && (pEnt->GetFlags() & FL_ONGROUND) && g_Menu.Config.Resolver)
		pEnt->SetLowerBodyYaw(AnimState->m_flGoalFeetYaw);

	AnimState = StoredAnimState[pEnt->EntIndex()];

	memcpy((void*)PosParams, &StoredPosParams[pEnt->EntIndex()], (sizeof(float) * 24));
	memcpy(pEnt->AnimOverlays(), StoredLayers[pEnt->EntIndex()], (sizeof(AnimationLayer) * pEnt->NumOverlays()));

	if (pEnt != g::pLocalEntity && pEnt->GetTeam() != g::pLocalEntity->GetTeam() && (pEnt->GetFlags() & FL_ONGROUND) && g_Menu.Config.Resolver && jittering[pEnt->EntIndex()])
		pEnt->SetAbsAngles(Vector(0, pEnt->GetEyeAngles().y, 0));
	else
		pEnt->SetAbsAngles(Vector(0, oldGoalfeetYaw[pEnt->EntIndex()], 0));

	*reinterpret_cast<int*>(uintptr_t(pEnt) + 0xA30) = g_pGlobalVars->framecount;
	*reinterpret_cast<int*>(uintptr_t(pEnt) + 0xA28) = 0;
}

void Resolver::LBY_FIX(C_BaseEntity* pEnt)
{
	int index = pEnt->EntIndex();
	static Vector Angle;
	Angle = pEnt->GetEyeAngles();
	AnimationLayer* anim = nullptr;

	static float moving_lby[65];
	static float moving_sim[65];
	static float stored_lby[65];
	static float old_lby[65];
	static float lby_delta[65];
	static float predicted_yaw[65];
	static bool lby_changes[65];
	static int shots_check[65];
	static float angle_brute[65];
	static float AtTargetAngle;
	static float FixPitch;
	static float FixPitch2;
	static bool HitNS[65];
	static Vector StoredAngle[65];
	static Vector Hitstored[65];
	static int StoredShots[65];
	static int HitShots[65];
	static int HitShotsStored[65];

	int Missedshots[65];

	if (stored_lby[index] != pEnt->GetLowerBodyYaw())
	{
		old_lby[index] = stored_lby[index];
		lby_changes[index] = true;
		stored_lby[index] = pEnt->GetLowerBodyYaw();
	}

	lby_delta[index] = NormalizeYaw180(stored_lby[index] - old_lby[index]);

	if (lby_changes[index])
	{
		if ((pEnt->GetSimulationTime() - moving_sim[index]) > .22f)
			predicted_yaw[index] = lby_delta[index];

		lby_changes[index] = false;
	}

	if (stored_lby[index] != pEnt->GetLowerBodyYaw())
	{
		old_lby[index] = stored_lby[index];
		Angle.y = pEnt->GetLowerBodyYaw();
		lby_changes[index] = true;
		stored_lby[index] = pEnt->GetLowerBodyYaw();
	}
	else if (abs(pEnt->GetVelocity().Length2D()) > 29.f && (pEnt->GetFlags() & FL_ONGROUND))
	{
		Angle.y = pEnt->GetLowerBodyYaw();
		moving_lby[index] = pEnt->GetLowerBodyYaw();
		moving_sim[index] = pEnt->GetSimulationTime();
		lby_changes[index] = false;
		predicted_yaw[index] = 0;
		Missedshots[index] = 0;
		angle_brute[index] = 0;
	}
	else if ((pEnt->GetFlags() & FL_ONGROUND))
	{
		if (shots_check[index] != Missedshots[index])
		{
			angle_brute[index] += predicted_yaw[index];
			shots_check[index] = Missedshots[index];
		}

		Angle.y = NormalizeYaw180(angle_brute[index] + moving_lby[index]);
	}
	else
	{
		Angle.y = pEnt->GetLowerBodyYaw();
	}

	pEnt->SetEyeAngles(Angle);

}

void HandleHits(C_BaseEntity* pEnt)
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
			g_Resolver.pitchHit[pEnt->EntIndex()] = pEnt->GetEyeAngles().x;
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

void Resolver::OnCreateMove() // cancer v2
{
	if (!g_Menu.Config.aimbot_resolver)
		return;

	if (!g::pLocalEntity->IsAlive())
		return;

	if (!g::pLocalEntity->GetActiveWeapon() || g::pLocalEntity->IsKnifeorNade())
		return;


	for (int i = 1; i < g_pEngine->GetMaxClients(); ++i)
	{
		C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(i);

		if (!pPlayerEntity
			|| !pPlayerEntity->IsAlive()
			|| pPlayerEntity->IsDormant()
			|| pPlayerEntity == g::pLocalEntity
			|| pPlayerEntity->GetTeam() == g::pLocalEntity->GetTeam())
		{
			UseFreestandAngle[i] = false;
			continue;
		}

		if (abs(pPlayerEntity->GetVelocity().Length2D()) > 29.f)
			UseFreestandAngle[pPlayerEntity->EntIndex()] = false;

		if (abs(pPlayerEntity->GetVelocity().Length2D()) <= 29.f && !UseFreestandAngle[pPlayerEntity->EntIndex()])
		{
			bool Autowalled = false, HitSide1 = false, HitSide2 = false;

			float angToLocal = g_Math.CalcAngle(g::pLocalEntity->GetOrigin(), pPlayerEntity->GetOrigin()).y;
			Vector ViewPoint = g::pLocalEntity->GetOrigin() + Vector(0, 0, 90);

			Vector2D Side1 = { (45 * sin(g_Math.GRD_TO_BOG(angToLocal))),(45 * cos(g_Math.GRD_TO_BOG(angToLocal))) };
			Vector2D Side2 = { (45 * sin(g_Math.GRD_TO_BOG(angToLocal + 180))) ,(45 * cos(g_Math.GRD_TO_BOG(angToLocal + 180))) };

			Vector2D Side3 = { (50 * sin(g_Math.GRD_TO_BOG(angToLocal))),(50 * cos(g_Math.GRD_TO_BOG(angToLocal))) };
			Vector2D Side4 = { (50 * sin(g_Math.GRD_TO_BOG(angToLocal + 180))) ,(50 * cos(g_Math.GRD_TO_BOG(angToLocal + 180))) };

			Vector Origin = pPlayerEntity->GetOrigin();

			Vector2D OriginLeftRight[] = { Vector2D(Side1.x, Side1.y), Vector2D(Side2.x, Side2.y) };

			Vector2D OriginLeftRightLocal[] = { Vector2D(Side3.x, Side3.y), Vector2D(Side4.x, Side4.y) };

			for (int side = 0; side < 2; side++)
			{
				Vector OriginAutowall = { Origin.x + OriginLeftRight[side].x,  Origin.y - OriginLeftRight[side].y , Origin.z + 90 };
				Vector OriginAutowall2 = { ViewPoint.x + OriginLeftRightLocal[side].x,  ViewPoint.y - OriginLeftRightLocal[side].y , ViewPoint.z };

				if (g_Autowall.CanHitFloatingPoint(OriginAutowall, ViewPoint))
				{
					if (side == 0)
					{
						HitSide1 = true;
						FreestandAngle[pPlayerEntity->EntIndex()] = 87;
					}
					else if (side == 1)
					{
						HitSide2 = true;
						FreestandAngle[pPlayerEntity->EntIndex()] = -88;
					}

					Autowalled = true;
				}
				else
				{
					for (int side222 = 0; side222 < 2; side222++)
					{
						Vector OriginAutowall222 = { Origin.x + OriginLeftRight[side222].x,  Origin.y - OriginLeftRight[side222].y , Origin.z + 90 };

						if (g_Autowall.CanHitFloatingPoint(OriginAutowall222, OriginAutowall2))
						{
							if (side222 == 0)
							{
								HitSide1 = true;
								FreestandAngle[pPlayerEntity->EntIndex()] = 87;
							}
							else if (side222 == 1)
							{
								HitSide2 = true;
								FreestandAngle[pPlayerEntity->EntIndex()] = -86;
							}

							Autowalled = true;
						}
					}
				}
			}

			if (Autowalled)
			{
				if (HitSide1 && HitSide2)
					UseFreestandAngle[pPlayerEntity->EntIndex()] = false;
				else
					UseFreestandAngle[pPlayerEntity->EntIndex()] = true;
			}
		}
	}
}

float Resolver::get_weighted_desync_delta(C_BaseEntity* player, float abs_angle, bool breaking_lby)
{
	float delta = player->GetMaxDelta(player->AnimState()) + breaking_lby ? 30.f : 0.f;

	float relative = -abs_angle;
	float positive = abs_angle + delta;
	float negative = abs_angle - delta;

	float positive_delta = abs(g_Math.NormalizeYaw(relative - positive));
	float negative_delta = abs(g_Math.NormalizeYaw(relative - negative));

	return positive_delta > negative_delta ? -delta : delta;
}

bool is_slow_walking(C_BaseEntity* entity) {
	float velocity_2D[64], old_velocity_2D[64];

	if (entity->GetVelocity().Length2D() != velocity_2D[entity->EntIndex()] && entity->GetVelocity().Length2D() != NULL) {
		old_velocity_2D[entity->EntIndex()] = velocity_2D[entity->EntIndex()];
		velocity_2D[entity->EntIndex()] = entity->GetVelocity().Length2D();
	}

	if (velocity_2D[entity->EntIndex()] > 0.1) {
		int tick_counter[64];

		if (velocity_2D[entity->EntIndex()] == old_velocity_2D[entity->EntIndex()])
			++tick_counter[entity->EntIndex()];
		else
			tick_counter[entity->EntIndex()] = 0;

		while (tick_counter[entity->EntIndex()] > (1 / g_pGlobalVars->intervalPerTick) * fabsf(0.1f))// should give use 100ms in ticks if their speed stays the same for that long they are definetely up to something..
			return true;

	}

}

void Resolver::SanaPidaras(C_BaseEntity* entity)
{
	auto local_player = g::pLocalEntity;
	auto animstate = entity->AnimState();
	if (animstate)
	{                                             // inlined max_desync_delta
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
						case 7:
						animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw + animstate->m_flCurrentFeetYaw + 15.0;
						case 8:
						animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw - animstate->m_flCurrentFeetYaw - 15.0;
					default:
						return;
					}
				}
			}
		}
	}
}

bool breaking_lby_animations(C_BaseEntity* e)
{
	if (!e || e->IsDormant() || !e->IsAlive())
		return false;

	for (size_t i = 0; i < e->NumOverlays(); i++)
	{
		auto layer = e->get_anim_overlay_index(i);
		if (e->get_sequence_act(layer->m_nSequence) == 979)
		{
			if (layer->m_flCycle != layer->m_flCycle || layer->m_flWeight == 1.f)
				return true;
		}
	}

	return false;
}

void fix_local_player_animations()
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

void Resolver::Lbyman(C_BaseEntity* pEnt)
{



	int index = pEnt->EntIndex();
	static Vector Angle;
	Angle = pEnt->GetEyeAngles();
	AnimationLayer* anim = nullptr;

	static float moving_lby[65];
	static float moving_sim[65];
	static float stored_lby[65];
	static float old_lby[65];
	static float lby_delta[65];
	static float predicted_yaw[65];
	static bool lby_changes[65];
	static int shots_check[65];
	static float angle_brute[65];
	static float AtTargetAngle;
	static float FixPitch;
	static float FixPitch2;
	static bool HitNS[65];
	static Vector StoredAngle[65];
	static Vector Hitstored[65];
	static int StoredShots[65];
	static int HitShots[65];
	static int HitShotsStored[65];

	int Missedshots[65];

	if (stored_lby[index] != pEnt->GetLowerBodyYaw())
	{
		old_lby[index] = stored_lby[index];
		lby_changes[index] = true;
		stored_lby[index] = pEnt->GetLowerBodyYaw();
	}

	lby_delta[index] = NormalizeYaw180(stored_lby[index] - old_lby[index]);

	if (lby_changes[index])
	{
		if ((pEnt->GetSimulationTime() - moving_sim[index]) > .22f)
			predicted_yaw[index] = lby_delta[index];

		lby_changes[index] = false;
	}

	if (stored_lby[index] != pEnt->GetLowerBodyYaw())
	{
		old_lby[index] = stored_lby[index];
		Angle.y = pEnt->GetLowerBodyYaw();
		lby_changes[index] = true;
		stored_lby[index] = pEnt->GetLowerBodyYaw();
	}
	else if (abs(pEnt->GetVelocity().Length2D()) > 29.f && (pEnt->GetFlags() & FL_ONGROUND))
	{
		Angle.y = pEnt->GetLowerBodyYaw();
		moving_lby[index] = pEnt->GetLowerBodyYaw();
		moving_sim[index] = pEnt->GetSimulationTime();
		lby_changes[index] = false;
		predicted_yaw[index] = 0;
		Missedshots[index] = 0;
		angle_brute[index] = 0;
	}
	else if ((pEnt->GetFlags() & FL_ONGROUND))
	{
		if (shots_check[index] != Missedshots[index])
		{
			angle_brute[index] += predicted_yaw[index];
			shots_check[index] = Missedshots[index];
		}

		Angle.y = NormalizeYaw180(angle_brute[index] + moving_lby[index]);
	}
	else
	{
		Angle.y = pEnt->GetLowerBodyYaw();
	}

	pEnt->SetEyeAngles(Angle);

}

void update_Fake_state(CCSGOPlayerAnimState* state, Vector ang) {
	using fn = void(__vectorcall*)(void*, void*, float, float, float, void*);
	static auto ret = reinterpret_cast<fn>(Utils::FindSignature("client_panorama.dll", "55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 F3 0F 11 54 24"));

	if (!ret)
		return;

	ret(state, NULL, NULL, ang.y, ang.x, NULL);
}

void Resolver::CreateAnimationState(CCSGOPlayerAnimState* state)
{
	using CreateAnimState_t = void(__thiscall*)(CCSGOPlayerAnimState*, C_BaseEntity*);
	static auto CreateAnimState = (CreateAnimState_t)Utils::FindSignature("client_panorama.dll", "55 8B EC 56 8B F1 B9 ? ? ? ? C7 46");
	if (!CreateAnimState)
		return;

	CreateAnimState(state, g::pLocalEntity);
}

void Resolver::manage_local_fake_animstate()
{
	g_pMemalloc = *reinterpret_cast<IMemAlloc * *>(GetProcAddress(GetModuleHandle("tier0.dll"), "g_pMemAlloc"));
	if (!g::pLocalEntity || !g::pLocalEntity->IsAlive())
		return;

	if (!g_Menu.Config.FakeChams)
		return;

	if (m_fake_spawntime != g::pLocalEntity->m_flSpawnTime() || m_should_update_fake)
	{
		init_fake_anim = false;
		m_fake_spawntime = g::pLocalEntity->m_flSpawnTime();
		m_should_update_fake = false;
	}

	if (!init_fake_anim)
	{
		m_fake_state = static_cast<CCSGOPlayerAnimState*> (g_pMemalloc->Alloc(sizeof(CCSGOPlayerAnimState)));

		if (m_fake_state != nullptr)
			CreateAnimationState(m_fake_state);

		init_fake_anim = true;
	}
	float frametime = g_pGlobalVars->frametime;

	if (g::bSendPacket)
	{
		std::array<AnimationLayer, 13> networked_layers;
		std::memcpy(&networked_layers, g::pLocalEntity->AnimOverlays(), sizeof(AnimationLayer) * 13);

		auto backup_abs_angles = g::pLocalEntity->GetAbsAngles();
		auto backup_poses = g::pLocalEntity->m_flPoseParameter();
		if (g::pLocalEntity->GetFlags2() & FL_ONGROUND)
			g::pLocalEntity->GetFlags2() |= FL_ONGROUND;
		else
		{
			if (g::pLocalEntity->AnimOverlays()[4].m_flWeight != 1.f && g::pLocalEntity->AnimOverlays()[5].m_flWeight != 0.f)
				g::pLocalEntity->GetFlags2() |= FL_ONGROUND;

			if (g::pLocalEntity->GetFlags2() & FL_ONGROUND)
				g::pLocalEntity->GetFlags2() &= ~FL_ONGROUND;
		}




		*reinterpret_cast<int*>(g::pLocalEntity + 0xA68) = g_pGlobalVars->frametime;
		g_pGlobalVars->frametime = 23.91753135f; // :^)

		update_Fake_state(m_fake_state, g::pCmd->viewangles);
		g::should_setup_local_bones = true;
		m_got_fake_matrix = g::pLocalEntity->SetupBones(g::fakematrix, 128, 524032 - 66666/*g_Menu.Config.nightmodeval*/, false);
		const auto org_tmp = g::pLocalEntity->GetRenderOrigin();
		if (m_got_fake_matrix)
		{
			for (auto& i : g::fakematrix)
			{
				i[0][3] -= org_tmp.x;
				i[1][3] -= org_tmp.y;
				i[2][3] -= org_tmp.z;
			}
		}
		std::memcpy(g::pLocalEntity->AnimOverlays(), &networked_layers, sizeof(AnimationLayer) * 13);

		g::pLocalEntity->m_flPoseParameter() = backup_poses;
		backup_abs_angles = g::pLocalEntity->GetAbsAngles();
	}
	g_pGlobalVars->frametime = frametime;

}

void Resolver::FrameStage(ClientFrameStage_t stage)
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
			HandleHits(pPlayerEntity);
			AnimationFix(pPlayerEntity);
		}

		if (stage == FRAME_NET_UPDATE_END && pPlayerEntity != g::pLocalEntity)
		{
			auto VarMap = reinterpret_cast<uintptr_t>(pPlayerEntity) + 36;
			auto VarMapSize = *reinterpret_cast<int*>(VarMap + 20);

			for (auto index = 0; index < VarMapSize; index++)
				*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(VarMap) + index * 12) = 0;
		}

		wasDormant[i] = false;
	}
}

int delta, yaw, pitch;
static int r_yaw, r_pitch;
static bool r_e;
void fix_p(int p, int fake_p, int r_pitch)
{
	p = pitch;
	r_pitch = r_yaw - p;
	fake_p = p - r_pitch;
	p = r_pitch + fake_p;
}

void fix_y(int y, int fake_y, int r_yaw)
{
	y = yaw;
	r_yaw = r_pitch - y;
	fake_y = y - r_yaw;
	y = fake_y + r_yaw;
}
void fix_delta(int p, int y, int d)
{
	p = pitch;
	y = yaw;
	d = delta;
	d = p - y;
}