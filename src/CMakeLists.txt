add_library(libultraship STATIC)
set_target_properties(libultraship PROPERTIES PREFIX "")
set_property(TARGET libultraship PROPERTY CXX_STANDARD 20)
# Need to set C11 for libgfxd (_Alignas)
set_property(TARGET libultraship PROPERTY C_STANDARD 11)

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
option(USE_OPENGLES "Enable GLES3" OFF)
option(GFX_DEBUG_DISASSEMBLER "Enable libgfxd" OFF)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
endif()

#=================== Top-Level ===================

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/install_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/install_config.h @ONLY)

file(GLOB Source_Files__TopLevel RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h" "*.cpp")

source_group("" FILES ${Source_Files__TopLevel})
target_sources(libultraship PRIVATE ${Source_Files__TopLevel})

#=================== Audio ===================

file(GLOB Source_Files__Audio RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "audio/*.h" "audio/*.cpp")

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(FILTER Source_Files__Audio EXCLUDE REGEX "audio/WasapiAudioPlayer.*")
endif()

source_group("audio" FILES ${Source_Files__Audio})
target_sources(libultraship PRIVATE ${Source_Files__Audio})

#=================== Controller ===================

file(GLOB_RECURSE Source_Files__Controller RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "controller/*.h" "controller/*.cpp")

if (CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    list(FILTER Source_Files__Controller EXCLUDE REGEX "controller/controldevice/controller/mapping/keyboard/*")
    list(FILTER Source_Files__Controller EXCLUDE REGEX "controller/controldevice/controller/mapping/sdl/*")
    list(FILTER Source_Files__Controller EXCLUDE REGEX "controller/deviceindex/ShipDeviceIndexToSDLDeviceIndexMapping.*")
else()
    list(FILTER Source_Files__Controller EXCLUDE REGEX "controller/controldevice/controller/mapping/wiiu/*")
    list(FILTER Source_Files__Controller EXCLUDE REGEX "controller/deviceindex/ShipDeviceIndexToWiiUDeviceIndexMapping.*")
endif()

source_group("controller" FILES ${Source_Files__Controller})
target_sources(libultraship PRIVATE ${Source_Files__Controller})

#=================== Config ===================

file(GLOB Source_Files__Config RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "config/*.h" "config/*.cpp")

source_group("config" FILES ${Source_Files__Config})
target_sources(libultraship PRIVATE ${Source_Files__Config})

#=================== Public ===================

source_group("public")

file(GLOB Source_Files__Public__Libultra RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "public/libultra/*.h" "public/libultra/*.cpp")

source_group("public/libultra" FILES ${Source_Files__Public__Libultra})

file(GLOB Source_Files__Public__Bridge RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "public/bridge/*.h" "public/bridge/*.cpp")

source_group("public/bridge" FILES ${Source_Files__Public__Bridge})
target_sources(libultraship PRIVATE ${Source_Files__Public__Libultra} ${Source_Files__Public__Bridge})

#=================== Debug ===================

file(GLOB Source_Files__Debug RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "debug/*.h" "debug/*.cpp")

source_group("debug" FILES ${Source_Files__Debug})
target_sources(libultraship PRIVATE ${Source_Files__Debug})

#=================== Log ===================

file(GLOB Source_Files__Log RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "log/*.h" "log/*.cpp")

source_group("log" FILES ${Source_Files__Log})

target_sources(libultraship PRIVATE ${Source_Files__Log} ${Source_Files__Log__SPDLog})

#=================== Window ===================

file(GLOB Source_Files__Window RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "window/*.h" "window/*.cpp")

source_group("window" FILES ${Source_Files__Window})
target_sources(libultraship PRIVATE ${Source_Files__Window})

#=================== Gui ===================

file(GLOB_RECURSE Source_Files__Window__Gui RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "window/gui/*.h" "window/gui/*.cpp")

source_group("window/gui" FILES ${Source_Files__Window__Gui})
target_sources(libultraship PRIVATE ${Source_Files__Window__Gui})

#=================== Utils ===================

file(GLOB Source_Files__Utils RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "utils/*.h" "utils/*.cpp" "utils/*.c")

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    list(APPEND Source_Files__Utils ${CMAKE_CURRENT_SOURCE_DIR}/utils/AppleFolderManager.mm)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/utils/AppleFolderManager.mm PROPERTIES COMPILE_FLAGS -fno-objc-arc)
else()
    list(REMOVE_ITEM Header_Files__include ${CMAKE_CURRENT_SOURCE_DIR}/utils/AppleFolderManager.h)
endif()

source_group("utils" FILES ${Source_Files__Utils})
target_sources(libultraship PRIVATE ${Source_Files__Utils})

#=================== BinaryTools ==================

file(GLOB Source_Files__Utils__BinaryTools RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "utils/binarytools/*.h" "utils/binarytools/*.cpp")

source_group("utils/binarytools" FILES ${Source_Files__Utils__BinaryTools})
target_sources(libultraship PRIVATE ${Source_Files__Utils__BinaryTools})

#=================== Port ===================

if (CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    set(Source_Files__Port
        ${CMAKE_CURRENT_SOURCE_DIR}/port/wiiu/WiiUImpl.h
        ${CMAKE_CURRENT_SOURCE_DIR}/port/wiiu/WiiUImpl.cpp
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    set(Source_Files__Port
        ${CMAKE_CURRENT_SOURCE_DIR}/port/switch/SwitchImpl.h
        ${CMAKE_CURRENT_SOURCE_DIR}/port/switch/SwitchImpl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/port/switch/SwitchPerformanceProfiles.h
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Android" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(Source_Files__Port
        ${CMAKE_CURRENT_SOURCE_DIR}/port/mobile/MobileImpl.h
        ${CMAKE_CURRENT_SOURCE_DIR}/port/mobile/MobileImpl.cpp
    )
endif()

source_group("port" FILES ${Source_Files__Port})
target_sources(libultraship PRIVATE ${Source_Files__Port})

#=================== Resource ===================

file(GLOB Source_Files__Resource RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "resource/*.h" "resource/*.cpp")
source_group("resource" FILES ${Source_Files__Resource})

file(GLOB Source_Files__Resource__Types RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "resource/type/*.h" "resource/type/*.cpp")
source_group("resource/type" FILES ${Source_Files__Resource__Types})

file(GLOB Source_Files__Resource__Factories RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "resource/factory/*.h" "resource/factory/*.cpp")
source_group("resource/factory" FILES ${Source_Files__Resource__Factories})

file(GLOB Source_Files__Resource__Archive RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "resource/archive/*.h" "resource/archive/*.cpp")
source_group("resource/archive" FILES ${Source_Files__Resource__Archive})

target_sources(libultraship PRIVATE ${Source_Files__Resource} ${Source_Files__Resource__Types} ${Source_Files__Resource__Factories} ${Source_Files__Resource__Archive})

#=================== Graphic ===================

file(GLOB Source_Files__Graphic RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "graphic/Fast3D/*.h" "graphic/Fast3D/*.cpp" "graphic/Fast3D/*.c")

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "graphic/Fast3D/gfx_dxgi*")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "graphic/Fast3D/gfx_direct3d*")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "graphic/Fast3D/dxsdk/*")
endif()

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "graphic/Fast3D/gfx_metal*")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "graphic/Fast3D/gfx_opengl*")
endif()

if (NOT CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "graphic/Fast3D/gfx_wiiu*")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "graphic/Fast3D/gx2*")
else()
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "graphic/Fast3D/gfx_opengl*")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "graphic/Fast3D/gfx_sdl*")
endif()

source_group("graphic" FILES ${Source_Files__Graphic})
target_sources(libultraship PRIVATE ${Source_Files__Graphic})

#=================== metal-cpp ===================

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_include_directories(libultraship PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../extern/metal-cpp)
endif()

#=================== STB ===================
# hack: no idea why wii u doesn't want to link this when we build it
#       as a separate static lib
if (CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    set(Source_Files__Lib__stb
        "${CMAKE_CURRENT_SOURCE_DIR}/../extern/stb/stb_image.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/../extern/stb/stb_image_write.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/../extern/stb/stb_impl.c"
    )
    source_group("extern/stb" FILES ${Source_Files__Lib__stb})
    target_sources(libultraship PRIVATE ${Source_Files__Lib__stb})
endif()

#=================== Packages & Includes ===================

target_include_directories(libultraship
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../extern ${CMAKE_CURRENT_BINARY_DIR} $<$<BOOL:${GFX_DEBUG_DISASSEMBLER}>:${CMAKE_CURRENT_SOURCE_DIR}/../../ZAPDTR/lib/libgfxd>
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../extern/spdlog/include ${CMAKE_CURRENT_SOURCE_DIR}/../extern/stb ${ADDITIONAL_LIB_INCLUDES}
)

if (CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${DEVKITPRO}/portlibs/wiiu/include/
    )
endif()

#=================== Linking ===================

if (NOT CMAKE_SYSTEM_NAME STREQUAL "CafeOS" AND NOT CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch" AND NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(libultraship PUBLIC "$<LINK_LIBRARY:WHOLE_ARCHIVE,storm>")
else()
    target_link_libraries(libultraship PUBLIC storm)
endif()
target_link_libraries(libultraship
    PRIVATE StrHash64
    PUBLIC ZAPDUtils ImGui tinyxml2
)

# hack cont.: wii u just includes these files, everyone
#             else handles linking it just fine
if (NOT CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    target_link_libraries(libultraship PRIVATE stb)
endif()

if (GFX_DEBUG_DISASSEMBLER)
    target_link_libraries(libultraship PRIVATE libgfxd)
endif()

find_package(libzip REQUIRED)
target_link_libraries(libultraship PRIVATE libzip::zip)

find_package(nlohmann_json REQUIRED)
target_link_libraries(libultraship PUBLIC nlohmann_json::nlohmann_json)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(libultraship PRIVATE Threads::Threads)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    find_Library(OSX_FOUNDATION Foundation)
    find_Library(OSX_AVFOUNDATION AVFoundation)
    find_library(METAL Metal)
    find_library(QUARTZCORE QuartzCore)
    target_link_libraries(libultraship PRIVATE ${OSX_FOUNDATION} ${OSX_AVFOUNDATION} ${METAL} ${QUARTZCORE} ${CMAKE_DL_LIBS})
endif()

if (USE_OPENGLES)
    if (CMAKE_SYSTEM_NAME STREQUAL "Android")
        find_library(OPENGLES_LIBRARY GLESv3)
        target_link_libraries(libultraship PRIVATE GLESv3)
    else()
        find_library(OPENGLES_LIBRARY GLESv2)
        target_link_libraries(libultraship PRIVATE GLESv2)
    endif()
endif()

#=================== Compile Options & Defs ===================

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIN32
        _CONSOLE
        _CRT_SECURE_NO_WARNINGS
        ENABLE_DX11
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    )
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    target_compile_definitions(libultraship PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
        SPDLOG_NO_THREAD_ID
        SPDLOG_NO_TLS
        STBI_NO_THREAD_LOCALS
        SPDLOG_ACTIVE_LEVEL=3
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_definitions(libultraship PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=0>
        $<$<NOT:$<CONFIG:Debug>>:SPDLOG_ACTIVE_LEVEL=1>
        __IOS__
    )

    set_xcode_property(${PROJECT_NAME} PRODUCT_BUNDLE_IDENTIFIER ${BUNDLE_ID} All)

    if(NOT SIGN_LIBRARY)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
            XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
        )

        set_target_properties(SDL2 PROPERTIES
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
            XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
        )

        set_target_properties(zip PROPERTIES
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
            XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
        )
    endif()
else()
    target_compile_definitions(libultraship PRIVATE
        ENABLE_OPENGL
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=0>
        $<$<NOT:$<CONFIG:Debug>>:SPDLOG_ACTIVE_LEVEL=1>
        $<$<BOOL:${USE_OPENGLES}>:USE_OPENGLES>
        $<$<BOOL:${GFX_DEBUG_DISASSEMBLER}>:GFX_DEBUG_DISASSEMBLER>
    )
endif()

if(MSVC)
    target_compile_options(libultraship PRIVATE
        $<$<CONFIG:Debug>:
            /Od;
            /Oi-
        >
        $<$<CONFIG:Release>:
            /Oi;
            /Gy
        >
        /permissive-;
        /MP;
        /sdl;
        /W3;
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        ${DEFAULT_CXX_EXCEPTION_HANDLING};
    )
    target_link_options(libultraship PRIVATE
        $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:ICF
        >
        /SUBSYSTEM:CONSOLE
    )
endif()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|Clang")
	target_compile_options(libultraship PRIVATE
		-Wall
		-Wextra
		-Wno-error
		-Wno-unused-variable
		-Wno-unused-parameter
		-Wno-unused-function
		-Wno-parentheses
		-Wno-narrowing
		-Wno-missing-field-initializers
        -Wno-implicit-function-declaration
        $<$<COMPILE_LANGUAGE:C,OBJC>:-Wno-int-conversion>
	)
endif()
