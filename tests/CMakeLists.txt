include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.16.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(lus_tests
    binary_io_tests.cpp
    string_helper_tests.cpp
    crc64_tests.cpp
    stox_tests.cpp
    utils_tests.cpp
    bitconverter_tests.cpp
    event_system_tests.cpp
    sound_matrix_decoder_tests.cpp
    path_file_helper_tests.cpp
    archive_resource_tests.cpp
    glob_tests.cpp
    splittext_tests.cpp
    path_diskfile_tests.cpp
    resource_type_tests.cpp
    archive_self_tests.cpp
    fast3d_tests.cpp
    fast3d_layer4_tests.cpp
)

if(NOT DISABLE_SCRIPTING)
    target_sources(lus_tests PRIVATE script_test.cpp)
endif()

set_property(TARGET lus_tests PROPERTY CXX_STANDARD 20)

target_link_libraries(lus_tests PRIVATE
    libultraship
    gtest_main
)

target_compile_definitions(lus_tests PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF)

include(GoogleTest)
gtest_discover_tests(lus_tests)
