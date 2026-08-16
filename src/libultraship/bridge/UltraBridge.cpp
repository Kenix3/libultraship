#include "UltraBridge.h"

#include "fast/Fast3dWindow.h"
#include "fast/debug/GfxDebugger.h"
#include "libultraship/bridge/audiobridge.h"
#include "libultraship/bridge/consolevariablebridge.h"
#include "libultraship/bridge/controllerbridge.h"
#include "libultraship/bridge/crashhandlerbridge.h"
#include "libultraship/bridge/eventsbridge.h"
#include "libultraship/bridge/gfxbridge.h"
#include "libultraship/bridge/gfxdebuggerbridge.h"
#include "libultraship/bridge/resourcebridge.h"
#include "libultraship/bridge/windowbridge.h"
#include "ship/audio/Audio.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/core/Context.h"
#include "ship/debug/CrashHandler.h"
#include "ship/events/Events.h"
#include "ship/resource/ResourceManager.h"
#include "ship/window/Window.h"
#ifdef ENABLE_SCRIPTING
#include "libultraship/bridge/scriptingbridge.h"
#include "ship/scripting/ScriptLoader.h"
#endif

namespace LUS {

UltraBridge::UltraBridge() : Bridge("UltraBridge") {
}

UltraBridge::~UltraBridge() {
    ClearCaches();
}

void UltraBridge::UpdateCaches(const std::shared_ptr<Ship::Context>& context) {
    ResourceSetResourceManager(context->GetChildren().GetFirst<Ship::ResourceManager>());
    CVarSetConsoleVariable(context->GetChildren().GetFirst<Ship::ConsoleVariable>());
    WindowSetWindowComponent(context->GetChildren().GetFirst<Ship::Window>());
    ControllerSetControlDeck(context->GetChildren().GetFirst<Ship::ControlDeck>());
    EventSystemSetEvents(context->GetChildren().GetFirst<Ship::Events>());
    AudioSetAudioComponent(context->GetChildren().GetFirst<Ship::Audio>());
    CrashHandlerSetComponent(context->GetChildren().GetFirst<Ship::CrashHandler>());
    GfxDebuggerSetComponent(context->GetChildren().GetFirst<Fast::GfxDebugger>());
    GfxSetFast3dWindow(std::dynamic_pointer_cast<Fast::Fast3dWindow>(context->GetChildren().GetFirst<Ship::Window>()));
#ifdef ENABLE_SCRIPTING
    ScriptSetLoader(context->GetChildren().GetFirst<Ship::ScriptLoader>());
#endif
}

void UltraBridge::ClearCaches() {
    ResourceSetResourceManager(nullptr);
    CVarSetConsoleVariable(nullptr);
    WindowSetWindowComponent(nullptr);
    ControllerSetControlDeck(nullptr);
    EventSystemSetEvents(nullptr);
    AudioSetAudioComponent(nullptr);
    CrashHandlerSetComponent(nullptr);
    GfxDebuggerSetComponent(nullptr);
    GfxSetFast3dWindow(nullptr);
#ifdef ENABLE_SCRIPTING
    ScriptSetLoader(nullptr);
#endif
}

} // namespace LUS
