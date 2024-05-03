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

#=================== thread-pool ===================

set(THREADPOOL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thread-pool)
add_library(ThreadPool INTERFACE)

target_include_directories(ThreadPool INTERFACE ${THREADPOOL_DIR})
