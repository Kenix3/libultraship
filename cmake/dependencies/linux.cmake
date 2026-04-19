#=================== ImGui ===================
find_package(SDL2 REQUIRED)
target_link_libraries(ImGui PUBLIC SDL2::SDL2)

if (USE_OPENGLES)
    target_link_libraries(ImGui PUBLIC ${OPENGL_GLESv2_LIBRARY})
    add_compile_definitions(IMGUI_IMPL_OPENGL_ES3)
else()
    target_link_libraries(ImGui PUBLIC ${OPENGL_opengl_LIBRARY})
endif()

if (ENABLE_VULKAN)
    find_package(Vulkan REQUIRED)
    # SDL2 built with Vulkan support is needed; no extra target_link_libraries
    # required here as Vulkan::Vulkan is handled in src/CMakeLists.txt.
endif()
