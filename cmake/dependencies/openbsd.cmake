#=================== ImGui ===================

find_package(SDL2 REQUIRED)

# XXX sdl2-config.cmake doesn't include /usr/X11R6/include but sdl2-config does
# ideally this could be fixed from sdl2 port
# set(SDL2_INCLUDE_DIRS "${SDL2_INCLUDE_DIRS};/usr/X11R6/include")
# note this is a central place to bundle such hacks for libultraship consumers
# otherwise each game may need its own fix as we are not in the same scope
include_directories("/usr/X11R6/include")

target_link_libraries(ImGui PUBLIC SDL2::SDL2)

if (USE_OPENGLES)
    # XXX copy linux.cmake but maybe this actually need gles3_LIBRARY
    target_link_libraries(ImGui PUBLIC ${OPENGL_gles2_LIBRARY})
    add_compile_definitions(IMGUI_IMPL_OPENGL_ES3)
else()
    target_link_libraries(ImGui PUBLIC ${OPENGL_gl_LIBRARY})
endif()
