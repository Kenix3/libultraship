//
//  gfx_metal.h
//  libultraship
//
//  Created by David Chavez on 16.08.22.
//

#ifdef SDL_PLATFORM_APPLE

#ifndef GFX_METAL_H
#define GFX_METAL_H

#include "gfx_rendering_api.h"
#include <imgui_impl_sdl3.h>

extern struct GfxRenderingAPI gfx_metal_api;

ImTextureID gfx_metal_get_texture_by_id(int id);

bool Metal_IsSupported();

bool Metal_Init(SDL_Renderer* renderer);
void Metal_SetupFrame(SDL_Renderer* renderer);
void Metal_NewFrame(SDL_Renderer* renderer);
void Metal_SetupFloatingFrame();
void Metal_RenderDrawData(ImDrawData* draw_data);

#endif
#endif
