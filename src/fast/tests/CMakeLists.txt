#=================== Fast3D Tests ===================
#
# Tests organized by layer:
#   Layer 1: Pure computation (macros, matrix math, CC features, texture import)
#   Layer 2: Single interpreter methods with state (RDP/RSP state, matrix ops)
#   Layer 3: Multi-step pipelines (vertex+lighting, display list dispatch)
#   Layer 4: End-to-end rendering pipeline (triangle draw → API calls)
#   Layer 5: Reference RDP implementation comparison (N64 RDP spec + ParallelRDP)
#   Prism:   Shader template processor tests

target_sources(lus_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/fast3d_layer1_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fast3d_layer2_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fast3d_layer3_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fast3d_layer4_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fast3d_layer5_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fast3d_dl_pixel_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fast3d_prism_tests.cpp
)

target_include_directories(lus_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ParallelRDP comparison tests (optional, requires Vulkan)
if(LUS_BUILD_PRDP_TESTS)
    target_link_libraries(lus_tests PRIVATE parallel-rdp)
    target_compile_definitions(lus_tests PRIVATE LUS_PRDP_TESTS_ENABLED=1)
endif()
