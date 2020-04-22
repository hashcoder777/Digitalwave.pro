#pragma once
#include "..\..\Utils\GlobalVars.h"
#include "..\..\SDK\CGlobalVarsBase.h"
#include "..\..\Hooks.h"

class AntiAim
{
public:
	bool invert;
	void FakeDuck(CUserCmd* cmd);
	void SlowWalk(CUserCmd* cmd);
	void Yaw();
	void Pitch();
	void PitchSwitch();
	bool Inverter;
	void niggacheese();
	bool invertsyka;
	bool GoalChams;
	void LegitOnCreateMove();
	void OnCreateMove();
	
private:
	
};
extern AntiAim g_AntiAim;