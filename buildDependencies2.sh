#!/bin/bash

# This script can be executed both from container and from host

curDir=$(pwd)
downloads="${curDir}/downloads"
libs="${curDir}/libs"

function err() {
  >&2 echo "$1"
  exit 1
}

function main() {
  if ! [ -r "$curDir" ] || ! [ -w "$curDir" ]; then err "Cannot read or write to current directory"; fi

  if [ -d "$downloads" ]; then rm -r "$downloads"; fi
  if [ -d "$libs" ]; then rm -r "$libs"; fi

  mkdir "$downloads" "$libs"
}

function sdl() {
  echo "Retrieving & building SDL2..."

  lib="${libs}/sdl"
  bin="${lib}/bin"
  include="${lib}/include"

  if ! (mkdir "$lib" && mkdir "$bin" && mkdir "$include")
  then err "unable to create directories"; fi

  if ! (cd "$downloads" \
    && wget https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.26.5.tar.gz \
    && tar -xf release-2.26.5.tar.gz && cd SDL-release-2.26.5 \
    && mkdir build && cd build && cmake .. && make all \
    && cp *.so *.so.* *.a "$bin" \
    && cd .. \
    && cp include/*.h "$include")
  then err "failed to retrieve & build SDL2"; fi

  echo "SDL2 finished"
}

function sdlNet() {
  echo "Retrieving & building SDL2Net..."

  lib="${libs}/sdl"
  bin="${lib}/bin"
  include="${lib}/include"

  if ! (cd "$downloads" \
    && wget https://github.com/libsdl-org/SDL_net/releases/download/release-2.2.0/SDL2_net-2.2.0.tar.gz \
    && tar -xf SDL2_net-2.2.0.tar.gz && cd SDL2_net-2.2.0 \
    && mkdir build && cd build && cmake .. && make all \
    && cp *.so *.so.* "$bin" \
    && cd .. \
    && cp *.h "$include")
  then err "failed to retrieve & build SDL2Net"; fi

  echo "SDL2Net finished"
}

function sodium() {
  echo "Retrieving & building Sodium..."

  lib="${libs}/sodium"
  bin="${lib}/bin"
  include="${lib}/include"

  if ! (mkdir "$lib" && mkdir "$bin" && mkdir "$include")
  then err "unable to create directories"; fi

  if ! (cd "$downloads" \
    && wget https://download.libsodium.org/libsodium/releases/libsodium-1.0.18.tar.gz \
    && tar -xf libsodium-1.0.18.tar.gz && cd libsodium-1.0.18 \
    && ./configure && make \
    && cd src/libsodium/.libs \
    && cp *.so *.so.* *.a "$bin" \
    && cd "${downloads}/libsodium-1.0.18/src/libsodium/include" \
    && cp sodium.h "$include" \
    && cp -r sodium "$include")
  then err "failed to retrieve & build Sodium"; fi

  echo "Sodium finished"
}

function sqlite() {
  echo "Retrieving & building SQLite3..."

  lib="${libs}/sqlite3"
  bin="${lib}/bin"
  include="${lib}/include"

  if ! (mkdir "$lib" && mkdir "$bin" && mkdir "$include")
  then err "unable to create directories"; fi

  if ! (cd "$downloads" \
    && wget https://sqlite.org/2023/sqlite-autoconf-3420000.tar.gz \
    && tar -xf sqlite-autoconf-3420000.tar.gz && cd sqlite-autoconf-3420000 \
    && ./configure && make \
    && cp sqlite3.h "$include" \
    && cd .libs \
    && cp *.so *.so.* *.a "$bin")
  then err "failed to retrieve & build SQLite3"; fi

  echo "SQLite3 finished"
}

function nuklear() {
  echo "Retrieving Nuklear..."

  lib="${libs}/nuklear"
  include="${lib}/include"

  if ! (mkdir "$lib" && mkdir "$include")
  then err "unable to create directories"; fi

  if ! (cd "$downloads" \
    && wget https://github.com/Immediate-Mode-UI/Nuklear/archive/refs/tags/4.10.5.tar.gz \
    && tar -xf 4.10.5.tar.gz && cd Nuklear-4.10.5 \
    && cd src && ./paq.sh > ../nuklear.h \
    && cp ../nuklear.h "$include" \
    && cp ../demo/sdl_renderer/nuklear_sdl_renderer.h "$include")
  then err "failed to retrieve Nuklear"; fi

  echo "Nuklear finished"
}

main && sdl && sdlNet && sodium && sqlite && nuklear
