#pragma once

#if defined(ENABLE_DX11) || defined(ENABLE_DX12)

#ifdef __cplusplus
#include "gfx_cc.h"
#include <cstdint>
#include <string>

std::string gfx_direct3d_common_build_shader(size_t& num_floats, const CCFeatures& cc_features,
                                             bool include_root_signature, bool three_point_filtering, bool use_srgb);

#endif
#endif
