#!/bin/bash

BUILD=build.sh
CONFIG=configure.sh

for dir in */
do
    cd ${dir}
    echo "${dir}"
    if [[ -f "$CONFIG" ]]; then
        echo "$CONFIG file exists, configuring"
        ."/$CONFIG"
    fi
    if [[ -f "$BUILD" ]]; then
        echo "$BUILD file exists, building"
        ."/$BUILD"
    fi
    cd ..
done
