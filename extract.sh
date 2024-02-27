#!/bin/bash

executable="$(pwd)/build/ExchatgeDesktopClient"
extracted="$(pwd)/extracted"
ttf="$(pwd)/build/font.ttf"
bmp="$(pwd)/build/icon.bmp"

if ! [ -r "$(pwd)" ] || ! [ -w "$(pwd)" ]; then exit 1; fi
if ! [ -x "$executable" ]; then exit 1; fi
if [ -d "$extracted" ]; then exit 0; fi

main() {
  if ! [ -d "$extracted" ]; then
    mkdir "$extracted"
  fi

  linkedLibs=$(ldd "$executable" | awk '{print $3}')
  neededLibs=$(objdump -p "$executable" | grep NEEDED | awk '{print $2}')

  while read -r lib; do
    found=$((0))

    while read -r lib2; do
      if [[ "$lib" == *"$lib2"* ]]; then
        found=1
        break
      fi
    done <<< "$neededLibs"

    if [[ $found -eq 1 ]]; then processLib "$lib"; fi
  done <<< "$linkedLibs"

  cp "$executable" "$extracted"
  cp "$ttf" "$extracted"
  cp "$bmp" "$extracted"
}

function processLib() {
  if [ -f "$1" ]; then
    strip -s "$1" > /dev/null 2>&1
    cp "$1" "$extracted"
  fi
}

main
