#pragma once

#include <string>
#include "ship/Component.h"

namespace Ship {
/**
 * @brief Abstract base class for all visible GUI elements (windows, menu bars, overlays).
 *
 * GuiElement provides the lifecycle skeleton (Init -> Draw/Update -> Show/Hide) that
 * every ImGui panel in the Ship GUI system must implement. Concrete subclasses are
 * GuiWindow (for floating ImGui windows) and GuiMenuBar (for the top-of-screen menu
 * bar), both of which override DrawElement(), OnInit(), and UpdateElement().
 *
 * GuiElement uses the Component initialization system. Calling Init() delegates to
 * Component::Init() → GuiElement::OnInit() exactly once. Subclasses override OnInit()
 * and must call the parent (GuiElement::OnInit() or Component::OnInit()) first.
 * Draw() and Update() are called by the Gui layer every frame for visible elements.
 *
 * @code
 * class MyWindow : public Ship::GuiWindow {
 *   public:
 *     using GuiWindow::GuiWindow;
 *   protected:
 *     void OnInit(const nlohmann::json& initArgs = nlohmann::json::object()) override {
 *         GuiWindow::OnInit(initArgs);
 *         // widget/resource setup goes here
 *     }
 *     void UpdateElement() override { }
 *     void DrawElement() override { ImGui::Text("Hello World"); }
 * };
 * @endcode
 */
class GuiElement : public Component {
  public:
    /**
     * @brief Constructs a GuiElement with an explicit initial visibility.
     * @param name      Component name.
     * @param isVisible true if the element should start visible.
     */
    GuiElement(const std::string& name, bool isVisible);

    /** @brief Constructs a GuiElement that starts hidden.
     * @param name Component name.
     */
    GuiElement(const std::string& name);
    virtual ~GuiElement();

    /**
     * @brief Renders the element for the current frame.
     *
     * Called every frame by the Gui layer when the element is visible. Delegates to
     * DrawElement() after any subclass-specific boilerplate (e.g. Begin/End calls).
     */
    virtual void Draw() = 0;

    /**
     * @brief Updates element state for the current frame (called before Draw).
     *
     * Delegates to UpdateElement() for subclass-specific logic.
     */
    void Update();

    /** @brief Makes the element visible (equivalent to SetVisibility(true)). */
    void Show();

    /** @brief Hides the element (equivalent to SetVisibility(false)). */
    void Hide();

    /** @brief Flips the visibility state between shown and hidden. */
    void ToggleVisibility();

    /** @brief Returns true if the element is currently visible. */
    bool IsVisible();

    /**
     * @brief Renders the element's content (pure ImGui draw calls).
     *
     * Implemented by concrete subclasses to emit ImGui widgets. For GuiWindow this
     * is called between ImGui::Begin() and ImGui::End(); for GuiMenuBar between
     * ImGui::BeginMainMenuBar() and ImGui::EndMainMenuBar().
     */
    virtual void DrawElement() = 0;

  protected:
    /**
     * @brief Component initialization hook.
     *
     * Called by Component::Init() exactly once. Subclasses override OnInit() for
     * their specific setup and must call GuiElement::OnInit(initArgs) (or
     * Component::OnInit(initArgs)) to preserve the initialization contract.
     */
    void OnInit(const nlohmann::json& initArgs = nlohmann::json::object()) override;

    /** @brief Per-frame logic update called by Update(). Subclasses perform state updates here. */
    virtual void UpdateElement() = 0;

    /**
     * @brief Changes the visibility flag and optionally persists it to a CVar.
     *
     * The base implementation simply sets mIsVisible. GuiWindow and GuiMenuBar override
     * this to also sync the value to their backing console variable.
     *
     * @param visible New visibility state.
     */
    virtual void SetVisibility(bool visible);

    bool mIsVisible; ///< Current visibility state. Subclasses may read this directly.
};

} // namespace Ship
