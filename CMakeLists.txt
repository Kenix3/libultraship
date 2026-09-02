cmake_minimum_required(VERSION 3.24.0)

# Enable /Z7 (embedded debug info) instead of /Zi (shared PDB) on MSVC so that
# sccache can cache object files without racing over a shared .pdb file.
# CMP0141 was introduced in CMake 3.25, so guard it for older versions.
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
endif()

option(NON_PORTABLE "Build a non-portable version" OFF)
option(INCLUDE_PROFILING "Enable action and tickable profiling timers" OFF)
option(COMPONENT_THREAD_SAFE "Enable thread safety for Component parent/child lists" OFF)
option(DISABLE_DLL_LOADER "Disable dynamic library loading support" OFF)
option(ENABLE_SCRIPTING "Enable TCC-based C mod compilation and scripting system" OFF)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    option(SIGN_LIBRARY "Enable xcode signing" OFF)
    option(BUNDLE_ID "Bundle ID for xcode signing" "com.example.libultraship")
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
endif()

project(libultraship LANGUAGES C CXX)

set(_lus_version_epoch_default "2")
set(_lus_version_major_default "0")
set(_lus_version_minor_default "0")
set(_lus_version_patch_default "0")

if(DEFINED ENV{LUS_VERSION_EPOCH} AND NOT "$ENV{LUS_VERSION_EPOCH}" STREQUAL "")
    set(_lus_version_epoch_default "$ENV{LUS_VERSION_EPOCH}")
endif()
if(DEFINED ENV{LUS_VERSION_MAJOR} AND NOT "$ENV{LUS_VERSION_MAJOR}" STREQUAL "")
    set(_lus_version_major_default "$ENV{LUS_VERSION_MAJOR}")
endif()
if(DEFINED ENV{LUS_VERSION_MINOR} AND NOT "$ENV{LUS_VERSION_MINOR}" STREQUAL "")
    set(_lus_version_minor_default "$ENV{LUS_VERSION_MINOR}")
endif()
if(DEFINED ENV{LUS_VERSION_PATCH} AND NOT "$ENV{LUS_VERSION_PATCH}" STREQUAL "")
    set(_lus_version_patch_default "$ENV{LUS_VERSION_PATCH}")
endif()

set(LUS_VERSION_EPOCH "${_lus_version_epoch_default}" CACHE STRING "libultraship version epoch (0-255)")
set(LUS_VERSION_MAJOR "${_lus_version_major_default}" CACHE STRING "libultraship version major (0-255)")
set(LUS_VERSION_MINOR "${_lus_version_minor_default}" CACHE STRING "libultraship version minor (0-255)")
set(LUS_VERSION_PATCH "${_lus_version_patch_default}" CACHE STRING "libultraship version patch (0-255)")

foreach(_lus_version_component IN ITEMS LUS_VERSION_EPOCH LUS_VERSION_MAJOR LUS_VERSION_MINOR LUS_VERSION_PATCH)
    if(NOT ${_lus_version_component} MATCHES "^[0-9]+$")
        message(FATAL_ERROR "${_lus_version_component} must be an integer between 0 and 255")
    endif()

    if(${_lus_version_component} GREATER 255)
        message(FATAL_ERROR "${_lus_version_component} must be between 0 and 255")
    endif()
endforeach()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    enable_language(OBJCXX)
    set(CMAKE_OBJC_FLAGS "${CMAKE_OBJC_FLAGS} -fobjc-arc")
    set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -fobjc-arc")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	# Default to embedded debug info (/Z7) so sccache can cache object files
	# without multiple CL.EXE processes racing over a shared .pdb file.
	if(POLICY CMP0141)
		set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:Embedded>")
	endif()
endif()

include(cmake/Utils.cmake)
set(ADDITIONAL_LIB_INCLUDES "")

# ========= Configuration Options =========
option(INCLUDE_MPQ_SUPPORT "Enable StormLib and MPQ archive support" OFF)
option(GBI_UCODE "Specify the GBI ucode version" F3DEX_GBI_2)

# =========== Dependencies =============
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    include(cmake/dependencies/windows-vcpkg.cmake)
endif()

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

if(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    include(cmake/dependencies/openbsd.cmake)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    include(cmake/dependencies/windows.cmake)
endif()

# === Platform Specific Configuration ===
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    include(cmake/ios-toolchain-populate.cmake)
endif()

# =========== Optional graphics backends =============
option(LUS_BUILD_LLGL "Build with LLGL (Low Level Graphics Library) rendering backend" OFF)
option(LUS_BUILD_VULKAN_PRDP "Build with Vulkan/ParallelRDP rendering backend" OFF)
option(LUS_BUILD_PRDP_TESTS "Build ParallelRDP comparison tests (requires Vulkan)" OFF)

if(LUS_BUILD_LLGL AND (LUS_BUILD_PRDP_TESTS OR LUS_BUILD_VULKAN_PRDP))
    message(FATAL_ERROR
        "LUS_BUILD_LLGL and ParallelRDP cannot both be enabled in the same target")
endif()

if(LUS_BUILD_LLGL)
    include(cmake/dependencies/llgl.cmake)
endif()

if(LUS_BUILD_VULKAN_PRDP OR LUS_BUILD_PRDP_TESTS)
    find_package(Threads REQUIRED)
    include(cmake/dependencies/parallel-rdp.cmake)
endif()

# =========== Tests (configure-time options) =============
option(LUS_BUILD_TESTS "Build unit tests" OFF)
if(LUS_BUILD_TESTS)
    set(COMPONENT_THREAD_SAFE ON CACHE BOOL "Enable thread safety for Component parent/child lists" FORCE)
endif()

# =========== Sources =============
add_subdirectory("src")

# =========== Tests =============
if(LUS_BUILD_TESTS)
    enable_testing()
    add_subdirectory("tests")
endif()
