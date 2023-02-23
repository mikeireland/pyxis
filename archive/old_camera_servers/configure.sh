#!/bin/bash
# Project build directory relative to current directory.
BUILD_DIR="$(realpath --relative-to=$(pwd) $( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd ))/build"

# Exit on first error.
set -e

QHY_SDK=qhy_sdk_linux64_22.03.11

unzip ${QHY_SDK}.zip
cd ${QHY_SDK}
sudo bash install.sh
cd ..
rm -rf ${QHY_SDK}



mkdir -p $BUILD_DIR
cd $BUILD_DIR



# Get dependencies and prepare build directory.
conan install .. --build=missing -pr default -pr:b=default $@

# Let conan invoke cmake configure.
conan build .. -c
