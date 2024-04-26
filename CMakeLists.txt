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

include(cmake/dependencies/common.cmake)
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
	include(cmake/dependencies/android.cmake)
endif()
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	include(cmake/dependencies/windows.cmake)
endif()
if (CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
	include(cmake/dependencies/switch.cmake)
endif()

add_subdirectory("extern")
add_subdirectory("src")

