#=================== Top-Level ===================
file(GLOB Source_Files__TopLevel RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")

source_group("" FILES ${Source_Files__TopLevel})
target_sources(libultraship PRIVATE ${Source_Files__TopLevel})

#=================== Debug ===================

file(GLOB Source_Files__Debug RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "debug/*.cpp")

source_group("debug" FILES ${Source_Files__Debug})
target_sources(libultraship PRIVATE ${Source_Files__Debug})

#=================== Resource ===================

file(GLOB Source_Files__Resource__Types RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "resource/type/*.h" "resource/type/*.cpp")
source_group("resource/type" FILES ${Source_Files__Resource__Types})

file(GLOB Source_Files__Resource__Factories RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "resource/factory/*.h" "resource/factory/*.cpp")
source_group("resource/factory" FILES ${Source_Files__Resource__Factories})

target_sources(libultraship PRIVATE ${Source_Files__Resource} ${Source_Files__Resource__Types} ${Source_Files__Resource__Factories} ${Source_Files__Resource__Archive})

#=================== Graphic ===================

file(GLOB Source_Files__Graphic RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "backends/*.cpp")

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "^backends/gfx_dxgi.*\\.cpp$")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "^backends/gfx_direct3d.*\\.cpp$")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "graphic/Fast3D/backends/dxsdk/*")
endif()

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "^backends/gfx_metal.*\\.cpp$")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "^backends/gfx_opengl.*\\.cpp$")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "^backends/gfx_sdl2\\.cpp$")
else()
    # GX2/WUT sources are compiled only for CaféOS.  Keeping them out of the
    # host build is necessary because ExtractAssets is a native desktop target.
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "^backends/gfx_gx2\\.cpp$")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "^backends/gfx_wiiu\\.cpp$")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "^backends/imgui_impl_gx2\\.cpp$")
endif()

source_group("graphic" FILES ${Source_Files__Graphic})
target_sources(libultraship PRIVATE ${Source_Files__Graphic})

if (CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    # The implementation has a .c extension but consumes the C++ Fast3D interface.
    # Compile it through the C++ wrapper because devkitPro classifies .c as C unconditionally.
    set(GX2_SHADER_GENERATOR ${CMAKE_CURRENT_SOURCE_DIR}/backends/gx2_shader_gen.cpp)
    target_sources(libultraship PRIVATE ${GX2_SHADER_GENERATOR})
endif()
