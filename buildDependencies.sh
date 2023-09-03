#!/bin/sh

# DEPRECATED

# This script is expected to be executed inside a container and it is not intended to be executed on host systems

mkdir /downloads && mkdir /ExchatgeDesktopClient && mkdir /ExchatgeDesktopClient/libs

mkdir /ExchatgeDesktopClient/libs/sdl && mkdir /ExchatgeDesktopClient/libs/sdl/bin && mkdir /ExchatgeDesktopClient/libs/sdl/include

cd /downloads \
    && wget https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.26.5.tar.gz \
    && tar -xf release-2.26.5.tar.gz && cd SDL-release-2.26.5 \
    && mkdir build && cd build && cmake .. && make all \
    && cp *.so *.so.* *.a /ExchatgeDesktopClient/libs/sdl/bin \
    && cd .. \
    && cp include/*.h /ExchatgeDesktopClient/libs/sdl/include

# apt install libsdl2-dev # so cmake can build the sdl2-net

cd /downloads \
    && wget https://github.com/libsdl-org/SDL_net/releases/download/release-2.2.0/SDL2_net-2.2.0.tar.gz \
    && tar -xf SDL2_net-2.2.0.tar.gz && cd SDL2_net-2.2.0 \
    && mkdir build && cd build && cmake .. && make all \
    && cp *.so *.so.* /ExchatgeDesktopClient/libs/sdl/bin \
    && cd .. \
    && cp *.h /ExchatgeDesktopClient/libs/sdl/include

mkdir /ExchatgeDesktopClient/libs/sodium && mkdir /ExchatgeDesktopClient/libs/sodium/bin && mkdir /ExchatgeDesktopClient/libs/sodium/include

cd /downloads \
    && wget https://download.libsodium.org/libsodium/releases/libsodium-1.0.18.tar.gz \
    && tar -xf libsodium-1.0.18.tar.gz && cd libsodium-1.0.18 \
    && ./configure && make \
    && cd src/libsodium/.libs \
    && cp *.so *.so.* *.a /ExchatgeDesktopClient/libs/sodium/bin \
    && cd /downloads/libsodium-1.0.18/src/libsodium/include \
    && cp sodium.h /ExchatgeDesktopClient/libs/sodium/include \
    && cp -r sodium /ExchatgeDesktopClient/libs/sodium/include

mkdir /ExchatgeDesktopClient/libs/sqlite3 && mkdir /ExchatgeDesktopClient/libs/sqlite3/bin && mkdir /ExchatgeDesktopClient/libs/sqlite3/include

cd /downloads \
    && wget https://sqlite.org/2023/sqlite-autoconf-3420000.tar.gz \
    && tar -xf sqlite-autoconf-3420000.tar.gz && cd sqlite-autoconf-3420000 \
    && ./configure && make \
    && cp sqlite3.h /ExchatgeDesktopClient/libs/sqlite3/include \
    && cd .libs \
    && cp *.so *.so.* *.a /ExchatgeDesktopClient/libs/sqlite3/bin

mkdir /ExchatgeDesktopClient/libs/nuklear && mkdir /ExchatgeDesktopClient/libs/nuklear/include

# apt install python-is-python3

cd /downloads \
    && wget https://github.com/Immediate-Mode-UI/Nuklear/archive/refs/tags/4.10.5.tar.gz \
    && tar -xf 4.10.5.tar.gz && cd Nuklear-4.10.5 \
    && cd src && ./paq.sh > ../nuklear.h \
    && cp ../nuklear.h /ExchatgeDesktopClient/libs/nuklear/include \
    && cp ../demo/sdl_renderer/nuklear_sdl_renderer.h /ExchatgeDesktopClient/libs/nuklear/include
