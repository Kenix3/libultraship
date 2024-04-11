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

set(Source_Files__TopLevel
    ${CMAKE_CURRENT_SOURCE_DIR}/Context.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Context.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/install_config.h
)

source_group("" FILES ${Source_Files__TopLevel})
target_sources(libultraship PRIVATE ${Source_Files__TopLevel})

#=================== Audio ===================

set(Source_Files__Audio
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/Audio.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/Audio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/AudioPlayer.h
	${CMAKE_CURRENT_SOURCE_DIR}/audio/AudioPlayer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/SDLAudioPlayer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/SDLAudioPlayer.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND Source_Files__Audio ${CMAKE_CURRENT_SOURCE_DIR}/audio/WasapiAudioPlayer.h)
    list(APPEND Source_Files__Audio ${CMAKE_CURRENT_SOURCE_DIR}/audio/WasapiAudioPlayer.cpp)
endif()

source_group("audio" FILES ${Source_Files__Audio})
target_sources(libultraship PRIVATE ${Source_Files__Audio})

#=================== Controller ===================

set(Source_Files__Controller
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldeck/ControlDeck.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldeck/ControlDeck.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldeck/ControlPort.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldeck/ControlPort.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/ControlDevice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/ControlDevice.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/ControllerButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/ControllerButton.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/Controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/Controller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/ControllerGyro.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/ControllerGyro.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/ControllerLED.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/ControllerLED.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/ControllerRumble.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/ControllerRumble.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/ControllerStick.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/ControllerStick.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/ControllerButtonMapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/ControllerButtonMapping.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/ControllerGyroMapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/ControllerGyroMapping.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/ControllerInputMapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/ControllerInputMapping.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/ControllerLEDMapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/ControllerLEDMapping.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/ControllerMapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/ControllerMapping.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/ControllerRumbleMapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/ControllerRumbleMapping.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/factories/AxisDirectionMappingFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/factories/AxisDirectionMappingFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/factories/ButtonMappingFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/factories/ButtonMappingFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/factories/GyroMappingFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/factories/GyroMappingFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/factories/LEDMappingFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/factories/LEDMappingFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/factories/RumbleMappingFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/factories/RumbleMappingFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/deviceindex/ControllerDisconnectedWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/deviceindex/ControllerDisconnectedWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/deviceindex/ControllerReorderingWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/deviceindex/ControllerReorderingWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/deviceindex/LUSDeviceIndex.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/deviceindex/LUSDeviceIndexMappingManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/deviceindex/LUSDeviceIndexMappingManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/deviceindex/LUSDeviceIndexToPhysicalDeviceIndexMapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/deviceindex/LUSDeviceIndexToPhysicalDeviceIndexMapping.h
)

if (CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    list(APPEND Source_Files__Controller
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/wiiu/WiiUAxisDirectionToAxisDirectionMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/wiiu/WiiUAxisDirectionToAxisDirectionMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/wiiu/WiiUButtonToAnyMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/wiiu/WiiUButtonToAnyMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/wiiu/WiiUButtonToAxisDirectionMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/wiiu/WiiUButtonToAxisDirectionMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/wiiu/WiiUButtonToButtonMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/wiiu/WiiUButtonToButtonMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/wiiu/WiiUGyroMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/wiiu/WiiUGyroMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/wiiu/WiiUMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/wiiu/WiiUMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/wiiu/WiiURumbleMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/wiiu/WiiURumbleMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/deviceindex/LUSDeviceIndexToWiiUDeviceIndexMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/deviceindex/LUSDeviceIndexToWiiUDeviceIndexMapping.h
    )
endif()

if (NOT CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    list(APPEND Source_Files__Controller
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/keyboard/KeyboardKeyToAnyMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/keyboard/KeyboardKeyToAnyMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/keyboard/KeyboardKeyToAxisDirectionMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/keyboard/KeyboardKeyToAxisDirectionMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/keyboard/KeyboardKeyToButtonMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/keyboard/KeyboardKeyToButtonMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToAnyMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToAnyMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToAxisDirectionMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToAxisDirectionMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToButtonMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToButtonMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLButtonToAnyMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLButtonToAnyMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLButtonToAxisDirectionMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLButtonToAxisDirectionMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLButtonToButtonMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLButtonToButtonMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLGyroMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLGyroMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLLEDMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLLEDMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLRumbleMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/controldevice/controller/mapping/sdl/SDLRumbleMapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/deviceindex/LUSDeviceIndexToSDLDeviceIndexMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/deviceindex/LUSDeviceIndexToSDLDeviceIndexMapping.h
    )
endif()

source_group("controller" FILES ${Source_Files__Controller})
target_sources(libultraship PRIVATE ${Source_Files__Controller})

#=================== Config ===================

set(Source_Files__Config
    ${CMAKE_CURRENT_SOURCE_DIR}/config/ConsoleVariable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/config/ConsoleVariable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/config/Config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/config/Config.cpp
)

source_group("config" FILES ${Source_Files__Config})
target_sources(libultraship PRIVATE ${Source_Files__Config})

#=================== Public ===================

source_group("public")

set(Source_Files__Public__Libultra
    ${CMAKE_CURRENT_SOURCE_DIR}/public/libultra/os.h
    ${CMAKE_CURRENT_SOURCE_DIR}/public/libultra/os.cpp
)

source_group("public/libultra" FILES ${Source_Files__Public__Libultra})

set(Source_Files__Public__Bridge
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/resourcebridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/resourcebridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/windowbridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/windowbridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/audiobridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/audiobridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/gfxbridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/gfxbridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/gfxdebuggerbridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/gfxdebuggerbridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/controllerbridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/controllerbridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/consolevariablebridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/consolevariablebridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/crashhandlerbridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/public/bridge/crashhandlerbridge.cpp
)

source_group("public/bridge" FILES ${Source_Files__Public__Bridge})
target_sources(libultraship PRIVATE ${Source_Files__Public__Libultra} ${Source_Files__Public__Bridge})

#=================== Debug ===================

set(Source_Files__Debug
    ${CMAKE_CURRENT_SOURCE_DIR}/debug/CrashHandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/debug/CrashHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/debug/Console.h
    ${CMAKE_CURRENT_SOURCE_DIR}/debug/Console.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/debug/GfxDebugger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/debug/GfxDebugger.h
)

source_group("debug" FILES ${Source_Files__Debug})
target_sources(libultraship PRIVATE ${Source_Files__Debug})

#=================== Log ===================

set(Source_Files__Log
    ${CMAKE_CURRENT_SOURCE_DIR}/log/luslog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/log/luslog.cpp
)

source_group("log" FILES ${Source_Files__Log})

target_sources(libultraship PRIVATE ${Source_Files__Log} ${Source_Files__Log__SPDLog})

#=================== Window ===================
set(Source_Files__Window
    ${CMAKE_CURRENT_SOURCE_DIR}/window/Window.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/Window.cpp
)

source_group("window" FILES ${Source_Files__Window})
target_sources(libultraship PRIVATE ${Source_Files__Window})

#=================== Gui ===================

set(Source_Files__Window__Gui
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/ConsoleWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/ConsoleWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/Gui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/Gui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/InputEditorWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/InputEditorWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/GameOverlay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/GameOverlay.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/Fonts.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/IconsFontAwesome4.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/StatsWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/StatsWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/GuiWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/GuiWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/GuiElement.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/GuiElement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/GuiMenuBar.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/GuiMenuBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/GfxDebuggerWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/GfxDebuggerWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/resource/Font.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/resource/Font.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/resource/FontFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/resource/FontFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/resource/GuiTexture.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/resource/GuiTexture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/resource/GuiTextureFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window/gui/resource/GuiTextureFactory.cpp
)

source_group("window/gui" FILES ${Source_Files__Window__Gui})
target_sources(libultraship PRIVATE ${Source_Files__Window__Gui})

if (GFX_DEBUG_DISASSEMBLER)
    set(Header_Files__Libraries__libgfxd
        "../../ZAPDTR/lib/libgfxd/gbi.h"
        "../../ZAPDTR/lib/libgfxd/gfxd.h"
        "../../ZAPDTR/lib/libgfxd/priv.h"
    )
    source_group("Header Files\\Libraries\\libgfxd" FILES ${Header_Files__Libraries__libgfxd})

    set(Source_Files__Libraries__libgfxd
        "../../ZAPDTR/lib/libgfxd/gfxd.c"
        "../../ZAPDTR/lib/libgfxd/uc.c"
        "../../ZAPDTR/lib/libgfxd/uc_f3d.c"
        "../../ZAPDTR/lib/libgfxd/uc_f3db.c"
        "../../ZAPDTR/lib/libgfxd/uc_f3dex.c"
        "../../ZAPDTR/lib/libgfxd/uc_f3dex2.c"
        "../../ZAPDTR/lib/libgfxd/uc_f3dexb.c"
    )
    source_group("Source Files\\Libraries\\libgfxd" FILES ${Source_Files__Libraries__libgfxd})

    target_sources(libultraship PRIVATE ${Header_Files__Libraries__libgfxd} ${Source_Files__Libraries__libgfxd})
endif()

#=================== Utils ===================

set(Source_Files__Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/stox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/stox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/Utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/Utils.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND Source_Files__Utils
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/OSXFolderManager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/OSXFolderManager.mm
    )

    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/utils/OSXFolderManager.mm PROPERTIES COMPILE_FLAGS -fno-objc-arc)
endif()

source_group("utils" FILES ${Source_Files__Utils})
target_sources(libultraship PRIVATE ${Source_Files__Utils})

#=================== BinaryTools ==================

set(Source_Files__Utils__BinaryTools
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/binarytools/BinaryReader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/binarytools/BinaryReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/binarytools/BinaryWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/binarytools/BinaryWriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/binarytools/endianness.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/binarytools/MemoryStream.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/binarytools/MemoryStream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/binarytools/Stream.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/binarytools/Stream.cpp
)

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
elseif (CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(Source_Files__Port
        ${CMAKE_CURRENT_SOURCE_DIR}/port/android/AndroidImpl.h
        ${CMAKE_CURRENT_SOURCE_DIR}/port/android/AndroidImpl.cpp
    )
endif()

source_group("port" FILES ${Source_Files__Port})
target_sources(libultraship PRIVATE ${Source_Files__Port})

#=================== Resource ===================

set(Source_Files__Resource
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/File.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/Resource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceType.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/Resource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceFactoryBinary.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceFactoryBinary.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceFactoryXML.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/ResourceFactoryXML.cpp
)
source_group("resource" FILES ${Source_Files__Resource})

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
source_group("resource/type" FILES ${Source_Files__Resource__Types})

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
source_group("resource/factory" FILES ${Source_Files__Resource__Factories})

set(Source_Files__Resource__Archive
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/archive/Archive.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/archive/Archive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/archive/OtrArchive.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/archive/OtrArchive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/archive/O2rArchive.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/archive/O2rArchive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/archive/ArchiveManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/archive/ArchiveManager.cpp
)
source_group("resource/archive" FILES ${Source_Files__Resource__Archive})

target_sources(libultraship PRIVATE ${Source_Files__Resource} ${Source_Files__Resource__Types} ${Source_Files__Resource__Factories} ${Source_Files__Resource__Archive})

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
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND Source_Files__Graphic
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_metal.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_metal.h
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_metal_shader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic/Fast3D/gfx_metal_shader.h
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

source_group("graphic" FILES ${Source_Files__Graphic})
target_sources(libultraship PRIVATE ${Source_Files__Graphic})

#=================== STB ===================

set(Source_Files__Lib__stb
    "${CMAKE_CURRENT_SOURCE_DIR}/../extern/stb/stb_image.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../extern/stb/stb_image_write.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../extern/stb/stb_impl.c"
)
source_group("extern/stb" FILES ${Source_Files__Lib__stb})
target_sources(libultraship PRIVATE ${Source_Files__Lib__stb})

#=================== metal-cpp ===================

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_include_directories(libultraship PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../extern/metal-cpp)
endif()


#=================== Packages & Includes ===================

target_include_directories(libultraship
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../extern ${CMAKE_CURRENT_BINARY_DIR} $<$<BOOL:${GFX_DEBUG_DISASSEMBLER}>:${CMAKE_CURRENT_SOURCE_DIR}/../../ZAPDTR/lib/libgfxd>
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../extern/spdlog/include ${CMAKE_CURRENT_SOURCE_DIR}/../extern/stb
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
    PUBLIC ZAPDUtils ImGui tinyxml2 nlohmann_json::nlohmann_json
)

find_package(libzip REQUIRED)
target_link_libraries(libultraship PRIVATE libzip::zip)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(libultraship PRIVATE Threads::Threads)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
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

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    )
endif()

if (NOT CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    target_compile_definitions(libultraship PRIVATE
        ENABLE_OPENGL
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=0>
        $<$<NOT:$<CONFIG:Debug>>:SPDLOG_ACTIVE_LEVEL=1>
        $<$<BOOL:${USE_OPENGLES}>:USE_OPENGLES>
        $<$<BOOL:${GFX_DEBUG_DISASSEMBLER}>:GFX_DEBUG_DISASSEMBLER>
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
        $<$<COMPILE_LANGUAGE:C,OBJC>:-Wno-int-conversion>
	)
endif()
