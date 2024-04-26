#=================== SDL2 ===================
find_package(SDL2 QUIET)
if (NOT ${SDL2_FOUND})
    include(FetchContent)
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.28.1
        OVERRIDE_FIND_PACKAGE
    )
    message("SDL2 not found. Downloading now...")
    FetchContent_MakeAvailable(SDL2)
    message("SDL2 downloaded to " ${FETCHCONTENT_BASE_DIR}/sdl2-src)
endif()

#=================== nlohmann-json ===================
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    OVERRIDE_FIND_PACKAGE
)
message("nlohmann_json not found. Downloading now...")
FetchContent_MakeAvailable(nlohmann_json)
message("nlohmann_json downloaded to " ${nlohmann_json_SOURCE_DIR})

#=================== libzip ===================
set(BUILD_TOOLS OFF)
set(BUILD_REGRESS OFF)
set(BUILD_EXAMPLES OFF)
set(BUILD_DOC OFF)
set(BUILD_OSSFUZZ OFF)
set(BUILD_SHARED_LIBS OFF)
FetchContent_Declare(
    libzip
    GIT_REPOSITORY https://github.com/nih-at/libzip.git
    GIT_TAG v1.10.1
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(libzip)
list(APPEND ADDITIONAL_LIB_INCLUDES ${libzip_SOURCE_DIR}/lib ${libzip_BINARY_DIR})