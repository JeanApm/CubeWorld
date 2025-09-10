# DLL du mod
add_library(RecruitMod SHARED
  src/dllmain.cpp
  src/RecruitMod_CWSDK.cpp
)

target_include_directories(RecruitMod PRIVATE "${CWSKD_ROOT}")
target_link_libraries(RecruitMod PRIVATE cwsdk)

# Définitions identiques au SDK (évite un layout différent dans nos TU)
target_compile_definitions(RecruitMod PRIVATE
  _ITERATOR_DEBUG_LEVEL=0 _HAS_ITERATOR_DEBUGGING=0 _SECURE_SCL=0
  _SCL_SECURE_NO_WARNINGS _CRT_SECURE_NO_WARNINGS NDEBUG
  # 👉 Décommente si tu désactives les asserts côté lib
  # CWSKD_NO_SIZE_ASSERT=1
)

if (MSVC)
  target_compile_options(RecruitMod PRIVATE /W4 /EHsc)
  set_target_properties(RecruitMod PROPERTIES OUTPUT_NAME "RecruitMod")
endif()

# Copie auto dans le dossier du jeu (optionnel)
# Passe -DCUBEWORLD_DIR="C:/Users/Etern/Desktop/Dev/Cube World" à la config
if(DEFINED CUBEWORLD_DIR)
  add_custom_command(TARGET RecruitMod POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CUBEWORLD_DIR}/Mods"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:RecruitMod>"
            "${CUBEWORLD_DIR}/Mods/$<TARGET_FILE_NAME:RecruitMod>"
  )
endif()
