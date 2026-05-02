/**
 * @file classes.h
 * @brief Convenience umbrella header that includes all major public libultraship headers.
 *
 * Include this single header to gain access to the full public API surface of
 * libultraship: archives, resources, windowing, GUI, controllers, audio, and
 * configuration.
 */
#pragma once
#ifdef __cplusplus

#include "ship/resource/archive/ArchiveManager.h"
#include "ship/resource/archive/Archive.h"
#include "ship/resource/archive/OtrArchive.h"
#include "ship/resource/archive/O2rArchive.h"
#include "ship/resource/ResourceManager.h"
#include "ship/Context.h"
#include "ship/Part.h"
#include "ship/PartList.h"
#include "ship/Component.h"
#include "ship/ComponentList.h"
#include "ship/Action.h"
#include "ship/ActionList.h"
#include "ship/Tickable.h"
#include "ship/TickableComponent.h"
#include "ship/window/Window.h"
#include "ship/debug/Console.h"
#include "ship/debug/CrashHandler.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/config/Config.h"
#include "ship/window/gui/ConsoleWindow.h"
#include "ship/window/gui/GameOverlay.h"
#include "ship/window/gui/Gui.h"
#include "ship/window/gui/GuiMenuBar.h"
#include "ship/window/gui/GuiElement.h"
#include "ship/window/gui/GuiWindow.h"
#include "ship/window/gui/InputEditorWindow.h"
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
#include "ship/log/LoggerComponent.h"
#include "ship/thread/ThreadPoolComponent.h"
#ifdef ENABLE_SCRIPTING
#include "ship/scripting/ScriptLoader.h"
#endif
#ifdef __APPLE__
#include "ship/utils/AppleFolderManager.h"
#endif
#endif
