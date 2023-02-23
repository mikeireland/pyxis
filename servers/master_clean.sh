#!/bin/bash

BUILD=build

for dir in */
do
    cd ${dir}
    echo "${dir}"
    if [[ -d "build" ]]; then
        echo "build folder exists, removing"
        rm -rf build
    fi
    if [[ -d "bin" ]]; then
        echo "binary folder exists, removing"
        rm -rf bin
    fi
    cd ..
done
