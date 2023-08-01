
cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_COMPILER "/usr/bin/clang")
project(ExchatgeDesktopClient C)

set(CMAKE_C_STANDARD 11)

add_compile_options("-Wall")
add_compile_options("-Wextra")
add_compile_options("-Wpedantic")

add_compile_options("-Wno-nullability-completeness")
add_compile_options("-Wno-nullability-extension")
add_compile_options("-Wno-unknown-pragmas")
add_compile_options("-fms-extensions")
add_compile_options("-Wno-gnu-folding-constant")
add_compile_options("-Wno-microsoft-anon-tag")
add_compile_options("-Wno-gnu-empty-initializer")
add_compile_options("-Wno-language-extension-token")
add_compile_options("-Wno-microsoft-fixed-enum")
add_compile_options("-Wno-empty-body")

add_compile_options("-std=gnu11")

add_compile_options("-Ofast") # -O3
add_link_options("-s")

file(GLOB sources CONFIGURE_DEPENDS "src/*.c" "src/*.h")
add_executable(${PROJECT_NAME} ${sources})

# SDL2_Net included
include_directories(libs/sdl/include)
file(GLOB sdl_binaries CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/libs/sdl/bin/*")
target_link_libraries(${PROJECT_NAME} ${sdl_binaries})

include_directories(libs/sodium/include)
file(GLOB sodium_binaries CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/libs/sodium/bin/*")
target_link_libraries(${PROJECT_NAME} ${sodium_binaries})

set(LIB_RENDER_NAME "render")
add_subdirectory(src/render)
target_link_libraries(${PROJECT_NAME} ${LIB_RENDER_NAME})

set(LIB_COLLECTIONS_NAME "collections")
add_subdirectory(src/collections)
target_link_libraries(${PROJECT_NAME} ${LIB_COLLECTIONS_NAME})

set(LIB_DATABASE_NAME "database")
add_subdirectory(src/database)
target_link_libraries(${PROJECT_NAME} ${LIB_DATABASE_NAME})

file(COPY res DESTINATION ${CMAKE_BINARY_DIR})

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(SEND_ERROR "Target is not 64 bits")
endif()

if (NOT CMAKE_C_BYTE_ORDER STREQUAL LITTLE_ENDIAN)
    message(SEND_ERROR "Target is not little endian")
endif()

#
# libs
#     nuklear
#         include
#             <.h>
#     sdl # sdl2 and sdl2-net
#         bin
#             <.so|.a>
#         include
#             <.h>
#     sodium
#         bin
#             <.so|.a>
#         include
#             <.h>
#     sqlite
#         bin
#             <.so|.a>
#         include
#             <.h>
#
