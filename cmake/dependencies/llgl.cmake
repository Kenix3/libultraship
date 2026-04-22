# LLGL – Low Level Graphics Library (Lukas Hermanns)
# https://github.com/LukasBanana/LLGL (BSD 3-Clause)
#
# We build LLGL as a static library so that the renderer backends are
# directly linked into the consumer binary — no run-time dlopen / .so
# search-path issues.  Only the renderers that are appropriate for the
# current platform are enabled; callers can override individual options
# before including this file.

include(FetchContent)

# ---------- Disable LLGL sub-targets we never use ----------
set(LLGL_BUILD_TESTS        OFF CACHE BOOL "" FORCE)
set(LLGL_BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
set(LLGL_BUILD_WRAPPER_C99  OFF CACHE BOOL "" FORCE)
set(LLGL_ENABLE_EXCEPTIONS  OFF CACHE BOOL "" FORCE)
set(LLGL_BUILD_RENDERER_NULL            OFF CACHE BOOL "" FORCE)

# ---------- Pick the renderer per platform (can be overridden) ----------
if(NOT DEFINED LLGL_BUILD_RENDERER_OPENGL)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(LLGL_BUILD_RENDERER_OPENGL  OFF CACHE BOOL "" FORCE)
    else()
        set(LLGL_BUILD_RENDERER_OPENGL  ON  CACHE BOOL "" FORCE)
    endif()
endif()

if(NOT DEFINED LLGL_BUILD_RENDERER_VULKAN)
    set(LLGL_BUILD_RENDERER_VULKAN      OFF CACHE BOOL "" FORCE)
endif()

if(NOT DEFINED LLGL_BUILD_RENDERER_METAL)
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(LLGL_BUILD_RENDERER_METAL   ON  CACHE BOOL "" FORCE)
    else()
        set(LLGL_BUILD_RENDERER_METAL   OFF CACHE BOOL "" FORCE)
    endif()
endif()

if(NOT DEFINED LLGL_BUILD_RENDERER_DIRECT3D11)
    if(WIN32)
        set(LLGL_BUILD_RENDERER_DIRECT3D11  ON  CACHE BOOL "" FORCE)
    else()
        set(LLGL_BUILD_RENDERER_DIRECT3D11  OFF CACHE BOOL "" FORCE)
    endif()
endif()

# Enable D3D12 on Windows alongside D3D11 so the backend can try D3D12 first.
if(NOT DEFINED LLGL_BUILD_RENDERER_DIRECT3D12)
    if(WIN32)
        set(LLGL_BUILD_RENDERER_DIRECT3D12  ON  CACHE BOOL "" FORCE)
    else()
        set(LLGL_BUILD_RENDERER_DIRECT3D12  OFF CACHE BOOL "" FORCE)
    endif()
endif()

# ---------- Build LLGL as a static library ----------
set(LLGL_BUILD_STATIC_LIB ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    LLGL
    GIT_REPOSITORY https://github.com/LukasBanana/LLGL.git
    GIT_TAG        383fa7af5487b508f9842709e07fd1072ff8a06f
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(LLGL)

# Expose a convenience target name consistent with the rest of this project.
# LLGL's own CMakeLists creates a target named "LLGL".
if(NOT TARGET LLGL)
    message(WARNING "LLGL FetchContent did not create the expected 'LLGL' target.")
endif()

# Suppress all warnings coming from LLGL source files.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach(_t LLGL LLGL_OpenGL LLGL_Vulkan LLGL_Metal LLGL_Direct3D11 LLGL_Direct3D12 LLGL_Null)
        if(TARGET ${_t})
            target_compile_options(${_t} PRIVATE -w)
        endif()
    endforeach()
endif()

# ==========================================================================
# glslang – GLSL → SPIR-V compiler (used by the shader translation layer)
# ==========================================================================
set(ENABLE_GLSLANG_BINARIES  OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL              OFF CACHE BOOL "" FORCE)
set(ENABLE_CTEST             OFF CACHE BOOL "" FORCE)
set(BUILD_EXTERNAL           OFF CACHE BOOL "" FORCE)
set(GLSLANG_TESTS            OFF CACHE BOOL "" FORCE)
set(GLSLANG_ENABLE_INSTALL   OFF CACHE BOOL "" FORCE)
set(ENABLE_OPT               OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG        15.1.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glslang)

# Suppress warnings from glslang.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach(_t glslang GenericCodeGen glslang-default-resource-limits MachineIndependent OGLCompiler OSDependent SPIRV SPVRemapper)
        if(TARGET ${_t})
            target_compile_options(${_t} PRIVATE -w)
        endif()
    endforeach()
elseif(MSVC)
    foreach(_t glslang GenericCodeGen glslang-default-resource-limits MachineIndependent OGLCompiler OSDependent SPIRV SPVRemapper)
        if(TARGET ${_t})
            target_compile_options(${_t} PRIVATE /w)
        endif()
    endforeach()
endif()

# ==========================================================================
# SPIRV-Cross – SPIR-V → GLSL / HLSL / MSL cross-compiler
# ==========================================================================
set(SPIRV_CROSS_SHARED                  OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_STATIC                  ON  CACHE BOOL "" FORCE)
set(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS            OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_CLI                     OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_C_API            OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    spirv_cross
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
    GIT_TAG        sdk-1.3.261.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(spirv_cross)

# Suppress warnings from SPIRV-Cross.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach(_t spirv-cross-core spirv-cross-glsl spirv-cross-hlsl spirv-cross-msl spirv-cross-cpp spirv-cross-reflect spirv-cross-util)
        if(TARGET ${_t})
            target_compile_options(${_t} PRIVATE -w)
        endif()
    endforeach()
elseif(MSVC)
    foreach(_t spirv-cross-core spirv-cross-glsl spirv-cross-hlsl spirv-cross-msl spirv-cross-cpp spirv-cross-reflect spirv-cross-util)
        if(TARGET ${_t})
            target_compile_options(${_t} PRIVATE /w)
        endif()
    endforeach()
endif()
