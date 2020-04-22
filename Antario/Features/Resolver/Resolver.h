#pragma once
#include "..\..\Utils\GlobalVars.h"
#include "..\..\SDK\CGlobalVarsBase.h"
#include "..\..\SDK\IClientMode.h"
#include <deque>

struct resolver_infos
{
	AnimationLayer animation;

	bool jitter_desync = false, high_delta = false, low_delta = false, static_desync = false, lby_changed = false;

	float last_lby = FLT_MAX, lby_delta = FLT_MAX, last_eyes = FLT_MAX, eye_delta = FLT_MAX, eye_lby_delta = FLT_MAX, eyes = FLT_MAX;

};

struct resolverInfo
{
public:
	float fakegoalfeetyaw;
	resolver_infos current_tick;
	resolver_infos previous_tick;

};

class Resolver
{
public:
	bool UseFreestandAngle[65];
	float FreestandAngle[65];
	Vector absOriginBackup;
	float pitchHit[65];
	resolverInfo resolverinfoo;
	resolverInfo __player[64];
	int resolvetype;
	float m_fake_spawntime = 0.f;
	float m_fake_delta = 0.f;
	bool m_should_update_fake = false;
	bool init_fake_anim = false;
	CCSGOPlayerAnimState* m_fake_state = nullptr;
	bool m_got_fake_matrix = false;
	void FixAnims(C_BaseEntity * ent);
	float GetEntityAbsYaw(C_BaseEntity * entity);
	bool AntiPener(C_BaseEntity * entity, float & yaw, int damage_tolerance);
	void AnimationFix(C_BaseEntity * pEnt);
	void NewAnimationFix(C_BaseEntity * pEnt, C_BaseEntity * pLocalEnt);
	float get_gun(C_BaseCombatWeapon * weapon);
	void LBY_FIX(C_BaseEntity * pEnt);
	void OnCreateMove();
	void bruhResolver(C_BaseEntity* ent);
	void SanaPidaras(C_BaseEntity * entity);
	void Anims(ClientFrameStage_t stage);
	void FrameStage(ClientFrameStage_t stage);
	float get_weighted_desync_delta(C_BaseEntity* player, float abs_angle, bool breaking_lby);
	void NewAnimationFix(C_BaseEntity* pEnt);

	void manage_local_fake_animstate();
	void UpdateAnimationState(C_AnimState* state, Vector angle);
	void Lbyman(C_BaseEntity * pEnt);
	void CreateAnimationState(CCSGOPlayerAnimState* state);
	void ResetAnimationState(CCSGOPlayerAnimState* state);
private:
	float GetMaxDDelta(C_AnimState* animstate);
};
extern Resolver g_Resolver;