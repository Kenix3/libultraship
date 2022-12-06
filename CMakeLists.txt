cmake_minimum_required(VERSION 3.16.0)

project(libultraship LANGUAGES C CXX)
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    enable_language(OBJCXX)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(cmake/Utils.cmake)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND USE_AUTO_VCPKG)
    include(cmake/automate-vcpkg.cmake)

    set(VCPKG_TRIPLET x64-windows-static)
    set(VCPKG_TARGET_TRIPLET x64-windows-static)

    vcpkg_bootstrap()
    vcpkg_install_packages(zlib bzip2 SDL2 GLEW)
endif()

add_subdirectory("extern")
add_subdirectory("src")

set_target_properties(libultraship PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

include (GNUInstallDirs)

install(TARGETS libultraship ImGui tinyxml2 storm StrHash64 Mercury nlohmann_json ZAPDUtils
    EXPORT libultraship-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/libultraship
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT libultraship-targets
    FILE libultraship-targets.cmake
    NAMESPACE libultraship::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libultraship
)

include (CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/libultraship-config.cmake.in
    ${CMAKE_BINARY_DIR}/cmake/libultraship-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libultraship
)

# Uncomment when a version is present in the project()
# write_basic_package_version_file(
#     ${CMAKE_BINARY_DIR}/cmake/libultraship-config-version.cmake
#     VERSION ${LIBULTRASHIP_VERSION}
#     COMPATIBILITY SameMajorVersion
# )

install(
    FILES
        ${CMAKE_BINARY_DIR}/cmake/libultraship-config.cmake
        ${CMAKE_BINARY_DIR}/cmake/libultraship-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libultraship
)

export(EXPORT libultraship-targets
    FILE ${CMAKE_BINARY_DIR}/cmake/libultraship-targets.cmake
    NAMESPACE libultraship::
)