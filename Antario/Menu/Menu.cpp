#include "Menu.h"
#include "TGFCfg.h"
#include "..\SDK\Vector.h"
#include "..\SDK\ISurface.h"
#include "..\Utils\Color.h"
#include "..\Utils\GlobalVars.h"
#include "..\Utils\XorStr.h"
#include "..\Hooks.h"
#include "../Features/rankchanger/Protobuff/profile_info_changer.h"
#include <windows.h>
#include <string>
#include "../SDK/CHudChat.h"
#include "../gui/zgui.h"

Menu g_Menu;

/*bool SendClientHello()
{
	// message CMsgClientHello {
	// 	repeated .CMsgSOCacheHaveVersion socache_have_versions = 2;
	// 	optional uint32 client_session_need = 3;
	// 	optional uint32 partner_accountflags = 7;
	// }
	ProtoWriter msg(7);
	msg.add(Field(3, TYPE_UINT32, (int64_t)1)); //client_session_need
	auto packet = msg.serialize();

	void* ptr = malloc(packet.size() + 8);

	if (!ptr)
		return false;

	((uint32_t*)ptr)[0] = k_EMsgGCClientHello | ((DWORD)1 << 31);
	((uint32_t*)ptr)[1] = 0;

	memcpy((void*)((DWORD)ptr + 8), (void*)packet.data(), packet.size());
	bool result = g_SteamGameCoordinator->GCSendMessage(k_EMsgGCClientHello | ((DWORD)1 << 31), ptr, packet.size() + 8) == k_EGCResultOK;
	free(ptr);

	return result;
}*/

void Menu::Render()
{
	zgui::poll_input("Valve001");

	if (zgui::begin_window ("digitalwave", { 500, 600 }, g::Verdana, zgui::zgui_window_flags_none))
	{
		if (zgui::tab_button(" legitbot", { 69, 30 }, Tabs.legitbot))
		{
			Tabs.legitbot = true;
			Tabs.ragebot = false;
			Tabs.visual = false;
			Tabs.misc = false;
			Tabs.config = false;
			Tabs.plist = false;
			Tabs.debug = false;
		}

		zgui::same_line();
		zgui::next_column(-5, 0);
		if (zgui::tab_button("ragebot", { 69, 30 }, Tabs.ragebot))
		{
			Tabs.legitbot = false;
			Tabs.ragebot = true;
			Tabs.visual = false;
			Tabs.misc = false;
			Tabs.config = false;
			Tabs.plist = false;
			Tabs.debug = false;
		}
		zgui::same_line();
		zgui::next_column(-5, 0);

		if (zgui::tab_button("visuals", { 69, 30 }, Tabs.visual))
		{
			Tabs.legitbot = false;
			Tabs.ragebot = false;
			Tabs.visual = true;
			Tabs.misc = false;
			Tabs.config = false;
			Tabs.plist = false;
			Tabs.debug = false;
		}

		zgui::same_line();
		zgui::next_column(-5, 0);


		if (zgui::tab_button("misc", { 69, 30 }, Tabs.misc))
		{
			Tabs.legitbot = false;
			Tabs.ragebot = false;
			Tabs.visual = false;
			Tabs.misc = true;
			Tabs.config = false;
			Tabs.plist = false;
			Tabs.debug = false;
		}

		zgui::same_line();
		zgui::next_column(-5, 0);

		if (zgui::tab_button("config", { 69, 30 }, Tabs.config))
		{
			Tabs.legitbot = false;
			Tabs.ragebot = false;
			Tabs.visual = false;
			Tabs.misc = false;
			Tabs.config = true;
			Tabs.plist = false;
			Tabs.debug = false;
		}

		zgui::same_line();
		zgui::next_column(-5, 0);

		if (zgui::tab_button(" player list", { 69, 30 }, Tabs.plist))
		{
			Tabs.legitbot = false;
			Tabs.ragebot = false;
			Tabs.visual = false;
			Tabs.misc = false;
			Tabs.config = false;
			Tabs.plist = true;
			Tabs.debug = false;
		}

		if (GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(0x44))
		{
			Tabs.legitbot = false;
			Tabs.ragebot = false;
			Tabs.visual = false;
			Tabs.misc = false;
			Tabs.config = false;
			Tabs.plist = false;
			Tabs.debug = true;
		}


		if (Tabs.legitbot)
			legittab();

		if (Tabs.ragebot) {
			ragetab();
			if (Tabs.ragebot2)
				ragetab2();
			else if (Tabs.ragebot3)
				ragetab3();
		}

		if (Tabs.visual)
			vistab();

		if (Tabs.misc)
			misctab();

		if (Tabs.config)
			cfgtab();

		if (Tabs.plist)
			plisttab();

		if (Tabs.debug)
			debugtab();

		zgui::end_window();
	}
}

void  Menu::legittab()
{
	zgui::next_column(-401, 39);

	zgui::begin_groupbox("aim", { 230 , 520 }, zgui::zgui_window_flags_none);
	{
		zgui::checkbox("enable", Config.Legitbot);
		if (Config.Legitbot)
		{
			Config.Aimbot = false;
		}
		zgui::checkbox("aimbot", Config.LegitAimbot);
		if (Config.LegitAimbot) {
			zgui::slider_int("aim fov", 0, 30, Config.LegitFOV);
			zgui::slider_int("aim smooth", 0, 100, Config.LegitSmooth);
			zgui::checkbox("rcs", Config.AimRCS);
			if (Config.AimRCS)
				zgui::slider_int("rcs amount", 0, 100, Config.RCSamount);
		}
		zgui::checkbox("backtrack", Config.LegitBacktrack);
		zgui::end_groupbox();
	}
	zgui::next_column(230, 39);
	zgui::begin_groupbox("individual", { 230 , 520 }, zgui::zgui_window_flags_none);
	{
		if (!Config.LegitAimbot) {
			zgui::checkbox("standalone rcs", Config.StandaloneRCS);
			Config.RCSamount = 0;
			if (Config.StandaloneRCS)
				zgui::slider_int("standalone amount", 0, 100, Config.StandaloneRCSSlider);
		}
		zgui::checkbox("antiaim", Config.LegitAA);
		if (Config.LegitAA)
		{
			zgui::checkbox("static", Config.LegitAAStatic);
		}
		zgui::slider_int("additive", 0, 180, Config.additive);

		if (Config.LegitAA)
			Config.Antiaim = false;
		zgui::end_groupbox();
	}
}

void Menu::ragetab2()
{
	zgui::next_column(-262, 80);
	zgui::begin_groupbox("RageBot", { 230 , 470 }, zgui::zgui_window_flags_none);
	{
		zgui::checkbox("Enable", Config.Aimbot);
		if (Config.Aimbot)
		{
			Config.Legitbot = false;
		}
		zgui::checkbox("Multipoint", Config.MultiPoint);
		if (Config.MultiPoint)
		{
			zgui::slider_int("Head scale", 0, 100, Config.HeadScale);
			zgui::slider_int("Body scale", 0, 100, Config.BodyScale);
		}
		zgui::combobox("Select weapon", { "Auto", "Scout", "Pistol", "Deagle/R8", "Awp", "Other" }, Config.Weaponss);
		if (Config.Weaponss == 0)
		{
			zgui::slider_int("Auto HitChance", 0, 100, Config.autohitchance);
			zgui::slider_int("Auto MinDamage", 0, 120, Config.autodmg);
		}
		else if (Config.Weaponss == 1)
		{
			zgui::slider_int("Scout HitChance", 0, 100, Config.ssghitchance);
			zgui::slider_int("Scout MinDamage", 0, 100, Config.ssgdmg);
		}
		else if (Config.Weaponss == 2)
		{
			zgui::slider_int("Pistol HitChance", 0, 100, Config.pistolhitchance);
			zgui::slider_int("Pistol MinDamage", 0, 100, Config.pistoldmg);
		}
		else if (Config.Weaponss == 3)
		{
			zgui::slider_int("Heavy pistol HitChance", 0, 100, Config.heavypistolhitchance);
			zgui::slider_int("Heavy pistol MinDamage", 0, 120, Config.heavypistoldmg);
		}
		else if (Config.Weaponss == 4)
		{
			zgui::slider_int("Awp HitChance", 0, 100, Config.riflehitchance);
			zgui::slider_int("Awp MinDamage", 0, 100, Config.rifledmg);
		}
		else if (Config.Weaponss == 5)
		{
			zgui::slider_int("Other HitChance", 0, 100, Config.otherhitchance);
			zgui::slider_int("Other MinDamage", 0, 120, Config.otherdmg);
		}
		zgui::multi_combobox("hitboxes to scan", std::vector< zgui::multi_select_item >{ { "Head", & Config.Head }, { "Body", &Config.Chest }, { "Stomach", &Config.Pelvis }, { "Arms", &Config.Arms }, { "Legs", &Config.Legs }});
		zgui::checkbox(("Extras"), Config.automode);
		if (Config.automode) {
			zgui::multi_combobox(("Auto"), std::vector< zgui::multi_select_item >{ { "Autostop", & Config.Autostop }, { "AutoRevolver", &Config.autorevolver }, { "Autoscope", &Config.Autoscope }});
		}
		zgui::checkbox(("Resolver"), Config.aimbot_resolver);

		zgui::checkbox(("Fix Feet"), Config.fixfeet);

		zgui::checkbox(("Doubletap"), Config.doubletap);
		if (Config.aimbot_resolver) {
			zgui::combobox(("Type"), { "Default", "ResolverTest" }, Config.ResolverType);
		}
		zgui::end_groupbox();
	}
	zgui::next_column(242, 94);
	zgui::begin_groupbox("RageBot Accuracy", { 230 , 470 }, zgui::zgui_window_flags_none);
	{
		//zgui::multi_combobox("LC exploit", std::vector< zgui::multi_select_item >{ { "Shot", & Config.ShotBacktrack }, { "Position", &Config.PosBacktrack }});
		zgui::checkbox("Delay shot", Config.DelayShot);
		if (Config.Resolver)
		{
			zgui::checkbox(("Animfix"), Config.Animfix);
		}
		zgui::checkbox("Ignore limbs on-move", Config.IgnoreLimbs);
		zgui::checkbox("Fake Lag prediction", Config.PredictFakeLag);
		zgui::checkbox("No Recoil", Config.aimbot_norecoil);
		zgui::checkbox("Optimized Aimbot", Config.OptimizedAimbot);
		if (Config.OptimizedAimbot)
			zgui::checkbox("Adaptive Multipoints", Config.AdaptiveMulti);
		//zgui::checkbox("Only Baim", Config.BaimAlways);
		zgui::checkbox("Body aim after", Config.BaimAfterEnable);
		if (Config.BaimAfterEnable)
		{
			zgui::slider_int("x missed shots", 0, 10, Config.BaimSlider);
		}
		zgui::key_bind("Force body aim key", Config.BaimKey);
		zgui::multi_combobox("Body aim conditions", std::vector< zgui::multi_select_item >{ { "If lethal", &Config.BaimLethal }, { "In air", &Config.BaimInAir }, { "Always on", &Config.BaimAlways }});
		if (Config.BaimLethal)
		{
			zgui::slider_int("Lethal HP", 0, 100, Config.BaimLethalSlider);
		}
		zgui::checkbox("Adaptive BodyAim", Config.adaptivebody);
		if (Config.adaptivebody)
		zgui::combobox("BodyAim Hitboxes", { "Upper chest", "Lower chest", "Stomach", "thorax", "pelvis", "Legs", "Feet" }, Config.bodyhitbox);
		zgui::end_groupbox();
	}
}

void Menu::ragetab3()
{
	zgui::next_column(-262, 80);
	zgui::begin_groupbox("Anti-Hit", { 230 , 470 }, zgui::zgui_window_flags_none);
	{
		zgui::checkbox("Enable ", Config.Antiaim);
		if (Config.Antiaim)
			Config.LegitAA = false;
		//zgui::combobox("AA type", { "advanced", "default" }, Config.aatype);
		zgui::combobox("Adjust pitch", { "off", "Down", "Up", "Switch" }, Config.PitchType);
		zgui::combobox("Adjust yaw", { "off", "Backward" }, Config.YawType);
		zgui::checkbox("At targets", Config.AtTarget);
		if (Config.aatype == 0) {
			zgui::combobox("Jitter type", { "off", "Jitter", "Switch", "Random" }, Config.Jitter);
			if (Config.Jitter > 0)
				zgui::slider_int("Add range", -180, 180, Config.AddRange);
			zgui::combobox("Desync Type", { "Static", "Jitter" }, Config.disinctype);
			zgui::key_bind("Invert real key", Config.inverter);
			zgui::slider_int("Body lean", -50, 50, Config.bodylean2);
			zgui::slider_int("Inverted body lean", -50, 50, Config.BodyLean);
		}
		else if (Config.aatype == 1)
		{
			zgui::checkbox("enable tickbase aa", Config.tbaas);
			if (Config.tbaas)
			{
				zgui::slider_int("jitter1", -120, 120, Config.jitter1);
				zgui::slider_int("jitter2", -120, 120, Config.jitter2);
				zgui::slider_int("jitter3", -120, 120, Config.jitter3);
				zgui::slider_int("jitter4", -120, 120, Config.jitter4);
			}
		}

		zgui::checkbox("Anti Backshoot", Config.OnShotAA); // ? xd
		zgui::end_groupbox();
	}
}

void  Menu::ragetab()
{
	zgui::next_column(-403, 39);
	if (zgui::tab_button("AimBot", { 200, 30 }, Tabs.ragebot2))
	{
		Tabs.ragebot2 = true;
		Tabs.ragebot3 = false;
	}
	zgui::same_line();
	zgui::next_column(50, 39);
	if (zgui::tab_button("Anti-Aim", { 200, 30 }, Tabs.ragebot3))
	{
		Tabs.ragebot2 = false;
		Tabs.ragebot3 = true;
	}
}

void  Menu::vistab()
{
	zgui::next_column(-401, 39);

	zgui::begin_groupbox("Players", { 230 , 270 }, zgui::zgui_window_flags_none);
	{
		zgui::checkbox("Enable#1", Config.Esp);
		zgui::checkbox("2d box", Config.Box);
		zgui::colorpicker("box color", Config.BoxColor);
		zgui::checkbox("draw name", Config.Name);
		zgui::colorpicker("font color", Config.FontColor);
		zgui::checkbox("draw weapon", Config.Weapon);
		zgui::colorpicker("wep color", Config.WepColor);
		zgui::checkbox("health bar", Config.HealthBar);
		zgui::checkbox("glow", Config.Glow);
		zgui::colorpicker("glow color", Config.GlowColor);
		zgui::multi_combobox("skeleton", std::vector< zgui::multi_select_item >{ { "normal", & Config.Skeleton[0] }, { "backtrack", &Config.Skeleton[1] }});
		zgui::colorpicker("skele color", Config.SkeletonColor);
		zgui::checkbox("chams", Config.Chams);
		zgui::colorpicker("chams color", Config.XQZ);
		zgui::checkbox("chams xqz", Config.ChamsXQZ);
		zgui::colorpicker("chams xqz color", Config.ChamsColor);
		//zgui::checkbox("desync chams(test)", Config.FakeChams);
		//zgui::checkbox("Damage ESP(test)", Config.DamageESP);
		if (Config.Antiaim && Config.DesyncTest)
		{
			//zgui::checkbox("Desync Chams/Ghost Chams", Config.DesyncChams);
			//if (Config.DesyncChams)
			//{
				//zgui::colorpicker("Desync Chams Color", Config.fake_chams_color);
				//zgui::slider_int("Desync Chams Transparency", 255, Config.DesyncChamsTransparency);
			//}
		}
		zgui::checkbox("backtrack chams", Config.Backtrackchams);
		zgui::colorpicker("backtrack chams color", Config.BacktrackchamsColor);
		zgui::checkbox("desync chams", Config.FakeChams);
		zgui::colorpicker("desync chams color", Config.fake_chams_color);
		zgui::checkbox("local chams", Config.LocalChams);
		zgui::colorpicker("local chams color", Config.LocalChamsColor);
		zgui::checkbox("hand chams", Config.HandChams);
		zgui::colorpicker("hand chams color", Config.HandChamsColor);
		zgui::checkbox("fake lag model", Config.FakeLagModel);
		zgui::colorpicker("lag model chams color", Config.FakeLagModelColor);
		zgui::checkbox("flags", Config.Flags);
		zgui::end_groupbox();
	}

	zgui::begin_groupbox("world", { 230 , 240 }, zgui::zgui_window_flags_none);
	{
		zgui::slider_int("nightmode value", 0, 100, Config.NightModeSlider);
		zgui::slider_int("asus props", 0, 100, Config.AsusProps);
		zgui::text("skybox color");
		zgui::colorpicker("skybox color", Config.SkyboxColor);
		zgui::checkbox("enable world modulation", Config.NightMode);
		zgui::checkbox("bomb esp", Config.BombEsp);
		zgui::checkbox("projectile esp", Config.Projectiles);
		zgui::checkbox("dropped weapon esp", Config.DroppedWeapons);
		zgui::checkbox("show enemies on radar", Config.EngineRadar);
		zgui::checkbox("show aimspot", Config.ShowAimSpot);
		zgui::multi_combobox("removals", std::vector< zgui::multi_select_item >{ { "zoom", & Config.NoZoom }, { "scope", &Config.NoScope }, { "smoke", &Config.NoSmoke },{ "vis recoil", &Config.NoRecoil },{ "post processing", &Config.PostProcess } });

		zgui::end_groupbox();
	}

	zgui::same_line();
	zgui::next_column(230, 39);
	zgui::begin_groupbox("others", { 230 , 520 }, zgui::zgui_window_flags_none);
	{
		zgui::checkbox("out of view indicator", Config.OutOfView);
		if (Config.OutOfView)
		{
			zgui::colorpicker("out of view color", Config.OutOfViewColor);
			zgui::slider_int("radius", 0, 300, Config.OutOfViewRadius);
			zgui::slider_int("size", 0, 18, Config.OutOfViewSize);
		}
		zgui::checkbox("antiaim arrows", Config.Arrows);
		zgui::checkbox("antiaim monitor", Config.AntiaimMonitor);
		zgui::checkbox("show impacts", Config.Impacts);
		zgui::checkbox("watermark", Config.Watermark);
		zgui::checkbox("draw hitboxes on hit", Config.DrawHitboxes);
		zgui::colorpicker("hitbox color", Config.ImpactColor);
		if (Config.DrawHitboxes)
		{
			zgui::slider_int("time", 0, 10, Config.DrawHitboxTime);
		}
		zgui::combobox("show weapon spread", { "off", "square", "circle", "filled circle", "fading dots", "filled dots" }, Config.SpreadCrosshair);
		if (g_Menu.Config.SpreadCrosshair != 1)
			zgui::colorpicker("spread color", Config.SpreadColor);
		zgui::multi_combobox("crosshair", std::vector< zgui::multi_select_item >{ { "override", & Config.Crosshair[0] }, { "penetration", &Config.Crosshair[1] }, { "recoil", &Config.Crosshair[2] }, });
		zgui::combobox("chams material", { "metallic", "flat", "crystal blue", "pulse", "gold", "gloss", "crystal clear", "glass", "glow" }, Config.ChamsMaterial);
		zgui::combobox("hand chams material", { "metallic", "flat", "crystal blue", "pulse", "gold", "gloss", "crystal clear", "glass", "glow" }, Config.HandChamsMaterial);
		zgui::combobox("shadow material", {  "pulse", "gloss", "crystal clear", "glass", "chrome", "lightray"}, Config.ShadowMaterial);
		zgui::checkbox("visual hitmarker", Config.VisHitmarker);
		zgui::colorpicker("hitmarker color", Config.HitmarkerColor);
		if (Config.VisHitmarker)
		{
			zgui::slider_float("expire time", 0, 4, Config.ExpireTime);
			zgui::slider_float("line size", 0, 16, Config.MarkerSize);
			zgui::checkbox("show damage", Config.VisHitDmg);
		}
		zgui::checkbox("desync invert indicator", Config.AAInvertIndicators);
		zgui::end_groupbox();
	}
}

void  Menu::misctab()
{
	zgui::next_column(-401, 39);

	zgui::begin_groupbox("Misc options", { 230 , 520 }, zgui::zgui_window_flags_none);
	{
		zgui::slider_int("field of view", 0, 150, Config.Fov);
		zgui::checkbox("Autojump", Config.Bhop);
		zgui::checkbox("Autostrafe", Config.AutoStrafe);
		zgui::combobox("Hitmarker sound type", { "off", "door", "metallic", "scaleform button", "warning", "bell", "custom", "buble", "fatality" }, Config.Hitmarker);
		//zgui::combobox("Main buy bot", { "NONE", "AUTO", "AWP", "SSG" }, Config.MainWeaponBB);
		//zgui::combobox("Second buy bot", { "NONE", "Deagle/R8", "Duals" }, Config.SecondWeaponBB);
		//zgui::multi_combobox("Other buy bot", std::vector< zgui::multi_select_item >{ { "vesthelm", &Config.OtherBB[0] },
		//	{ "hegrenade", &Config.OtherBB[1] }, { "smoke", &Config.OtherBB[2] }, { "fire nade", &Config.OtherBB[3] },
		//	{ "taser", &Config.OtherBB[4] }, { "defuse kit", &Config.OtherBB[5] } });
		zgui::checkbox("Bullet tracer", Config.Tracer);
		if (Config.Tracer) {
			zgui::slider_int("Tracer life", 0, 5, Config.HitmarkerTime);
			zgui::combobox("Tracer material", { "blueglow1", "bubble", "glow01", "physbeam", "purpleglow1", "purplelaser1", "radio", "white" }, Config.TracerMaterial);
		}
		zgui::checkbox("fuck duck", Config.FakeDuck);
		zgui::key_bind("fake duck key", Config.FakeduckKey);
		//zgui::checkbox("Kill Counter", Config.KillCounter);
		//zgui::checkbox("Shot Counter", Config.shotcounter);
		zgui::checkbox("clantag", Config.Clantag);
		zgui::checkbox("thirdperson", Config.Thirdperson);
		zgui::key_bind("thirdperson key", Config.ThirdpersonKey);
		zgui::checkbox("show impacts", Config.Impacts);
		//zgui::combobox("font", { "small fonts", "visitor" }, Config.Font);
		zgui::checkbox("aspect ratio", Config.AspectRatio);
		if (Config.AspectRatio) {
			zgui::slider_int("value", 0, 100, Config.AspectRatioSlider);
		}
		zgui::end_groupbox();
	}
	zgui::same_line();
	zgui::next_column(230, 39);
	zgui::begin_groupbox("Misc options", { 230 , 300 }, zgui::zgui_window_flags_none);
	{
		//zgui::combobox("Fake Lag Type", { "Factor", "Adaptive", "Switch" }, Config.FakelagType);
		//zgui::slider_int( "Fake Lag Value On Ground", 0, 20, Config.FakeLagValue);
		//zgui::slider_int( "Fake Lag Value In Air", 0, 20, Config.FakeLagValueGround);
	     zgui::checkbox("fakelag", Config.FakeLagEnable);
		if (Config.FakeLagEnable) {
			zgui::checkbox("while shooting", Config.FakeLagShooting);
			zgui::combobox("type", { "Adaptive", "Maximum", "Factor" }, Config.FakelagType);
			if (Config.FakelagType == 2) {
				zgui::slider_int("On move", 0, 16, Config.FakeLagValue);
				zgui::slider_int("On stand", 0, 16, Config.FakelagThreshold);
			}
		}
		
		zgui::combobox(("Slowwalk"), { "off", "antiaim", "speed" }, Config.SlowWalk); // <-- lol
		if (Config.SlowWalk == 2) {
			zgui::slider_int(("speed"), 0, 100, Config.SlowWalkSlider);
		}
		//if (Config.SlowWalk == 3) {
			//zgui::key_bind(("tick count exploit key"), Config.SlowWalkExploitKey); // not worked
		//}
//
		//zgui::combobox("slowwalk", { "off", "antiaim", "speed" }, Config.SlowWalk);
		//if (Config.SlowWalk == 2) {
		//	zgui::slider_int("speed", 0, 100, Config.SlowWalkSlider);
		//}
//
		/*zgui::checkbox("Rackchanger", Config.RankChangerenabled);
		if (Config.RankChangerenabled)
		{
			zgui::checkbox("Rank", Config.RankChangerrank_id);
			zgui::slider_int("Level", 0, 40, Config.player_level);
			zgui::slider_int("Xp", 0, 5000, Config.RankChangerplayer_level_xp);
			zgui::slider_int("Wins", 0, 5000, Config.RankChangerwins);
			zgui::slider_int("Friend", 0, 5000, Config.RankChangerfriendly);
			zgui::slider_int("Teach", 0, 5000, Config.RankChangerteaching);
			zgui::slider_int("Leader", 0, 5000, Config.RankChangerleader);

		}*/

		//zgui::checkbox("aspect ratio", Config.AspectRatio);
		//if (Config.AspectRatio) {
		//	zgui::slider_int("value", 0, 100, Config.AspectRatioSlider);
	    //}
		zgui::end_groupbox();
	}
	zgui::begin_groupbox("exploits", { 230, 210 }, zgui::zgui_window_flags_none); // ez
	{
		//zgui::checkbox("Tick Manipulation", Config.Exploit.TickManipulation); // wtf
		//zgui::key_bind("Tick Manipulation Key", Config.Exploit.TickManipulationKey);
		//zgui::checkbox("Teleportation [double ctrl while using]", Config.Exploit.TickFreeze);
		//zgui::key_bind("Teleportation Key", Config.Exploit.TickFreezeKey); // idk
		//zgui::checkbox("Rcon Lag Exploit [L]", Config.Exploit.RconLagExploit); // not worked
		//zgui::checkbox("Lag Exploit", Config.Exploit.CrashExploit); // not worked
		if (zgui::button("unload", { 50, 20 })) {
			Hooks::Restore();
		}
		zgui::end_groupbox();
	}
}

static bool unload;	
void  Menu::cfgtab()
{
	zgui::next_column(-401, 39);

	zgui::begin_groupbox("configs", { 230 , 520 }, zgui::zgui_window_flags_none);
	{
		zgui::combobox("configs", { "LEGIT", "SEMIRAGE", "RAGE", "EXPEREMENTAL" }, Tabs.Config);
		zgui::end_groupbox();
	}
	zgui::same_line();
	zgui::next_column(230, 39);
	zgui::begin_groupbox("functions", { 230 , 520 }, zgui::zgui_window_flags_none);
	{
		auto cfilter = CHudChat::ChatFilters::CHAT_FILTER_NONE;
		if (Tabs.Config == 0 && zgui::button("load", { 50,20 })) {
			if (g_pEngine->IsInGame() && g_pEngine->IsConnected()) g_ChatElement->ChatPrintf(0, cfilter, " \x02[digitalwave] \x01""config loaded"" ");
			g_Config->Load();
		}
		else if (Tabs.Config == 1 && zgui::button("load", { 50,20 })) {
			if (g_pEngine->IsInGame() && g_pEngine->IsConnected()) g_ChatElement->ChatPrintf(0, cfilter, " \x02[digitalwave] \x01""config loaded"" ");
			g_Config->Load2();
		}
		else if (Tabs.Config == 2 && zgui::button("load", { 50,20 })) {
			if (g_pEngine->IsInGame() && g_pEngine->IsConnected()) g_ChatElement->ChatPrintf(0, cfilter, " \x02[digitalwave] \x01""config loaded"" ");
			g_Config->Load3();
		}
		else if (Tabs.Config == 3 && zgui::button("load", { 50,20 })) {
			if (g_pEngine->IsInGame() && g_pEngine->IsConnected()) g_ChatElement->ChatPrintf(0, cfilter, " \x02[digitalwave] \x01""config loaded"" ");
			g_Config->Load4();
		}

		if (Tabs.Config == 0 && zgui::button(" save", { 50,20 })) {
			if (g_pEngine->IsInGame() && g_pEngine->IsConnected()) g_ChatElement->ChatPrintf(0, cfilter, " \x02[digitalwave] \x01""config saved"" ");
			g_Config->Save();
		}
		else if (Tabs.Config == 1 && zgui::button(" save", { 50,20 })) {
			if (g_pEngine->IsInGame() && g_pEngine->IsConnected()) g_ChatElement->ChatPrintf(0, cfilter, " \x02[digitalwave] \x01""config saved"" ");
			g_Config->Save2();
		}
		else if (Tabs.Config == 2 && zgui::button(" save", { 50,20 })) {
			if (g_pEngine->IsInGame() && g_pEngine->IsConnected()) g_ChatElement->ChatPrintf(0, cfilter, " \x02[digitalwave] \x01""config saved"" ");
			g_Config->Save3();
		}
		else if (Tabs.Config == 3 && zgui::button(" save", { 50,20 })) {
			if (g_pEngine->IsInGame() && g_pEngine->IsConnected()) g_ChatElement->ChatPrintf(0, cfilter, " \x02[digitalwave] \x01""config saved"" ");
			g_Config->Save4();
		}
		zgui::text("");
		std::stringstream ss;
		ss << "Last update: " << __DATE__ << " " << __TIME__; 
		zgui::text(ss.str().c_str()); 
		zgui::end_groupbox();
	}
}

void  Menu::plisttab()
{
	zgui::next_column(-401, 39);

	zgui::begin_groupbox("players", { 470 , 520 }, zgui::zgui_window_flags_none);
	{

		for (int i = 1; i < g_pEngine->GetMaxClients(); ++i)
		{
			static auto sanitize = [](char* name) -> std::string {
				name[127] = '\0';

				std::string tmp(name);

				if (tmp.length() > 20) {
					tmp.erase(20, (tmp.length() - 20));
					tmp.append("...");
				}

				return tmp;
			};

			C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(i);

			if (!pPlayerEntity
				|| !pPlayerEntity->IsAlive()
				|| pPlayerEntity == g::pLocalEntity
				|| pPlayerEntity->GetTeam() == g::pLocalEntity->GetTeam())
			{
				continue;
			}

			Vector oldang = pPlayerEntity->GetEyeAngles();

			PlayerInfo_t player_info;
			if (g_pEngine->GetPlayerInfo(pPlayerEntity->EntIndex(), &player_info)) {
				std::string name = sanitize(player_info.szName);

				std::transform(name.begin(), name.end(), name.begin(), tolower);

				if (pPlayerEntity->IsDormant()) {
					name += " (dormant)";
				}

				std::string pitchmeme = "force baim (id:" + std::to_string(i) + ")";

				zgui::checkbox(name.c_str(), g::plist_player[i]);

				if (g::plist_player[i])
				{
					zgui::checkbox(pitchmeme.c_str(), Config.forcebaim[i]);
				}
			}
		}
		zgui::end_groupbox();
	}

}

void Menu::debugtab()
{
	zgui::next_column(-401, 39);

	zgui::begin_groupbox("1", { 470 , 520 }, zgui::zgui_window_flags_none);
	{
		zgui::text("welcome to the super secret debug menu");
		zgui::text("");
		zgui::checkbox("debug show real (local server only)", Config.DebugRealAngle);
	}
}