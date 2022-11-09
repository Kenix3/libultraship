add_library(libultraship STATIC)
set_property(TARGET libultraship PROPERTY CXX_STANDARD 20)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)

#=================== Audio ===================

set(Source_Files__Audio
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/AudioPlayer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/mixer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/mixer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/SDLAudioPlayer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/SDLAudioPlayer.cpp
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
)

source_group("Core" FILES ${Source_Files__Core})
target_sources(libultraship PRIVATE ${Source_Files__Core})

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
)

source_group("Menu" FILES ${Source_Files__Menu})
target_sources(libultraship PRIVATE ${Source_Files__Menu})

#=================== Misc ===================

set(Source_Files__Misc
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/Cvar.h
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/Cvar.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/OtrFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/Resource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/Resource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceMgr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceMgr.cpp
)
source_group("Resource" FILES ${Source_Files__Resource})

set(Source_Files__Resource__Factory
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/ArrayFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/ArrayFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/AudioFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/AudioFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/BlobFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/BlobFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/CollisionHeaderFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/CollisionHeaderFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/CutsceneFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/CutsceneFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/DisplayListFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/DisplayListFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/MaterialFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/MaterialFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/MtxFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/MtxFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/PathFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/PathFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/PlayerAnimationFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/PlayerAnimationFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/ResourceLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/ResourceLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/SceneFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/SceneFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/SkeletonFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/SkeletonFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/SkeletonLimbFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/SkeletonLimbFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/TextFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/TextFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/TextureFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/TextureFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/VtxFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/factory/SoH/VtxFactory.h
)
source_group("Resource\\Factory" FILES ${Source_Files__Resource__Factory})

set(Source_Files__Resource__Types
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Animation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Animation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Array.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Array.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Audio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Audio.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Blob.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Blob.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/CollisionHeader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/CollisionHeader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Cutscene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Cutscene.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/DisplayList.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/DisplayList.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Material.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Material.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Matrix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Matrix.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Model.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Path.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Path.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/PlayerAnimation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/PlayerAnimation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Scene.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Skeleton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Skeleton.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/SkeletonLimb.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/SkeletonLimb.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Text.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Text.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Texture.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Vertex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/types/Vertex.h
)
source_group("Resource\\Types" FILES ${Source_Files__Resource__Types})


target_sources(libultraship PRIVATE ${Source_Files__Resource} ${Source_Files__Resource__Factory} ${Source_Files__Resource__Types})

#=================== Graphic ===================
# TODO: Make this a lib but currently it uses files from src

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


#=================== Packages & Includes ===================

target_include_directories(libultraship
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../extern
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${INCLUDE_DIR}
)

if (CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${DEVKITPRO}/portlibs/wiiu/include/
    )
endif()

#=================== Linking ===================

target_link_libraries(libultraship
    PRIVATE ZAPDUtils tinyxml2 StrHash64
    PUBLIC spdlog ImGui Mercury stb
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
    target_link_libraries(libultraship PRIVATE ${X11_LIBRARIES} ${PULSEAUDIO_LIBRARIES})
endif()

#=================== Compile Options & Defs ===================

if (NOT CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    target_compile_definitions(libultraship PRIVATE ENABLE_OPENGL)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(libultraship PRIVATE
        ENABLE_DX11
        "UNICODE;"
        "_UNICODE"
    )
endif()
