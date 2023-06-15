
add_library(${LIB_RENDER_NAME} render.c)
target_include_directories(${LIB_RENDER_NAME} PUBLIC ..)

# unnecessary inclusion of SDL library as it's included in the parent build script
include_directories(libs/sdl/include)
file(GLOB sdl_binaries CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/libs/sdl/bin/*")
target_link_libraries(${PROJECT_NAME} ${sdl_binaries})

# changed SDL-related includes ('#include "*.h"' | '#include <SDL2/*.h>') to '#include <sdl/*.h>' in 'nuklear_sdl_renderer.h'
include_directories(../../libs/nuklear/include)
target_link_libraries(${LIB_RENDER_NAME} m) # c math lib used by nuklear
target_precompile_headers(${LIB_RENDER_NAME} PRIVATE nuklear.h)
