#! /usr/bin/env bash
set -e

ARCH=$1
BUILD_ROOT=`pwd`
ANDROID_PLATFORM=android-14

BUILD_NAME=
CROSS_PREFIX=
DEP_OPENSSL_INC=
DEP_OPENSSL_LIB=
CFG_FLAGS=
EXTRA_CFLAGS=
EXTRA_LDFLAGS=
DEP_LIBS=
ASSEMBLER_SUB_DIRS=
. ./do-detect-env.sh
MAKE_TOOLCHAIN_FLAGS=$MAKE_TOOLCHAIN_FLAGS
MAKE_FLAGS=$MAKE_FLAG
GCC_VER=$GCC_VER
GCC_64_VER=$GCC_64_VER

if [ "$ARCH" = "armeabi-v7a" ]; then
    BUILD_NAME=armeabi-v7a
    CROSS_PREFIX=arm-linux-androideabi
    TOOLCHAIN_NAME=${CROSS_PREFIX}-${GCC_VER}
    CFG_FLAGS="$CFG_FLAGS --arch=arm --cpu=cortex-a8"
    CFG_FLAGS="$CFG_FLAGS --enable-neon"
    CFG_FLAGS="$CFG_FLAGS --enable-thumb"
    EXTRA_CFLAGS="$EXTRA_CFLAGS -march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp -mfpu=neon -D__ARM_ARCH_7__ -D__ARM_ARCH_7A__  -Wno-psabi -msoft-float -mthumb -Os -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64 -DANDROID  -Wa,--noexecstack -MMD -MP"
    EXTRA_LDFLAGS="$EXTRA_LDFLAGS -Wl,--fix-cortex-a8"
    ASSEMBLER_SUB_DIRS="arm"
elif [ "$ARCH" = "armeabi" ]; then
    BUILD_NAME=armeabi
    CROSS_PREFIX=arm-linux-androideabi
    TOOLCHAIN_NAME=${CROSS_PREFIX}-${GCC_VER}
    CFG_FLAGS="$CFG_FLAGS --arch=arm"
    EXTRA_CFLAGS="$EXTRA_CFLAGS -march=armv5te -mtune=arm9tdmi -msoft-float"
    EXTRA_LDFLAGS="$EXTRA_LDFLAGS"
    ASSEMBLER_SUB_DIRS="arm"
elif [ "$ARCH" = "x86" ]; then
    BUILD_NAME=x86
    CROSS_PREFIX=i686-linux-android
    TOOLCHAIN_NAME=x86-${GCC_VER}
    CFG_FLAGS="$CFG_FLAGS --arch=x86 --cpu=i686 --enable-yasm"
    EXTRA_CFLAGS="$EXTRA_CFLAGS -march=atom -msse3 -ffast-math -mfpmath=sse"
    EXTRA_LDFLAGS="$EXTRA_LDFLAGS"
    ASSEMBLER_SUB_DIRS="x86"
elif [ "$ARCH" = "x86-64" ]; then
    ANDROID_PLATFORM=android-21
    BUILD_NAME=x86-64
    CROSS_PREFIX=x86_64-linux-android
    TOOLCHAIN_NAME=${CROSS_PREFIX}-${GCC_64_VER}
    CFG_FLAGS="$CFG_FLAGS --arch=x86_64 --enable-yasm"
    EXTRA_CFLAGS="$EXTRA_CFLAGS"
    EXTRA_LDFLAGS="$EXTRA_LDFLAGS"
    ASSEMBLER_SUB_DIRS="x86"
elif [ "$ARCH" = "arm64-v8a" ]; then
    ANDROID_PLATFORM=android-21
    BUILD_NAME=arm64-v8a
    CROSS_PREFIX=aarch64-linux-android
    TOOLCHAIN_NAME=${CROSS_PREFIX}-${GCC_64_VER}
    CFG_FLAGS="$CFG_FLAGS --arch=aarch64 --enable-yasm"
    EXTRA_CFLAGS="$EXTRA_CFLAGS -enable-yasm -march=armv8-a -DANDROID -D__ARM_ARCH_8__ -D__ARM_ARCH_8A__ -D__ARM_NEON__ -O3"
    EXTRA_LDFLAGS="$EXTRA_LDFLAGS"
    ASSEMBLER_SUB_DIRS="aarch64 neon"

else
    echo "unknown architecture $ARCH";
    exit 1
fi

echo "do-compile-ffmpeg"
TOOLCHAIN_PATH=$BUILD_ROOT/build/ffmpeg/$BUILD_NAME/toolchain
MAKE_TOOLCHAIN_FLAGS="$MAKE_TOOLCHAIN_FLAGS --install-dir=$TOOLCHAIN_PATH"

SYSROOT=$TOOLCHAIN_PATH/sysroot
PREFIX=$BUILD_ROOT/build/ffmpeg/$BUILD_NAME/

DEP_X264_INC=$BUILD_ROOT/build/x264/$BUILD_NAME/include
DEP_X264_LIB=$BUILD_ROOT/build/x264/$BUILD_NAME/lib

DEP_FDK_AAC_INC=$BUILD_ROOT/build/fdk-aac/$BUILD_NAME/output/include
DEP_FDK_AAC_LIB=$BUILD_ROOT/build/fdk-aac/$BUILD_NAME/output/lib

case "$UNAME_S" in
    CYGWIN_NT-*)
        SYSROOT="$(cygpath -am $SYSROOT)"
        PREFIX="$(cygpath -am $PREFIX)"
    ;;
esac

mkdir -p $PREFIX
mkdir -p $SYSROOT

TOOLCHAIN_TOUCH="$TOOLCHAIN_PATH/touch"
if [ ! -f "$TOOLCHAIN_TOUCH" ]; then
    $ANDROID_NDK/build/tools/make-standalone-toolchain.sh \
        $MAKE_TOOLCHAIN_FLAGS \
        --platform=$ANDROID_PLATFORM \
        --toolchain=$TOOLCHAIN_NAME --force
    touch $TOOLCHAIN_TOUCH;
fi

#--------------------
echo ""
echo "--------------------"
echo "[*] check ffmpeg env"
echo "--------------------"
export PATH=$TOOLCHAIN_PATH/bin:$PATH
export CC="${CROSS_PREFIX}-gcc"
export LD=${CROSS_PREFIX}-ld
export AR=${CROSS_PREFIX}-ar
export STRIP=${CROSS_PREFIX}-strip

CFLAGS="-O3 -Wall -pipe \
    -std=c99 \
    -ffast-math \
    -fstrict-aliasing -Werror=strict-aliasing \
    -Wno-psabi -Wa,--noexecstack \
    -DANDROID -DNDEBUG"

export COMMON_CFG_FLAGS=
. module.sh

#with x264
if [ -f "${DEP_X264_LIB}/libx264.a" ]; then
# FF_CFG_FLAGS="$FF_CFG_FLAGS --enable-nonfree"
    COMMON_CFG_FLAGS="$COMMON_CFG_FLAGS --enable-libx264"
    COMMON_CFG_FLAGS="$COMMON_CFG_FLAGS --enable-encoder=libx264"

    EXTRA_CFLAGS="$EXTRA_CFLAGS -I${DEP_X264_INC}"
    DEP_LIBS="$DEP_LIBS -L${DEP_X264_LIB} -lx264"
fi

if [ -f "${DEP_FDK_AAC_LIB}/libfdk-aac.a" ]; then
    CFG_FLAGS="$CFG_FLAGS --enable-nonfree"
    COMMON_CFG_FLAGS="$COMMON_CFG_FLAGS --enable-libfdk_aac"
    COMMON_CFG_FLAGS="$COMMON_CFG_FLAGS --enable-encoder=libfdk_aac"
    COMMON_CFG_FLAGS="$COMMON_CFG_FLAGS --enable-muxer=adts"

    EXTRA_CFLAGS="$EXTRA_CFLAGS -I${DEP_FDK_AAC_INC}"
    DEP_LIBS="$DEP_LIBS -L${DEP_FDK_AAC_LIB} -lfdk-aac"
fi

echo "COMMON_CFG_FLAGS $COMMON_CFG_FLAGS"
CFG_FLAGS="$CFG_FLAGS $COMMON_CFG_FLAGS"

CFG_FLAGS="$CFG_FLAGS --prefix=$PREFIX"
CFG_FLAGS="$CFG_FLAGS --cross-prefix=${CROSS_PREFIX}-"
CFG_FLAGS="$CFG_FLAGS --target-os=linux"
CFG_FLAGS="$CFG_FLAGS --enable-asm"
CFG_FLAGS="$CFG_FLAGS --enable-inline-asm"
cd ffmpeg

if [ -f "compat/strtod.o" ]; then
  make clean
  rm -rf compat/strtod.o
fi

if [ -f "./config.h" ]; then
    make clean
    rm -rf config.h
    echo 'reuse configure'
fi

echo "cflags  $CFLAGS"
echo "DEP_LIBS $DEP_LIBS"
echo "CFG_FLAGS $CFG_FLAGS"

which $CC
./configure $CFG_FLAGS \
    --extra-cflags="$CFLAGS $EXTRA_CFLAGS" \
    --extra-ldflags="$DEP_LIBS"

cp config.* $PREFIX
make $MAKE_FLAGS
make install
cd -
