# ParallelRDP integration for comparison tests.
# This fetches the standalone ParallelRDP codebase (MIT licensed) from
# Themaister and builds it as a static library for use in GPU-based
# reference rendering tests.

include(FetchContent)

FetchContent_Declare(
    parallel-rdp-standalone
    GIT_REPOSITORY https://github.com/Themaister/parallel-rdp-standalone.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(parallel-rdp-standalone)

set(PRDP_SRC ${parallel-rdp-standalone_SOURCE_DIR})

# --- Build the parallel-rdp static library from source ---

# Collect C++ sources from config.mk
set(PRDP_CXX_SOURCES
    ${PRDP_SRC}/parallel-rdp/command_ring.cpp
    ${PRDP_SRC}/parallel-rdp/rdp_device.cpp
    ${PRDP_SRC}/parallel-rdp/rdp_dump_write.cpp
    ${PRDP_SRC}/parallel-rdp/rdp_renderer.cpp
    ${PRDP_SRC}/parallel-rdp/video_interface.cpp
    ${PRDP_SRC}/vulkan/buffer.cpp
    ${PRDP_SRC}/vulkan/buffer_pool.cpp
    ${PRDP_SRC}/vulkan/command_buffer.cpp
    ${PRDP_SRC}/vulkan/command_pool.cpp
    ${PRDP_SRC}/vulkan/context.cpp
    ${PRDP_SRC}/vulkan/cookie.cpp
    ${PRDP_SRC}/vulkan/descriptor_set.cpp
    ${PRDP_SRC}/vulkan/device.cpp
    ${PRDP_SRC}/vulkan/event_manager.cpp
    ${PRDP_SRC}/vulkan/fence.cpp
    ${PRDP_SRC}/vulkan/fence_manager.cpp
    ${PRDP_SRC}/vulkan/image.cpp
    ${PRDP_SRC}/vulkan/indirect_layout.cpp
    ${PRDP_SRC}/vulkan/memory_allocator.cpp
    ${PRDP_SRC}/vulkan/pipeline_event.cpp
    ${PRDP_SRC}/vulkan/query_pool.cpp
    ${PRDP_SRC}/vulkan/render_pass.cpp
    ${PRDP_SRC}/vulkan/sampler.cpp
    ${PRDP_SRC}/vulkan/semaphore.cpp
    ${PRDP_SRC}/vulkan/semaphore_manager.cpp
    ${PRDP_SRC}/vulkan/shader.cpp
    ${PRDP_SRC}/vulkan/texture/texture_format.cpp
    ${PRDP_SRC}/util/arena_allocator.cpp
    ${PRDP_SRC}/util/logging.cpp
    ${PRDP_SRC}/util/thread_id.cpp
    ${PRDP_SRC}/util/aligned_alloc.cpp
    ${PRDP_SRC}/util/timer.cpp
    ${PRDP_SRC}/util/timeline_trace_file.cpp
    ${PRDP_SRC}/util/environment.cpp
    ${PRDP_SRC}/util/thread_name.cpp
)

# Volk (Vulkan loader) is a C source
set(PRDP_C_SOURCES
    ${PRDP_SRC}/volk/volk.c
)

add_library(parallel-rdp STATIC ${PRDP_CXX_SOURCES} ${PRDP_C_SOURCES})

target_include_directories(parallel-rdp PUBLIC
    ${PRDP_SRC}/parallel-rdp
    ${PRDP_SRC}/volk
    ${PRDP_SRC}/vulkan
    ${PRDP_SRC}/vulkan-headers/include
    ${PRDP_SRC}/util
)

# ParallelRDP needs the pre-compiled SPIR-V shaders from slangmosh.hpp.
# By NOT defining PARALLEL_RDP_SHADER_DIR, we use the embedded shader bank
# which avoids the Granite filesystem dependency entirely.
target_compile_definitions(parallel-rdp PRIVATE
    GRANITE_VULKAN_MT=0
)

target_link_libraries(parallel-rdp PRIVATE
    ${CMAKE_DL_LIBS}
    Threads::Threads
)

set_target_properties(parallel-rdp PROPERTIES
    CXX_STANDARD 17
    C_STANDARD 11
    POSITION_INDEPENDENT_CODE ON
)

# Suppress warnings in third-party code
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(parallel-rdp PRIVATE -w)
endif()
