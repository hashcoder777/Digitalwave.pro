#include "Aimbot.h"
#include "Autowall.h"
#include "LagComp.h"
#include "..\AntiAim\AntiAim.h"
#include "..\..\Utils\Utils.h"
#include "..\..\SDK\IVEngineClient.h"
#include "..\..\SDK\PlayerInfo.h"
#include "..\..\SDK\ICvar.h"
#include "..\..\Utils\Math.h"
#include "..\..\SDK\Hitboxes.h"
#include "..\..\Menu\Menu.h"
#include "../../SDK/IGameEvent.h"

Aimbot g_Aimbot;

float Aimbot::SynchronizeHitchance() {
	auto me = g::pLocalEntity;
	auto cmd = g::pCmd;

	if (!me->GetActiveWeapon())
		return 0;

	if (me->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_SCAR20 || me->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_G3SG1) {
		return g_Menu.Config.autohitchance;
	}
	else if (me->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_SSG08) {
		return g_Menu.Config.ssghitchance;
	}
	else if (me->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_AWP) {
		return g_Menu.Config.riflehitchance;
	}
	else if (me->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_REVOLVER || me->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_DEAGLE) {
		return g_Menu.Config.heavypistolhitchance;
	}
	else {
		return g_Menu.Config.otherhitchance;
	}
}

int Aimbot::SynchronizeMinDamage() {
	auto me = g::pLocalEntity;
	auto cmd = g::pCmd;

	if (!me->GetActiveWeapon())
		return 0;

	if (me->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_SCAR20 || me->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_G3SG1) {
		return g_Menu.Config.autodmg;
	}
	else if (me->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_SSG08) {
		return g_Menu.Config.ssgdmg;
	}
	else if (me->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_AWP) {
		return g_Menu.Config.rifledmg;
	}
	else if (me->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_REVOLVER || me->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_DEAGLE) {
		return g_Menu.Config.heavypistoldmg;
	}
	else {
		return g_Menu.Config.otherdmg;
	}
}

void Aimbot::Autostop()
{
	if (g::pLocalEntity->zeus())
		return;

	if (!g_Menu.Config.Autostop)
		return;

	if (!(g::pLocalEntity->GetFlags() & FL_ONGROUND))
		return;

	Vector Velocity = g::pLocalEntity->GetVelocity();
	static float Speed = 450.f;

	Vector Direction;
	Vector RealView;
	g_Math.VectorAngles(Velocity, Direction);
	g_pEngine->GetViewAngles(RealView += 180);
	Direction.y = RealView.y - Direction.y;

	Vector Forward;
	g_Math.AngleVectors(Direction, &Forward);
	Vector NegativeDirection = Forward * -Speed;

	g::pCmd->forwardmove = NegativeDirection.x;
	g::pCmd->sidemove = NegativeDirection.y;

}
void autoscope() {
	auto me = g::pLocalEntity;
	auto cmd = g::pCmd;
	if ((g::pLocalEntity->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_SCAR20 || g::pLocalEntity->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_G3SG1 || g::pLocalEntity->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_SSG08)) {
		if (g_Menu.Config.Aimbot) {
			if (!me->IsScoped()) {
				cmd->buttons |= IN_ATTACK2;
				return;
			}
			else if (me->IsScoped())
			{
				return;
			}
		}
	}
}

void SinCosBitch(float a, float* s, float*c)
{
	*s = sin(a);
	*c = cos(a);
}
void AngleVectorsv2(const Vector &angles, Vector& forward)
{
	float	sp, sy, cp, cy;

	SinCosBitch(DEG2RAD(angles[1]), &sy, &cy);
	SinCosBitch(DEG2RAD(angles[0]), &sp, &cp);

	forward.x = cp * cy;
	forward.y = cp * sy;
	forward.z = -sp;
}

int Aimbot::GetTickbase(CUserCmd* ucmd) {

	static int g_tick = 0;
	static CUserCmd* g_pLastCmd = nullptr;

	if (!ucmd)
		return g_tick;

	if (!g_pLastCmd || g_pLastCmd->hasbeenpredicted) {
		g_tick = g::pLocalEntity->GetTickBase();
	}
	else {
		// Required because prediction only runs on frames, not ticks
		// So if your framerate goes below tickrate, m_nTickBase won't update every tick
		++g_tick;
	}

	g_pLastCmd = ucmd;
	return g_tick;
}

void Aimbot::player_death_event(IGameEvent * pEvent)
{
	if (g_pEngine->GetPlayerForUserID(pEvent->GetInt("attacker")) == g_pEngine->GetLocalPlayer() && g_pEngine->GetPlayerForUserID(pEvent->GetInt("userid")) != g_pEngine->GetLocalPlayer())
	{
		shotsMissed = 0;
	}
}

void Aimbot::player_hurt_event(IGameEvent * pEvent)
{
	int pDamage = pEvent->GetInt("dmg_health");
	int pVictimID = pEvent->GetInt("userid");
	int pAttackID = pEvent->GetInt("attacker");

	if (g_pEngine->GetPlayerForUserID(pAttackID) == g_pEngine->GetLocalPlayer() && g_pEngine->GetPlayerForUserID(pVictimID) != g_pEngine->GetLocalPlayer())
	{
		--shotsMissed;
	}


}

void Aimbot::round_start_event(IGameEvent * pEvent) //rabbit or e0v or skythecoder idk some gay idiot for all the shotsMissed shit
{
	shotsMissed = 0;
}

bool Aimbot::BaimIfLethal(C_BaseEntity* player)
{
	if (!g_Menu.Config.BaimLethal)
		return false;

	if (player->GetHealth() <= g_Menu.Config.BaimLethalSlider)
		return true;
	else
		return false;
}

bool Aimbot::HitChance(C_BaseEntity* pEnt, C_BaseCombatWeapon* pWeapon, Vector Angle, Vector Point, int chance)
{
	if (chance == 0 || SynchronizeHitchance() == 0)
		return true;

	float Seeds = 256.f;

	Angle -= (g::pLocalEntity->GetAimPunchAngle() * g_pCvar->FindVar("weapon_recoil_scale")->GetFloat());

	Vector forward, right, up;

	g_Math.AngleVectors(Angle, &forward, &right, &up);

	int Hits = 0, neededHits = (Seeds * (chance / 100.f));

	float weapSpread = pWeapon->GetSpread(), weapInaccuracy = pWeapon->GetInaccuracy();

	for (int i = 0; i < Seeds; i++)
	{
		float Inaccuracy = g_Math.RandomFloat(0.f, 1.f) * weapInaccuracy;
		float Spread = g_Math.RandomFloat(0.f, 1.f) * weapSpread;

		Vector spreadView((cos(g_Math.RandomFloat(0.f, 2.f * M_PI)) * Inaccuracy) + (cos(g_Math.RandomFloat(0.f, 2.f * M_PI)) * Spread), (sin(g_Math.RandomFloat(0.f, 2.f * M_PI)) * Inaccuracy) + (sin(g_Math.RandomFloat(0.f, 2.f * M_PI)) * Spread), 0), direction;
		direction = Vector(forward.x + (spreadView.x * right.x) + (spreadView.y * up.x), forward.y + (spreadView.x * right.y) + (spreadView.y * up.y), forward.z + (spreadView.x * right.z) + (spreadView.y * up.z)).Normalize();

		Vector viewanglesSpread, viewForward;

		g_Math.VectorAngles(direction, up, viewanglesSpread);
		g_Math.NormalizeAngles(viewanglesSpread);

		g_Math.AngleVectors(viewanglesSpread, &viewForward);
		viewForward.NormalizeInPlace();

		viewForward = g::pLocalEntity->GetEyePosition() + (viewForward * pWeapon->GetCSWpnData()->range);

		C_Trace Trace;

		g_pTrace->ClipRayToEntity(C_Ray(g::pLocalEntity->GetEyePosition(), viewForward), mask_shot | contents_grate, pEnt, &Trace);

		if (Trace.m_pEnt == pEnt)
			Hits++;

		if (((Hits / Seeds) * 100.f) >= chance)
			return true;

		if ((Seeds - i + Hits) < neededHits)
			return false;
	}

	return false;
}

bool ShouldBaim(C_BaseEntity* pEnt) // probably dosnt make sense
{
	static float oldSimtime[65];
	static float storedSimtime[65];

	static float ShotTime[65];
	static float NextShotTime[65];
	static bool BaimShot[65];

	if (storedSimtime[pEnt->EntIndex()] != pEnt->GetSimulationTime())
	{
		oldSimtime[pEnt->EntIndex()] = storedSimtime[pEnt->EntIndex()];
		storedSimtime[pEnt->EntIndex()] = pEnt->GetSimulationTime();
	}

	float simDelta = storedSimtime[pEnt->EntIndex()] - oldSimtime[pEnt->EntIndex()];

	bool Shot = false;

	if (pEnt->GetActiveWeapon() && !pEnt->IsKnifeorNade())
	{
		if (ShotTime[pEnt->EntIndex()] != pEnt->GetActiveWeapon()->GetLastShotTime())
		{
			Shot = true;
			BaimShot[pEnt->EntIndex()] = false;
			ShotTime[pEnt->EntIndex()] = pEnt->GetActiveWeapon()->GetLastShotTime();
		}
		else
			Shot = false;
	}
	else
	{
		Shot = false;
		ShotTime[pEnt->EntIndex()] = 0.f;
	}

	if (Shot)
	{
		NextShotTime[pEnt->EntIndex()] = pEnt->GetSimulationTime() + pEnt->FireRate();

		if (simDelta >= pEnt->FireRate())
			BaimShot[pEnt->EntIndex()] = true;
	}

	if (BaimShot[pEnt->EntIndex()])
	{
		if (pEnt->GetSimulationTime() >= NextShotTime[pEnt->EntIndex()])
			BaimShot[pEnt->EntIndex()] = false;
	}

	auto pWeapon = g::pLocalEntity->GetActiveWeapon();

	std::vector<int> baim_hitboxes;


	baim_hitboxes.push_back((int)HITBOX_UPPER_CHEST);
	baim_hitboxes.push_back((int)HITBOX_LOWER_CHEST);
	baim_hitboxes.push_back((int)HITBOX_PELVIS);
	baim_hitboxes.push_back((int)HITGROUP_STOMACH);

	float Radius = 0;
	Radius *= (g_Menu.Config.BodyScale / 100.f);

	float Damage = 0;
	for (auto HitBoxID : baim_hitboxes)
	{
		Damage = g_Autowall.Damage(pEnt->GetHitboxPosition(HitBoxID, g_Aimbot.Matrix[pEnt->EntIndex()], &Radius));
	}

	if (g::pLocalEntity->GetVelocity().Length2D() > 125 && g_Menu.Config.forcebaim[2])
		return true;

	if (BaimShot[pEnt->EntIndex()] && g_Menu.Config.forcebaim[3])
		return true;

	if (!(pEnt->GetFlags() & FL_ONGROUND) && g_Menu.Config.forcebaim[0])
		return true;

	if (g_Menu.Config.forcebaim[1] && (pEnt->GetVelocity().Length2D() > 15 && pEnt->GetVelocity().Length2D() < 150))
		return true;

	if (pEnt->GetHealth() <= Damage && g_Menu.Config.forcebaim[4])
		return true;

	return false;
}

int Aimbot::GetBaimHitbox()
{
	if (g_Menu.Config.bodyhitbox == 0)
		return HITBOX_UPPER_CHEST;
	if (g_Menu.Config.bodyhitbox == 1)
		return HITBOX_LOWER_CHEST;
	if (g_Menu.Config.bodyhitbox == 2)
		return HITBOX_BELLY;
	if (g_Menu.Config.bodyhitbox == 3)
		return HITBOX_PELVIS;
	if (g_Menu.Config.bodyhitbox == 4)
		return HITBOX_THORAX;
}

std::vector<Vector> Aimbot::GetMultiplePointsForHitbox(C_BaseEntity* local, C_BaseEntity* entity, int iHitbox, matrix3x4_t BoneMatrix[128]) //stackhack
{
	auto VectorTransform_Wrapper = [](const Vector& in1, const matrix3x4_t& in2, Vector& out)
	{
		auto VectorTransform = [](const float* in1, const matrix3x4_t& in2, float* out)
		{
			auto DotProducts = [](const float* v1, const float* v2)
			{
				return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
			};
			out[0] = DotProducts(in1, in2[0]) + in2[0][3];
			out[1] = DotProducts(in1, in2[1]) + in2[1][3];
			out[2] = DotProducts(in1, in2[2]) + in2[2][3];
		};
		VectorTransform(&in1.x, in2, &out.x);
	};

	studiohdr_t* pStudioModel = g_pModelInfo->GetStudiomodel(entity->GetModel());
	mstudiohitboxset_t* set = pStudioModel->GetHitboxSet(0);
	mstudiobbox_t* hitbox = set->GetHitbox(iHitbox);

	std::vector<Vector> vecArray;

	Vector max;
	Vector min;
	VectorTransform_Wrapper(hitbox->max, BoneMatrix[hitbox->bone], max);
	VectorTransform_Wrapper(hitbox->min, BoneMatrix[hitbox->bone], min);

	auto center = (min + max) * 0.5f;

	Vector CurrentAngles = g_Math.CalcAngle(center, local->GetEyePosition());

	Vector Forward;
	g_Math.AngleVectors(CurrentAngles, &Forward);


	Vector right = Forward.Cross(Vector(0, 0, 1));
	Vector left = Vector(-right.x, -right.y, right.z);
	Vector top = Vector(0.f, 0.f, 1.f);
	Vector bot = Vector(0.f, 0.f, -1.f);
	Vector v1 = Vector(right.x, right.y, 0.7f)/*Vector(right.x - vars.ragebot.ps_x, right.y - vars.ragebot.ps_y, vars.ragebot.ps_z)*/;
	Vector v2 = Vector(-right.x, -right.y, 0.7f);
	Vector v3 = Vector(right.x / 2, right.y / 2, 0.6);
	Vector v4 = Vector(-right.x / 2, -right.y / 2, 0.6);

	Vector Right = Forward.Cross(Vector(0, 0, 1));

	Vector Top = Vector(0, 0, 1);
	Vector Bot = Vector(0, 0, -1);
	Vector Left = Vector(-Right.x, -Right.y, Right.z);
	if (g_Menu.Config.OptimizedAimbot)
	{
		if (g_Menu.Config.AdaptiveMulti) // ghetto fps fix
		{
			switch (iHitbox) {
			case HITBOX_HEAD:
			{
				for (auto i = 0; i < 4; ++i)
					vecArray.emplace_back(center);

				vecArray[1] += top * (hitbox->radius * g_Menu.Config.HeadScale / 100.f);
				vecArray[2] += right * (hitbox->radius * g_Menu.Config.HeadScale / 100.f);
				vecArray[3] += left * (hitbox->radius * g_Menu.Config.HeadScale / 100.f);
				break;
			}
			case HITBOX_BELLY:
			case HITBOX_PELVIS:
			case HITBOX_LOWER_CHEST:
			case HITBOX_UPPER_CHEST:
			case HITBOX_THORAX:
			{

				for (auto i = 0; i < 3; ++i)
					vecArray.emplace_back(center);


				vecArray[1] += Right * (hitbox->radius * g_Menu.Config.BodyScale / 100.f);
				vecArray[2] += Left * (hitbox->radius * g_Menu.Config.BodyScale / 100.f);
				break;
			}

			}
		}
		else
		{
			switch (iHitbox) {
			case HITBOX_HEAD:
			{
				for (auto i = 0; i < 7; ++i)
					vecArray.emplace_back(center);

				vecArray[1] += top * (hitbox->radius * g_Menu.Config.HeadScale / 100.f);
				vecArray[2] += right * (hitbox->radius * g_Menu.Config.HeadScale / 100.f);
				vecArray[3] += left * (hitbox->radius * g_Menu.Config.HeadScale / 100.f);
				vecArray[4] += bot * (hitbox->radius * g_Menu.Config.HeadScale / 100.f);
				vecArray[5] += v1 * (hitbox->radius * g_Menu.Config.HeadScale / 100.f);
				vecArray[6] += v2 * (hitbox->radius * g_Menu.Config.HeadScale / 100.f);
				break;
			}
			case HITBOX_BELLY:
			case HITBOX_PELVIS:
			case HITBOX_LOWER_CHEST:
			case HITBOX_UPPER_CHEST:
			case HITBOX_THORAX:
			{

				for (auto i = 0; i < 3; ++i)
					vecArray.emplace_back(center);


				vecArray[1] += Right * (hitbox->radius * g_Menu.Config.BodyScale / 100.f);
				vecArray[2] += Left * (hitbox->radius * g_Menu.Config.BodyScale / 100.f);
				break;
			}

			}
		}
	}
	else
	{
		switch (iHitbox) {
		case HITBOX_HEAD:
		{
			for (auto i = 0; i < 7; ++i)
				vecArray.emplace_back(center);

			vecArray[1] += top * (hitbox->radius * g_Menu.Config.HeadScale / 100.f);
			vecArray[2] += right * (hitbox->radius * g_Menu.Config.HeadScale / 100.f);
			vecArray[3] += left * (hitbox->radius * g_Menu.Config.HeadScale / 100.f);
			vecArray[4] += bot * (hitbox->radius * g_Menu.Config.HeadScale / 100.f);
			vecArray[5] += v1 * (hitbox->radius * g_Menu.Config.HeadScale / 100.f);
			vecArray[6] += v2 * (hitbox->radius * g_Menu.Config.HeadScale / 100.f);
			break;
		}
		case HITBOX_BELLY:
		case HITBOX_PELVIS:
		case HITBOX_LOWER_CHEST:
		case HITBOX_UPPER_CHEST:
		case HITBOX_THORAX:
		{

			for (auto i = 0; i < 3; ++i)
				vecArray.emplace_back(center);


			vecArray[1] += Right * (hitbox->radius * g_Menu.Config.BodyScale / 100.f);
			vecArray[2] += Left * (hitbox->radius * g_Menu.Config.BodyScale / 100.f);
			break;
		}

		}
	}

	return vecArray;
}

static int stfu;

static bool Zeus;

Vector Aimbot::Hitscan(C_BaseEntity * pEnt) // supremeemmemememememe
{
	//static int maxDamage;

	Baim[pEnt->EntIndex()] = ShouldBaim(pEnt);

	if (GetAsyncKeyState(g_Menu.Config.BaimKey))
		Baim[pEnt->EntIndex()] = true;

	if (g::pLocalEntity->zeus())
		Zeus = true;
	else
		Zeus = false;

	if (g_Menu.Config.BaimAfterEnable)
		if (shotsMissed >= g_Menu.Config.BaimSlider)
			Baim[pEnt->EntIndex()] = true;

	if (BaimIfLethal(pEnt) && g_Menu.Config.BaimLethal)
		Baim[pEnt->EntIndex()] = true;

	if (g_Menu.Config.forcebaim[pEnt->EntIndex()])
		Baim[pEnt->EntIndex()] = true;
	std::vector<int> Scan;

	if (!Baim[pEnt->EntIndex()] && g_Menu.Config.Head && !Zeus && !g_Menu.Config.forcebaim[pEnt->EntIndex()] && !GetAsyncKeyState(g_Menu.Config.BaimKey)) {
		Scan.push_back(HITBOX_HEAD);
		g_Menu.Config.espbaim[pEnt->EntIndex()] = true;
	}
	else {
		g_Menu.Config.espbaim[pEnt->EntIndex()] = false;
	}

	if (g_Menu.Config.Chest) {
		Scan.push_back(HITBOX_THORAX);
		Scan.push_back(HITBOX_LOWER_CHEST);
		Scan.push_back(HITBOX_UPPER_CHEST);
	}
	if (g_Menu.Config.Pelvis) {
		Scan.push_back(HITBOX_PELVIS);
		Scan.push_back(HITBOX_BELLY);
	}
	if (g_Menu.Config.Legs && !Zeus) {
		Scan.push_back(HITBOX_LEFT_FOOT);
		Scan.push_back(HITBOX_RIGHT_FOOT);
		Scan.push_back(HITBOX_LEFT_THIGH);
		Scan.push_back(HITBOX_RIGHT_THIGH);
		Scan.push_back(HITBOX_RIGHT_CALF);
		Scan.push_back(HITBOX_LEFT_CALF);
	}
	if (g_Menu.Config.Arms && !Zeus) {
		Scan.push_back(HITBOX_LEFT_UPPER_ARM);
		Scan.push_back(HITBOX_RIGHT_UPPER_ARM);
	}

	int bestHitbox = -1;
	Vector vector_best_point = Vector(0, 0, 0);
	Vector Hitbox;
	int bestHitboxint = 0;
	Vector BodyHitbox;
	float damageBody;
	Vector PriorHitbox;
	float priordamage;
	PlayerRecords pPlayerEntityRecord = g_LagComp.PlayerRecord[pEnt->EntIndex()].at(0);

	Backtrack[pEnt->EntIndex()] = false;
	ShotBacktrack[pEnt->EntIndex()] = false;
	int maxDamage = SynchronizeMinDamage();
	matrix3x4_t matrix[128];
	if (!pEnt->SetupBones(matrix, 128, 256, 0)) return Vector(0, 0, 0);

	if (g_Menu.Config.MultiPoint)
	{
		for (auto HitBoxID : Scan)
		{
			for (auto point : GetMultiplePointsForHitbox(g::pLocalEntity, pEnt, HitBoxID, matrix))
			{
				BodyHitbox = pEnt->GetHitboxPosition(GetBaimHitbox(), matrix);
				damageBody = g_Autowall.Damage(BodyHitbox);
				auto HeadHitbox = pEnt->GetHitboxPosition(HITBOX_HEAD, matrix);
				float HeadDamage = g_Autowall.Damage(HeadHitbox);
				auto Pelvis = pEnt->GetHitboxPosition(HITBOX_PELVIS, matrix);
				float BodyDamageKey = g_Autowall.Damage(Pelvis);
				priordamage = g_Autowall.Damage(PriorHitbox);
				int damage = g_Autowall.Damage(point);
				if (!GetAsyncKeyState(g_Menu.Config.BaimKey)) {

					if (g::enemyshot[pEnt->EntIndex()] && HeadDamage)
					{
						for (auto shotpoint : GetMultiplePointsForHitbox(g::pLocalEntity, pEnt, HITBOX_HEAD, matrix))
							return shotpoint;
					}
					else
					{
						if (damageBody > pEnt->GetHealth() + 20 && g_Menu.Config.adaptivebody)
						{
							for (auto bodypoint : GetMultiplePointsForHitbox(g::pLocalEntity, pEnt, GetBaimHitbox(), matrix))
								return bodypoint;
						}
						else if (damage > maxDamage && damage)
						{
							bestHitbox = HitBoxID;
							maxDamage = damage;
							vector_best_point = point;

							if (maxDamage >= pEnt->GetHealth())
								return vector_best_point;
						}
					}

				}
				else
				{
					if (g_Menu.Config.adaptivebody && damageBody)
					{
						for (auto bodypoint : GetMultiplePointsForHitbox(g::pLocalEntity, pEnt, GetBaimHitbox(), matrix))
							return bodypoint;
					}
					else if (BodyDamageKey)
					{
						for (auto bodypoint : GetMultiplePointsForHitbox(g::pLocalEntity, pEnt, HITBOX_PELVIS, matrix))
							return bodypoint;
					}
				}
			}


		}
	}
	else
	{
		for (auto HitBoxID : Scan)
		{

			BodyHitbox = pEnt->GetHitboxPosition(GetBaimHitbox(), matrix);
			damageBody = g_Autowall.Damage(BodyHitbox);
			auto HeadHitbox = pEnt->GetHitboxPosition(HITBOX_HEAD, matrix);
			float HeadDamage = g_Autowall.Damage(HeadHitbox);
			auto Pelvis = pEnt->GetHitboxPosition(HITBOX_PELVIS, matrix);
			float BodyDamageKey = g_Autowall.Damage(Pelvis);
			priordamage = g_Autowall.Damage(PriorHitbox);
			auto point = pEnt->GetHitboxPosition(HitBoxID, matrix);
			int damage = g_Autowall.Damage(point);

			if (!GetAsyncKeyState(g_Menu.Config.BaimKey)) {

				if (g::enemyshot[pEnt->EntIndex()] && HeadDamage)
				{
					for (auto shotpoint : GetMultiplePointsForHitbox(g::pLocalEntity, pEnt, HITBOX_HEAD, matrix))
						return shotpoint;
				}
				else
				{
					if (damageBody > pEnt->GetHealth() + 20 && g_Menu.Config.adaptivebody)
					{
						for (auto bodypoint : GetMultiplePointsForHitbox(g::pLocalEntity, pEnt, GetBaimHitbox(), matrix))
							return bodypoint;
					}
					else if (damage > maxDamage && damage)
					{
						bestHitbox = HitBoxID;
						maxDamage = damage;
						vector_best_point = point;

						if (maxDamage >= pEnt->GetHealth())
							return vector_best_point;
					}
				}

			}
			else
			{
				if (g_Menu.Config.adaptivebody && damageBody)
				{
					for (auto bodypoint : GetMultiplePointsForHitbox(g::pLocalEntity, pEnt, GetBaimHitbox(), matrix))
						return bodypoint;
				}
				else if (BodyDamageKey)
				{
					for (auto bodypoint : GetMultiplePointsForHitbox(g::pLocalEntity, pEnt, HITBOX_PELVIS, matrix))
						return bodypoint;
				}
			}

		}
	}
	return vector_best_point;
}

void Aimbot::OnCreateMove()
{
	if (!g_pEngine->IsInGame())
		return;

	Vector Aimpoint = { 0,0,0 };
	C_BaseEntity* Target = nullptr;
	targetID = 0;

	int tempDmg = 0;
	static bool shot = false;
	CanShootWithCurrentHitchanceAndValidTarget = false;

	for (int i = 1; i <= g_pEngine->GetMaxClients(); ++i)
	{
		C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(i);

		if (!pPlayerEntity
			|| !pPlayerEntity->IsAlive()
			|| pPlayerEntity->IsDormant()
			|| pPlayerEntity->IsImmune())
		{
			g_LagComp.ClearRecords(i);
			continue;
		}

		g_LagComp.StoreRecord(pPlayerEntity);

		if (pPlayerEntity == g::pLocalEntity || pPlayerEntity->GetTeam() == g::pLocalEntity->GetTeam())
			continue;

		g::EnemyEyeAngs[i] = pPlayerEntity->GetEyeAngles();

		if (g_LagComp.PlayerRecord[i].size() == 0 || !g::pLocalEntity->IsAlive() || !g_Menu.Config.Aimbot || g_Menu.Config.LegitBacktrack)
			continue;

		if (!g::pLocalEntity->GetActiveWeapon() || g::pLocalEntity->IsKnifeorNade())
			continue;

		bestEntDmg = 0;

		Vector Hitbox = Hitscan(pPlayerEntity);

		if (Hitbox != Vector(0, 0, 0) && tempDmg <= bestEntDmg)
		{
			Aimpoint = Hitbox;
			Target = pPlayerEntity;
			targetID = Target->EntIndex();
			tempDmg = bestEntDmg;
		}
	}

	if (!g::pLocalEntity->IsAlive())
	{
		shot = false;
		return;
	}

	if (!g::pLocalEntity->GetActiveWeapon() || g::pLocalEntity->IsKnifeorNade())
	{
		shot = false;
		return;
	}

	if (shot)
	{
		if (g_Menu.Config.OnShotAA) // ik it dosnt realy fix much just makes ur pitch go down faster
		{
			g::bSendPacket = true;
			g_AntiAim.OnCreateMove();
		}
		if (g_Menu.Config.OnShotAA) {
			g::bSendPacket = true;
			g::pCmd->viewangles.x = 120 - g::pCmd->viewangles.x;
			g::pCmd->viewangles.y = g::pCmd->viewangles.y + 120;
		}
		shot = false;
	}

	float flServerTime = g::pLocalEntity->GetTickBase() * g_pGlobalVars->intervalPerTick;
	bool canShoot = (g::pLocalEntity->GetActiveWeapon()->GetNextPrimaryAttack() <= flServerTime && g::pLocalEntity->GetActiveWeapon()->GetAmmo() > 0);
	if (Target)
	{
		g::TargetIndex = targetID;

		float SimulationTime = 0.f;

		SimulationTime = g_LagComp.PlayerRecord[targetID].at(g_LagComp.PlayerRecord[targetID].size() - 1).SimTime;

		Vector Angle = g_Math.CalcAngle(g::pLocalEntity->GetEyePosition(), Aimpoint);

		static int MinimumVelocity = 0;

		if (g::pLocalEntity->GetVelocity().Length() >= (g::pLocalEntity->GetActiveWeapon()->GetCSWpnData()->max_speed_alt * .34f) - 5 && !GetAsyncKeyState(VK_SPACE))
			Autostop();

		if (g_Menu.Config.Autoscope)
			autoscope();

		auto me = g::pLocalEntity;
		auto cmd = g::pCmd;

		/*if (g_Menu.Config.FixSpreed = true && g::pLocalEntity->IsAlive()) 
		{
			if ((g::pLocalEntity->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_SSG08 || g::pLocalEntity->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_REVOLVER))
			{
				g_Menu.Config.Head = true;
				g_Menu.Config.Chest = true;
				g_Menu.Config.Pelvis = true;
				g_Menu.Config.Arms = false;
				g_Menu.Config.Legs = false;
			}
			else if ((g::pLocalEntity->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_SCAR20 || g::pLocalEntity->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_G3SG1))
			{
				g_Menu.Config.Head = true;
				g_Menu.Config.Chest = true;
				g_Menu.Config.Pelvis = true;
				g_Menu.Config.Arms = false;
				g_Menu.Config.Legs = true;
			}
			else
			{
				g_Menu.Config.Head = true;
				g_Menu.Config.Chest = true;
				g_Menu.Config.Pelvis = true;
				g_Menu.Config.Arms = false;
				g_Menu.Config.Legs = false;
			}
		}*/

		if (canShoot && HitChance(Target, g::pLocalEntity->GetActiveWeapon(), Angle, Aimpoint, SynchronizeHitchance()))
		{
			CanShootWithCurrentHitchanceAndValidTarget = true;
			if (g::pCmd->buttons & IN_ATTACK)
				return;

			if (!Backtrack[targetID] && !ShotBacktrack[targetID])
				g::Shot[targetID] = true;

			g::bSendPacket = true;
			shot = true;

			g::pCmd->viewangles = Angle;

			g::pCmd->buttons |= IN_ATTACK;

			g::pCmd->tick_count = TIME_TO_TICKS(SimulationTime + g_LagComp.LerpTime());
		}
	}
}