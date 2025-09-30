#=================== Controller ===================

file(GLOB_RECURSE Source_Files__Controller RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "controller/*.cpp")

source_group("controller" FILES ${Source_Files__Controller})
target_sources(libultraship PRIVATE ${Source_Files__Controller})

#=================== Public ===================

source_group("public")

file(GLOB Source_Files__Public__Libultra RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "public/libultra/*.cpp")

source_group("public/libultra" FILES ${Source_Files__Public__Libultra})

target_sources(libultraship PRIVATE ${Source_Files__Public__Libultra})

#=================== Log ===================

file(GLOB Source_Files__Log RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "log/*.cpp")

source_group("log" FILES ${Source_Files__Log})

target_sources(libultraship PRIVATE ${Source_Files__Log})

#=================== Window ===================

file(GLOB Source_Files__Window RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "window/*.cpp")

source_group("window" FILES ${Source_Files__Window})
target_sources(libultraship PRIVATE ${Source_Files__Window})

#=================== Gui ===================

file(GLOB_RECURSE Source_Files__Window__Gui RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "window/gui/*.cpp")

source_group("window/gui" FILES ${Source_Files__Window__Gui})
target_sources(libultraship PRIVATE ${Source_Files__Window__Gui})
