cmake_minimum_required(VERSION 3.16.0)

project(libultraship LANGUAGES C CXX)
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    enable_language(OBJCXX)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(cmake/Utils.cmake)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND USE_AUTO_VCPKG)
    include(cmake/automate-vcpkg.cmake)

    set(VCPKG_TRIPLET x64-windows-static)
    set(VCPKG_TARGET_TRIPLET x64-windows-static)

    vcpkg_bootstrap()
    vcpkg_install_packages(zlib bzip2 SDL2 GLEW)
endif()

add_subdirectory("extern")
add_subdirectory("src")

