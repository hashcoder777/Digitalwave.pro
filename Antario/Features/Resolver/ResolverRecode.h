#pragma once
#include "..\..\Utils\GlobalVars.h"
#include "..\..\SDK\CGlobalVarsBase.h"
#include "..\..\SDK\IClientMode.h"
#include "..\..\Utils\Utils.h"
#include "..\..\SDK\IVEngineClient.h"
#include "..\..\SDK\Hitboxes.h"
#include "..\..\SDK\PlayerInfo.h"
#include "..\..\Utils\Math.h"
#include "..\..\SDK\IBaseClientDll.h"
#include "../../SDK/ICvar.h"
#include "..\..\Menu\Menu.h"
#include <deque>

struct resolver_info
{
	AnimationLayer animation;

	bool jitter_desync = false, high_delta = false, low_delta = false, static_desync = false, lby_changed = false;

	float last_lby = FLT_MAX, lby_delta = FLT_MAX, last_eyes = FLT_MAX, eye_delta = FLT_MAX, eye_lby_delta = FLT_MAX, eyes = FLT_MAX;

};

struct resolverInfo2
{
public:
	float fakegoalfeetyaw;
	resolver_info current_tick;
	resolver_info previous_tick;

};

class ResolverRecode
{
public:
	bool UseFreestandAngle[65];
	float FreestandAngle[65];
	Vector absOriginBackup;
	float pitchHit[65];
	resolverInfo2 resolverinfoo;
	resolverInfo2 __player[64];
	int resolvetype;
	void resolve(C_BaseEntity* ent);
	void HandleHits(C_BaseEntity* pEnt);
	void fix_local_player_animations();
	float approach_angle(float cur, float target, float inc);
	//void SanaPidaras(C_BaseEntity * entity);
	float approach(float cur, float target, float inc);
	float NormalizeYaw180(float yaw);
	float AngleNormalize(float angle);
	float angle_difference(float a, float b);
	void AnimationFix(C_BaseEntity* pEnt);
	void FrameStage(ClientFrameStage_t stage);
};
extern ResolverRecode g_ResolverRecode;