#! /usr/bin/env bash
UNI_BUILD_ROOT=`pwd`
FF_TARGET=$1
set -e
set +x

FF_ACT_ARCHS_32="armeabi armeabi-v7a x86"
FF_ACT_ARCHS_64="armeabi armeabi-v7 arm64-v8a x86 x86-64"
FF_ACT_ARCHS_ALL=$FF_ACT_ARCHS_64

echo_archs() {
    echo "===================="
    echo "[*] check archs"
    echo "===================="
    echo "FF_ALL_ARCHS = $FF_ACT_ARCHS_ALL"
    echo "FF_ACT_ARCHS = $*"
    echo ""
}

echo_usage() {
    echo "Usage:"
    echo "  compile-ffmpeg.sh armv5|armv7a|arm64|x86|x86_64"
    echo "  compile-ffmpeg.sh all|all32"
    echo "  compile-ffmpeg.sh all64"
    echo "  compile-ffmpeg.sh clean"
    echo "  compile-ffmpeg.sh check"
    exit 1
}

echo_nextstep_help() {
    echo ""
    echo "--------------------"
    echo "[*] Finished"
}

#----------
case "$FF_TARGET" in
    "")
        sh ./do-compile-ffmpeg.sh armeabi-v7a
    ;;
    armeabi|armeabi-v7a|arm64-v8a|x86|x86-64)
        echo_archs $FF_TARGET
        sh ./do-compile-ffmpeg.sh $FF_TARGET
    ;;
    all32)
        echo_archs $FF_ACT_ARCHS_32
        for ARCH in $FF_ACT_ARCHS_32
        do
            sh ./do-compile-ffmpeg.sh $ARCH
        done
    ;;
    all|all64)
        echo_archs $FF_ACT_ARCHS_64
        for ARCH in $FF_ACT_ARCHS_64
        do
            sh ./do-compile-ffmpeg.sh $ARCH
        done
    ;;
    clean)
        echo_archs FF_ACT_ARCHS_64
        for ARCH in $FF_ACT_ARCHS_ALL
        do
            if [ -d ffmpeg-$ARCH ]; then
                cd ffmpeg-$ARCH && git clean -xdf && cd -
            fi
        done
        rm -rf ./build/ffmpeg-*
    ;;
    check)
        echo_archs FF_ACT_ARCHS_ALL
    ;;
    *)
        echo_usage
        exit 1
    ;;
esac
