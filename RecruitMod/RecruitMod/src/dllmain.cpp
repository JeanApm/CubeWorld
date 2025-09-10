// RecruitMod/src/dllmain.cpp
#include <Windows.h>
#include <atomic>

#include <map>
#include <string>
#include <sstream>
#include <iomanip>

#include "cwsdk/cube.h"

namespace RecruitMod {
    void Init();
    void CmdRecruit();
    void CmdDismiss();
    void CmdParty();
}

static std::atomic<bool> g_running{ true };
static HANDLE g_singletonMutex = nullptr;

static void Log(const char* msg) {
    OutputDebugStringA(msg);
}

DWORD WINAPI ModThread(LPVOID) {
    Log("[RecruitMod] Init()\n");

    // 1) Attendre le GameController
    cube::GameController* gc = nullptr;
    while (!gc) {
        gc = cube::GetGameController();
        Sleep(100);
    }
    char buf[128];
    sprintf_s(buf, "[RecruitMod] GameController: %p\n", gc);
    Log(buf);

    RecruitMod::Init();

    // 2) Heartbeat + 3) Hotkeys F6/F7/F8
    SHORT prevF6 = 0, prevF7 = 0, prevF8 = 0;
    DWORD lastBeat = GetTickCount();

    while (g_running.load()) {
        DWORD now = GetTickCount();
        if (now - lastBeat > 2000) {           // heartbeat toutes les 2s
            Log("[RecruitMod] heartbeat\n");
            lastBeat = now;
        }

        SHORT f6 = GetAsyncKeyState(VK_F6);
        SHORT f7 = GetAsyncKeyState(VK_F7);
        SHORT f8 = GetAsyncKeyState(VK_F8);

        // détection d’appui (front montant)
        if ((f6 & 0x8001) && !(prevF6 & 0x8001)) {
            Log("[RecruitMod] Hotkey F6\n");
            RecruitMod::CmdRecruit();
        }
        if ((f7 & 0x8001) && !(prevF7 & 0x8001)) {
            Log("[RecruitMod] Hotkey F7\n");
            RecruitMod::CmdDismiss();
        }
        if ((f8 & 0x8001) && !(prevF8 & 0x8001)) {
            Log("[RecruitMod] Hotkey F8\n");
            RecruitMod::CmdParty();
        }

        prevF6 = f6; prevF7 = f7; prevF8 = f8;
        Sleep(16); // ~60 Hz
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);

        g_singletonMutex = CreateMutexA(nullptr, TRUE, "RecruitMod.SingleInstance");
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            // Déjà lancé : on sort proprement
            return TRUE;
        }

        CreateThread(nullptr, 0, ModThread, nullptr, 0, nullptr);
    }
    else if (reason == DLL_PROCESS_DETACH) {
        if (g_singletonMutex) { CloseHandle(g_singletonMutex); g_singletonMutex = nullptr; }
        g_running = false;
    }
    return TRUE;
}
