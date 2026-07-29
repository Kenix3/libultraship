#pragma once

#include "ship/window/gui/GuiWindow.h"
#include <memory>

namespace Ship {
class ConsoleVariable;
class ControlDeck;
class Window;

/**
 * @brief Hidden GuiWindow that listens for SDL controller add/remove events.
 *
 * SDLAddRemoveDeviceEventHandler is registered as a GuiWindow solely so it
 * participates in the update loop. It does not draw any UI; instead its
 * UpdateElement() polls SDL events and forwards device connection/disconnection
 * notifications to the ConnectedPhysicalDeviceManager.
 */
class SDLAddRemoveDeviceEventHandler : public GuiWindow {
  public:
    SDLAddRemoveDeviceEventHandler(std::shared_ptr<ConsoleVariable> consoleVariable, std::shared_ptr<Window> window,
                                   std::shared_ptr<ControlDeck> controlDeck, const std::string& visibilityCvar,
                                   const std::string& name);
    virtual ~SDLAddRemoveDeviceEventHandler();

  protected:
    /** @brief No-op – this handler does not draw any visible UI. */
    void DrawElement() override;

    /** @brief Polls SDL events and dispatches device connect/disconnect callbacks. */
    void UpdateElement() override;

  private:
    std::shared_ptr<ControlDeck> mControlDeck;
};
} // namespace Ship
