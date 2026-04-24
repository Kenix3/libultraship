#pragma once

#include "ship/window/gui/GuiWindow.h"

namespace Ship {

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
    using GuiWindow::GuiWindow;
    virtual ~SDLAddRemoveDeviceEventHandler();

  protected:
    /** @brief No-op – this handler has no ImGui state to initialize. */
    void InitElement() override;

    /** @brief No-op – this handler does not draw any visible UI. */
    void DrawElement() override;

    /** @brief Polls SDL events and dispatches device connect/disconnect callbacks. */
    void UpdateElement() override;
};
} // namespace Ship
