
file(GLOB lib_render_sources CONFIGURE_DEPENDS "*.c" "*.h")
add_library(${LIB_RENDER_NAME} ${lib_render_sources})
target_include_directories(${LIB_RENDER_NAME} PUBLIC ..)

include_directories("${CMAKE_SOURCE_DIR}/libs/nuklear/include")
target_link_libraries(${LIB_RENDER_NAME} m) # c math lib used by nuklear
