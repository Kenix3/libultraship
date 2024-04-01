#ifndef GFX_BRIDGE_H
#define GFX_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum UcodeHandlers {
	ucode_f3dex2,
	ucode_s2dex,
	ucode_max,
} UcodeHandlers;

void gfx_set_ucode_handler(UcodeHandlers ucode);


#ifdef __cplusplus
}
#endif

#endif
