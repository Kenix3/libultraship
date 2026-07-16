# lus_setup_tcc_runtime(TARGET_NAME [RESOURCES_DIR <dest>])
#
# Adds post-build staging + install rules for the TCC runtime files that
# ScriptLoader expects at runtime:
#
#   .tcc/include/   — TCC standard headers (+ win32 headers on Windows)
#   .tcc/lib/       — libtcc1 and (Windows) a .def file for mod linking
#
# POST_BUILD staging writes to <target_file_dir>/.tcc/ so devs can run
# from the build tree.  Install rules read *directly from source* so they
# never depend on the staging step having run — this is the main reason
# the .tcc tree can silently go missing in packaged builds.
#
# Parameters:
#   TARGET_NAME    — CMake target to attach commands to.
#   RESOURCES_DIR  — Install destination for the .tcc/ tree.
#                    Defaults to ".tcc" (next to the binary).
#                    Pass "../Resources/.tcc" for macOS app bundles.
#
# Must be called after add_subdirectory(libultraship) so that the tinycc
# FetchContent population has already happened inside libultraship.

function(lus_setup_tcc_runtime TARGET_NAME)
    cmake_parse_arguments(PARSE_ARGV 1 LUS_TCC "" "RESOURCES_DIR" "")

    if(NOT ENABLE_SCRIPTING)
        return()
    endif()

    include(FetchContent)
    FetchContent_GetProperties(tinycc)
    if(NOT tinycc_POPULATED OR NOT tinycc_SOURCE_DIR)
        message(WARNING "lus_setup_tcc_runtime: tinycc not populated — skipping TCC runtime setup for ${TARGET_NAME}")
        return()
    endif()

    if(NOT LUS_TCC_RESOURCES_DIR)
        set(LUS_TCC_RESOURCES_DIR ".tcc")
    endif()

    set(_stage "$<TARGET_FILE_DIR:${TARGET_NAME}>/.tcc")

    # ------------------------------------------------------------------ #
    #  Windows (MSVC)                                                      #
    # ------------------------------------------------------------------ #
    if(MSVC)
        # Bundle all generated .lib files so mods can link against the host.
        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMENT "Packaging import libs into ${TARGET_NAME}.sdk..."
            COMMAND cmd /c "lib.exe /NOLOGO /OUT:\"$<TARGET_FILE_DIR:${TARGET_NAME}>/${TARGET_NAME}.sdk\" \"$<TARGET_FILE_DIR:${TARGET_NAME}>/*.lib\""
        )

        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMENT "Staging TCC runtime headers and libs..."
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_stage}/lib/"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_stage}/include/"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${tinycc_SOURCE_DIR}/include/"        "${_stage}/include/"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${tinycc_SOURCE_DIR}/win32/include/"  "${_stage}/include/"
            COMMAND ${CMAKE_COMMAND} -E copy "${tinycc_SOURCE_DIR}/win32/lib/libtcc1.a" "${_stage}/lib/libtcc1.a"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:libtcc>" "$<TARGET_FILE_DIR:${TARGET_NAME}>/$<TARGET_FILE_NAME:libtcc>"
            VERBATIM
        )

        # Ensure tcc.exe is built before the POST_BUILD step that needs it.
        if(TARGET tcc_win32_exe)
            add_dependencies(${TARGET_NAME} tcc_win32_exe)
        endif()

        # Generate a .def so mods can resolve host symbols at TCC link time.
        # Uses the pre-built bootstrapping tcc.exe that ships with TCC source.
        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMENT "Generating ${TARGET_NAME}.def for mod linking..."
            COMMAND "${tinycc_SOURCE_DIR}/win32/tcc.exe"
                    -impdef "$<TARGET_FILE:${TARGET_NAME}>"
                    -o "${_stage}/lib/${TARGET_NAME}.def"
            VERBATIM
        )

        # Install from source — never depends on staging having run.
        install(DIRECTORY "${tinycc_SOURCE_DIR}/include/"
            DESTINATION "${LUS_TCC_RESOURCES_DIR}/include"
            COMPONENT ${TARGET_NAME}
        )
        install(DIRECTORY "${tinycc_SOURCE_DIR}/win32/include/"
            DESTINATION "${LUS_TCC_RESOURCES_DIR}/include"
            COMPONENT ${TARGET_NAME}
        )
        install(FILES "${tinycc_SOURCE_DIR}/win32/lib/libtcc1.a"
            DESTINATION "${LUS_TCC_RESOURCES_DIR}/lib"
            COMPONENT ${TARGET_NAME}
        )
        install(FILES "$<TARGET_FILE:libtcc>"
            DESTINATION "."
            COMPONENT ${TARGET_NAME}
        )
        # .def is generated at build time; OPTIONAL so packaging doesn't
        # hard-fail if the impdef step was skipped or unavailable.
        install(FILES "${_stage}/lib/${TARGET_NAME}.def"
            DESTINATION "${LUS_TCC_RESOURCES_DIR}/lib"
            COMPONENT ${TARGET_NAME}
            OPTIONAL
        )

    # ------------------------------------------------------------------ #
    #  Unix (Linux + macOS)                                                #
    # ------------------------------------------------------------------ #
    elseif(UNIX)
        if(TARGET libtcc1_make_build)
            add_dependencies(${TARGET_NAME} libtcc1_make_build)
        endif()
        set(_tcc_crti "")
        set(_tcc_crtn "")
        foreach(_dir ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
            if(EXISTS "${_dir}/crti.o" AND NOT _tcc_crti)
                set(_tcc_crti "${_dir}/crti.o")
            endif()
            if(EXISTS "${_dir}/crtn.o" AND NOT _tcc_crtn)
                set(_tcc_crtn "${_dir}/crtn.o")
            endif()
        endforeach()

        if(NOT _tcc_crti OR NOT _tcc_crtn)
            message(WARNING "lus_setup_tcc_runtime: crti.o/crtn.o not found — mods may fail to compile on other distros")
        endif()

        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMENT "Staging TCC runtime headers and libs..."
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_stage}/lib/"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_stage}/include/"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${tinycc_SOURCE_DIR}/include/" "${_stage}/include/"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:libtcc1>" "${_stage}/lib/$<TARGET_FILE_NAME:libtcc1>"
            VERBATIM
        )

        if(_tcc_crti)
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${_tcc_crti}" "${_stage}/lib/crti.o"
                COMMAND ${CMAKE_COMMAND} -E copy "${_tcc_crtn}" "${_stage}/lib/crtn.o"
                VERBATIM
            )
        endif()

        # Install from source — never depends on staging having run.
        install(DIRECTORY "${tinycc_SOURCE_DIR}/include/"
            DESTINATION "${LUS_TCC_RESOURCES_DIR}/include"
            COMPONENT ${TARGET_NAME}
        )
        install(FILES "$<TARGET_FILE:libtcc1>"
            DESTINATION "${LUS_TCC_RESOURCES_DIR}/lib"
            COMPONENT ${TARGET_NAME}
        )
        if(_tcc_crti)
            install(FILES "${_tcc_crti}" DESTINATION "${LUS_TCC_RESOURCES_DIR}/lib" RENAME "crti.o" COMPONENT ${TARGET_NAME})
            install(FILES "${_tcc_crtn}" DESTINATION "${LUS_TCC_RESOURCES_DIR}/lib" RENAME "crtn.o" COMPONENT ${TARGET_NAME})
        endif()
    endif()
endfunction()
