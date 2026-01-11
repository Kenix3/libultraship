#=================== Top-Level ===================

file(GLOB Source_Files__TopLevel RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")

source_group("" FILES ${Source_Files__TopLevel})
target_sources(libultraship PRIVATE ${Source_Files__TopLevel})

#=================== Audio ===================

file(GLOB Source_Files__Audio RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "audio/*.cpp")

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(FILTER Source_Files__Audio EXCLUDE REGEX "audio/WasapiAudioPlayer.*")
endif()

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(FILTER Source_Files__Audio EXCLUDE REGEX "audio/CoreAudioAudioPlayer.*")
endif()

source_group("audio" FILES ${Source_Files__Audio})
target_sources(libultraship PRIVATE ${Source_Files__Audio})

#=================== Controller ===================

file(GLOB_RECURSE Source_Files__Controller RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "controller/*.cpp")

source_group("controller" FILES ${Source_Files__Controller})
target_sources(libultraship PRIVATE ${Source_Files__Controller})

#=================== Config ===================

file(GLOB Source_Files__Config RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "config/*.cpp")

source_group("config" FILES ${Source_Files__Config})
target_sources(libultraship PRIVATE ${Source_Files__Config})

#=================== Debug ===================

file(GLOB Source_Files__Debug RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "debug/*.cpp")

source_group("debug" FILES ${Source_Files__Debug})
target_sources(libultraship PRIVATE ${Source_Files__Debug})

#=================== Window ===================

file(GLOB Source_Files__Window RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "window/*.cpp")

source_group("window" FILES ${Source_Files__Window})
target_sources(libultraship PRIVATE ${Source_Files__Window})

#=================== Gui ===================

file(GLOB_RECURSE Source_Files__Window__Gui RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "window/gui/*.cpp")

source_group("window/gui" FILES ${Source_Files__Window__Gui})
target_sources(libultraship PRIVATE ${Source_Files__Window__Gui})

#=================== Utils ===================

file(GLOB_RECURSE Source_Files__Utils RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "utils/*.cpp" "utils/*.c")

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    list(APPEND Source_Files__Utils ${CMAKE_CURRENT_SOURCE_DIR}/utils/AppleFolderManager.mm)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND Source_Files__Utils ${CMAKE_CURRENT_SOURCE_DIR}/utils/macUtils.mm)
endif()

source_group("utils" FILES ${Source_Files__Utils})
target_sources(libultraship PRIVATE ${Source_Files__Utils})

#=================== Port ===================

if (CMAKE_SYSTEM_NAME STREQUAL "Android" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(Source_Files__Port
        ${CMAKE_CURRENT_SOURCE_DIR}/port/mobile/MobileImpl.cpp
    )
endif()

source_group("port" FILES ${Source_Files__Port})
target_sources(libultraship PRIVATE ${Source_Files__Port})

#=================== Resource ===================

file(GLOB Source_Files__Resource RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "resource/*.cpp")
source_group("resource" FILES ${Source_Files__Resource})

file(GLOB Source_Files__Resource__Types RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "resource/type/*.cpp")
source_group("resource/type" FILES ${Source_Files__Resource__Types})

file(GLOB Source_Files__Resource__Factories RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "resource/factory/*.cpp")
source_group("resource/factory" FILES ${Source_Files__Resource__Factories})

file(GLOB Source_Files__Resource__Archive RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "resource/archive/*.cpp")
source_group("resource/archive" FILES ${Source_Files__Resource__Archive})

target_sources(libultraship PRIVATE ${Source_Files__Resource} ${Source_Files__Resource__Types} ${Source_Files__Resource__Factories} ${Source_Files__Resource__Archive})
