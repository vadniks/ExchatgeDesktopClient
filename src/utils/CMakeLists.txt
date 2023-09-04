
file(GLOB lib_utils_sources CONFIGURE_DEPENDS "*.c" "*.h")
add_library(${LIB_UTILS_NAME} ${lib_utils_sources})
target_include_directories(${LIB_UTILS_NAME} PUBLIC ..)
