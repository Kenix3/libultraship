/**
 * @file classes.h
 * @brief Convenience umbrella header that includes all major public libultraship headers.
 *
 * Include this single header to gain access to the full public API surface of
 * libultraship: archives, resources, windowing, GUI, controllers, audio, and
 * configuration.
 */
#pragma once

#include "ship/core/Version.h"

#ifdef __cplusplus

#include "ship/resource/archive/ArchiveManager.h"
#include "ship/resource/archive/Archive.h"
#include "ship/resource/archive/OtrArchive.h"
#include "ship/resource/archive/O2rArchive.h"
#include "ship/resource/ResourceManager.h"
#include "ship/core/Context.h"
#include "ship/core/Part.h"
#include "ship/core/PartList.h"
#include "ship/core/Component.h"
#include "ship/core/ComponentList.h"
#include "ship/core/Action.h"
#include "ship/core/ActionList.h"
#include "ship/core/Tickable.h"
#include "ship/core/TickableComponent.h"
#include "ship/window/Window.h"
#include "ship/debug/Console.h"
#include "ship/debug/CrashHandler.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/config/Config.h"
#include "ship/window/gui/ComponentHierarchyWindow.h"
#include "ship/window/gui/ConsoleWindow.h"
#include "ship/window/gui/GameOverlay.h"
#include "ship/window/gui/Gui.h"
#include "ship/window/gui/GuiMenuBar.h"
#include "ship/window/gui/GuiElement.h"
#include "ship/window/gui/GuiWindow.h"
#include "ship/window/gui/StatsWindow.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "ship/controller/controldevice/controller/Controller.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/utils/binarytools/BinaryReader.h"
#include "ship/utils/binarytools/MemoryStream.h"
#include "ship/utils/binarytools/BinaryWriter.h"
#include "ship/audio/Audio.h"
#include "ship/audio/AudioPlayer.h"
#if defined(_WIN32)
#include "ship/audio/WasapiAudioPlayer.h"
#endif
#include "ship/audio/SDLAudioPlayer.h"
#include "ship/events/EventSystem.h"
#include "ship/security/Keystore.h"
#include "ship/log/Logger.h"
#include "ship/thread/ThreadPool.h"
#ifdef ENABLE_SCRIPTING
#include "ship/scripting/ScriptLoader.h"
#endif
#ifdef __APPLE__
#include "ship/utils/AppleFolderManager.h"
#endif
#endif
