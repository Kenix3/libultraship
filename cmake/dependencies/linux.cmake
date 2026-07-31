#=================== ImGui ===================
find_package(SDL3 QUIET CONFIG COMPONENTS SDL3)
if(NOT SDL3_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.12
        OVERRIDE_FIND_PACKAGE
    )
    message("SDL3 not found. Downloading now...")
    FetchContent_MakeAvailable(SDL3)
endif()
target_link_libraries(ImGui PUBLIC SDL3::SDL3)

if (USE_OPENGLES)
    target_link_libraries(ImGui PUBLIC ${OPENGL_GLESv2_LIBRARY})
    add_compile_definitions(IMGUI_IMPL_OPENGL_ES3)
else()
    target_link_libraries(ImGui PUBLIC ${OPENGL_opengl_LIBRARY})
endif()
