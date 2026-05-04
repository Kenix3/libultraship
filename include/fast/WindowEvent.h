#pragma once

// Fixes issue #926: ship/window/gui/Gui.h previously defined WindowEvent in the Ship namespace,
// but this type is only ever constructed and consumed by Fast3D backend code (gfx_sdl2.cpp,
// gfx_dxgi.cpp) and processed by Fast3dGui::HandleWindowEvents. Moving it to the Fast namespace
// enforces the correct include hierarchy: fast can include fast, ship cannot include fast.

namespace Fast {

/**
 * @brief Platform-agnostic wrapper around a window system event.
 *
 * Constructed by the Fast3D backends (gfx_sdl2, gfx_dxgi) and passed to
 * Fast3dGui::HandleWindowEvents() so that the ImGui backend can process input
 * events without the ship layer depending on any Fast3D or platform-specific type.
 */
typedef union {
    struct {
        void* Handle; ///< HWND
        int Msg;      ///< Windows message ID.
        int Param1;   ///< WPARAM
        int Param2;   ///< LPARAM
    } Win32;
    struct {
        void* Event; ///< SDL_Event*
    } Sdl;
    struct {
        void* Input; ///< GX2 input structure pointer.
    } Gx2;
} WindowEvent;

} // namespace Fast
