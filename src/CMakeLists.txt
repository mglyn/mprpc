aux_source_directory(. SRC_LIST)
add_library(mprpc ${SRC_LIST})

add_compile_definitions(THREADED ZOO_ENABLE_SYNC_API)

target_link_libraries(mprpc muduo_net muduo_base pthread zookeeper_mt)