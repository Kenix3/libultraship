cmake_minimum_required(VERSION 3.24.0)

option(NON_PORTABLE "Build a non-portable version" OFF)
option(SIGN_LIBRARY "Enable xcode signing" OFF)
option(BUNDLE_ID "Bundle ID for xcode signing" "com.example.libultraship")

project(libultraship LANGUAGES C CXX)
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    enable_language(OBJCXX)
    set(CMAKE_OBJC_FLAGS "${CMAKE_OBJC_FLAGS} -fobjc-arc")
    set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -fobjc-arc")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(cmake/Utils.cmake)
set(ADDITIONAL_LIB_INCLUDES "")

include(cmake/dependencies/common.cmake)
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
	include(cmake/dependencies/android.cmake)
elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(PLATFORM "OS64")
    include(cmake/dependencies/ios.cmake)
    include(cmake/ios.toolchain.cmake)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    include(cmake/dependencies/windows.cmake)
endif()

add_subdirectory("extern")
add_subdirectory("src")
