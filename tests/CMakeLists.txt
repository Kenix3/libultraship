include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(libultraship_tests
    PartTest.cpp
    ComponentTest.cpp
    ActionTest.cpp
    TickableTest.cpp
)

target_link_libraries(libultraship_tests
    PRIVATE
    GTest::gtest_main
    libultraship
)

target_include_directories(libultraship_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${ADDITIONAL_LIB_INCLUDES}
)

# Propagate compile definitions from the main library
target_compile_definitions(libultraship_tests PRIVATE
    $<TARGET_PROPERTY:libultraship,COMPILE_DEFINITIONS>
)

include(GoogleTest)
gtest_discover_tests(libultraship_tests)
