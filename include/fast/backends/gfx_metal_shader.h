//
//  gfx_metal_shader.h
//  libultraship
//
//  Created by David Chavez on 16.08.22.
//

#ifdef __APPLE__
#ifdef __cplusplus
#pragma once
#include <stdio.h>
#include <string>

struct CCFeatures;

MTL::VertexDescriptor* gfx_metal_build_shader(std::string& result, size_t& numFloats, const CCFeatures& cc_features,
                                              bool three_point_filtering);

#endif
#endif
