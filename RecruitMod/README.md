# RecruitMod (CWSDK)

Build:
  cmake -S . -B build -A Win32 -DCWSKD_ROOT="C:\dev\cwsdk-alpha"
  cmake --build build --config Release

DÃ©ploiement:
  - Si -CubeWorldDir est passÃ© au script, le .dll est copiÃ© dans \\Mods\.
