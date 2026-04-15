# Libultraship (LUS) Porting Guide

**Libultraship (LUS)** is a library designed to facilitate PC ports of Nintendo 64 games. It is not a recompilation tool; rather, it allows developers to build a game's original source code (derived from decompilation) on modern hardware by providing a header-compatible wrapper around modern reimplementations of N64 SDK functions.

In essence, LUS acts as **"compile-time WINE"** for the N64.

---

## Supported Platforms

LUS supports the following platforms and rendering backends:

| Platform | Rendering Backends |
| :--- | :--- |
| **Windows** | OpenGL, Direct3D 11 |
| **macOS** | OpenGL, Metal |
| **Linux** | OpenGL |
| **iOS** | OpenGL, Metal |
| **Android** | OpenGL ES |
| **OpenBSD** | OpenGL |

---

## Prerequisites

1.  **A Game Decompilation:** You must have the source code of the game (e.g., *Star Fox 64*, *Banjo-Kazooie*) ready to build.
2.  **Template Project:** Use [**Ghostship**](https://github.com/HarbourMasters/Ghostship) as your reference. This is the standard example project used by existing ports to handle CMake configurations and asset management. Ghostship is a separate repository and is not included in this one.

---

## The Porting Process

### Phase 1: The "Stub" Build
The initial goal is not a playable game, but simply a functional executable that links against LUS.

1.  **Setup:**
    * Copy the CMake files from the [**Ghostship**](https://github.com/HarbourMasters/Ghostship) template into your decompilation directory.
    * Ensure `libultraship` is added as a submodule.

2.  **Header Replacement:**
    * Replace N64 SDK includes (e.g., `#include <ultra64.h>`) in the source code with LUS includes:
      ```c
      #include <libultraship/libultraship.h>
      ```
    * Use the public include path shown above unless you have explicitly configured your project's include directories to support a different form.

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
    * LUS supports the following GBI microcode variants via the `GBI_UCODE` CMake option: `F3DEX_GBI_2` (default), `F3DEX_GBI`, and `F3DLP_GBI`. Set this to match your game's microcode.
    * Custom UCodes (e.g., specific Rareware implementations) may need manual implementation or adjustments in the renderer.

### Phase 3: Assets (The Fork in the Road)
You have two options for handling game assets (textures, models, etc.):

#### Option A: OTR/O2R (The "Torch" Method) *Recommended*
* **Description:** Convert assets to the archive format used by LUS.
* **Archive Formats:**
  * **O2R** (`.o2r`): ZIP-based archive format, the modern default. No additional CMake options needed.
  * **OTR** (`.otr`): MPQ-based archive format (legacy). Requires enabling `INCLUDE_MPQ_SUPPORT` in CMake.
  * **Folder Archive**: For development, assets can be loaded directly from a folder without packaging.
* **Tools:** Use [**Torch**](https://github.com/HarbourMasters/Torch), an external tool designed to swap C files to path references and regenerate headers automatically. Torch is not included in this repository.
* **Pros:** Significantly faster workflow (e.g., enabled a *Star Fox 64* mini-build in one day).
* **Process:** Write exporters for Torch and importers for LUS to handle the specific game's asset types.

#### Option B: The Simple Method
* **Description:** Use the raw assets without OTR/O2R conversion.
* **Requirements:** You must manually "massage" the assets to handle endianness (byte order) and bit-width differences between N64 and PC.
* **Note:** It is a myth that OTR/O2R is required immediately. You can start testing a port without it, though it may be "illegal" or messy depending on the context.

### Phase 4: Audio
Audio is generally the last step. It requires significant effort to hook into LUS's audio system once the rest of the game is stable.

LUS only exposes PC audio drivers — it does not interface with N64 audio. The available audio backends are:
* **SDL** — cross-platform (Linux, Windows, macOS, etc.)
* **CoreAudio** — macOS / iOS
* **WASAPI** — Windows
* **Null** — silent backend for testing

Your port is responsible for implementing the game's audio pipeline and routing its output through one of these drivers.

---

## Build Configuration Options

Key CMake options that porters should be aware of:

| Option | Default | Description |
| :--- | :--- | :--- |
| `GBI_UCODE` | `F3DEX_GBI_2` | Graphics microcode variant (`F3DEX_GBI_2`, `F3DEX_GBI`, `F3DLP_GBI`) |
| `INCLUDE_MPQ_SUPPORT` | `OFF` | Enable OTR/MPQ archive support (via StormLib) |
| `NON_PORTABLE` | `OFF` | Build a non-portable version |
| `DISABLE_DLL_LOADER` | `OFF` | Disable dynamic library/script loading |
| `LUS_BUILD_TESTS` | `OFF` | Build unit tests |

---

## LUS Architecture

### Bridge Pattern

LUS uses a "bridge" pattern to connect N64 SDK function calls to modern implementations. Bridge modules exist for audio, graphics, controllers, resources, events, scripting, console variables, crash handling, and window management. When porting, you interact with these bridges through the standard N64 SDK API — LUS routes calls to the appropriate modern backend automatically.

The bridge implementations live in `src/libultraship/bridge/`.

### Directory Structure

Understanding the LUS codebase is helpful for debugging and contribution:

| Directory | Description |
| :--- | :--- |
| **`src/libultraship/`** | Contains N64-specific code including the libultra reimplementation (`libultra/`), bridge layers to modern backends (`bridge/`), controller support, window management, and logging. |
| **`src/ship/`** | Platform-agnostic code: resource/archive management, audio backends (SDL, CoreAudio, WASAPI), controller abstractions, configuration, scripting/DLL loading, security/keystore, and utility functions. |
| **`src/fast/`** | The Fast3D Renderer. Interprets N64 display lists as commands for modern GPUs. Supports OpenGL, Direct3D 11, and Metal backends. |
| **`include/`** | Public headers mirroring the `src/` structure. The main entry point is `libultraship/libultraship.h`. |