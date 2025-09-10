cmake_minimum_required(VERSION 3.20)
project(RecruitMod LANGUAGES CXX)

# ==== Config globale ====
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)

# Runtime /MD ou /MDd suivant la config (MSVC)
if (MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# ==== Chemin vers le CWSDKDEF ====
# passe-le en -DCWSKD_ROOT="C:/Users/Etern/Desktop/Dev/cwsdkDef"
if (NOT DEFINED CWSKD_ROOT)
  if (DEFINED ENV{CWSKD_ROOT})
    set(CWSKD_ROOT "$ENV{CWSKD_ROOT}")
  else()
    message(FATAL_ERROR "CWSKD_ROOT non défini (passez -DCWSKD_ROOT=chemin/vers/cwsdkDef)")
  endif()
endif()

# ==== Bibliothèque CWSDK (sources du SDK) ====
file(GLOB CWSKD_CPP
  "${CWSKD_ROOT}/cwsdk/*.cpp"
  "${CWSKD_ROOT}/cwsdk/cube/*.cpp"
)

add_library(cwsdk STATIC ${CWSKD_CPP})
target_include_directories(cwsdk PUBLIC "${CWSKD_ROOT}")

# Définitions pour aligner le layout MSVC et couper le bruit warnings
if (MSVC)
  target_compile_definitions(cwsdk PRIVATE
    _ITERATOR_DEBUG_LEVEL=0
    _HAS_ITERATOR_DEBUGGING=0
    _SECURE_SCL=0
    _SCL_SECURE_NO_WARNINGS
    _CRT_SECURE_NO_WARNINGS
    NDEBUG
    # 👉 Décommente si tu veux désactiver les assert_size le temps de tester
    # CWSKD_NO_SIZE_ASSERT=1
  )
  target_compile_options(cwsdk PRIVATE /EHsc /W4 /Zp8)
endif()

# ==== Notre mod ====
add_subdirectory(RecruitMod)
