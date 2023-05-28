cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_COMPILER "/usr/bin/clang")
project(ExchatgeDesktopClient C)

set(CMAKE_C_STANDARD 11)

#add_compile_options("-O3")
#add_link_options("-s")

file(GLOB sources CONFIGURE_DEPENDS "src/*.c" "src/*.h")
add_executable(${PROJECT_NAME} ${sources})

include_directories(libs/sdl/include)
file(GLOB sdl_binaries CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/libs/sdl/bin/*")
target_link_libraries(${PROJECT_NAME} ${sdl_binaries})

# changed SDL-related includes ('#include "*.h"' including *_code.h) to '#include <sdl/*.h>' in 'SDL_ttf.h'
include_directories(libs/sdl_ttf/include)
file(GLOB sdl_ttf_binaries CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/libs/sdl_ttf/bin/*")
target_link_libraries(${PROJECT_NAME} ${sdl_ttf_binaries})

# changed SDL-related includes ('#include "*.h"' including *_code.h) to '#include <sdl/*.h>' in 'SDL_net.h'
include_directories(libs/sdl_net/include)
file(GLOB sdl_net_binaries CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/libs/sdl_net/bin/*")
target_link_libraries(${PROJECT_NAME} ${sdl_net_binaries})

include_directories(libs/nuklear/include)

file(COPY res DESTINATION ${CMAKE_BINARY_DIR})

# libs
#     nuklear
#         include
#             nuklear
#                 <.h>
#     sdl
#         bin
#             <.so|.a>
#         include
#             sdl
#                 <.h>
#     sdl_net
#         bin
#             <.so|.a>
#         include
#             sdl_net
#                 <.h>
#     sdl_ttf
#         bin
#             <.so|.a>
#         include
#             sdl_ttf
#                 <.h>
#
