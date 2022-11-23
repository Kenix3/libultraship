add_library(libultraship STATIC)
set_property(TARGET libultraship PROPERTY CXX_STANDARD 20)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
endif()

#=================== Audio ===================

set(Source_Files__Audio
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/AudioPlayer.h
	${CMAKE_CURRENT_SOURCE_DIR}/audio/AudioPlayer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/mixer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/mixer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/SDLAudioPlayer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/SDLAudioPlayer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/PulseAudioPlayer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/PulseAudioPlayer.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND Source_Files__Audio ${CMAKE_CURRENT_SOURCE_DIR}/audio/WasapiAudioPlayer.h)
    list(APPEND Source_Files__Audio ${CMAKE_CURRENT_SOURCE_DIR}/audio/WasapiAudioPlayer.cpp)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND Source_Files__Audio ${CMAKE_CURRENT_SOURCE_DIR}/audio/PulseAudioPlayer.h)
    list(APPEND Source_Files__Audio ${CMAKE_CURRENT_SOURCE_DIR}/audio/PulseAudioPlayer.cpp)
endif()

source_group("Audio" FILES ${Source_Files__Audio})

target_sources(libultraship PRIVATE ${Source_Files__Audio})

#=================== BinaryTools ==================

set(Source_Files__BinaryTools
    ${CMAKE_CURRENT_SOURCE_DIR}/binarytools/BinaryReader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/binarytools/BinaryReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/binarytools/BinaryWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/binarytools/BinaryWriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/binarytools/endianness.h
    ${CMAKE_CURRENT_SOURCE_DIR}/binarytools/MemoryStream.h
    ${CMAKE_CURRENT_SOURCE_DIR}/binarytools/MemoryStream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/binarytools/Stream.h
    ${CMAKE_CURRENT_SOURCE_DIR}/binarytools/Stream.cpp
)

source_group("BinaryTools" FILES ${Source_Files__BinaryTools})

target_sources(libultraship PRIVATE ${Source_Files__BinaryTools})

#=================== Controller ===================

set(Source_Files__Controller
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/ControlDeck.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/ControlDeck.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/Controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/Controller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/DummyController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/DummyController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/KeyboardController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/KeyboardController.h
)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    list(APPEND Source_Files__Controller
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/SDLController.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/SDLController.h
    )
endif()

source_group("Controller" FILES ${Source_Files__Controller})

set(Source_Files__Controller__Attachments
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/attachment/ControllerAttachment.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/attachment/ControllerAttachment.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/attachment/MemoryPack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/attachment/MemoryPack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/attachment/RumblePack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/attachment/RumblePack.h
)

source_group("Controller\\Attachments" FILES ${Source_Files__Controller__Attachments})

target_sources(libultraship PRIVATE ${Source_Files__Controller} ${Source_Files__Controller__Attachments})

#=================== Core ===================

set(Source_Files__Core
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Window.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/ConsoleVariable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/ConsoleVariable.cpp
)

source_group("Core" FILES ${Source_Files__Core})

set(Source_Files__Core__Libultra
    ${CMAKE_CURRENT_SOURCE_DIR}/core/libultra/os.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/libultra/os.cpp
)
 
source_group("Core\\libultra" FILES ${Source_Files__Core__Libultra})

set(Source_Files__Core__Bridge
    ${CMAKE_CURRENT_SOURCE_DIR}/core/bridge/resourcebridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/bridge/resourcebridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/bridge/windowbridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/bridge/windowbridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/bridge/audioplayerbridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/bridge/audioplayerbridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/bridge/controldeckbridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/bridge/controldeckbridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/bridge/consolevariablebridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/bridge/consolevariablebridge.cpp
)
 
source_group("Core\\Bridge" FILES ${Source_Files__Core__Bridge})

target_sources(libultraship PRIVATE ${Source_Files__Core} ${Source_Files__Core__Libultra} ${Source_Files__Core__Bridge})

#=================== Debug ===================

set(Source_Files__Debug
    ${CMAKE_CURRENT_SOURCE_DIR}/debug/CrashHandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/debug/CrashHandler.cpp
)

source_group("Debug" FILES ${Source_Files__Debug})
target_sources(libultraship PRIVATE ${Source_Files__Debug})

#=================== Log ===================

set(Source_Files__Log
    ${CMAKE_CURRENT_SOURCE_DIR}/log/luslog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/log/luslog.cpp
)

source_group("Log" FILES ${Source_Files__Log})

set(Source_Files__Log__SPDLog
    ${CMAKE_CURRENT_SOURCE_DIR}/log/spd/sohconsole_sink.h
)

source_group("Log\\SPDLog" FILES ${Source_Files__Log__SPDLog})

target_sources(libultraship PRIVATE ${Source_Files__Log} ${Source_Files__Log__SPDLog})

#=================== Menu ===================

set(Source_Files__Menu
    ${CMAKE_CURRENT_SOURCE_DIR}/menu/Console.h
    ${CMAKE_CURRENT_SOURCE_DIR}/menu/Console.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/menu/ImGuiImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/menu/ImGuiImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/menu/InputEditor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/menu/InputEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/menu/GameOverlay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/menu/GameOverlay.h
)

source_group("Menu" FILES ${Source_Files__Menu})
target_sources(libultraship PRIVATE ${Source_Files__Menu})

#=================== Misc ===================

set(Source_Files__Misc
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/Hooks.h
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/Hooks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/LUSMacros.h
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/stox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/stox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/Utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/Utils.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND Source_Files__Misc
        ${CMAKE_CURRENT_SOURCE_DIR}/misc/OSXFolderManager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/misc/OSXFolderManager.mm
    )
endif()

source_group("Misc" FILES ${Source_Files__Misc})
target_sources(libultraship PRIVATE ${Source_Files__Misc})

#=================== Port ===================

if (CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    set(Source_Files__Port
        ${CMAKE_CURRENT_SOURCE_DIR}/port/wiiu/WiiUController.h
        ${CMAKE_CURRENT_SOURCE_DIR}/port/wiiu/WiiUController.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/port/wiiu/WiiUGamepad.h
        ${CMAKE_CURRENT_SOURCE_DIR}/port/wiiu/WiiUGamepad.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/port/wiiu/WiiUImpl.h
        ${CMAKE_CURRENT_SOURCE_DIR}/port/wiiu/WiiUImpl.cpp
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    set(Source_Files__Port
        ${CMAKE_CURRENT_SOURCE_DIR}/port/switch/SwitchImpl.h
        ${CMAKE_CURRENT_SOURCE_DIR}/port/switch/SwitchImpl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/port/switch/SwitchPerformanceProfiles.h
    )
endif()

source_group("Port" FILES ${Source_Files__Port})
target_sources(libultraship PRIVATE ${Source_Files__Port})

#=================== Resource ===================

set(Source_Files__Resource
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/Archive.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/Archive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/OtrFile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/Resource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceType.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceVersion.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/Resource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceMgr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceMgr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceFactory.cpp
)
source_group("Resource" FILES ${Source_Files__Resource})

set(Source_Files__Resource__Types
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/type/Array.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/type/Array.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/type/Blob.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/type/Blob.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/type/DisplayList.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/type/DisplayList.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/type/Matrix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/type/Matrix.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/type/Texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/type/Texture.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/type/Vertex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/type/Vertex.h
)
source_group("Resource\\Type" FILES ${Source_Files__Resource__Types})

set(Source_Files__Resource__Factories
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/ArrayFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/ArrayFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/BlobFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/BlobFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/DisplayListFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/DisplayListFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/MatrixFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/MatrixFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/TextureFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/TextureFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/VertexFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/VertexFactory.h
)
source_group("Resource\\Factory" FILES ${Source_Files__Resource__Factories})

target_sources(libultraship PRIVATE ${Source_Files__Resource} ${Source_Files__Resource__Types} ${Source_Files__Resource__Factories})

#=================== Graphic ===================

set(Source_Files__Graphic
    ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_cc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_cc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_pc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_pc.cpp
)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    list(APPEND Source_Files__Graphic
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_opengl.h
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_opengl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_sdl.h
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_sdl2.cpp
    )
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND Source_Files__Graphic
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_dxgi.h
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_dxgi.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_direct3d11.h
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_direct3d11.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_direct3d12.h
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_direct3d12.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_direct3d_common.h
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_direct3d_common.cpp
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND Source_Files__Graphic
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_glx.h
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_glx.cpp
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    list(APPEND Source_Files__Graphic
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_wiiu.h
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_wiiu.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_gx2.h
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_gx2.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gx2_shader_gen.h
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gx2_shader_gen.c
    )
endif()

source_group("Graphic" FILES ${Source_Files__Graphic})
target_sources(libultraship PRIVATE ${Source_Files__Graphic})

#=================== STB ===================

set(Source_Files__Lib__stb
    "${CMAKE_CURRENT_SOURCE_DIR}/../extern/stb/stb_image.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../extern/stb/stb_image_write.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../extern/stb/stb_impl.c"
)
source_group("Extern\\stb" FILES ${Source_Files__Lib__stb})
target_sources(libultraship PRIVATE ${Source_Files__Lib__stb})


#=================== Packages & Includes ===================

target_include_directories(libultraship
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../extern
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../extern/spdlog/include ${CMAKE_CURRENT_SOURCE_DIR}/../extern/stb
)

if (CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${DEVKITPRO}/portlibs/wiiu/include/
    )
endif()

#=================== Linking ===================

target_link_libraries(libultraship
    PRIVATE ZAPDUtils tinyxml2 StrHash64
    PUBLIC ImGui Mercury storm
)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(libultraship PRIVATE Threads::Threads)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    find_Library(OSX_FOUNDATION Foundation)
    target_link_libraries(libultraship PRIVATE ${OSX_FOUNDATION} ${CMAKE_DL_LIBS})
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(X11)
    find_package(PulseAudio)
    target_link_libraries(libultraship PRIVATE ${X11_LIBRARIES} ${PULSEAUDIO_LIBRARY})
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

if (NOT CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    target_compile_definitions(libultraship PRIVATE
        ENABLE_OPENGL
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
        SPDLOG_ACTIVE_LEVEL=0
    )
else ()
    target_compile_definitions(libultraship PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
        SPDLOG_NO_THREAD_ID
        SPDLOG_NO_TLS
        STBI_NO_THREAD_LOCALS
        SPDLOG_ACTIVE_LEVEL=3
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

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
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
	)
endif()
