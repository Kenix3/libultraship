#pragma once

#include "ship/resource/Resource.h"
#include <stb_image.h>

namespace Ship {

/** @brief Resource type identifier for GUI texture resources (ASCII: "GTEX"). */
#define RESOURCE_TYPE_GUI_TEXTURE 0x47544558 // GTEX

/**
 * @brief Metadata describing a GUI texture after it has been uploaded to the renderer.
 */
struct GuiTextureMetadata {
    /** @brief Renderer-assigned texture identifier (e.g. OpenGL texture name). */
    uint32_t RendererTextureId;
    /** @brief Width of the texture in pixels. */
    int32_t Width;
    /** @brief Height of the texture in pixels. */
    int32_t Height;
};

/**
 * @brief A resource representing a texture intended for use in the GUI (ImGui).
 *
 * GuiTexture holds the raw decoded pixel data loaded from an archive and
 * associated renderer metadata once the texture has been uploaded.
 */
class GuiTexture : public Resource<void> {
  public:
    using Resource::Resource;

    /** @brief Constructs an empty GuiTexture resource. */
    GuiTexture();

    /** @brief Destructor. Frees the decoded pixel data. */
    ~GuiTexture();

    /**
     * @brief Returns a pointer to the raw pixel data.
     * @return Pointer to the pixel data buffer.
     */
    void* GetPointer() override;

    /**
     * @brief Returns the size of the raw pixel data in bytes.
     * @return Size of the pixel data buffer.
     */
    size_t GetPointerSize() override;

    /** @brief Raw decoded pixel data (RGBA). */
    uint8_t* Data;

    /** @brief Size of the pixel data buffer in bytes. */
    size_t DataSize;

    /** @brief Renderer metadata for this texture (ID, dimensions). */
    GuiTextureMetadata Metadata;
};
}; // namespace Ship
