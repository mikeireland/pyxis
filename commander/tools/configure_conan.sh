#!/bin/sh

# Adds cosmic remote if missing.
# If you need to access cosmic remote, uncomment the following line.
conan remote list | grep cosmic > /dev/null || conan remote add cosmic https://odp2.jfrog.io/artifactory/api/conan/cosmic

# Adds default profile if missing.
conan profile list | grep default > /dev/null || conan profile new default --detect

# Set C++ abi to libstdc++11. This is required for building with GCC>5.
conan profile update settings.compiler.libcxx=libstdc++11 default

echo "Remotes list:"
conan remote list
echo
conan profile show default
