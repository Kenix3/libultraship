---
title: Fast3D Renderer
nav_order: 4
---

# Nintendo 64 Fast3D Renderer

Implementation of a Fast3D renderer for games built originally for the Nintendo 64 platform.

For rendering, OpenGL, Direct3D 11, and Metal are supported.

Supported windowing systems are DXGI (used on Windows) and SDL (generic).

## Usage

See `interpreter.h` and the GBI headers in `include/libultraship/libultra/gbi.h`.

Each game main loop iteration should look like this:

```c
gfx_start_frame(); // Handles input events such as keyboard and window events
// perform game logic here
gfx_run(cmds); // submit display list and render a frame
// do more expensive work here, such as play audio
gfx_end_frame(); // this just waits until the frame is shown on the screen (vsync), to provide correct game timing
```

For the best experience, please change the `Vtx` and `Mtx` structures to use floats instead of fixed point arithmetic (`GBI_FLOATS`).
