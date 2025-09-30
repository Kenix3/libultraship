add_subdirectory("fast")
add_subdirectory("libultraship")
add_subdirectory("ship")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/install_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/install_config.h @ONLY)
