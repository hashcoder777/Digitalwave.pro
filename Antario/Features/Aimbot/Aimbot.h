#pragma once
#include "..\..\Utils\GlobalVars.h"
#include "..\..\SDK\CGlobalVarsBase.h"
#include "..\..\SDK\IGameEvent.h"
#include <deque>

struct PointInfo
{
public:
	PointInfo(float damage = 0.f, Vector point = Vector(0.f, 0.f, 0.f))
	{
		this->damage = damage;
		this->point = point;
	}

	float damage;
	Vector point;
};
struct Target_t
{
public:
	Target_t(C_BaseEntity* pEnt = nullptr, Vector vPos = Vector(0.f, 0.f, 0.f))
	{
		this->pEnt = pEnt;
		this->vPos = vPos;
	}

	C_BaseEntity* pEnt = nullptr;
	Vector vPos = { 0.f, 0.f, 0.f };
};

class Aimbot
{
public:
	matrix3x4_t Matrix[65][128];
	int Sequence;
	void OnCreateMove();
	int bestEntDmg;
	bool crouch;
	bool isbodyaiming;
	int targetID = 0;
	int shotsMissed = 0;
	bool CanShootWithCurrentHitchanceAndValidTarget;
	bool fired;
	bool HitChance(C_BaseEntity* pEnt, C_BaseCombatWeapon* pWeapon, Vector Angle, Vector Point, int chance);
	bool BaimIfLethal(C_BaseEntity* player);
	bool Baim[65];
	float SynchronizeHitchance();
	int SynchronizeMinDamage();
	float GetInterp();
	void Extrapolate();
	void round_start_event(IGameEvent* pEvent);
	void weapon_fire_event(IGameEvent* pEvent);
	void player_hurt_event(IGameEvent* pEvent);
	void player_death_event(IGameEvent* pEvent);
	int GetTickbase(CUserCmd* ucmd);
	int aimspotx[65];
	int aimspoty[65];
	int aimspotz[65];
	bool locking;
	int GetBaimHitbox();
	bool baiming;
	int niggashots;
	void run(CUserCmd * cmd);
	void no_recoil(CUserCmd * cmd);
	//int shotsMissed;

private:
	bool Backtrack[65];
	bool ShotBacktrack[65];
	std::vector<Vector> GetMultiplePointsForHitbox(C_BaseEntity* local, C_BaseEntity* entity, int iHitbox, matrix3x4_t BoneMatrix[]);
	Vector Hitscan(C_BaseEntity* pEnt);
	bool ZeusHitChance(C_BaseEntity* pEnt, C_BaseCombatWeapon* pWeapon, Vector Angle, Vector Point, int chance);
	void slowwalk(CUserCmd* get_cmd, float get_speed);
	void Autostop();
};
extern Aimbot g_Aimbot;


