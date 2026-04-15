include(FetchContent)

find_package(OpenGL QUIET)

#=================== ImGui ===================
set(imgui_fixes_and_config_patch_file ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/patches/imgui-fixes-and-config.patch)
set(imgui_apply_patch_command ${CMAKE_COMMAND} -Dpatch_file=${imgui_fixes_and_config_patch_file} -Dwith_reset=TRUE -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/git-patch.cmake)

FetchContent_Declare(
    ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9b-docking
    PATCH_COMMAND ${imgui_apply_patch_command}
)
FetchContent_MakeAvailable(ImGui)
list(APPEND ADDITIONAL_LIB_INCLUDES ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

add_library(ImGui STATIC)
set_property(TARGET ImGui PROPERTY CXX_STANDARD 20)

target_sources(ImGui
    PRIVATE
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
)

target_sources(ImGui
    PRIVATE
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
)

target_include_directories(ImGui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends PRIVATE ${SDL2_INCLUDE_DIRS})

# ========= StormLib =============
if(INCLUDE_MPQ_SUPPORT)
    set(stormlib_patch_file ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/patches/stormlib-optimizations.patch)
    set(stormlib_apply_patch_command ${CMAKE_COMMAND} -Dpatch_file=${stormlib_patch_file} -Dwith_reset=TRUE -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/git-patch.cmake)

    FetchContent_Declare(
        StormLib
        GIT_REPOSITORY https://github.com/ladislav-zezula/StormLib.git
        GIT_TAG v9.25
        PATCH_COMMAND ${stormlib_apply_patch_command}
    )
    FetchContent_MakeAvailable(StormLib)
    list(APPEND ADDITIONAL_LIB_INCLUDES ${stormlib_SOURCE_DIR}/src)
endif()

#=================== STB ===================
set(STB_DIR ${CMAKE_BINARY_DIR}/_deps/stb)
file(DOWNLOAD "https://github.com/nothings/stb/raw/0bc88af4de5fb022db643c2d8e549a0927749354/stb_image.h" "${STB_DIR}/stb_image.h")
file(WRITE "${STB_DIR}/stb_impl.c" "#define STB_IMAGE_IMPLEMENTATION\n#include \"stb_image.h\"")

add_library(stb STATIC)

target_sources(stb PRIVATE
    ${STB_DIR}/stb_image.h
    ${STB_DIR}/stb_impl.c
)

target_include_directories(stb PUBLIC ${STB_DIR})
list(APPEND ADDITIONAL_LIB_INCLUDES ${STB_DIR})

#=================== libgfxd ===================
if (GFX_DEBUG_DISASSEMBLER)
    FetchContent_Declare(
        libgfxd
        GIT_REPOSITORY https://github.com/glankk/libgfxd.git
        GIT_TAG 008f73dca8ebc9151b205959b17773a19c5bd0da
    )
    FetchContent_MakeAvailable(libgfxd)

    add_library(libgfxd STATIC)
    set_property(TARGET libgfxd PROPERTY C_STANDARD 11)

    target_sources(libgfxd PRIVATE
        ${libgfxd_SOURCE_DIR}/gbi.h
        ${libgfxd_SOURCE_DIR}/gfxd.h
        ${libgfxd_SOURCE_DIR}/priv.h
        ${libgfxd_SOURCE_DIR}/gfxd.c
        ${libgfxd_SOURCE_DIR}/uc.c
        ${libgfxd_SOURCE_DIR}/uc_f3d.c
        ${libgfxd_SOURCE_DIR}/uc_f3db.c
        ${libgfxd_SOURCE_DIR}/uc_f3dex.c
        ${libgfxd_SOURCE_DIR}/uc_f3dex2.c
        ${libgfxd_SOURCE_DIR}/uc_f3dexb.c
    )

    target_include_directories(libgfxd PUBLIC ${libgfxd_SOURCE_DIR})
endif()

#======== thread-pool ========
FetchContent_Declare(
    ThreadPool
    GIT_REPOSITORY https://github.com/bshoshany/thread-pool.git
    GIT_TAG v4.1.0
)
FetchContent_MakeAvailable(ThreadPool)

list(APPEND ADDITIONAL_LIB_INCLUDES ${threadpool_SOURCE_DIR}/include)

#=========== prism ===========
option(PRISM_STANDALONE "Build prism as a standalone library" OFF)
FetchContent_Declare(
    prism
    GIT_REPOSITORY https://github.com/KiritoDv/prism-processor.git
    GIT_TAG 1de054450e7b3c5f777d2e3dfcb228ad120c329d
)
FetchContent_MakeAvailable(prism)

#=========== monocypher ===========
FetchContent_Declare(
    monocypher
    GIT_REPOSITORY https://github.com/LoupVaillant/Monocypher.git
    GIT_TAG 0d85f98c9d9b0227e42cf795cb527dff372b40a4
)
FetchContent_MakeAvailable(monocypher)

add_library(monocypher STATIC)
set_property(TARGET monocypher PROPERTY C_STANDARD 11)

target_sources(monocypher PRIVATE
    ${monocypher_SOURCE_DIR}/src/monocypher.c
    ${monocypher_SOURCE_DIR}/src/optional/monocypher-ed25519.c
)

target_include_directories(monocypher PUBLIC 
    ${monocypher_SOURCE_DIR}/src
    ${monocypher_SOURCE_DIR}/src/optional
)

#=========== libtcc ===========
FetchContent_Declare(
    tinycc
    GIT_REPOSITORY https://github.com/TinyCC/tinycc.git
    GIT_TAG        mob
)

FetchContent_MakeAvailable(tinycc)
if(NOT TARGET libtcc)
    # Enable symbol exporting for Windows DLLs
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

    if(NOT EXISTS "${tinycc_SOURCE_DIR}/config.h")
        message(STATUS "Configuring TinyCC to generate config.h...")
        if(WIN32)
            execute_process(
                COMMAND cmd /c build-tcc.bat -c cl
                WORKING_DIRECTORY "${tinycc_SOURCE_DIR}/win32"
                RESULT_VARIABLE tcc_config_result
            )
        else()
            execute_process(
                COMMAND ./configure
                WORKING_DIRECTORY "${tinycc_SOURCE_DIR}"
                RESULT_VARIABLE tcc_config_result
            )
        endif()

        if(NOT tcc_config_result EQUAL 0)
            message(WARNING "TinyCC configuration script returned non-zero. The build might fail.")
        endif()

        if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
            message(STATUS "iOS target detected: Disabling CONFIG_CODESIGN...")
            file(APPEND "${tinycc_SOURCE_DIR}/config.h" "\n/* Force disable code signing for iOS cross-compilation */\n#undef CONFIG_CODESIGN\n")
        endif()
    endif()

    if(CMAKE_CROSSCOMPILING)
        find_program(HOST_C_COMPILER NAMES cc clang gcc REQUIRED)
        set(C2STR_EXE "${tinycc_BINARY_DIR}/tcc_c2str_host")
        if(CMAKE_HOST_WIN32)
            set(C2STR_EXE "${C2STR_EXE}.exe")
        endif()

        set(SIGN_COMMAND "")
        if(CMAKE_HOST_APPLE)
            set(SIGN_COMMAND COMMAND codesign -f -s - "${C2STR_EXE}")
        endif()

        add_custom_command(
            OUTPUT "${C2STR_EXE}"
            COMMAND ${CMAKE_COMMAND} -E env --unset=SDKROOT --unset=IPHONEOS_DEPLOYMENT_TARGET --unset=TVOS_DEPLOYMENT_TARGET
                    ${HOST_C_COMPILER} -DC2STR -o "${C2STR_EXE}" "${tinycc_SOURCE_DIR}/conftest.c"
            ${SIGN_COMMAND}
            DEPENDS "${tinycc_SOURCE_DIR}/conftest.c"
            COMMENT "Compiling host tool c2str natively..."
        )

        add_custom_command(
            OUTPUT "${tinycc_BINARY_DIR}/tccdefs_.h"
            COMMAND "${C2STR_EXE}" "${tinycc_SOURCE_DIR}/include/tccdefs.h" "${tinycc_BINARY_DIR}/tccdefs_.h"
            DEPENDS "${tinycc_SOURCE_DIR}/include/tccdefs.h" "${C2STR_EXE}"
            COMMENT "Generating tccdefs_.h for TinyCC (Cross-compiling)..."
        )
    else()
        add_executable(tcc_c2str "${tinycc_SOURCE_DIR}/conftest.c")
        target_compile_definitions(tcc_c2str PRIVATE C2STR)
        target_include_directories(tcc_c2str PRIVATE "${tinycc_SOURCE_DIR}")

        add_custom_command(
            OUTPUT "${tinycc_BINARY_DIR}/tccdefs_.h"
            COMMAND tcc_c2str "${tinycc_SOURCE_DIR}/include/tccdefs.h" "${tinycc_BINARY_DIR}/tccdefs_.h"
            DEPENDS "${tinycc_SOURCE_DIR}/include/tccdefs.h" tcc_c2str
            COMMENT "Generating tccdefs_.h for TinyCC..."
        )
    endif()

    add_library(libtcc SHARED
        "${tinycc_SOURCE_DIR}/libtcc.c"
        "${tinycc_BINARY_DIR}/tccdefs_.h"
    )

    add_library(libtcc1 STATIC
        "${tinycc_SOURCE_DIR}/lib/libtcc1.c"
    )
    
    target_include_directories(libtcc1 PRIVATE 
        "${tinycc_SOURCE_DIR}"
        "${tinycc_BINARY_DIR}"
    )

    if(MSVC)
        if(CMAKE_GENERATOR_PLATFORM MATCHES "ARM64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
            target_compile_definitions(libtcc1 PRIVATE __aarch64__)
            target_compile_definitions(libtcc  PRIVATE __aarch64__ TCC_TARGET_ARM64)
        else()
            target_compile_definitions(libtcc1 PRIVATE __x86_64__)
            target_compile_definitions(libtcc  PRIVATE __x86_64__ TCC_TARGET_X86_64)
        endif()
    endif()

    set(TCC_SAFE_INCLUDE_DIR "${tinycc_BINARY_DIR}/safe_include")
    configure_file(
        "${tinycc_SOURCE_DIR}/libtcc.h"
        "${TCC_SAFE_INCLUDE_DIR}/libtcc.h"
        COPYONLY
    )

    target_include_directories(libtcc PRIVATE
        "${tinycc_SOURCE_DIR}"
        "${tinycc_BINARY_DIR}"
    )
    target_include_directories(libtcc PUBLIC
        $<BUILD_INTERFACE:${TCC_SAFE_INCLUDE_DIR}>
    )

    if(ANDROID)
        target_link_libraries(libtcc PRIVATE dl m)
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(libtcc PRIVATE dl m pthread)
    endif()
    
    set_target_properties(libtcc1 PROPERTIES OUTPUT_NAME "tcc1")
endif()