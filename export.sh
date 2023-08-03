#!/bin/sh

# This script is expected to be executed on host systems while the container is running

exported="$(pwd)/exported"

if ! [ -w "$(pwd)" ]; then exit 1; fi
if [ -d "$exported" ]; then exit 0; fi

docker cp exchatge_client_builder:/extracted/. "${exported}/"
