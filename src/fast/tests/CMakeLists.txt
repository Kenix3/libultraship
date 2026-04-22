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

    # PNG output for three-way texture comparison tests
    find_package(PNG REQUIRED)
    target_link_libraries(lus_tests PRIVATE PNG::PNG)

    # Ensure the docs/images directory exists at build time so tests can write
    # reference images without having to create it at runtime.
    set(DOCS_IMAGES_DIR "${CMAKE_SOURCE_DIR}/docs/images")
    file(MAKE_DIRECTORY "${DOCS_IMAGES_DIR}")
endif()

# LLGL backend for offscreen texture tests.
# NOTE: LUS_BUILD_LLGL and LUS_BUILD_PRDP_TESTS are mutually exclusive (see
# the root CMakeLists.txt for details).  This block therefore only fires when
# LLGL is requested without ParallelRDP.
if(TARGET LLGL)
    target_compile_definitions(lus_tests PRIVATE LUS_LLGL_TESTS_ENABLED=1)
    target_link_libraries(lus_tests PRIVATE LLGL)
    # When LLGL is built as a static library every platform renderer that was
    # compiled needs to be linked so that its StaticModule global object is
    # pulled in (LLGL's StaticModuleInterface.cpp references them directly).
    foreach(_llgl_renderer LLGL_OpenGL LLGL_Vulkan LLGL_Metal LLGL_Direct3D11 LLGL_Direct3D12 LLGL_Null)
        if(TARGET ${_llgl_renderer})
            target_link_libraries(lus_tests PRIVATE ${_llgl_renderer})
        endif()
    endforeach()
    target_include_directories(lus_tests PRIVATE
        $<TARGET_PROPERTY:LLGL,INTERFACE_INCLUDE_DIRECTORIES>
        ${CMAKE_SOURCE_DIR}/src/fast/shaders
    )
endif()
