include(FetchContent)

# The Wii U port uses SDL2 for audio and shared ImGui types even though its
# renderer is GX2 rather than SDL/OpenGL.
find_package(SDL2 REQUIRED)
list(APPEND ADDITIONAL_LIB_INCLUDES ${SDL2_INCLUDE_DIRS})

# devkitPro provides the binary port libraries (libzip and tinyxml2), but does
# not currently package nlohmann-json or spdlog for CafeOS.  Match the pinned
# fallback versions used by the upstream iOS dependency route.
find_package(nlohmann_json QUIET)
if (NOT nlohmann_json_FOUND)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(spdlog QUIET)
if (NOT spdlog_FOUND)
    set(spdlog_wiiu_patch_file ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/patches/spdlog-wiiu-no-tls.patch)
    set(spdlog_wiiu_apply_patch_command ${CMAKE_COMMAND} -Dpatch_file=${spdlog_wiiu_patch_file} -Dwith_reset=FALSE -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/git-patch.cmake)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.14.1
        OVERRIDE_FIND_PACKAGE
        PATCH_COMMAND ${spdlog_wiiu_apply_patch_command}
    )
    FetchContent_MakeAvailable(spdlog)
endif()
