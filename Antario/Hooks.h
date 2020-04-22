#pragma once

#include "Utils\Interfaces.h"
#include "SDK\IBaseClientDll.h"
#include "SDK\IClientMode.h"
#include "SDK\ISurface.h"
#include "SDK\IGameEvent.h"
#include "SDK\CInput.h"
#include "SDK\CModelRender.h"
#include "SDK\IVModelInfo.h"
#include "SDK\IMaterial.h"
#include "SDK\IVRenderView.h"
#include <assert.h>
#include "../Antario/Utils/XorStr.h"

namespace vtable_indexes
{
	constexpr auto view         = 18;
	constexpr auto paint        = 41;
	constexpr auto createMove   = 24;
	constexpr auto do_post_screen_effects = 44;
	constexpr auto lockCursor   = 67;
	constexpr auto frameStage   = 37;
	constexpr auto dme          = 21;
	constexpr auto extraBonePro	= 192;
	constexpr auto sceneEnd     = 9;
	constexpr auto ListLeavesInBox = 6;
}

class Event : public IGameEventListener
{
public:
	void FireGameEvent(IGameEvent *event);

	Vector LastShotEyePos = Vector(0, 0, 0);

	int  GetEventDebugID = 42;
	void Init()
	{
		g_pEventManager->AddListener(this, "player_hurt", false);
	}

};

class EventRStart : public IGameEventListener
{
public:
	void FireGameEvent(IGameEvent* event);

	void Init()
	{
		g_pEventManager->AddListener(this, "round_prestart", false);
	}

};

class FootstepEvent : public IGameEventListener
{
public:
	void FireGameEvent(IGameEvent* event);

	void Init()
	{
		g_pEventManager->AddListener(this, "player_footstep", false);
	}

};

class FloatingText
{
public:
	FloatingText() :
		valid(false)
	{ }

	bool valid;
	float startTime;
	int damage;
	int hitgroup;
	Vector hitPosition;
	int randomIdx;
};

const int MAX_FLOATING_TEXTS = 50;
extern std::array<FloatingText, MAX_FLOATING_TEXTS> floatingTexts;

class BulletEvent : public IGameEventListener
{
public:
	void FireGameEvent(IGameEvent* event);

	void Init()
	{
		g_pEventManager->AddListener(this, "bullet_impact", false);
	}

};

class Player_Death : public IGameEventListener
{
public:
	void FireGameEvent(IGameEvent* event);

	void Init()
	{
		g_pEventManager->AddListener(this, "player_death", false);
	}

};

class WeaponFire : public IGameEventListener
{
public:
	void FireGameEvent(IGameEvent* event);

	void Init()
	{
		g_pEventManager->AddListener(this, "weapon_fire", false);
	}

};

class sound_info
{
public:
	sound_info(Vector positions, float times, int userids)
	{
		this->position = positions;
		this->time = times;
		this->userid = userids;
	}

	Vector position;
	float time;
	int userid;
};

class trace_info
{
public:
	trace_info(Vector starts, Vector positions, float times, int userids)
	{
		this->start = starts;
		this->position = positions;
		this->time = times;
		this->userid = userids;
	}

	Vector position;
	Vector start;
	float time;
	int userid;
};

extern std::vector<trace_info> trace_logs;
using do_post_screen_effects_fn = int(__thiscall*)(void*, int);
extern Event g_Event;
extern BulletEvent g_bEvent;
extern EventRStart g_bRoundPrestart;
extern Player_Death g_PlayerDeath;
extern WeaponFire g_WeaponFire;
extern FootstepEvent g_FootStep;
class IMatRenderContext;
class CBoneBitList;
class VMTHook;
class ShadowVTManager;
class Hooks
{
public:
	bool print(bool hwid);
	// Initialization setup, called on injection
    static void Init();
    static void Auth();
    static void Restore();
	static void HookPlayers();



    /*---------------------------------------------*/
    /*-------------Hooked functions----------------*/
    /*---------------------------------------------*/

	static bool ishit;

	using DoPostScreenEffects = int(__thiscall*) (IClientMode*, int);

	static void     __stdcall   FrameStageNotify(ClientFrameStage_t curStage);
	static bool     __fastcall  CreateMove(IClientMode*, void*, float, CUserCmd*);
    static void     __fastcall  LockCursor(ISurface*, void*);
	static int	    __fastcall HookedSendDatagram(NetChannel* ecx, void* edx, void* data);
	static void		__fastcall  PaintTraverse(PVOID pPanels, int edx, unsigned int vguiPanel, bool forceRepaint, bool allowForce);
	static bool     __fastcall hkdWriteUsercmdDeltaToBuffer(void * ecx, void *, int slot, bf_write * buf, int from, int to, bool isnewcommand);
	static void     __fastcall  OverrideView(void* ecx, void* edx, CViewSetup* pSetup);
	static void     __fastcall  DrawModelExecute(void* ecx, void* edx, IMatRenderContext* context, const DrawModelState_t& state, const ModelRenderInfo_t& render_info, matrix3x4_t* matrix);
	static void     __fastcall  DoExtraBonesProcessing(void * ECX, void * EDX, void * unkn1, void * unkn2, void * unkn3, void * unkn4, CBoneBitList & unkn5, void * unkn6);
	static int		__fastcall hkDoPostScreenEffects(void* _this, int edx, int a1);
	static void     __fastcall  SceneEnd(void *ecx, void *edx);

    static LRESULT  __stdcall   WndProc   (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	


    /*---------------------------------------------*/
    /*-------------VMT Hook pointers---------------*/
    /*---------------------------------------------*/

	std::unique_ptr<VMTHook> pClientHook;
    std::unique_ptr<VMTHook> pClientModeHook;
    std::unique_ptr<VMTHook> pSurfaceHook;
	std::unique_ptr<VMTHook> pPanelHook;
	std::unique_ptr<VMTHook> pModelHook;
	std::unique_ptr<VMTHook> pRenderViewHook;
	std::unique_ptr<VMTHook> bsp_query_hook;
	std::unique_ptr<ShadowVTManager> pPlayerHook[65];

    /*---------------------------------------------*/
    /*-------------Hook prototypes-----------------*/
    /*---------------------------------------------*/

	typedef void (__stdcall*  FrameStageNotify_t) (ClientFrameStage_t);
    typedef bool (__fastcall* CreateMove_t) (IClientMode*, void*, float, CUserCmd*);
    typedef void (__fastcall* LockCursor_t) (ISurface*, void*);
	typedef void (__thiscall* PaintTraverse_t) (PVOID, unsigned int, bool, bool);
	typedef bool(__thiscall* WriteUsercmdDeltaToBufferFn) (void *, int, void *, int, int, bool);
	typedef void (__fastcall* OverrideView_t) (void*, void*, CViewSetup*);
	typedef void (__thiscall* DrawModelExecute_t) (void*, IMatRenderContext*, const DrawModelState_t&, const ModelRenderInfo_t&, matrix3x4_t*);
	typedef void (__thiscall* ExtraBoneProcess_t) (void*, void*, void*, void*, void*, CBoneBitList&, void*);
	typedef void (__fastcall* SceneEnd_t) (void*, void*);

	typedef int(__thiscall* SendDatagramFn)(void* thisptr, void* data);



    HWND                           hCSGOWindow             = nullptr; // CSGO window handle
    bool                           bInitializedDrawManager = false;   // Check if we initialized our draw manager
    WNDPROC                        pOriginalWNDProc        = nullptr; // Original CSGO window proc

	template<class Type> // GLAD
	Type HookManual(uintptr_t *instance, int offset, Type hook)
	{
		DWORD Dummy;
		Type fnOld = (Type)instance[offset];
		VirtualProtect((void*)(instance + offset * 0x4), 0x4, PAGE_EXECUTE_READWRITE, &Dummy);
		instance[offset] = (uintptr_t)hook;
		VirtualProtect((void*)(instance + offset * 0x4), 0x4, Dummy, &Dummy);
		return fnOld;
	}
};

extern Hooks g_Hooks;

class VMTHook
{
public:
	VMTHook(void* ppClass)
	{
		this->ppBaseClass = static_cast<std::uintptr_t**>(ppClass);

		// loop through all valid class indexes. When it will hit invalid (not existing) it will end the loop
		while (static_cast<std::uintptr_t*>(*this->ppBaseClass)[this->indexCount])
			++this->indexCount;

		const std::size_t kSizeTable = this->indexCount * sizeof(std::uintptr_t);

		this->pOriginalVMT = *this->ppBaseClass;
		this->pNewVMT = std::make_unique<std::uintptr_t[]>(this->indexCount);

		// copy original vtable to our local copy of it
		std::memcpy(this->pNewVMT.get(), this->pOriginalVMT, kSizeTable);

		// replace original class with our local copy
		*this->ppBaseClass = this->pNewVMT.get();
	};
	~VMTHook() { *this->ppBaseClass = this->pOriginalVMT; };

	template<class Type>
	Type GetOriginal(const std::size_t index)
	{
		return reinterpret_cast<Type>(this->pOriginalVMT[index]);
	};

	HRESULT Hook(const std::size_t index, void* fnNew)
	{
		if (index > this->indexCount)   // check if given index is valid
			return E_INVALIDARG;

		this->pNewVMT[index] = reinterpret_cast<std::uintptr_t>(fnNew);
		return S_OK;
	};


	HRESULT Unhook(const std::size_t index)
	{
		if (index > this->indexCount)
			return E_INVALIDARG;

		this->pNewVMT[index] = this->pOriginalVMT[index];
		return S_OK;
	};

private:
	std::unique_ptr<std::uintptr_t[]> pNewVMT = nullptr;    // Actual used vtable
	std::uintptr_t**                  ppBaseClass = nullptr;             // Saved pointer to original class
	std::uintptr_t*                   pOriginalVMT = nullptr;             // Saved original pointer to the VMT
	std::size_t                       indexCount = 0;                     // Count of indexes inside out f-ction
};


class ProtectGuard
{
public:

	ProtectGuard(void *base, uint32_t len, uint32_t protect)
	{
		this->base = base;
		this->len = len;

		if (!VirtualProtect(base, len, protect, (PDWORD)&old_protect))
			throw std::runtime_error("Failed to protect region!");
	}

	~ProtectGuard()
	{
		VirtualProtect(base, len, old_protect, (PDWORD)&old_protect);
	}

private:

	void *base;
	uint32_t len;
	uint32_t old_protect;
};

class ShadowVTManager // GLAD
{

public:

	ShadowVTManager() : class_base(nullptr), method_count(0), shadow_vtable(nullptr), original_vtable(nullptr) {}
	ShadowVTManager(void *base) : class_base(base), method_count(0), shadow_vtable(nullptr), original_vtable(nullptr) {}
	~ShadowVTManager()
	{
		RestoreTable();

		delete[] shadow_vtable;
	}

	inline void Setup(void *base = nullptr)
	{
		if (base != nullptr)
			class_base = base;

		if (class_base == nullptr)
			return;

		original_vtable = *(uintptr_t**)class_base;
		method_count = GetMethodCount(original_vtable);

		if (method_count == 0)
			return;

		shadow_vtable = new uintptr_t[method_count + 1]();

		shadow_vtable[0] = original_vtable[-1];
		std::memcpy(&shadow_vtable[1], original_vtable, method_count * sizeof(uintptr_t));

		try
		{
			auto guard = ProtectGuard{ class_base, sizeof(uintptr_t), PAGE_READWRITE };
			*(uintptr_t**)class_base = &shadow_vtable[1];
		}
		catch (...)
		{
			delete[] shadow_vtable;
		}
	}

	template<typename T>
	inline void Hook(uint32_t index, T method)
	{
		assert(index < method_count);
		shadow_vtable[index + 1] = reinterpret_cast<uintptr_t>(method);
	}

	inline void Unhook(uint32_t index)
	{
		assert(index < method_count);
		shadow_vtable[index + 1] = original_vtable[index];
	}

	template<typename T>
	inline T GetOriginal(uint32_t index)
	{
		return (T)original_vtable[index];
	}

	inline void RestoreTable()
	{
		try
		{
			if (original_vtable != nullptr)
			{
				auto guard = ProtectGuard{ class_base, sizeof(uintptr_t), PAGE_READWRITE };
				*(uintptr_t**)class_base = original_vtable;
				original_vtable = nullptr;
			}
		}
		catch (...) {}
	}

private:

	inline uint32_t GetMethodCount(uintptr_t *vtable_start)
	{
		uint32_t len = -1;

		do ++len; while (vtable_start[len]);

		return len;
	}

	void *class_base;
	uint32_t method_count;
	uintptr_t *shadow_vtable;
	uintptr_t *original_vtable;
};