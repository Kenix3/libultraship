#version 450
// Vertex format: [x, y, z, w, u, v] – the same interleaved layout used in
// RenderFast3DTexturedQuad.  Positions are in "game clip space":
//   x = pixel_x - 160   (for a 320-wide framebuffer)
//   y = 120 - pixel_y   (for a 240-tall framebuffer)
// We convert to Vulkan NDC here: x/160, -y/120.
layout(location = 0) in vec4 inPos;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 outUV;

void main() {
    gl_Position = vec4(inPos.x / 160.0, -inPos.y / 120.0, 0.0, 1.0);
    outUV = inUV;
}
