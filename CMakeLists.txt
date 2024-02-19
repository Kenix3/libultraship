cmake_minimum_required(VERSION 3.24.0)

option(NON_PORTABLE "Build a non-portable version" OFF)

project(libultraship LANGUAGES C CXX)
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    enable_language(OBJCXX)
    set(CMAKE_OBJC_FLAGS "${CMAKE_OBJC_FLAGS} -fobjc-arc")
    set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -fobjc-arc")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(cmake/Utils.cmake)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND USE_AUTO_VCPKG)
    include(cmake/automate-vcpkg.cmake)

	if ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
		set(VCPKG_TRIPLET x86-windows-static)
		set(VCPKG_TARGET_TRIPLET x86-windows-static)
	elseif ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
		set(VCPKG_TRIPLET x64-windows-static)
		set(VCPKG_TARGET_TRIPLET x64-windows-static)
	endif()

    vcpkg_bootstrap()
    vcpkg_install_packages(zlib bzip2 sdl2 glew libzip)
endif()

add_subdirectory("extern")
add_subdirectory("src")

