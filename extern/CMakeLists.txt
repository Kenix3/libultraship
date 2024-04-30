find_package(OpenGL QUIET)

#=================== DrLibs ===================

set(DRLIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dr_libs)
add_library(DrLibs INTERFACE)

target_include_directories(DrLibs INTERFACE ${DRLIBS_DIR})

#=================== Storm ===================

set(STORMLIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/StormLib)
add_subdirectory(${STORMLIB_DIR})
if((CMAKE_SYSTEM_NAME MATCHES "Windows") AND ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang"))
    include(../cmake/HandleCompilerRT.cmake)
    find_compiler_rt_library(builtins CLANG_RT_BUILTINS_LIBRARY)
    get_filename_component(LIBDIR "${CLANG_RT_BUILTINS_LIBRARY}" DIRECTORY)
    if(IS_DIRECTORY "${LIBDIR}")
        target_link_libraries(storm ${CLANG_RT_BUILTINS_LIBRARY})
    endif()
endif()

#=================== ZAPDUtils ===================

set(ZAPDUTILS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ZAPDUtils)
add_subdirectory(${ZAPDUTILS_DIR})

#=================== libgfxd ===================
# This used to be included from ZAPDTR, meaning setting GFX_DEBUG_DISASSEMBLER
# required the port include ZAPDTR. The version in extern is the version that was
# in ZAPDTR, which is
# https://github.com/glankk/libgfxd/tree/008f73dca8ebc9151b205959b17773a19c5bd0da
# with a couple small changes:
# * priv.h now has `#define CONFIG_MT` added right above the #ifdef CONFIG_MT
# * the Makefile now has `CC ?= gcc` as the first line

if (GFX_DEBUG_DISASSEMBLER)
    set(LIBGFXD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libgfxd)
    add_library(libgfxd STATIC)
    set_property(TARGET libgfxd PROPERTY C_STANDARD 11)

    target_sources(libgfxd PRIVATE
        ${LIBGFXD_DIR}/gbi.h
        ${LIBGFXD_DIR}/gfxd.h
        ${LIBGFXD_DIR}/priv.h
        ${LIBGFXD_DIR}/gfxd.c
        ${LIBGFXD_DIR}/uc.c
        ${LIBGFXD_DIR}/uc_f3d.c
        ${LIBGFXD_DIR}/uc_f3db.c
        ${LIBGFXD_DIR}/uc_f3dex.c
        ${LIBGFXD_DIR}/uc_f3dex2.c
        ${LIBGFXD_DIR}/uc_f3dexb.c
    )

    target_include_directories(libgfxd PUBLIC ${LIBGFXD_DIR})
endif()

#=================== StrHash64 ===================

set(STRHASH64_DIR ${CMAKE_CURRENT_SOURCE_DIR}/StrHash64)
add_library(StrHash64 STATIC)

target_sources(StrHash64 PRIVATE ${STRHASH64_DIR}/StrHash64.cpp)

target_include_directories(StrHash64 PUBLIC ${STRHASH64_DIR})

#=================== thread-pool ===================

set(THREADPOOL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thread-pool)
add_library(ThreadPool INTERFACE)

target_include_directories(ThreadPool INTERFACE ${THREADPOOL_DIR})

#=================== STB ===================

set(STB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stb)
add_library(stb STATIC)

target_sources(stb PRIVATE
    ${STB_DIR}/stb_image.h
    ${STB_DIR}/stb_image_write.h
    ${STB_DIR}/stb_impl.c
)

target_include_directories(stb PUBLIC ${STB_DIR})
