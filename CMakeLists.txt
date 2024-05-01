cmake_minimum_required(VERSION 3.24.0)

option(NON_PORTABLE "Build a non-portable version" OFF)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    option(SIGN_LIBRARY "Enable xcode signing" OFF)
    option(BUNDLE_ID "Bundle ID for xcode signing" "com.example.libultraship")
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
endif()

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

# ======== Dependencies ================
include(cmake/dependencies/common.cmake)

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
	include(cmake/dependencies/android.cmake)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include(cmake/dependencies/mac.cmake)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    include(cmake/dependencies/ios.cmake)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(cmake/dependencies/linux.cmake)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    include(cmake/dependencies/windows.cmake)
endif()



if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(PLATFORM "OS64COMBINED")
    include(FetchContent)
    FetchContent_Declare(iostoolchain
        GIT_REPOSITORY https://github.com/leetal/ios-cmake
        GIT_TAG 06465b27698424cf4a04a5ca4904d50a3c966c45
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
    )
    FetchContent_GetProperties(iostoolchain)
    if(NOT iostoolchain_POPULATED)
        FetchContent_Populate(iostoolchain)
    endif()
    set(CMAKE_IOS_TOOLCHAIN_FILE ${iostoolchain_SOURCE_DIR}/ios.toolchain.cmake)
    set_property(GLOBAL PROPERTY IOS_TOOLCHAIN_FILE ${CMAKE_IOS_TOOLCHAIN_FILE})
    include(${CMAKE_IOS_TOOLCHAIN_FILE})
endif()

add_subdirectory("extern")
add_subdirectory("src")
