cmake_minimum_required(VERSION 3.24.0)

option(NON_PORTABLE "Build a non-portable version" OFF)
option(DISABLE_DLL_LOADER "Disable dynamic library loading support" OFF)
option(DISABLE_SCRIPTING "Disable TCC-based C mod compilation and scripting system" OFF)

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

# =========== Sources =============
add_subdirectory("src")

# =========== LLGL =============
option(LUS_BUILD_LLGL "Build with LLGL (Low Level Graphics Library) rendering backend" OFF)

# =========== Vulkan / ParallelRDP Backend =============
option(LUS_BUILD_VULKAN_PRDP "Build with Vulkan/ParallelRDP rendering backend" OFF)

# =========== Tests =============
option(LUS_BUILD_TESTS "Build unit tests" OFF)
option(LUS_BUILD_PRDP_TESTS "Build ParallelRDP comparison tests (requires Vulkan)" OFF)

# LLGL (static) and ParallelRDP are mutually exclusive: both pull in a Vulkan
# loader, and linking them into the same executable causes volk's global BSS
# function-pointer variables to shadow the real libvulkan.so symbols, making
# LLGL's Vulkan calls jump into non-executable BSS memory (SIGSEGV).
if(LUS_BUILD_LLGL AND (LUS_BUILD_PRDP_TESTS OR LUS_BUILD_VULKAN_PRDP))
    message(FATAL_ERROR
        "LUS_BUILD_LLGL and LUS_BUILD_PRDP_TESTS/LUS_BUILD_VULKAN_PRDP cannot both be "
        "enabled at the same time: LLGL's Vulkan backend and ParallelRDP's volk loader "
        "define conflicting Vulkan symbols when linked into the same executable. "
        "Enable one or the other.")
endif()

if(LUS_BUILD_LLGL)
    include(cmake/dependencies/llgl.cmake)
endif()

# Build parallel-rdp if needed by either the Vulkan rendering backend or the
# comparison tests.
if(LUS_BUILD_VULKAN_PRDP OR LUS_BUILD_PRDP_TESTS)
    find_package(Threads REQUIRED)
    include(cmake/dependencies/parallel-rdp.cmake)
endif()

if(LUS_BUILD_TESTS)
    enable_testing()
    add_subdirectory("tests")
endif()
