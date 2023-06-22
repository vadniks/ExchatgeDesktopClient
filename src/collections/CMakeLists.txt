
file(GLOB lib_collections_sources CONFIGURE_DEPENDS "*.c" "*.h")
add_library(${LIB_COLLECTIONS_NAME} ${lib_collections_sources})
target_include_directories(${LIB_COLLECTIONS_NAME} PUBLIC ..)
