#pragma once

#include "ship/window/gui/GuiWindow.h"

namespace Ship {

/**
 * @brief Auto-generated UI for shader-pack tweakables.
 *
 * Lists the post-processing passes contributed by archive manifests (with
 * enable toggles) and one widget per @setting(...) declaration discovered
 * while compiling custom shaders: sliders for float/int, checkboxes for
 * toggles, dropdowns for enums and color pickers for colors. Edits persist
 * to CVars (gShaderSettings.*) and recompile the shader cache on commit.
 *
 * Sections are labeled with the contributing shader pack's manifest name
 * when available, falling back to the shader template path.
 */
class ShaderSettingsWindow final : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    ~ShaderSettingsWindow() override = default;

    void InitElement() override{};
    void DrawElement() override;
    void UpdateElement() override{};
};

} // namespace Ship
