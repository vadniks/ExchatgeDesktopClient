
add_library(${LIB_DATABASE_NAME} database.c)
target_include_directories(${LIB_DATABASE_NAME} PUBLIC ..)

include_directories("${CMAKE_SOURCE_DIR}/libs/sqlite3/include")
file(GLOB sqlite_binaries CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/libs/sqlite3/bin/*")
target_link_libraries(${LIB_DATABASE_NAME} ${sqlite_binaries})
target_precompile_headers(${LIB_DATABASE_NAME} PRIVATE sqlite3proxy.h)
