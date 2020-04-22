#include "GlobalVars.h"

namespace g
{
    CUserCmd*      pCmd         = nullptr;
    C_BaseEntity*  pLocalEntity = nullptr;
    std::uintptr_t uRandomSeed  = NULL;
	Vector         OriginalView;
	bool		   IsHit;
	bool           should_setup_local_bones = false;
	bool           bSendPacket  = true;
	bool		   LagPeek      = false;
	matrix3x4_t         fakematrix[128];
	int            TargetIndex  = -1;
	Vector         EnemyEyeAngs[65];
	Vector         AimbotHitbox[65][28];
	int			   bombsite;
	Vector         RealAngle;
	Vector         FakeAngle;
	Vector         m_angFakeAnglesAbs;
	Vector		   pos;
	bool           Shot[65];
	bool           BreakingLC[65];
	bool           Hit[65];
	int			   flHurtTime;
	bool           enemyshot[65];
	int			   flHurtTick;
	int            MissedShots[65];
	bool		   MissedShot;
	bool		   plist_player[65];
	bool           FreezeTime = false;
	int            hits = 0;
	int            kills = 0;
	float          killtime = 0.0f;
	int			   plist_pitch[65];
	int			   shots;
	int			   resolvetype;
	int			   revolvertime;
	int            globalAlpha = 0;
	bool		   nightmodeupdated;
	bool           lcBroken;
	bool           fakeduck;
	bool		   isthirdperson;
	float          w2s_matrix[4][4];
	int m_nTickbaseShift;

	DWORD CourierNew;
	DWORD WeaponFont;
	DWORD memefont;
	DWORD KillCounter;
	DWORD supfont;
	DWORD Indicators;
	DWORD smallestpixel;
	DWORD Verdana;
	DWORD NiggaPee;
	DWORD SauIsAFatRetard;
	DWORD IndicatorFont;
	DWORD FlagFont;
	bool bFirstSendMovePack;
}
