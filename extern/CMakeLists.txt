#=================== thread-pool ===================

set(THREADPOOL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thread-pool)
add_library(ThreadPool INTERFACE)

target_include_directories(ThreadPool INTERFACE ${THREADPOOL_DIR})
