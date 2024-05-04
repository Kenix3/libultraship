set(PLATFORM "OS64COMBINED")
include(FetchContent)
FetchContent_Declare(iostoolchain
    GIT_REPOSITORY https://github.com/leetal/ios-cmake
    GIT_TAG 06465b27698424cf4a04a5ca4904d50a3c966c45
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
)
FetchContent_GetProperties(iostoolchain)
if(NOT iostoolchain_POPULATED)
    FetchContent_Populate(iostoolchain)
endif()
set(CMAKE_IOS_TOOLCHAIN_FILE ${iostoolchain_SOURCE_DIR}/ios.toolchain.cmake)
set_property(GLOBAL PROPERTY IOS_TOOLCHAIN_FILE ${CMAKE_IOS_TOOLCHAIN_FILE})
include(${CMAKE_IOS_TOOLCHAIN_FILE})