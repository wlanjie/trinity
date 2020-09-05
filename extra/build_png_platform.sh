#! /usr/bin/env bash
set -e
set -x

ARM_NEON=
API_LEVEL=

if [ "$1" = "armeabi-v7a" ]; then
  ARM_NEON=-DANDROID_ARM_NEON=ON
  API_LEVEL=14
elif [ "$1" = "armeabi" ]; then
  API_LEVEL=14
elif [ "$1" = "arm64-v8a" ]; then
  ARM_NEON=-DANDROID_ARM_NEON=ON
  API_LEVEL=21
elif [ "$1" = "x86" ]; then
  API_LEVEL=14
elif [ "$1" = "x86-64" ]; then
  API_LEVEL=21
else
  echo "unknown $ARCH"
fi
rm -rf build/$1
mkdir build/$1
mkdir build/$1/lib
mkdir build/$1/include
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=$1 \
  $ARM_NEON \
  -DANDROID_NATIVE_API_LEVEL=$API_LEVEL \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=$PWD/build/$1/lib \
  -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$PWD/build/$1 \
  -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=$PWD/build/$1
make
rm -rf CMakeFiles
rm cmake_install.cmake
rm CMakeCache.txt