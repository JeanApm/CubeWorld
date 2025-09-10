param(
  [string]$ModName = "RecruitMod",
  # Chemin racine du CWSDK alpha (le dossier qui contient "cwsdk/" et "main.cpp")
  [string]$CWSdkRoot = "C:\dev\cwsdk-alpha",
  # (Optionnel) Dossier du jeu Cube World pour copie auto du .dll
  [string]$CubeWorldDir = ""
)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path ".").Path
$ModDir = Join-Path $Root $ModName
$SrcDir = Join-Path $ModDir "src"
$CMakeDir = Join-Path $Root "cmake"

# --- Dossiers ---
New-Item -ItemType Directory -Force -Path $SrcDir | Out-Null
New-Item -ItemType Directory -Force -Path $CMakeDir | Out-Null

# --- .gitignore minimal ---
@'
/build/
/out/
/CMakeSettings.json
/*.user
/*.suo
/*.vcxproj*
'@ | Set-Content -Encoding UTF8 (Join-Path $Root ".gitignore")

# --- README ---
@"
# $ModName (CWSDK)

Build:
  cmake -S . -B build -A Win32 -DCWSKD_ROOT="$CWSdkRoot"
  cmake --build build --config Release

Déploiement:
  - Si -CubeWorldDir est passé au script, le .dll est copié dans `\$CubeWorldDir\Mods\`.
"@ | Set-Content -Encoding UTF8 (Join-Path $Root "README.md")

# --- CMake racine ---
@"
cmake_minimum_required(VERSION 3.20)
project($ModName LANGUAGES CXX)

# Config globale
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX -DWIN32_LEAN_AND_MEAN)

# Chemin CWSDK (passé en -DCWSKD_ROOT=... ou variable d'env)
if(NOT DEFINED CWSKD_ROOT)
  if(DEFINED ENV{CWSKD_ROOT})
    set(CWSKD_ROOT "$ENV{CWSKD_ROOT}")
  else()
    message(FATAL_ERROR "CWSKD_ROOT non défini (passer -DCWSKD_ROOT=path)")
  endif()
endif()

# Sources CWSDK (cpp fournis par le SDK)
file(GLOB CWSKD_CPP
  "\${CWSKD_ROOT}/cwsdk/*.cpp"
  "\${CWSKD_ROOT}/cwsdk/cube/*.cpp"
)

add_library(cwsdk STATIC \${CWSKD_CPP})
target_include_directories(cwsdk PUBLIC "\${CWSKD_ROOT}")

# Notre mod
add_subdirectory($ModName)
"@ | Set-Content -Encoding UTF8 (Join-Path $Root "CMakeLists.txt")

# --- CMake du mod ---
@"
# $ModName/CMakeLists.txt
add_library($ModName SHARED
  src/dllmain.cpp
  src/RecruitMod_CWSDK.cpp
)

target_include_directories($ModName PRIVATE "\${CWSKD_ROOT}")
target_link_libraries($ModName PRIVATE cwsdk)

# Options MSVC
if (MSVC)
  target_compile_options($ModName PRIVATE /W4 /EHsc)
  # évite l'UNDEF CRT
  target_compile_definitions($ModName PRIVATE _CRT_SECURE_NO_WARNINGS)
  set_target_properties($ModName PROPERTIES OUTPUT_NAME "$ModName")
endif()

# Copie auto vers le dossier du jeu si fourni à l'étape CMake (optionnelle)
if(DEFINED CUBEWORLD_DIR)
  add_custom_command(TARGET $ModName POST_BUILD
    COMMAND \${CMAKE_COMMAND} -E make_directory "\${CUBEWORLD_DIR}/Mods"
    COMMAND \${CMAKE_COMMAND} -E copy_if_different
      "\$<TARGET_FILE:$ModName>"
      "\${CUBEWORLD_DIR}/Mods/\$<TARGET_FILE_NAME:$ModName>"
  )
endif()
"@ | Set-Content -Encoding UTF8 (Join-Path $ModDir "CMakeLists.txt")

# --- dllmain.cpp ---
@'
#include <Windows.h>
#include "cwsdk/cube.h"
#include "RecruitMod_CWSDK.h"

static HANDLE g_thread = nullptr;

static DWORD WINAPI ModThread(LPVOID) {
    // Laisse le jeu démarrer
    Sleep(3000);
    RecruitMod::Init();

    // Boucle tick (~10 Hz)
    while (true) {
        RecruitMod::Tick();
        Sleep(100);
    }
    return 0;
}

extern "C" __declspec(dllexport) void OnChatCommand(const wchar_t* cmd) {
    if (!cmd) return;
    if (_wcsicmp(cmd, L"/recruit") == 0) { RecruitMod::CmdRecruit(); return; }
    if (_wcsicmp(cmd, L"/dismiss") == 0) { RecruitMod::CmdDismiss(); return; }
    if (_wcsicmp(cmd, L"/party")   == 0) { RecruitMod::CmdParty();   return; }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        g_thread = CreateThread(nullptr, 0, ModThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
'@ | Set-Content -Encoding UTF8 (Join-Path $SrcDir "dllmain.cpp")

# --- RecruitMod_CWSDK.h ---
@"
#pragma once
#include <vector>
#include <cstdint>

namespace RecruitMod {
    void Init();
    void Tick();       // ~10–20 Hz
    void CmdRecruit(); // /recruit
    void CmdDismiss(); // /dismiss
    void CmdParty();   // /party
}
"@ | Set-Content -Encoding UTF8 (Join-Path $SrcDir "RecruitMod_CWSDK.h")

# --- RecruitMod_CWSDK.cpp ---
@'
#include "RecruitMod_CWSDK.h"
#include "cwsdk/cube.h"
#include <cmath>

using namespace cube;

namespace {
    struct Recruit {
        cube::Creature* c = nullptr;
        uint64_t guid = 0;
    };
    std::vector<Recruit> g_party;

    inline GameController* GC() { return cube::GetGameController(); }

    inline double DistBlocks(cube::Creature* a, cube::Creature* b) {
        if (!a || !b) return 1e9;
        return a->DistanceFrom(b) / 65536.0; // 1 m = 65536
    }

    cube::Creature* FindNearestTarget(cube::Creature* player, double maxDist = 30.0) {
        auto gc = GC();
        if (!gc || !player) return nullptr;

        auto snapshot = gc->world.EntitesMap->CopyToSTDMap();
        cube::Creature* best = nullptr;
        double bestD = maxDist + 1.0;

        for (auto& kv : snapshot) {
            cube::Creature* c = kv.second;
            if (!c || c == player) continue;
            double d = DistBlocks(player, c);
            if (d < bestD && d <= maxDist) { bestD = d; best = c; }
        }
        return best;
    }

    void MakeFriendly(cube::Creature* target, cube::Creature* player) {
        if (!target || !player) return;
        // Copie hostilité + ownership "pet-like"
        target->entity_data.hostility_flags = player->entity_data.hostility_flags;
        target->entity_data.parent_owner = player->GUID;
        // NOTE: si tu veux l'AI compagnon native, appelle la routine world_init_companion (voir cube_util.cpp).
    }

    void TeleportBehindIfFar(cube::Creature* follower, cube::Creature* player, double far = 10.0) {
        if (!follower || !player) return;
        auto p = player->entity_data.position;
        auto f = follower->entity_data.position;

        double dx = double(p.X) - double(f.X);
        double dy = double(p.Y) - double(f.Y);
        double dz = double(p.Z) - double(f.Z);
        double d  = std::sqrt(dx*dx + dy*dy + dz*dz) / 65536.0;

        if (d > far) {
            const int64_t M = 65536; // 1 m
            auto behind = p;
            behind.X -= 2 * M;
            behind.Z -= 1 * M;
            follower->entity_data.position = behind;
        }
    }
}

void RecruitMod::Init() {
    g_party.clear();
}

void RecruitMod::CmdRecruit() {
    auto gc = GC();
    if (!gc || !gc->local_player) return;
    auto ply = gc->local_player;

    auto npc = FindNearestTarget(ply, 30.0);
    if (!npc) {
        if (gc->ChatWidget) gc->ChatWidget->Print(L"[Recruit] Aucune cible <=30m.", Color::Red());
        return;
    }

    MakeFriendly(npc, ply);
    g_party.push_back({ npc, npc->GUID });

    if (gc->ChatWidget) {
        std::wstring msg = L"[Recruit] + ";
        msg += npc->GetName();
        gc->ChatWidget->Print(msg, Color::Green());
    }
}

void RecruitMod::CmdDismiss() {
    auto gc = GC();
    if (!gc) return;

    if (g_party.empty()) {
        if (gc->ChatWidget) gc->ChatWidget->Print(L"[Recruit] Party vide.", Color::White());
        return;
    }
    auto r = g_party.back(); g_party.pop_back();
    if (r.c) r.c->entity_data.parent_owner = 0;

    if (gc->ChatWidget) gc->ChatWidget->Print(L"[Recruit] Dernier renvoyé.", Color::White());
}

void RecruitMod::CmdParty() {
    auto gc = GC();
    if (!gc) return;
    wchar_t buf[128];
    swprintf_s(buf, L"[Recruit] Party size = %zu", g_party.size());
    if (gc->ChatWidget) gc->ChatWidget->Print(buf, Color::White());
}

void RecruitMod::Tick() {
    auto gc = GC();
    if (!gc || !gc->local_player || g_party.empty()) return;
    auto ply = gc->local_player;

    for (auto& r : g_party) {
        if (r.c) TeleportBehindIfFar(r.c, ply, 10.0);
    }
}
'@ | Set-Content -Encoding UTF8 (Join-Path $SrcDir "RecruitMod_CWSDK.cpp")

# --- Conseils build (PS helper) ---
@"
# Tips:
# 1) Génération Win32:
#    cmake -S . -B build -A Win32 -DCWSKD_ROOT=""$CWSdkRoot"" -DCUBEWORLD_DIR=""$CubeWorldDir""
# 2) Build:
#    cmake --build build --config Release
# 3) Le .dll sera dans build\$ModName\Release\$ModName.dll
"@ | Set-Content -Encoding UTF8 (Join-Path $Root "BUILD_NOTES.txt")

# --- Fin : message & build cmd prêt ---
Write-Host "[OK] Squelette $ModName créé dans '$ModDir'."
Write-Host "Pour générer (Win32) :"
Write-Host "  cmake -S . -B build -A Win32 -DCWSKD_ROOT=""$CWSdkRoot""" -ForegroundColor Cyan
if ($CubeWorldDir -ne "") {
  Write-Host "  ...avec copie auto : ajoute -DCUBEWORLD_DIR=""$CubeWorldDir""" -ForegroundColor Cyan
}
Write-Host "Puis :  cmake --build build --config Release" -ForegroundColor Cyan
