#pragma once
#include "..\..\Utils\GlobalVars.h"
#include "..\..\SDK\CGlobalVarsBase.h"
#include "..\..\SDK\IEngineTrace.h"

struct FireBulletData
{
	FireBulletData(const Vector &eyePos, C_BaseEntity* entity) : src(eyePos) , filter(entity)
	{
	}

	Vector          src;
	C_Trace         enter_trace;
	Vector          direction;
	C_TraceFilter   filter;
	float           trace_length;
	float           trace_length_remaining;
	float           current_damage;
	int             penetrate_count;
};

class Autowall
{
public:
	void UTIL_TraceLine(Vector& vecAbsStart, Vector& vecAbsEnd, unsigned int mask, C_BaseEntity* ignore, C_Trace* ptr)
	{
		C_TraceFilter filter(ignore);
		filter.pSkip1 = ignore;
		g_pTrace->TraceRay(C_Ray(vecAbsStart, vecAbsEnd), mask, &filter, ptr);
	}
	void UTIL_ClipTraceToPlayers(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, ITraceFilter* filter, C_Trace* tr)
	{
		static DWORD dwAddress = Utils::FindSignature("client_panorama.dll", "53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 81 EC ? ? ? ? 8B 43 10");
		if (!dwAddress)
			return;

		_asm
		{
			MOV        EAX, filter
			LEA        ECX, tr
			PUSH    ECX
			PUSH    EAX
			PUSH    mask
			LEA        EDX, vecAbsEnd
			LEA        ECX, vecAbsStart
			CALL    dwAddress
			ADD        ESP, 0xC
		}
	}
	float GetDamage(const Vector& vecPoint);
	bool SimulateFireBullet2(FireBulletData& data);
	bool HandleBulletPenetration2(WeaponInfo_t* wpn_data, FireBulletData& data);
	bool TraceToExit(Vector& end, C_Trace& tr, float x, float y, float z, float x2, float y2, float z2, C_Trace* trace);
	bool CanHitFloatingPoint(const Vector &point, const Vector &source);
	float Damage(const Vector &point);
	bool CanWallbang(float& dmg);
private:
};
extern Autowall g_Autowall;