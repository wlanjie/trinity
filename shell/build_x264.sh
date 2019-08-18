#!/bin/bash

ARCH=$1


source config.sh $ARCH
LIBS_DIR=$(cd `dirname $0`; pwd)/libs/libx264
echo "LIBS_DIR="$LIBS_DIR

cd x264

PREFIX=$LIBS_DIR/$AOSP_ABI
TOOLCHAIN=$ANDROID_NDK_ROOT/toolchains/$TOOLCHAIN_BASE-$AOSP_TOOLCHAIN_SUFFIX/prebuilt/linux-x86_64
SYSROOT=$ANDROID_NDK_ROOT/platforms/$AOSP_API/arch-$AOSP_ARCH
CROSS_PREFIX=$TOOLCHAIN/bin/$TOOLNAME_BASE-


./configure --prefix=$PREFIX \
--disable-shared \
--enable-static \
--enable-pic \
--enable-neon \
--disable-cli \
--host=$HOST \
--cross-prefix=$CROSS_PREFIX \
--sysroot=$SYSROOT \
--extra-cflags="-Os -fpic" \
--extra-ldflags=""

make clean
make -j4
make install

cd ..
