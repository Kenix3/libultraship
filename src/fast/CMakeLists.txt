#=================== Top-Level ===================
file(GLOB Source_Files__TopLevel RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")

source_group("" FILES ${Source_Files__TopLevel})
target_sources(libultraship PRIVATE ${Source_Files__TopLevel})

#=================== Debug ===================

file(GLOB Source_Files__Debug RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "debug/*.cpp")

source_group("debug" FILES ${Source_Files__Debug})
target_sources(libultraship PRIVATE ${Source_Files__Debug})

#=================== Resource ===================

file(GLOB Source_Files__Resource__Types RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "resource/type/*.h" "resource/type/*.cpp")
source_group("resource/type" FILES ${Source_Files__Resource__Types})

file(GLOB Source_Files__Resource__Factories RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "resource/factory/*.h" "resource/factory/*.cpp")
source_group("resource/factory" FILES ${Source_Files__Resource__Factories})

target_sources(libultraship PRIVATE ${Source_Files__Resource} ${Source_Files__Resource__Types} ${Source_Files__Resource__Factories} ${Source_Files__Resource__Archive})

#=================== Graphic ===================

file(GLOB Source_Files__Graphic RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "backends/*.cpp")

# Exclude backends that are conditionally compiled
list(FILTER Source_Files__Graphic EXCLUDE REGEX "backends/gfx_llgl.*")
list(FILTER Source_Files__Graphic EXCLUDE REGEX "backends/gfx_vulkan.*")

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "graphic/Fast3D/backends/gfx_dxgi*")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "graphic/Fast3D/backends/gfx_direct3d*")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "graphic/Fast3D/backends/dxsdk/*")
endif()

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "graphic/Fast3D/backends/gfx_metal*")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    list(FILTER Source_Files__Graphic EXCLUDE REGEX "graphic/Fast3D/backends/gfx_opengl*")
endif()

source_group("graphic" FILES ${Source_Files__Graphic})
target_sources(libultraship PRIVATE ${Source_Files__Graphic})

#=================== LLGL Backend ===================
if(LUS_BUILD_LLGL OR TARGET LLGL)
    target_sources(libultraship PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/backends/gfx_llgl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/backends/gfx_llgl_translation.cpp
    )
    target_link_libraries(libultraship PRIVATE LLGL)
    if(TARGET LLGL_OpenGL)
        target_link_libraries(libultraship PRIVATE LLGL_OpenGL)
    endif()
    if(TARGET LLGL_Vulkan)
        target_link_libraries(libultraship PRIVATE LLGL_Vulkan)
    endif()
    if(TARGET LLGL_Metal)
        target_link_libraries(libultraship PRIVATE LLGL_Metal)
    endif()
    if(TARGET LLGL_Direct3D11)
        target_link_libraries(libultraship PRIVATE LLGL_Direct3D11)
    endif()
    if(TARGET LLGL_Direct3D12)
        target_link_libraries(libultraship PRIVATE LLGL_Direct3D12)
    endif()
    # glslang (GLSL → SPIR-V) and SPIRV-Cross (SPIR-V → GLSL/HLSL/MSL)
    target_link_libraries(libultraship PRIVATE
        glslang
        glslang-default-resource-limits
        SPIRV          # glslang's SPIR-V writer
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-hlsl
        spirv-cross-msl
    )
    target_compile_definitions(libultraship PUBLIC ENABLE_LLGL=1)
    target_include_directories(libultraship PRIVATE
        $<TARGET_PROPERTY:LLGL,INTERFACE_INCLUDE_DIRECTORIES>
        ${glslang_SOURCE_DIR}
        ${spirv_cross_SOURCE_DIR}
    )
endif()

#=================== Vulkan / ParallelRDP Backend ===================
if(TARGET parallel-rdp)
    target_sources(libultraship PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/backends/gfx_vulkan.cpp
    )
    target_link_libraries(libultraship PRIVATE parallel-rdp)
    target_compile_definitions(libultraship PUBLIC ENABLE_VULKAN_PRDP=1)
endif()
