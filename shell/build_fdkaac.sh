#!/bin/bash

ARCH=$1

source config.sh $ARCH
LIBS_DIR=$(cd `dirname $0`; pwd)/libs/libfdk-aac
echo "LIBS_DIR="$LIBS_DIR

cd fdk-aac-0.1.6


PREFIX=$LIBS_DIR/$AOSP_ABI
TOOLCHAIN=$ANDROID_NDK_ROOT/toolchains/$TOOLCHAIN_BASE-$AOSP_TOOLCHAIN_SUFFIX/prebuilt/darwin-x86_64
SYSROOT=$ANDROID_NDK_ROOT/platforms/$AOSP_API/arch-$AOSP_ARCH
CROSS_PREFIX=$TOOLCHAIN/bin/$TOOLNAME_BASE-

CFLAGS=""

FLAGS="--enable-static  --host=$HOST --target=android --disable-asm "

export CXX="${CROSS_PREFIX}g++ --sysroot=${SYSROOT}"
export LDFLAGS=" -L$SYSROOT/usr/lib  $CFLAGS "
export CXXFLAGS=$CFLAGS
export CFLAGS=$CFLAGS
export CC="${CROSS_PREFIX}gcc --sysroot=${SYSROOT}"
export AR="${CROSS_PREFIX}ar"
export LD="${CROSS_PREFIX}ld"
export AS="${CROSS_PREFIX}gcc"

./autogen.sh

./configure $FLAGS \
--enable-pic \
--enable-strip \
--prefix=$PREFIX

$ADDITIONAL_CONFIGURE_FLAG

make clean
make -j8
make install

cd ..
