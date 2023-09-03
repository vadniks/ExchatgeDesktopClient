#!/bin/bash

executable="$(pwd)/build/ExchatgeDesktopClient"
extracted="$(pwd)/extracted"

if ! [ -r "$(pwd)" ] || ! [ -w "$(pwd)" ]; then exit 1; fi
if ! [ -x "$executable" ]; then exit 1; fi
if [ -d "$extracted" ]; then exit 0; fi

main() {
  if ! [ -d "$extracted" ]; then
    mkdir "$extracted"
  fi

  libs=$(ldd "$executable" | awk '{print $3}') # objdump -p "$executable" | grep NEEDED

  index=0
  while read -r lib; do
    if [[ $index -ne 0 ]]; then processLib "$lib"; fi
    ((index++))
  done <<< "$libs"

  cp "$executable" "$extracted"
}

function processLib() {
  if [ -f "$1" ]; then
    strip -s "$1" > /dev/null 2>&1
    cp "$1" "$extracted"
  fi
}

main
