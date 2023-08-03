#!/bin/bash

# This script is expected to be executed inside a container and it is not intended to be executed on host systems

executable="/ExchatgeDesktopClient/build/ExchatgeDesktopClient"
extracted="/extracted"

main() {
  if ! [ -d "$extracted" ]; then
    mkdir "$extracted"
  fi

  libs=$(ldd "$executable" | awk '{print $3}')

  index=0
  while read -r lib; do
    if [[ $index -ne 0 ]]; then parseLib "$lib"; fi
    ((index++))
  done <<< "$libs"

  cp "$executable" "$extracted"
}

function parseLib() {
  if [ -f "$1" ]; then
    cp "$1" "$extracted"
  fi
}

main
