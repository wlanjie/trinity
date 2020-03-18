set -e

if [ -z "$ANDROID_NDK" -o -z "$ANDROID_SDK" ]; then
    echo "You must define ANDROID_NDK, ANDROID_SDK before starting."
    echo "They must point to your NDK and SDK directories.\n"
    exit 1
fi

FF_ARCH=$1
if [ -z "$FF_ARCH" ]; then
    echo "You must specific an architecture 'arm, armv7a, x86, ...'.\n"
    exit 1
fi

FF_BUILD_ROOT=`pwd`
FF_ANDROID_PLATFORM=android-14
FF_HOST_NAME=

FF_BUILD_NAME=
FF_CROSS_PREFIX=

FF_CFG_FLAGS=
FF_PLATFORM_CFG_FLAGS=

FF_EXTRA_LDFLAGS=

FF_CFLAGS=
. ./do-detect-env.sh
FF_MAKE_TOOLCHAIN_FLAGS=$MAKE_TOOLCHAIN_FLAGS
FF_MAKE_FLAGS=$MAKE_FLAG
FF_GCC_VER=$GCC_VER
FF_GCC_64_VER=$GCC_64_VER


#----- armv7a begin -----
if [ "$FF_ARCH" = "armeabi-v7a" ]; then
    FF_BUILD_NAME=armeabi-v7a
    FF_CROSS_PREFIX=arm-linux-androideabi
	FF_TOOLCHAIN_NAME=${FF_CROSS_PREFIX}-${FF_GCC_VER}
    FF_PLATFORM_CFG_FLAGS="android-armv7"
    FF_HOST_NAME="arm-linux"
    FF_EXTRA_LDFLAGS="$FF_EXTRA_LDFLAGS -Wl,--fix-cortex-a8"
elif [ "$FF_ARCH" = "armeabi" ]; then
    FF_BUILD_NAME=armeabi
    FF_CFG_FLAGS="$FF_CFG_FLAGS --disable-asm"
    FF_CROSS_PREFIX=arm-linux-androideabi
	  FF_TOOLCHAIN_NAME=${FF_CROSS_PREFIX}-${FF_GCC_VER}
    FF_EXTRA_LDFLAGS="$FF_EXTRA_LDFLAGS"
    FF_PLATFORM_CFG_FLAGS="android"
    FF_HOST_NAME="arm-linux"
elif [ "$FF_ARCH" = "x86" ]; then
    FF_BUILD_NAME=x86
    FF_CFG_FLAGS="$FF_CFG_FLAGS --disable-asm"
    FF_CROSS_PREFIX=i686-linux-android
	  FF_TOOLCHAIN_NAME=x86-${FF_GCC_VER}
    FF_PLATFORM_CFG_FLAGS="android-x86"
    FF_HOST_NAME="x86-linux"
    FF_EXTRA_LDFLAGS="$FF_EXTRA_LDFLAGS"
elif [ "$FF_ARCH" = "x86-64" ]; then
    FF_ANDROID_PLATFORM=android-21
    FF_BUILD_NAME=x86-64
    FF_CFG_FLAGS="$FF_CFG_FLAGS --disable-asm"
    FF_CROSS_PREFIX=x86_64-linux-android
    FF_TOOLCHAIN_NAME=${FF_CROSS_PREFIX}-${FF_GCC_64_VER}
    FF_PLATFORM_CFG_FLAGS="linux-x86_64"
    FF_HOST_NAME="x86-linux"
    FF_EXTRA_LDFLAGS="$FF_EXTRA_LDFLAGS"
elif [ "$FF_ARCH" = "arm64-v8a" ]; then
    FF_ANDROID_PLATFORM=android-21
    FF_BUILD_NAME=arm64-v8a
    FF_CFG_FLAGS="$FF_CFG_FLAGS --disable-asm"
    FF_CROSS_PREFIX=aarch64-linux-android
    FF_TOOLCHAIN_NAME=${FF_CROSS_PREFIX}-${FF_GCC_64_VER}
    FF_PLATFORM_CFG_FLAGS="linux-aarch64"
    FF_HOST_NAME="aarch64-linux"
    FF_EXTRA_LDFLAGS="$FF_EXTRA_LDFLAGS"
else
    echo "unknown architecture $FF_ARCH";
    exit 1
fi

FF_TOOLCHAIN_PATH=$FF_BUILD_ROOT/build/fdk-aac/$FF_BUILD_NAME/toolchain

FF_SYSROOT=$FF_TOOLCHAIN_PATH/sysroot
FF_PREFIX=$FF_BUILD_ROOT/build/fdk-aac/$FF_BUILD_NAME/output

mkdir -p $FF_PREFIX
mkdir -p $FF_SYSROOT

echo "FF_TOOLCHAIN_NAME=$FF_TOOLCHAIN_NAME"
echo "FF_PREFIX=$FF_PREFIX FF_BUILD_NAME=$FF_BUILD_NAME"
#--------------------
echo ""
echo "--------------------"
echo "[*] make NDK standalone toolchain"
echo "--------------------"
. ./do-detect-env.sh
FF_MAKE_TOOLCHAIN_FLAGS=$MAKE_TOOLCHAIN_FLAGS
FF_MAKE_FLAGS=$MAKE_FLAG

FF_MAKE_TOOLCHAIN_FLAGS="$FF_MAKE_TOOLCHAIN_FLAGS --install-dir=$FF_TOOLCHAIN_PATH"
FF_TOOLCHAIN_TOUCH="$FF_TOOLCHAIN_PATH/touch"
if [ ! -f "$FF_TOOLCHAIN_TOUCH" ]; then
    $ANDROID_NDK/build/tools/make-standalone-toolchain.sh \
        $FF_MAKE_TOOLCHAIN_FLAGS \
        --platform=$FF_ANDROID_PLATFORM \
        --toolchain=$FF_TOOLCHAIN_NAME --force
    touch $FF_TOOLCHAIN_TOUCH;
fi

echo ""
echo "--------------------"
echo "[*] check fdk-aac env"
echo "--------------------"
export PATH=$FF_TOOLCHAIN_PATH/bin:$PATH

OS=`uname`
HOST_ARCH=`uname -m`
export CCACHE=; type ccache >/dev/null 2>&1 && export CCACHE=ccache
if [ $OS == 'Linux' ]; then
	export HOST_SYSTEM=linux-$HOST_ARCH
elif [ $OS == 'Darwin' ]; then
	export HOST_SYSTEM=darwin-$HOST_ARCH
fi

export COMMON_FF_CFG_FLAGS=""
echo "FF_PREFIX: $FF_PREFIX"
#--------------------
# Standard options:
FF_CFG_FLAGS="$FF_CFG_FLAGS --prefix=$FF_PREFIX"
FF_CFG_FLAGS="$FF_CFG_FLAGS --enable-static"
FF_CFG_FLAGS="$FF_CFG_FLAGS --disable-shared"
FF_CFG_FLAGS="$FF_CFG_FLAGS --disable-dependency-tracking"
FF_CFG_FLAGS="$FF_CFG_FLAGS --with-pic=no"
FF_CFG_FLAGS="$FF_CFG_FLAGS --host=$FF_HOST_NAME"
#FF_CFG_FLAGS="$FF_CFG_FLAGS --target=$FF_CROSS_PREFIX
CROSS_PREFIX=$ANDROID_NDK/toolchains/${FF_TOOLCHAIN_NAME}/prebuilt/$HOST_SYSTEM/bin/${FF_CROSS_PREFIX}-
#SYSROOT=${ANDROID_NDK}/platforms/${FF_ANDROID_PLATFORM}/arch-arm

echo ""
echo "--------------------"
echo "[*] configurate fdk-aac "
echo "--------------------"
cd fdk-aac
./autogen.sh
#make clean
./configure $FF_CFG_FLAGS \
      --with-sysroot=${FF_SYS_ROOT} \
      CC="${CROSS_PREFIX}gcc --sysroot=${FF_SYSROOT}" \
      CXX="${CROSS_PREFIX}g++ --sysroot=${FF_SYSROOT}" \
      RANLIB="${CROSS_PREFIX}ranlib" \
      AR="${CROSS_PREFIX}ar" \
      AR_FLAGS=rcu \
      STRIP="${CROSS_PREFIX}strip" \
      NM="${CROSS_PREFIX}nm" \
      CFLAGS="-O3  --sysroot=${FF_SYSROOT}" \
      CXXFLAGS="-O3 --sysroot=${FF_SYSROOT}" \


echo ""
echo "--------------------"
echo "[*] compile fdk-aac"
echo "--------------------"
make -j8
make install
cd -
