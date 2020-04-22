#include <thread>
#include "Hooks.h"
#include "Utils\Utils.h"
#include "Features\Features.h"
#include "Menu\Menu.h"
#include "SDK\Hitboxes.h"
#include "libloaderapi.h"
#include "SDK/texture_group_names.h"
#include "SDK/IVRenderBeam.h"
#include "SDK/CHudChat.h"
#include "gui/zgui.h"
#include "Utils/GlowAutism.h"
#include "Utils/XorStr.h"
#include "Features/Misc/hitsounds.h"


class Protobuffs;
extern Protobuffs* zwrite;


std::vector<trace_info> trace_logs;

Misc		 g_Misc;
Hooks		 g_Hooks;
Event		 g_Event;
BulletEvent	 g_bEvent;
EventRStart  g_bRoundPrestart;
WeaponFire   g_WeaponFire;
FootstepEvent g_FootStep;
Player_Death g_PlayerDeath;

void Warning(const char* msg, ...) // wintergang https://www.unknowncheats.me/forum/1923881-post1.html
{
	if (msg == nullptr)
		return;
	typedef void(__cdecl* MsgFn)(const char* msg, va_list);
	static MsgFn fn = (MsgFn)GetProcAddress(GetModuleHandle("tier0.dll"), "Warning");
	char buffer[989];
	va_list list;
	va_start(list, msg);
	vsprintf(buffer, msg, list);
	va_end(list);
	fn(buffer, list);
}

void __declspec(naked) LagFix() //pasted
{
	__asm {
		push edi
		xchg dword ptr[esp], edi
		push eax
		mov eax, 77
		mov eax, dword ptr[esp]
		add esp, 4
		pop edi

		cmp esp, 0
		jne fixentity

		_emit 0x88
		_emit 0xFF

		invlpg dword ptr[eax]

		int 2

		fixentity:
		sub esp, 4
			mov dword ptr[esp], ebp

			call cleanup

			pop ebp

			ret

			cleanup :

		ret
	}
}

void line(int x, int y, int x2, int y2, zgui::color c) noexcept { Color cc = Color(c.r, c.g, c.b, c.a); g_pSurface->Line(x, y, x2, y2, cc); }

void rect(int x, int y, int x2, int y2, zgui::color c) noexcept { Color cc = Color(c.r, c.g, c.b, c.a); g_pSurface->OutlinedRect(x, y, x2, y2, cc); }

void filled_rect(int x, int y, int x2, int y2, zgui::color c) noexcept { Color cc = Color(c.r, c.g, c.b, c.a); g_pSurface->FilledRect(x, y, x2, y2, cc); }

void text(int x, int y, zgui::color c, int font, bool center, const char* text) noexcept { Color cc = Color(c.r, c.g, c.b, c.a); g_pSurface->DrawT(x,y,cc,font,center,text); }

void get_text_size(unsigned long font, const char* text, int& wide, int& tall) noexcept 
{ 

	std::string str(text);
	std::wstring wstr(str.begin(), str.end());


	g_pSurface->GetTextSize(font, wstr.c_str(),wide,tall);
}

float get_frametime() noexcept { return g_pGlobalVars->frametime; }

void Hooks::Init()
{
    // Get window handle
    while (!(g_Hooks.hCSGOWindow = FindWindowA("Valve001", nullptr)))
    {
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(50ms);
    }

    interfaces::Init();                         // Get interfaces
    g_pNetvars = std::make_unique<NetvarTree>();// Get netvars after getting interfaces as we use them

    if (g_Hooks.hCSGOWindow)        // Hook WNDProc to capture mouse / keyboard input
        g_Hooks.pOriginalWNDProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_Hooks.hCSGOWindow, GWLP_WNDPROC,
                                                                              reinterpret_cast<LONG_PTR>(g_Hooks.WndProc)));

    // VMTHooks
	g_Hooks.pClientHook     = std::make_unique<VMTHook>(g_pClientDll);
    g_Hooks.pClientModeHook = std::make_unique<VMTHook>(g_pClientMode);
    g_Hooks.pSurfaceHook	= std::make_unique<VMTHook>(g_pSurface);
	g_Hooks.pPanelHook		= std::make_unique<VMTHook>(g_pPanel);
	g_Hooks.pModelHook      = std::make_unique<VMTHook>(g_pModelRender);
	g_Hooks.pRenderViewHook = std::make_unique<VMTHook>(g_pRenderView);

    // Hook the table functions
	g_Hooks.pClientHook    ->Hook(vtable_indexes::frameStage, Hooks::FrameStageNotify);
    g_Hooks.pClientModeHook->Hook(vtable_indexes::createMove, Hooks::CreateMove);
	//g_Hooks.pClientModeHook->Hook(vtable_indexes::do_post_screen_effects, Hooks::hkDoPostScreenEffects);
	g_Hooks.pClientModeHook->Hook(vtable_indexes::view, Hooks::OverrideView);
    g_Hooks.pSurfaceHook   ->Hook(vtable_indexes::lockCursor, Hooks::LockCursor);
	g_Hooks.pPanelHook	   ->Hook(vtable_indexes::paint, Hooks::PaintTraverse);
	g_Hooks.pModelHook	   ->Hook(vtable_indexes::dme, Hooks::DrawModelExecute);
	g_Hooks.pRenderViewHook->Hook(vtable_indexes::sceneEnd, Hooks::SceneEnd);
	g_Hooks.pClientModeHook->Hook(vtable_indexes::do_post_screen_effects, Hooks::hkDoPostScreenEffects);

	zgui::functions.draw_line = line;
	zgui::functions.draw_rect = rect;
	zgui::functions.draw_filled_rect = filled_rect;
	zgui::functions.draw_text = text;
	zgui::functions.get_text_size = get_text_size;
	zgui::functions.get_frametime = get_frametime;

	g_Event.Init();
	g_bEvent.Init();
	g_bRoundPrestart.Init();
	g_FootStep.Init();
	g_PlayerDeath.Init();
	g_WeaponFire.Init();

	g::WeaponFont = g_pSurface->FontCreate();
	g_pSurface->SetFontGlyphSet(g::WeaponFont, "Visitor TT2 -BRK-", 8, 16, 0, 0, FONTFLAG_OUTLINE); //300 
	g::CourierNew = g_pSurface->FontCreate();
	g_pSurface->SetFontGlyphSet(g::CourierNew, "Verdana", 14, 28, 0, 0, FONTFLAG_DROPSHADOW); //300 
	g::Verdana = g_pSurface->FontCreate();
	g_pSurface->SetFontGlyphSet(g::Verdana, "Verdana", 12, FW_BOLD, 0, 0, FONTFLAG_OUTLINE); //700 
	g::NiggaPee = g_pSurface->FontCreate();
	g_pSurface->SetFontGlyphSet(g::NiggaPee, "Visitor TT2 -BRK-", 9, 500, 0, 0, FONTFLAG_OUTLINE);
	g::SauIsAFatRetard = g_pSurface->FontCreate();
	g_pSurface->SetFontGlyphSet(g::SauIsAFatRetard, "Small Fonts", 9, 500, 0, 0, FONTFLAG_OUTLINE);
	g::IndicatorFont = g_pSurface->FontCreate();
	g_pSurface->SetFontGlyphSet(g::IndicatorFont, "Verdana", 27, FW_EXTRABOLD, 0, 0, FONTFLAG_DROPSHADOW);
	g::FlagFont = g_pSurface->FontCreate();
	g_pSurface->SetFontGlyphSet(g::FlagFont, "Verdana", 10, FW_EXTRABOLD, 0, 0, FONTFLAG_OUTLINE);
	g::memefont = g_pSurface->FontCreate();
	g_pSurface->SetFontGlyphSet(g::KillCounter, "Verdana", 24, 700, 0, 0, FONTFLAG_DROPSHADOW);
	g::KillCounter = g_pSurface->FontCreate();
	g_pSurface->SetFontGlyphSet(g::Indicators, "Verdana", 30, 10000, 0, 0, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
	g::Indicators = g_pSurface->FontCreate();
	g_pSurface->SetFontGlyphSet(g::memefont, "Verdana", 12, FW_THIN, 0, 0, FONTFLAG_OUTLINE);
	g::supfont = g_pSurface->FontCreate();
	g_pSurface->SetFontGlyphSet(g::supfont, "Verdana", 15, 15, 0, 0, FONTFLAG_NONE);
	g::smallestpixel = g_pSurface->FontCreate();
	g_pSurface->SetFontGlyphSet(g::smallestpixel, "Visitor TT2 -BRK-", 14, 500, 0, 0, FONTFLAG_OUTLINE);
	

	//g_pEngine->ExecuteClientCmd("clear");
	//Warning("	\n\n\n\n\n\n");
	//Warning("https://vk.com/exsapmle \n");
}

void Hooks::Restore()
{
	Utils::Log("Unhooking in progress...");
    {   // Unhook every function we hooked and restore original one
		g_Hooks.pClientHook->Unhook(vtable_indexes::frameStage);
        g_Hooks.pClientModeHook->Unhook(vtable_indexes::createMove);
		g_Hooks.pClientModeHook->Unhook(vtable_indexes::view);
        g_Hooks.pSurfaceHook->Unhook(vtable_indexes::lockCursor);
		g_Hooks.pPanelHook->Unhook(vtable_indexes::paint);
		g_Hooks.pModelHook->Unhook(vtable_indexes::dme);

        SetWindowLongPtr(g_Hooks.hCSGOWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_Hooks.pOriginalWNDProc));
        g_pNetvars.reset();   /* Need to reset by-hand, global pointer so doesnt go out-of-scope */
    }
    Utils::Log("Unhooking succeded!");
}

void Hooks::HookPlayers()
{
	static bool Init[65];
	static bool Hooked[65];

	for (int i = 1; i < g_pEngine->GetMaxClients(); ++i)
	{
		C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(i);

		if (!pPlayerEntity
			|| !pPlayerEntity->IsAlive()
			|| pPlayerEntity->IsDormant())
		{
			if (Hooked[i])
				g_Hooks.pPlayerHook[i]->Unhook(vtable_indexes::extraBonePro);

			Hooked[i] = false;
			continue;
		}

		if (!Init[i])
		{
			g_Hooks.pPlayerHook[i] = std::make_unique<ShadowVTManager>();
			Init[i] = true;
		}
		
		if (Hooked[i])
			g_Hooks.pPlayerHook[i]->Unhook(vtable_indexes::extraBonePro);

		if (!Hooked[i])
		{
			g_Hooks.pPlayerHook[i]->Setup(pPlayerEntity);
			g_Hooks.pPlayerHook[i]->Hook(vtable_indexes::extraBonePro, Hooks::DoExtraBonesProcessing);

			Hooked[i] = true;
		}
	}
}

template<class T, class U>
T fine(T in, U low, U high)
{
	if (in <= low)
		return low;

	if (in >= high)
		return high;

	return in;
}



bool __fastcall Hooks::CreateMove(IClientMode* thisptr, void* edx, float sample_frametime, CUserCmd* pCmd)
{
	// Call original createmove before we start screwing with it
	static auto oCreateMove = g_Hooks.pClientModeHook->GetOriginal<CreateMove_t>(vtable_indexes::createMove);
	oCreateMove(thisptr, edx, sample_frametime, pCmd);

    if (!pCmd || !pCmd->command_number)
		return oCreateMove;
    // Get globals
    g::pCmd         = pCmd;
    g::pLocalEntity = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
	g::bSendPacket  = true;
    if (!g::pLocalEntity)
		return oCreateMove;

	uintptr_t *framePtr;
	__asm mov framePtr, ebp;

	g::OriginalView = g::pCmd->viewangles;

	g_Aimbot.GetTickbase(g::pCmd);

//	HookPlayers();

    g_Misc.OnCreateMove();
	g_Resolver.OnCreateMove();
	
    engine_prediction::RunEnginePred();

	static int lastDoubleTapInTickcount = 2;

	static int since_use = 2;
	static int delay_shot = 0.2;
	static int shoot_state = 1;
	static int state_delay = 0.02;

	if (g_Menu.Config.doubletap)
	{

		if (delay_shot != 0.02 && shoot_state == 1)
		{
			delay_shot--;
			g::pCmd->buttons &= ~IN_ATTACK;
			since_use = 0;
			if (delay_shot == 0)
			{
				shoot_state = 2;
			}
		}
		if (g::pCmd->buttons & IN_ATTACK /*&& c_config::get().dt_hitchance != 0 && c_config::get().dt_delayshot != 0 && c_config::get().state_delay != 0*/)
		{
			if (since_use++ < 3) {
				g::m_nTickbaseShift = 40;
				delay_shot = 0.02;
				shoot_state = 1;
				state_delay = 0.02;
			}
		}
		if (shoot_state == 2)
		{
			state_delay--;
			if (shoot_state == 1)
				g::pCmd->buttons |= IN_ATTACK;
			if (state_delay == 0.01)
				shoot_state = 2;
		}
		if (state_delay <= 0)
			state_delay = 0;
		if (delay_shot <= 0)
			delay_shot = 0;
	}

	if (g_Menu.Config.PredictFakeLag)
	{
		for (int i = 1; i < g_pGlobalVars->maxClients; i++) {
			C_BaseEntity* e = g_pEntityList->GetClientEntity(i);
			if (!e
				|| !e->IsAlive()
				|| e->IsDormant()) {
				continue;
			}

			float simtime_delta = e->GetSimulationTime() - e->GetOldSimTime();
			int choked_ticks = simtime_delta;
			Vector lastOrig;

			if (lastOrig.Length() != e->GetOrigin().Length())
				lastOrig = e->GetOrigin();

			float delta_distance = (e->GetOrigin() - lastOrig).LengthSqr();
			if (delta_distance > 4096.f) {
				Vector velocity_per_tick = e->GetVelocity() * g_pGlobalVars->intervalPerTick;
				auto new_origin = e->GetOrigin() + (velocity_per_tick * choked_ticks);
				e->SetOrigin(new_origin);
			}
		}
	}

	if (g_Menu.Config.Clantag)
		g_Misc.clan_tag();

	g_legitbot.run(g::pCmd);

	g_Legitbot.CreateMove(g::pCmd);

	g_AntiAim.niggacheese();

//	g_Misc.LinearExtrapolations();

	g_AntiAim.OnCreateMove();

	LagFix(); // pasted xd

	g_Aimbot.OnCreateMove();

	g_Misc.NoRecoil(pCmd);

    engine_prediction::EndEnginePred();

	g_Misc.MovementFix(g::OriginalView);

	g_Math.Clamp(g::pCmd->viewangles);

	if (!g::bSendPacket)
		g::RealAngle = g::pCmd->viewangles;
	else
		g::FakeAngle = g::pCmd->viewangles;

	*(bool*)(*framePtr - 0x1C) = g::bSendPacket;

	g::pCmd->buttons |= IN_BULLRUSH; // hehe

    return false;
}


/*void world_modulation() 
{ 

	float nightamount = g_Menu.Config.NightModeSlider / 100.f;


	for (auto i = g_pMaterialSys->FirstMaterial(); i != g_pMaterialSys->InvalidMaterial(); i = g_pMaterialSys->NextMaterial(i)) {

		IMaterial* pMaterial = g_pMaterialSys->GetMaterial(i);

		if (!pMaterial || pMaterial->IsErrorMaterial())
			continue;

		if (g_Menu.Config.NightMode) {

			ConVar* r_DrawSpecificStaticProp = g_pCvar->FindVar("r_DrawSpecificStaticProp");
			*(int*)((DWORD)& r_DrawSpecificStaticProp->fnChangeCallback + 0xC) = 0; // ew
			r_DrawSpecificStaticProp->SetValue(0);

			if (strstr(pMaterial->GetTextureGroupName(), "World") || strstr(pMaterial->GetTextureGroupName(), "StaticProp")) {

				if (g_Menu.Config.NightModeSlider < 100) {
					pMaterial->ColorModulate(nightamount, nightamount, nightamount);
				}
			}

			if (strstr(pMaterial->GetTextureGroupName(), "SkyBox")) {

				pMaterial->ColorModulate(g_Menu.Config.SkyboxColor.red / 255.f, g_Menu.Config.SkyboxColor.green / 255.f, g_Menu.Config.SkyboxColor.blue / 255.f);

			}

			if (strstr(pMaterial->GetTextureGroupName(), "StaticProp textures")) {

				pMaterial->AlphaModulate(g_Menu.Config.AsusProps / 100.f);

			}

		}

	}
}*/


void night_mode()
{
	static bool OldNightmode;

	int OldNightmodeValue = g_Menu.Config.NightModeSlider;

	float nightamount = g_Menu.Config.NightModeSlider / 100.f;

	if (OldNightmode != g_Menu.Config.NightMode || OldNightmodeValue != g_Menu.Config.NightModeSlider)
	{
		ConVar* r_DrawSpecificStaticProp = g_pCvar->FindVar("r_DrawSpecificStaticProp");
		*(int*)((DWORD)& r_DrawSpecificStaticProp->fnChangeCallback + 0xC) = 0; // ew
		r_DrawSpecificStaticProp->SetValue(0);

		for (auto i = g_pMaterialSys->FirstMaterial(); i != g_pMaterialSys->InvalidMaterial(); i = g_pMaterialSys->NextMaterial(i))
		{
			IMaterial* pMaterial = g_pMaterialSys->GetMaterial(i);

			if (!pMaterial)
				continue;

			if (strstr(pMaterial->GetTextureGroupName(), "World") || strstr(pMaterial->GetTextureGroupName(), "StaticProp"))
			{
				if (g_Menu.Config.NightMode)
				{
					if (strstr(pMaterial->GetTextureGroupName(), "StaticProp"))
						pMaterial->ColorModulate(nightamount, nightamount, nightamount);
					else
						pMaterial->ColorModulate(nightamount, nightamount, nightamount);
				}
				else
					pMaterial->ColorModulate(1.0f, 1.0f, 1.0f);
			}

			if (strstr(pMaterial->GetTextureGroupName(), "SkyBox")) {
				if (g_Menu.Config.NightMode)
				{

					pMaterial->ColorModulate(g_Menu.Config.SkyboxColor.r / 255.f, g_Menu.Config.SkyboxColor.g / 255.f, g_Menu.Config.SkyboxColor.b / 255.f);
				}
			}
			

			//if (g_Menu.Config.AsusProps < 100) {

				//if (strstr(pMaterial->GetTextureGroupName(), "StaticProp textures")) {
				//	if (g_Menu.Config.NightMode)
				//	{
				//		pMaterial->AlphaModulate(g_Menu.Config.AsusProps / 100.f);
				//	}
				//}
			//}

		}

		OldNightmode = g_Menu.Config.NightMode;
	}
}

inline void MakeDefNormal(GlowObjectDefinition_t* def, Color color)
{
	def->m_bPulsatingChams = false;
	def->m_flGlowAlpha = color.alpha / 255.f;
	def->m_flRed = color.red / 255.f;
	def->m_flGreen = color.green / 255.f;
	def->m_flBlue = color.blue / 255.f;
	def->m_bRenderWhenOccluded = true;
	def->m_bRenderWhenUnoccluded = false;
}

int fps, cFps; 
clock_t lastClock = 0;

C_BaseEntity* UTIL_PlayerByIndex(int index)
{
	typedef C_BaseEntity*(__fastcall* PlayerByIndex)(int);
	static PlayerByIndex UTIL_PlayerByIndex = (PlayerByIndex)Utils::FindSignature("server.dll", "85 C9 7E 2A A1");

	if (!UTIL_PlayerByIndex)
		return false;

	return UTIL_PlayerByIndex(index);
}

float GetPulse()
{
	float alpha = sin((g_pGlobalVars->realtime / 0.75) * 4 + 2) * 127 + 128;
	return alpha;
}

void __fastcall Hooks::SceneEnd(void *ecx, void *edx)
{
	static auto oSceneEnd = g_Hooks.pRenderViewHook->GetOriginal<SceneEnd_t>(vtable_indexes::sceneEnd);
	oSceneEnd(ecx, edx);

	static int alpha;
	static int alpha2;
	static int timer;
	C_BaseEntity* ent;

	static int oldmaterial = 0;
	static IMaterial* Material = nullptr;
	static bool ResetMaterial = false;

	if (!g::pLocalEntity || !g_pEngine->IsInGame() || !g_pEngine->IsConnected() || !g_Menu.Config.Chams)
	{
		Material = nullptr;
		return;
	}

	if (g_Menu.Config.ChamsMaterial == 0) {
		Material = g_pMaterialSys->FindMaterial("vitality_metallic", "Model textures");
	}
	else if (g_Menu.Config.ChamsMaterial == 1) {
		Material = g_pMaterialSys->FindMaterial("yeti_flat", "Model textures");
	}
	else if (g_Menu.Config.ChamsMaterial == 2) {
		Material = g_pMaterialSys->FindMaterial("models/inventory_items/trophy_majors/crystal_blue", "Other textures");
	}
	else if (g_Menu.Config.ChamsMaterial == 3) {
		Material = g_pMaterialSys->FindMaterial("models/inventory_items/dogtags/dogtags_outline", "Other textures");
	}
	else if (g_Menu.Config.ChamsMaterial == 4) {
		Material = g_pMaterialSys->FindMaterial("models/inventory_items/trophy_majors/gold", "Other textures"); 
	}
	else if (g_Menu.Config.ChamsMaterial == 5) {
		Material = g_pMaterialSys->FindMaterial("models/inventory_items/trophy_majors/gloss", "Other textures");
	}
	else if (g_Menu.Config.ChamsMaterial == 6) {
		Material = g_pMaterialSys->FindMaterial("models/inventory_items/trophy_majors/crystal_clear", "Other textures");
	}
	else if (g_Menu.Config.ChamsMaterial == 7) {
		Material = g_pMaterialSys->FindMaterial("models/inventory_items/cologne_prediction/cologne_prediction_glass", "Other textures");
	}

	if (g::pLocalEntity->IsAlive()) {

		static uint32_t cnt = 0;
		float freq = .025f;

		if (cnt++ >= (uint32_t)-1) cnt = 0;

		if (g::pLocalEntity->IsScoped())
			alpha = (std::sin(freq * cnt + 0) * 127 + 128) / 4;
		else
			alpha = std::sin(freq * cnt + 0) * 127 + 128;
		if (alpha < 0)
			alpha = 0;

		if (g::pLocalEntity->IsScoped())
			alpha2 = 75;
		else
			alpha2 = 255;

		clock_t thisClock = clock();
		if ((((thisClock - lastClock) / (double)CLOCKS_PER_SEC) * 1000.0) > 1000)
		{
			lastClock = thisClock;
			fps = cFps;
			cFps = 0;
		}
		else
			cFps++;

		float time = .0165f / fps;

		if (g_pEngine->IsInGame())
		{
			if (g::pLocalEntity->IsAlive() && g_Misc.isthirdperson && g_Menu.Config.DebugRealAngle)
			{

				static uintptr_t pCall = (uintptr_t)Utils::FindSignature("server.dll", "55 8B EC 81 EC ? ? ? ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE");

				float fDuration = -1;

				PVOID pEntity = nullptr;
				pEntity = UTIL_PlayerByIndex(g::pLocalEntity->EntIndex());

				if (pEntity)
				{
					__asm
					{
						pushad
						movss xmm1, fDuration
						push 0 //bool monoColor
						mov ecx, pEntity
						call pCall
						popad
					}
				}
			}
		}

		if (!g::pLocalEntity || !g_pEngine->IsInGame() || !g_pEngine->IsConnected())
		{
			return;
		}
	}
	oSceneEnd(ecx, edx);
}

void __fastcall Hooks::DoExtraBonesProcessing(void * ECX, void * EDX, void * unkn1, void * unkn2, void * unkn3, void * unkn4, CBoneBitList & unkn5, void * unkn6)
{
	C_BaseEntity* pPlayerEntity = (C_BaseEntity*)ECX;

	if (!pPlayerEntity || pPlayerEntity == nullptr)
		return;

	if (!pPlayerEntity->IsAlive() || pPlayerEntity->IsDormant())
		return;

	if (!pPlayerEntity->AnimState())
		return;

	auto oDoExtraBonesProcessing = g_Hooks.pPlayerHook[pPlayerEntity->EntIndex()]->GetOriginal<ExtraBoneProcess_t>(vtable_indexes::extraBonePro);

	const auto backup_onground = pPlayerEntity->AnimState()->m_bOnGround;
	pPlayerEntity->AnimState()->m_bOnGround = false;
	oDoExtraBonesProcessing;
	pPlayerEntity->AnimState()->m_bOnGround = backup_onground;
}

int __fastcall  Hooks::hkDoPostScreenEffects(void* _this, int edx, int a1)
{
	static auto oDoPostScreenEffects = g_Hooks.pClientModeHook->GetOriginal<decltype(&hkDoPostScreenEffects)>(vtable_indexes::do_post_screen_effects);

	if (!g::pLocalEntity || !g_pEngine->IsInGame() || !g_pEngine->IsConnected())
		return oDoPostScreenEffects(g_pClientMode, edx, a1);

	for (auto i = 0; i < g_pGlowManager->max_size; i++)
	{
		auto def = &g_pGlowManager->m_GlowObjectDefinitions[i];

		if (!def || def->IsUnused() || !def->m_pEntity || !def->m_pEntity->GetClientClass())
			continue;

		auto classid = def->m_pEntity->GetClientClass()->ClassID;

		if (classid == ClassID::CPlantedC4 || classid == ClassID::CC4)
		{
			if (g_Menu.Config.BombEsp || g_Menu.Config.DroppedWeapons)
				MakeDefNormal(def, Color(255, 255, 255, 255));

		}
		if (classid == ClassID::CCSPlayer && def->m_pEntity != g::pLocalEntity && def->m_pEntity->IsAlive())
		{
			if (g_Menu.Config.Glow)
				MakeDefNormal(def, Color(g_Menu.Config.GlowColor.r, g_Menu.Config.GlowColor.b, g_Menu.Config.GlowColor.g, g_Menu.Config.GlowColor.a));

		}
		if (classid == ClassID::CBaseAnimating)
		{
			if (g_Menu.Config.DroppedWeapons)
				MakeDefNormal(def, Color(255, 255, 255, 255));

		}
	}

	return oDoPostScreenEffects(g_pClientMode, edx, a1);
}

bool fresh_tick(int tick) {

	static int lastTick = tick;

	if (tick > lastTick) {

		lastTick = tick;
		return true;
	}
	return false;
}

template<class T, class U>
static T clamp(T in, U low, U high) {
	if (in <= low)
		return low;

	if (in >= high)
		return high;

	return in;
}

void __fastcall Hooks::DrawModelExecute(void* ecx, void* edx, IMatRenderContext* context, const DrawModelState_t& state, const ModelRenderInfo_t& info, matrix3x4_t* matrix)
{
	static auto oDrawModelExecute = g_Hooks.pModelHook->GetOriginal<DrawModelExecute_t>(vtable_indexes::dme);

	const char* ModelName = g_pModelInfo->GetModelName((model_t*)info.pModel);

	if (!g::pLocalEntity || !g_pEngine->IsInGame() || !g_pEngine->IsConnected())
		return;

	C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(info.index);

	if (pPlayerEntity && pPlayerEntity->IsAlive() && !pPlayerEntity->IsDormant() && g_Aimbot.Matrix[info.index] && strstr(ModelName, "models/player")) {
		if (g_pEngine->IsInGame() && g_pEngine->IsConnected()) {
			if (g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer())->IsAlive()) {
				if (pPlayerEntity == g::pLocalEntity && g::pLocalEntity->IsScoped()) { 
					g_pRenderView->SetBlend(0.18f);
				}
				else {
					g_pRenderView->SetBlend(1.f);
				}
			}
		}
	}
	if (g_Menu.Config.HandChams && !strstr(ModelName, "arms"))
		oDrawModelExecute(ecx, context, state, info, matrix);
	else if (!g_Menu.Config.HandChams)
		oDrawModelExecute(ecx, context, state, info, matrix);

	if (g::pLocalEntity->IsAlive()) {
		g_Chams.OnDrawModelExecute(ecx, edx, context, state, info, matrix);
		g_Chams.BacktrackChams(ecx, edx, context, state, info, matrix);
	}
		if (g::pLocalEntity->IsAlive()) {
		if (pPlayerEntity && pPlayerEntity->IsAlive() && !pPlayerEntity->IsDormant() && g_Aimbot.Matrix[info.index] && strstr(ModelName, "models/player") && pPlayerEntity == g::pLocalEntity && g::pLocalEntity->IsAlive()) {
			matrix3x4_t BoneToWorld[MAXSTUDIOBONES];
			if (g_Menu.Config.FakeDuck && GetAsyncKeyState(g_Menu.Config.FakeduckKey)) {
				 
				float max_duck_height = g_pMovement->GetPlayerViewOffset(true).z;
				float max_stand_height = g_pMovement->GetPlayerViewOffset(false).z;

		//	matrix[8].SetOrigin(Vector(g::pLocalEntity->GetOrigin().x, g::pLocalEntity->GetOrigin().y, g::pLocalEntity->GetOrigin().z + clamp((1.0f - g::pLocalEntity->m_flDuckAmount()) * max_stand_height, max_duck_height, max_stand_height)));
			}
		}
	}
}
int m_nTickbaseShift = 0;
bool bInSendMove = false, bFirstSendMovePack = false;

bool __fastcall Hooks::hkdWriteUsercmdDeltaToBuffer(void * ecx, void *, int slot, bf_write * buf, int from, int to, bool isnewcommand)
{
	static auto ofunct = g_Hooks.pClientHook->GetOriginal <WriteUsercmdDeltaToBufferFn>(24);

	if (g::m_nTickbaseShift <= 0)
		return ofunct(ecx, slot, buf, from, to, isnewcommand);

	if (from != -1)
	{
		return true;
	}

	auto CL_SendMove = []() {
		using CL_SendMove_t = void(__fastcall*) (void);
		static CL_SendMove_t CL_SendMoveF = (CL_SendMove_t)Utils::FindSignature("engine.dll", "55 8B EC A1 ? ? ? ? 81 EC ? ? ? ? B9 ? ? ? ? 53 8B 98");

		CL_SendMoveF();
	};
	auto WriteUsercmd = [](bf_write * buf, CUserCmd * in, CUserCmd * out) {
		using WriteUsercmd_t = void(__fastcall*) (bf_write *, CUserCmd *, CUserCmd *);
		static WriteUsercmd_t WriteUsercmdF = (WriteUsercmd_t)Utils::FindSignature("client_panorama.dll", "55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D");
		WriteUsercmdF(buf, in, out);

	};
	int * pNumBackupCommands = (int *)(reinterpret_cast <uintptr_t> (buf) - 0x30);
	int * pNumNewCommands = (int *)(reinterpret_cast <uintptr_t> (buf) - 0x2C);
	auto net_channel = *reinterpret_cast <NetChannel * *> (reinterpret_cast <uintptr_t> (g_pClientState) + 0x9C);
	int32_t new_commands = *pNumNewCommands;

	/*if (!Globals::bInSendMove)
	{
		if (new_commands <= 0)
			return false;

		Globals::bInSendMove = true;
		Globals::bFirstSendMovePack = true;
		Globals::m_nTickbaseShift += new_commands;

		while (Globals::m_nTickbaseShift > 0) {
			CL_SendMove();
			Globals::bFirstSendMovePack = false;
		}

		Globals::bInSendMove = false;
		return false;
	}*/

	if (!g::bFirstSendMovePack) {
		int32_t loss = min(g::m_nTickbaseShift, 10);

		g::m_nTickbaseShift -= loss;
		net_channel->m_nOutSequenceNr += loss;
	}

	int32_t next_cmdnr = g_pClientState->m_nLastOutgoingCommand + g_pClientState->m_nChokedCommands + 1;
	int32_t total_new_commands = min(g::m_nTickbaseShift, 62);
	g::m_nTickbaseShift -= total_new_commands;

	from = -1;
	*pNumNewCommands = total_new_commands;
	*pNumBackupCommands = 0;

	for (to = next_cmdnr - new_commands + 1; to <= next_cmdnr; to++)
	{
		if (!ofunct(ecx, slot, buf, from, to, isnewcommand))
			return false;

		from = to;
	}

	CUserCmd * last_realCmd = g_GameInput->GetUserCmd(slot, from);
	CUserCmd fromCmd;
	if (last_realCmd)
		fromCmd = *last_realCmd;

	CUserCmd toCmd = fromCmd;
	toCmd.command_number++;
	toCmd.tick_count += 200;


	for (int i = new_commands; i <= total_new_commands; i++)
	{
		WriteUsercmd(buf, &toCmd, &fromCmd);
		fromCmd = toCmd;
		toCmd.command_number++;
		toCmd.tick_count++;
	}

	return true;
}

void DrawBeam(Vector src, Vector end, Color color, float life)
{
	BeamInfo_t beamInfo;
	beamInfo.m_nType = TE_BEAMPOINTS;
	char* modelName;
	switch (g_Menu.Config.TracerMaterial)
	{
	case 0:
		modelName = "sprites/blueglow1.vmt";
		break;
	case 1:
		modelName = "sprites/bubble.vmt";
		break;
	case 2:
		modelName = "sprites/glow01.vmt";
		break;
	case 3:
		modelName = "sprites/physbeam.vmt";
		break;
	case 4:
		modelName = "sprites/purpleglow1.vmt";
		break;
	case 5:
		modelName = "sprites/purplelaser1.vmt";
		break;
	case 6:
		modelName = "sprites/radio.vmt";
		break;
	case 7:
		modelName = "sprites/white.vmt";
		break;
	default:
		modelName = "sprites/white.vmt";
		break;
	}
	beamInfo.m_pszModelName = modelName;
	beamInfo.m_nModelIndex = -1; // will be set by CreateBeamPoints if its -1
	beamInfo.m_flHaloScale = 5.0f;
	beamInfo.m_flLife = life;
	beamInfo.m_flWidth = 2.0f;
	beamInfo.m_flEndWidth = 2.0f;
	beamInfo.m_flFadeLength = 3.0f;
	beamInfo.m_flAmplitude = 4.0f;
	beamInfo.m_flBrightness = color.alpha;
	beamInfo.m_flSpeed = 0.2f;
	beamInfo.m_nStartFrame = 0;
	beamInfo.m_flFrameRate = 0.f;
	beamInfo.m_flRed = color.red;
	beamInfo.m_flGreen = color.green;
	beamInfo.m_flBlue = color.blue;
	beamInfo.m_nSegments = 2;
	beamInfo.m_bRenderable = true;
	beamInfo.m_nFlags = FBEAM_ONLYNOISEONCE;

	beamInfo.m_vecStart = src;
	beamInfo.m_vecEnd = end;

	Beam_t* myBeam = g_pIViewRenderBeams->CreateBeamPoints(beamInfo);

	if (myBeam)
		g_pIViewRenderBeams->DrawBeam(myBeam);
}

auto LoadSky = reinterpret_cast<void(__fastcall*)(const char*)>(Utils::FindSignature("engine.dll", "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45"));
Color color;


void __stdcall Hooks::FrameStageNotify(ClientFrameStage_t curStage)
{
	static auto oFrameStage = g_Hooks.pClientHook->GetOriginal<FrameStageNotify_t>(vtable_indexes::frameStage);

	g_Misc.ThirdPerson(curStage);

	g_Misc.DoNoSmoke(curStage);

	g_Misc.ShowImpacts();

	g_Resolver.FrameStage(curStage);
	
	if (g_Menu.Config.NightMode) {
		LoadSky("sky_csgo_night02");
	}

	g_Misc.BootlegNightMode();

	g_Resolver.FrameStage(curStage);

	if (g_pEngine->IsInGame() || g_pEngine->IsConnected() && g::pLocalEntity)
			night_mode(); 


	if (g_pEngine->IsInGame() || g_pEngine->IsConnected())
	{
		if (g_Menu.Config.Tracer) {

			int beamtime;
			beamtime = g_Menu.Config.HitmarkerTime;
			if (beamtime <= 0)
				beamtime = 1;

			for (unsigned int i = 0; i < trace_logs.size(); i++)
			{
				/* draw the beam */
				DrawBeam(trace_logs[i].start, trace_logs[i].position, color, beamtime);

				trace_logs.erase(trace_logs.begin() + i);
			}
		}
	}

	oFrameStage(curStage);	
}

struct HitMarker_t
{
	float expireTime;
	int damage;
};

std::vector<HitMarker_t> HitMarkers;

void DamageListenerPaintTraverse()
{
	if (!g::pLocalEntity || !g_pEngine->IsInGame() || !g_Menu.Config.Esp)
		return;

	if (!g::pLocalEntity->IsAlive())
		return;

	static int width = 0;
	static int height = 0;
	if (height == 0 || width == 0)
		g_pEngine->GetScreenSize(width, height);

	float alpha = 0.f;

	for (size_t i = 0; i < HitMarkers.size(); i++)
	{
		float timeDiff = HitMarkers[i].expireTime - g_pGlobalVars->curtime;

		if (timeDiff < 0.f)
		{
			HitMarkers.erase(HitMarkers.begin() + i);
			continue;
		}

		std::string dmg = "-";
		dmg += std::to_string(HitMarkers[i].damage).c_str();

		int moveDist = 24;

		float ratio = 1.f - (timeDiff / g_Menu.Config.ExpireTime);
		alpha = (timeDiff / g_Menu.Config.ExpireTime) * 2.f;
		if (g_Menu.Config.VisHitDmg)
			g_pSurface->DrawT(width / 2 + 6 + ratio * moveDist / 2, height / 2 + 6 + ratio * moveDist, Color(255,255,255,255), g::memefont, false, dmg.c_str());
	}

	if (HitMarkers.size() > 0)
	{
		int lineSize = (int)g_Menu.Config.MarkerSize;
		int screenSizeX, screenCenterX;
		int screenSizeY, screenCenterY;
		g_pEngine->GetScreenSize(screenSizeX, screenSizeY);
		screenCenterX = screenSizeX / 2;
		screenCenterY = screenSizeY / 2;
		g_pSurface->Line(screenCenterX - lineSize, screenCenterY - lineSize, screenCenterX - (lineSize / 4), screenCenterY - (lineSize / 4), Color(g_Menu.Config.HitmarkerColor.r, g_Menu.Config.HitmarkerColor.g, g_Menu.Config.HitmarkerColor.b, g_Menu.Config.HitmarkerColor.a));
		g_pSurface->Line(screenCenterX - lineSize, screenCenterY + lineSize, screenCenterX - (lineSize / 4), screenCenterY + (lineSize / 4), Color(g_Menu.Config.HitmarkerColor.r, g_Menu.Config.HitmarkerColor.g, g_Menu.Config.HitmarkerColor.b, g_Menu.Config.HitmarkerColor.a));
		g_pSurface->Line(screenCenterX + lineSize, screenCenterY + lineSize, screenCenterX + (lineSize / 4), screenCenterY + (lineSize / 4), Color(g_Menu.Config.HitmarkerColor.r, g_Menu.Config.HitmarkerColor.g, g_Menu.Config.HitmarkerColor.b, g_Menu.Config.HitmarkerColor.a));
		g_pSurface->Line(screenCenterX + lineSize, screenCenterY - lineSize, screenCenterX + (lineSize / 4), screenCenterY - (lineSize / 4), Color(g_Menu.Config.HitmarkerColor.r, g_Menu.Config.HitmarkerColor.g, g_Menu.Config.HitmarkerColor.b, g_Menu.Config.HitmarkerColor.a));

	}
}
/*
#define D3D_COLOR_BLACK(a)	D3DCOLOR_ARGB(a, 0, 0, 0)   ТАПНУЛ КАК СУКУУ

void DrawStringWithFont(float x, float y, D3DCOLOR color, char *format, ...)
{
	char buffer[256];
	RECT fontRect = { (int)x, (int)y, (int)x, (int)y };

	va_list va_argList;

	va_start(va_argList, format);
	wvsprintf(buffer, format, va_argList);
	va_end(va_argList);

	fnt->DrawText(NULL, buffer, strlen(buffer), &fontRect, DT_NOCLIP, color);
}

#define D3D_COLOR_BLACK(a)	D3DCOLOR_ARGB(a, 0, 0, 0)
//#include "Features/damageesp/damageesp.h"
void DamageESP()
{
	if (g_Menu.Config.DamageESP || Shonax.Visuals.damagegroup)
		for (int i = 0; i < MAX_FLOATING_TEXTS; i++)
		{
			FloatingText *txt = &floatingTexts[i % MAX_FLOATING_TEXTS];

			if (!txt->valid)
				continue;

			float endTime = txt->startTime + 1.1f;

			if (endTime < g_pGlobalVars->curtime)
			{
				txt->valid = false;
				continue;
			}

			Vector2D screen;

			if (Utils::WorldToScreen(txt->hitPosition, screen))
			{
				float t = 1.0f - (endTime - g_pGlobalVars->curtime) / (endTime - txt->startTime);

				screen.y -= t * (35.0f);
				screen.x -= (float)txt->randomIdx * t * 3.0f;
				if (Shonax.Visuals.damageesp)
				{
					char msg[12];
					sprintf_s(msg, 12, "-%dHP", txt->damage);

					//int width = GetTextWitdh(msg, fntVerdana10);

					DrawStringWithFont(fntVerdana10, screen.x - width / 2, screen.y - 11 + 1, D3D_COLOR_BLACK((int)((1.0f - t) * (float)255)), msg);

					DrawStringWithFont(fntVerdana10, screen.x - width / 2, screen.y - 11, D3DCOLOR_ARGB(int(Shonax.Visuals.damageespcolor[3] * 255), int(Shonax.Visuals.damageespcolor[0] * 255), int(Shonax.Visuals.damageespcolor[1] * 255), int(Shonax.Visuals.damageespcolor[2] * 255)), msg);
				}
				if (Shonax.Visuals.damagegroup)
				{
					char msgxd[12];
					sprintf_s(msgxd, 12, GetHitgroupName(gitgrp)); //gitgrp
					int widthxd = GetTextWitdh(msgxd, fntVerdana10);


					g_pSurface->DrawT(fntVerdana10, screen.x - widthxd / 2, screen.y - 63 + 1, D3D_COLOR_BLACK((int)((1.0f - t) * (float)255)), msgxd);
					DrawStringWithFont(fntVerdana10, screen.x - widthxd / 2, screen.y - 63, D3DCOLOR_ARGB(int(Shonax.Visuals.damagegroupcolor[3] * 255), int(Shonax.Visuals.damagegroupcolor[0] * 255), int(Shonax.Visuals.damagegroupcolor[1] * 255), int(Shonax.Visuals.damagegroupcolor[2] * 255)), msgxd);
				}
			}
		}
}

*/
void __fastcall Hooks::PaintTraverse(PVOID pPanels, int edx, unsigned int vguiPanel, bool forceRepaint, bool allowForce)
{
	static auto oPaintTraverse = g_Hooks.pPanelHook->GetOriginal<PaintTraverse_t>(vtable_indexes::paint);
	static unsigned int panelID, panelHudID;

	if (!panelHudID)
		if (!strcmp("HudZoom", g_pPanel->GetName(vguiPanel)))
		{
			panelHudID = vguiPanel;
		}

	if (panelHudID == vguiPanel && g::pLocalEntity && g::pLocalEntity->IsAlive() && g_Menu.Config.NoScope)
	{
		if (g::pLocalEntity->IsScoped())
			return;
	}

	oPaintTraverse(pPanels, vguiPanel, forceRepaint, allowForce);

	if (!panelID)
		if (!strcmp("MatSystemTopPanel", g_pPanel->GetName(vguiPanel)))
		{
			panelID = vguiPanel;
			g_Hooks.bInitializedDrawManager = true;
		}
			
	if (panelID == vguiPanel) 
	{
		g_ESP.Render();
		g_Misc.Crosshair();
		g_Menu.Render();
		g_ESP.QuakeKillCounter();
		if (g_Menu.Config.VisHitmarker)
			DamageListenerPaintTraverse();


		if (false) // server hitboxes
		{
			static uintptr_t pCall = (uintptr_t)Utils::FindSignature("server.dll", "55 8B EC 81 EC ? ? ? ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE");

			float fDuration = -1.f;

			PVOID pEntity = nullptr;
			pEntity = UTIL_PlayerByIndex(g::pLocalEntity->EntIndex());

			if (pEntity)
			{
				__asm
				{
					pushad
					movss xmm1, fDuration
					push 0 //bool monoColor
					mov ecx, pEntity
					call pCall
					popad
				}
			}
		}
	}
}

float dot_product(const Vector& a, const Vector& b) {
	return (a.x * b.x
		+ a.y * b.y
		+ a.z * b.z);
}
static void CapsuleOverlay(C_BaseEntity * pPlayer, Color col, float duration)
{
	if (!pPlayer)
		return;

	studiohdr_t* pStudioModel = g_pModelInfo->GetStudiomodel((model_t*)pPlayer->GetModel());
	if (!pStudioModel)
		return;

	static matrix3x4_t pBoneToWorldOut[128];
	if (!pPlayer->SetupBones(pBoneToWorldOut, 128, 256, 0))
		return;

	mstudiohitboxset_t* pHitboxSet = pStudioModel->GetHitboxSet(0);
	if (!pHitboxSet)
		return;

	auto VectorTransform2 = [](const Vector in1, matrix3x4_t in2, Vector & out)
	{

		out[0] = dot_product(in1, Vector(in2[0][0], in2[0][1], in2[0][2])) + in2[0][3];
		out[1] = dot_product(in1, Vector(in2[1][0], in2[1][1], in2[1][2])) + in2[1][3];
		out[2] = dot_product(in1, Vector(in2[2][0], in2[2][1], in2[2][2])) + in2[2][3];
	};

	for (int i = 0; i < pHitboxSet->numhitboxes; i++)
	{
		mstudiobbox_t* pHitbox = pHitboxSet->GetHitbox(i);
		if (!pHitbox)
			continue;

		Vector vMin, vMax;
		VectorTransform2(pHitbox->min, pBoneToWorldOut[pHitbox->bone], vMin); //nullptr???
		VectorTransform2(pHitbox->max, pBoneToWorldOut[pHitbox->bone], vMax);

		if (pHitbox->radius > -1)
		{
			g_pIVDebugOverlay->AddCapsuleOverlay(vMin, vMax, pHitbox->radius, col.red, col.green, col.blue, col.alpha, duration);
		}
	}
}

PlayerInfo_t GetInfo(int Index) {
	PlayerInfo_t Info;
	g_pEngine->GetPlayerInfo(Index, &Info);
	return Info;
}

void Event::FireGameEvent(IGameEvent* event)
{
	if (!event)
		return;

	if (!g::pLocalEntity)
		return;

	auto attacker = g_pEntityList->GetClientEntity(g_pEngine->GetPlayerForUserID(event->GetInt("attacker")));
	if (!attacker)
		return;

	if (attacker != g::pLocalEntity)
		return;

	int index = g_pEngine->GetPlayerForUserID(event->GetInt("userid"));

	auto filter = CHudChat::ChatFilters::CHAT_FILTER_NONE;

	PlayerInfo_t pInfo;
	g_pEngine->GetPlayerInfo(index, &pInfo);

	g::Hit[index] = true;
	bool buybotfix;
	if (!strcmp(event->GetName(), "round_prestart") && g_pEngine->IsInGame() && g_pEngine->IsConnected() && g::pLocalEntity)
	{
		buybotfix = true;
	}

	if (g_pEngine->GetPlayerForUserID(event->GetInt("attacker")) == g_pEngine->GetLocalPlayer())
	{
		HitMarker_t temp = { g_pGlobalVars->curtime + g_Menu.Config.ExpireTime, event->GetInt("dmg_health") };

		if (temp.damage > 0 && temp.damage < 100)
			HitMarkers.push_back(temp);
	}
	//maatikk :/
	if (buybotfix)
	{
		if (g_Menu.Config.OtherBB[0])
		g_pEngine->ExecuteClientCmd("buy vesthelm");
		if (g_Menu.Config.OtherBB[1])
			g_pEngine->ExecuteClientCmd("buy vesthelm");
		if (g_Menu.Config.OtherBB[2])
			g_pEngine->ExecuteClientCmd("buy hegrenade");
		if (g_Menu.Config.OtherBB[3])
			g_pEngine->ExecuteClientCmd("buy smokegrenade");
		if (g_Menu.Config.OtherBB[4])
			g_pEngine->ExecuteClientCmd("buy molotov; buy incgrenade");
		if (g_Menu.Config.OtherBB[5])
			g_pEngine->ExecuteClientCmd("buy taser");
		if (g_Menu.Config.OtherBB[6])
			g_pEngine->ExecuteClientCmd("buy defuser"); // ? check

		if (g_Menu.Config.MainWeaponBB == 0)
			//g_pEngine->ExecuteClientCmd("buy g3sg1; buy scar20");
		if (g_Menu.Config.MainWeaponBB == 1)
			g_pEngine->ExecuteClientCmd("buy g3sg1; buy scar20");
		if (g_Menu.Config.MainWeaponBB == 2)
			g_pEngine->ExecuteClientCmd("buy awp");
		if (g_Menu.Config.MainWeaponBB == 3)
			g_pEngine->ExecuteClientCmd("buy ssg08");

		if (g_Menu.Config.SecondWeaponBB == 0)

		if (g_Menu.Config.SecondWeaponBB == 1)
			g_pEngine->ExecuteClientCmd("buy deagle; buy revolver");
		if (g_Menu.Config.SecondWeaponBB == 2)
			g_pEngine->ExecuteClientCmd("buy elite");
		if (g_Menu.Config.SecondWeaponBB == 1)
			g_pEngine->ExecuteClientCmd("buy hegrenade; buy molotov; buy smokegrenade; buy incgrenade; buy taser;");
		buybotfix = false; //ghetto fix buy bot

	}
	//maatikk и Я Mysorochek:/
	//PlayerInfo_t pInfo;
	//g_pEngine->GetPlayerInfo(index, &pInfo);

	g::Hit[index] = true;
	g::hits++;
	if (g_Menu.Config.Hitmarker) {
		g::flHurtTick = g_pGlobalVars->tickcount;
		if (g_Menu.Config.Hitmarker == 1)
			g_pEngine->ExecuteClientCmd("play doors/default_locked");
		else if (g_Menu.Config.Hitmarker == 2)
			g_pEngine->ExecuteClientCmd("play buttons/arena_switch_press_02.wav");
		else if (g_Menu.Config.Hitmarker == 3) 
			g_pEngine->ExecuteClientCmd("play ui/csgo_ui_button_rollover_large");
		else if (g_Menu.Config.Hitmarker == 4)
			g_pEngine->ExecuteClientCmd("play resource/warning");
		else if (g_Menu.Config.Hitmarker == 5)
			g_pEngine->ExecuteClientCmd("play training/timer_bell");
		else if (g_Menu.Config.Hitmarker == 6)
			PlaySoundA("csgo\\sound\\hitsound.wav", NULL, SND_ASYNC);
		else if (g_Menu.Config.Hitmarker == 7)
		PlaySoundA(bubble, NULL, SND_ASYNC | SND_MEMORY);
		else if (g_Menu.Config.Hitmarker == 8)
		PlaySoundA(fatality, NULL, SND_ASYNC | SND_MEMORY);
	    //else if (g_Menu.Config.Hitmarker == 8)
			//PlaySoundA(cod, NULL, SND_ASYNC | SND_MEMORY);
		//else if (g_Menu.Config.Hitmarker == 8)
			//PlaySoundA(laser, NULL, SND_ASYNC | SND_MEMORY);

	}
	auto localplayer = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());

	/* check if we are attacker */
	if (attacker == localplayer)
	{
		color = Color(0,255,0,255);
	}
	if (g_Menu.Config.DrawHitboxes)
	{
		C_BaseEntity* hittedplayer = (C_BaseEntity*)g_pEntityList->GetClientEntity(g_pEngine->GetPlayerForUserID(event->GetInt("userid")));

		if (hittedplayer && hittedplayer->EntIndex() > 0 && hittedplayer->EntIndex() < 64)
		{
			if (g::pLocalEntity && hittedplayer != g::pLocalEntity)
				CapsuleOverlay(hittedplayer, Color(g_Menu.Config.ImpactColor.r, g_Menu.Config.ImpactColor.g, g_Menu.Config.ImpactColor.b, g_Menu.Config.ImpactColor.a), g_Menu.Config.DrawHitboxTime);
		}
	}

	static auto get_hitgroup_name = [](int hitgroup) -> std::string {
		switch (hitgroup) {
		case HITGROUP_HEAD:
			return "head";
		case HITGROUP_LEFTLEG:
			return "left leg";
		case HITGROUP_RIGHTLEG:
			return "right leg";
		case HITGROUP_STOMACH:
			return "stomach";
		case HITGROUP_LEFTARM:
			return "left arm";
		case HITGROUP_RIGHTARM:
			return "right arm";
		default:
			return "body";
		}
	};

	int attackerid = event->GetInt("attacker");
	int entityid = g_pEngine->GetPlayerForUserID(attackerid);

	int miss = (g::shots - g::hits);

	int userid = event->GetInt("userid");
	int userentityid = g_pEngine->GetPlayerForUserID(userid);

	if (entityid == g_pEngine->GetLocalPlayer() && userentityid != g_pEngine->GetLocalPlayer())
	{
		int nUserID = event->GetInt("attacker");
		int nDead = event->GetInt("userid");
		if (nUserID || nDead)
		{
			PlayerInfo_t killer_info = GetInfo(g_pEngine->GetPlayerForUserID(nUserID));
			PlayerInfo_t killed_info = GetInfo(g_pEngine->GetPlayerForUserID(nDead)); 

			std::string hit = " \x02[digitalwave] \x01hit"" ";
			std::string player = killed_info.szName;
			std::string inthe = " in the ";
			std::string hitbox = get_hitgroup_name(event->GetInt("hitgroup"));
			std::string forr = " for ";
			std::string damage = event->GetString("dmg_health");
			std::string damage2 = " damage";
			std::string fourb = " (";
			std::string fiveb = event->GetString("health");
			std::string six = " health remaining)";

			g_ChatElement->ChatPrintf(0, filter, (hit + player + inthe + hitbox + forr + damage + damage2 + fourb + fiveb + six).c_str());
		}
	}


	g_Aimbot.player_hurt_event(event); 

	auto victim = g_pEntityList->GetClientEntity(g_pEngine->GetPlayerForUserID(event->GetInt("userid")));

};

void EventRStart::FireGameEvent(IGameEvent* event)
{
	if (event) {
		g_Aimbot.round_start_event(event);
	}
}

void FootstepEvent::FireGameEvent(IGameEvent* event)
{

	

}

void WeaponFire::FireGameEvent(IGameEvent* event)
{

	auto filter = CHudChat::ChatFilters::CHAT_FILTER_NONE;

	if (event) {

	}
}

void Player_Death::FireGameEvent(IGameEvent* event)
{
	if (event) {
		g_Aimbot.player_death_event(event);
		if (g_Menu.Config.KillCounter)
		{
			g::killtime = g_pGlobalVars->realtime;
			g::kills++;
		}

		g::globalAlpha = g_pGlobalVars->curtime;
	}
}

void BulletEvent::FireGameEvent(IGameEvent* event)
{
	//Impactss(event);
	Vector position(event->GetFloat("x"), event->GetFloat("y"), event->GetFloat("z"));

	/* get the shooter */
	auto shooter = g_pEntityList->GetClientEntity(g_pEngine->GetPlayerForUserID(event->GetInt("userid")));

	if (shooter == nullptr)
		return;

	if (shooter->IsDormant())
		return;

	if (shooter)
		trace_logs.push_back(trace_info(shooter->GetEyePosition(), position, g_pGlobalVars->curtime, event->GetInt("userid")));

	if (shooter == g::pLocalEntity)
		color = Color(0, 120, 255, 255);
	else if (shooter->GetTeam() != g::pLocalEntity->GetTeam())
		color = Color(255, 0, 0, 255);
	else
		color = Color(0, 0, 0, 0);
}

void __fastcall Hooks::OverrideView(void* ecx, void* edx, CViewSetup* pSetup)
{
	static auto oOverrideView = g_Hooks.pClientModeHook->GetOriginal<OverrideView_t>(vtable_indexes::view);

	if (g_pEngine->IsConnected() && g_pEngine->IsInGame())
	{
		if (!g::pLocalEntity)
			return;

		if (!g::pLocalEntity->IsAlive())
			return;

		if (g_Menu.Config.NoRecoil)
		{
			Vector viewPunch = g::pLocalEntity->GetViewPunchAngle();
			Vector aimPunch = g::pLocalEntity->GetAimPunchAngle();

			pSetup->angles[0] -= (viewPunch[0] + (aimPunch[0] * 2 * 0.4499999f));
			pSetup->angles[1] -= (viewPunch[1] + (aimPunch[1] * 2 * 0.4499999f));
			pSetup->angles[2] -= (viewPunch[2] + (aimPunch[2] * 2 * 0.4499999f));
		}

		if (g_Menu.Config.Fov != 0 && !g::pLocalEntity->IsScoped()) 
			pSetup->fov = g_Menu.Config.Fov;
			pSetup->viewmodel_fov = g_Menu.Config.Fov / 10;
			ConVar* viewmodel_offset_y = g_pCvar->FindVar("viewmodel_offset_y");
			*(int*)((DWORD)& viewmodel_offset_y->fnChangeCallback + 0xC) = 0; // ew
			viewmodel_offset_y->SetValue(g_Menu.Config.Fov / 10);
			ConVar* viewmodel_offset_x = g_pCvar->FindVar("viewmodel_offset_x");
			*(int*)((DWORD)& viewmodel_offset_x->fnChangeCallback + 0xC) = 0; // ew
			viewmodel_offset_x->SetValue(g_Menu.Config.Fov / 30);


		if (g_Menu.Config.NoZoom && g::pLocalEntity->IsScoped())
			pSetup->fov = (g_Menu.Config.Fov == 0) ? 90 : g_Menu.Config.Fov;
			pSetup->viewmodel_fov = g_Menu.Config.Fov;

		if (g_Menu.Config.FakeDuck && GetAsyncKeyState(g_Menu.Config.FakeduckKey)) {
			pSetup->origin.z = g::pLocalEntity->GetAbsOrigin().z + 64.f; // lol
		}
	}

	oOverrideView(ecx, edx, pSetup);
}

void __fastcall Hooks::LockCursor(ISurface* thisptr, void* edx)
{
    static auto oLockCursor = g_Hooks.pSurfaceHook->GetOriginal<LockCursor_t>(vtable_indexes::lockCursor);

    if (!g_Menu.menuOpened)
        return oLockCursor(thisptr, edx);

    g_pSurface->UnLockCursor();
}

LRESULT Hooks::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // for now as a lambda, to be transfered somewhere
    // Thanks uc/WasserEsser for pointing out my mistake!
    // Working when you HOLD th button, not when you press it.
    const auto getButtonHeld = [uMsg, wParam](bool& bButton, int vKey)
    {
		if (wParam != vKey) return;

        if (uMsg == WM_KEYDOWN)
            bButton = true;
        else if (uMsg == WM_KEYUP)
            bButton = false;
    };

	const auto getButtonToggle = [uMsg, wParam](bool& bButton, int vKey)
	{
		if (wParam != vKey) return;

		if (uMsg == WM_KEYUP)
			bButton = !bButton;
	};

    if (g_Hooks.bInitializedDrawManager)
    {
        // our wndproc capture fn
        if (g_Menu.menuOpened)
        {
            return true;
        }
    }


    return CallWindowProcA(g_Hooks.pOriginalWNDProc, hWnd, uMsg, wParam, lParam);
}

