
add_library(${LIB_RENDER_NAME} render.c)
target_include_directories(${LIB_RENDER_NAME} PUBLIC ..)

# changed SDL-related includes ('#include "*.h"' | '#include <SDL2/*.h>') to '#include <sdl/*.h>' in 'nuklear_sdl_renderer.h'
include_directories("${CMAKE_SOURCE_DIR}/libs/nuklear/include")
target_link_libraries(${LIB_RENDER_NAME} m) # c math lib used by nuklear
target_precompile_headers(${LIB_RENDER_NAME} PRIVATE nuklearDefs.h)
