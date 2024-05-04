find_package(SDL2 QUIET)
if (NOT ${SDL2_FOUND})
    include(FetchContent)
    FetchContent_Declare(
        SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.28.1 
    )
    message("SDL2 not found. Downloading now...")
    FetchContent_MakeAvailable(SDL2)
    message("SDL2 downloaded to " ${FETCHCONTENT_BASE_DIR}/sdl2-src)
endif()

target_link_libraries(ImGui PUBLIC SDL2::SDL2)
