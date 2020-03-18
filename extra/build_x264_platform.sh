#! /usr/bin/env bash
set -e
set -x
echo "arch $1"
echo "pwd $PWD"

HOST=
SYSROOT=
EXTRA_CFLAGS=
EXTRA_ASFLAGS=
EXTRA_LDFLAGS=
PREFIX=
TOOLCHAIN_PATH=
TOOLCHAIN=
INCLUDE=
ANDROID_LIB=
VERSION=
CROSS_PREFIX=
DISABLE_ASM=

if [ "$1" = "armeabi-v7a" ]; then
  VERSION=14
  PREFIX=build/x264/armeabi-v7a
  TOOLCHAIN_PATH=build/x264/armeabi-v7a/toolchain
  HOST=arm-linux
  SYSROOT=$ANDROID_NDK/platforms/android-$VERSION/arch-arm
  TOOLCHAIN=arm-linux-androideabi-4.9
  INCLUDE=$ANDROID_NDK/platforms/android-$VERSION/arch-arm/usr/include
  ANDROID_LIB=$ANDROID_NDK/platforms/android-$VERSION/arch-arm/usr/lib
  EXTRA_CFLAGS="-I$INCLUDE -fPIC -DANDROID -fpic -mthumb-interwork -ffunction-sections -funwind-tables -fstack-protector -fno-short-enums -march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp -mfpu=neon -D__ARM_ARCH_7__ -D__ARM_ARCH_7A__  -Wno-psabi -msoft-float -mthumb -Os -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64 -DANDROID  -Wa,--noexecstack -MMD -MP"
  EXTRA_ASFLAGS="-I$INCLUDE -fPIC -DANDROID -fpic -mthumb-interwork -ffunction-sections -funwind-tables -fstack-protector -fno-short-enums -march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp -D__ARM_ARCH_7__ -D__ARM_ARCH_7A__  -Wno-psabi -msoft-float -mthumb -Os -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64 -DANDROID  -Wa,--noexecstack -MMD -MP"
  EXTRA_LDFLAGS="-nostdlib -Bdynamic -Wl,--no-undefined -Wl,-z,noexecstack  -Wl,-z,nocopyreloc -Wl,-soname,/system/lib/libz.so -Wl,-rpath-link=$ANDROID_LIB,-dynamic-linker=/system/bin/linker -L$ANDROID_LIB -nostdlib $ANDROID_LIB/crtbegin_dynamic.o $ANDROID_LIB/crtend_android.o -lc -lm -ldl -lgcc"
  CROSS_PREFIX=../$TOOLCHAIN_PATH/bin/$HOST-androideabi-
elif [ "$1" = "armeabi" ]; then
  VERSION=14
  PREFIX=build/x264/armeabi
  TOOLCHAIN_PATH=build/x264/armeabi/toolchain
  HOST=arm-linux
  SYSROOT=$ANDROID_NDK/platforms/android-$VERSION/arch-arm
  TOOLCHAIN=arm-linux-androideabi-4.9
  INCLUDE=$ANDROID_NDK/platforms/android-$VERSION/arch-arm/usr/include
  ANDROID_LIB=$ANDROID_NDK/platforms/android-$VERSION/arch-arm/usr/lib
  EXTRA_CFLAGS="-I$INCLUDE -march=armv5te -DANDROID -mtune=arm9tdmi -msoft-float -O3"
  EXTRA_ASFLAGS="-I$INCLUDE -march=armv5te -DANDROID -mtune=arm9tdmi -msoft-float -O3"
  EXTRA_LDFLAGS="-nostdlib -Bdynamic -Wl,--no-undefined -Wl,-z,noexecstack  -Wl,-z,nocopyreloc -Wl,-soname,/system/lib/libz.so -Wl,-rpath-link=$ANDROID_LIB,-dynamic-linker=/system/bin/linker -L$ANDROID_LIB -nostdlib $ANDROID_LIB/crtbegin_dynamic.o $ANDROID_LIB/crtend_android.o -lc -lm -ldl -lgcc"
  CROSS_PREFIX=../$TOOLCHAIN_PATH/bin/$HOST-androideabi-
  DISABLE_ASM=--disable-asm
elif [ "$1" = "arm64-v8a" ]; then
  echo "armeabi-v8a"
  VERSION=21
  PREFIX=build/x264/arm64-v8a
  TOOLCHAIN_PATH=build/x264/arm64-v8a/toolchain
  HOST=aarch64-linux
  SYSROOT=$ANDROID_NDK/platforms/android-$VERSION/arch-arm64
  TOOLCHAIN=aarch64-linux-androideabi-4.9
  INCLUDE=$ANDROID_NDK/platforms/android-$VERSION/arch-arm64/usr/include
  ANDROID_LIB=$ANDROID_NDK/platforms/android-$VERSION/arch-arm64/usr/lib
  EXTRA_CFLAGS="-I$INCLUDEs -enable-yasm -march=armv8-a -DANDROID -D__ARM_ARCH_8__ -D__ARM_ARCH_8A__ -D__ARM_NEON__ -O3"
  EXTRA_ASFLAGS="-I$INCLUDE -enable-yasm -march=armv8-a -DANDROID -D__ARM_ARCH_8__ -D__ARM_ARCH_8A__ -D__ARM_NEON__ -O3"
  EXTRA_LDFLAGS="-nostdlib -Bdynamic -Wl,--no-undefined -Wl,-z,noexecstack  -Wl,-z,nocopyreloc -Wl,-soname,/system/lib/libz.so -Wl,-rpath-link=$ANDROID_LIB,-dynamic-linker=/system/bin/linker -L$ANDROID_LIB -nostdlib $ANDROID_LIB/crtbegin_dynamic.o $ANDROID_LIB/crtend_android.o -lc -lm -ldl -lgcc"
  CROSS_PREFIX=../$TOOLCHAIN_PATH/bin/$HOST-android-
elif [ "$1" = "x86" ]; then
  VERSION=14
  PREFIX=build/x264/x86
  TOOLCHAIN_PATH=build/x264/x86/toolchain
  HOST=i686-linux
  SYSROOT=$ANDROID_NDK/platforms/android-$VERSION/arch-x86
  TOOLCHAIN=x86-4.9
  INCLUDE=$ANDROID_NDK/platforms/android-$VERSION/arch-x86/usr/include
  ANDROID_LIB=$ANDROID_NDK/platforms/andrid-$VERSION/arch-x86/usr/lib
  #EXTRA_CFLAGS="-I$INCLUDE"
  #EXTRA_ASFLAGS="-I$INCLUDE"
  #EXTRA_LDFLAGS="-nostdlib -Bdynamic -Wl,--no-undefined -Wl,-z,noexecstack  -Wl,-z,nocopyreloc -Wl,-soname,/system/lib/libz.so -Wl,-rpath-link=$ANDROID_LIB,-dynamic-linker=/system/bin/linker -L$ANDROID_LIB -nostdlib $ANDROID_LIB/crtbegin_dynamic.o $ANDROID_LIB/crtend_android.o -lc -lm -ldl -lgcc"
  CROSS_PREFIX=../$TOOLCHAIN_PATH/bin/i686-linux-android-
  DISABLE_ASM=--disable-asm
elif [ "$1" = "x86-64" ]; then
  VERSION=21
  PREFIX=build/x264/x86-64
  TOOLCHAIN_PATH=build/x264/x86-64/toolchain
  HOST=x86_64-linux
  SYSROOT=$ANDROID_NDK/platforms/android-$VERSION/arch-x86_64
  TOOLCHAIN=x86_64-4.9
  INCLUDE=$ANDROID_NDK/platforms/android-$VERSION/arch-x86_64/usr/include
  ANDROID_LIB=$ANDROID_NDK/platforms/android-$VERSION/arch-x86_64/usr/lib
  EXTRA_CFLAGS="-I$INCLUDE -march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel"
  EXTRA_ASFLAGS="-I$INCLUDE -march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel"
  #EXTRA_LDFLAGS="-nostdlib -Bdynamic -Wl,--no-undefined -Wl,-z,noexecstack  -Wl,-z,nocopyreloc -Wl,-soname,/system/lib/libz.so -Wl,-rpath-link=$ANDROID_LIB,-dynamic-linker=/system/bin/linker -L$ANDROID_LIB -nostdlib $ANDROID_LIB/crtbegin_dynamic.o $ANDROID_LIB/crtend_android.o -lc -lm -ldl -lgcc"
  CROSS_PREFIX=../$TOOLCHAIN_PATH/bin/x86_64-linux-android-
  DISABLE_ASM=--disable-asm
else
  echo "unknown $ARCH"
fi

echo "prefix = $PREFIX"

rm -rf $PREFIX
mkdir $PREFIX
rm -rf $TOOLCHAIN_PATH
mkdir $TOOLCHAIN_PATH

$ANDROID_NDK/build/tools/make-standalone-toolchain.sh --toolchain=$TOOLCHAIN --platform=android-$VERSION --install-dir=$TOOLCHAIN_PATH --force

cd x264
make clean
./configure --prefix="../$PREFIX" \
            --host=$HOST \
            --enable-static \
            --enable-pic \
            --enable-strip \
            --disable-cli \
            --disable-opencl \
            --disable-interlaced \
            --disable-avs \
            --disable-swscale \
            --disable-lavf \
            --disable-ffms \
            $DISABLE_ASM \
            --disable-gpac \
            --extra-cflags="$EXTRA_CFLAGS" \
            --extra-asflags="$EXTRA_ASFLAGS" \
            --extra-ldflags="$EXTRA_LDFLAGS" \
            --sysroot=$SYSROOT \
            --cross-prefix=$CROSS_PREFIX
make && make install
cd -
rm -rf $TOOLCHAIN_PATH
