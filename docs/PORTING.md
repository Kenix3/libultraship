# Libultraship (LUS) Porting Guide

**Libultraship (LUS)** is a library designed to facilitate PC ports of Nintendo 64 games. It is not a recompilation tool; rather, it allows developers to build a game's original source code (derived from decompilation) on modern hardware by providing a header-compatible wrapper around modern reimplementations of N64 SDK functions.

In essence, LUS acts as **"compile-time WINE"** for the N64.

---

## Prerequisites

1.  **A Game Decompilation:** You must have the source code of the game (e.g., *Star Fox 64*, *Banjo-Kazooie*) ready to build.
2.  **Template Project:** Use **"Ghostship"** as your reference. This is the standard example project used by existing ports to handle CMake configurations and asset management.

---

## The Porting Process

### Phase 1: The "Stub" Build
The initial goal is not a playable game, but simply a functional executable that links against LUS.

1.  **Setup:** * Copy the CMake files from the **Ghostship** template into your decompilation directory.
    * Ensure `libultraship` is added as a submodule.

2.  **Header Replacement:**
    * Replace N64 SDK includes (e.g., `#include <ultra64.h>`) in the source code with LUS includes:
      ```c
      #include <libultraship/libultraship.h>
      // or
      #include <libultraship.h>
      ```

3.  **Compile & Fix:**
    * Attempt to build. You will encounter thousands of errors.
    * **Stubbing:** Comment out or stub functions that LUS handles automatically (e.g., `osCreateThread` for rumble, as LUS manages threads differently than the N64 CPU).
    * **Disable Systems:** Temporarily disable complex systems like Audio to simplify the build.
    * **64-bit Fixes:** Address pointer size differences and other architecture-specific issues.
    * **Iterate:** Rinse and repeat until you successfully generate an executable (`.exe` or binary).

### Phase 2: Runtime & Logic
Once the executable exists, the game will likely crash immediately or behave erratically.

1.  **Debugging:** Use breakpoints to trace the logic and fix crashes as they arise.
2.  **Microcode (UCode):** Verify the game's graphics microcode.
    * LUS's `Fast3D` renderer covers many standard UCodes.
    * Custom UCodes (e.g., specific Rareware implementations) may need manual implementation or adjustments in the renderer.

### Phase 3: Assets (The Fork in the Road)
You have two options for handling game assets (textures, models, etc.):

#### Option A: OTR (The "Torch" Method) *Recommended*
* **Description:** Convert assets to the OTR/O2R format used by LUS.
* **Tools:** Use **Torch**, a tool designed to swap C files to path references and regenerate headers automatically.
* **Pros:** Significantly faster workflow (e.g., enabled a *Star Fox 64* mini-build in one day).
* **Process:** Write exporters for Torch and importers for LUS to handle the specific game's asset types.

#### Option B: The Simple Method
* **Description:** Use the raw assets without OTR conversion.
* **Requirements:** You must manually "massage" the assets to handle endianness (byte order) and bit-width differences between N64 and PC.
* **Note:** It is a myth that OTR is required immediately. You can start testing a port without it, though it may be "illegal" or messy depending on the context.

### Phase 4: Audio
Audio is generally the last step. It requires significant effort to hook into LUS's audio backend once the rest of the game is stable.

---

## LUS Directory Structure

Understanding the LUS codebase is helpful for debugging and contribution:

| Directory | Description |
| :--- | :--- |
| **`LUS/`** | Contains N64-specific code and the libultra reimplementation. |
| **`Ship/`** | Intended to be platform-agnostic code (though may contain some residual N64 logic). |
| **`Fast/`** | The Renderer. It interprets PC-formatted display lists (which are not binary compatible with N64 on 64-bit machines) as commands for modern GPUs. |