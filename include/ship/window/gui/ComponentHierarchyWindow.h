#pragma once

#include "ship/window/gui/GuiWindow.h"
#include <memory>

namespace Ship {
class ConsoleVariable;
class Window;

/**
 * @brief A GuiWindow that displays the Component hierarchy tree and the list of
 * TickableComponents registered with the Context.
 *
 * This window is useful for debugging the component structure at runtime. It calls
 * Component::ToTreeString() on the Context and renders the result as selectable text
 * inside an ImGui child scrolling region.
 */
class ComponentHierarchyWindow : public GuiWindow {
  public:
    ComponentHierarchyWindow(std::shared_ptr<ConsoleVariable> consoleVariable, std::shared_ptr<Window> window,
                             const std::string& visibilityCvar, const std::string& name);
    virtual ~ComponentHierarchyWindow();

  protected:
    void DrawElement() override;
};
} // namespace Ship
