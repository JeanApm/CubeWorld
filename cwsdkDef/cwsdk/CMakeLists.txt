cmake_minimum_required (VERSION 3.8)

#add_subdirectory(CWSDK)
project ("cwsdk")

# Add source to this project's executable.
add_library (cwsdk "cube.cpp" "globals.cpp" "cube_funcs.cpp")
target_include_directories (cwsdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (cwsdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cwsdk)
target_include_directories (cwsdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cwsdk/cube)
target_include_directories (cwsdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cwsdk/msvc_bincompat)
target_include_directories (cwsdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cwsdk/plasma)
