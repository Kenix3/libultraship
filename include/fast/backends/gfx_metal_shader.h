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
#include <memory>
#include <string>

namespace Ship {
class ResourceManager;
} // namespace Ship

struct CCFeatures;

/**
 * @brief Sets the ResourceManager used by Metal shader generation/loading helpers.
 */
void gfx_metal_shader_set_resource_manager(std::shared_ptr<Ship::ResourceManager> resourceManager);

/**
 * @brief Builds a Metal shader pair for the given combiner features.
 * @param result Receives generated shader source or diagnostic text.
 * @param numFloats Receives the number of float inputs consumed by the shader.
 * @param cc_features Color-combiner feature flags and mux decomposition.
 * @param three_point_filtering Enables three-point filtering support in generated code.
 * @return Vertex descriptor describing the shader's expected vertex layout.
 */
MTL::VertexDescriptor* gfx_metal_build_shader(std::string& result, size_t& numFloats, const CCFeatures& cc_features,
                                              bool three_point_filtering);

#endif
#endif
