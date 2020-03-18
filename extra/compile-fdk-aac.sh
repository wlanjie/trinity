#! /usr/bin/env bash
UNI_BUILD_ROOT=`pwd`
FF_TARGET=$1
set -e
set +x

FF_ACT_ARCHS_32="armeabi armeabi-v7a x86"
FF_ACT_ARCHS_64="armeabi armeabi-v7a arm64-v8a x86 x86-64"
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
    echo "  compile-fdk-aac.sh armv5|armv7a|arm64|x86|x86_64"
    echo "  compile-fdk-aac.sh all|all32"
    echo "  compile-fdk-aac.sh all64"
    echo "  compile-fdk-aac.sh clean"
    echo "  compile-fdk-aac.sh check"
    exit 1
}

echo_nextstep_help() {
    #----------
    echo ""
    echo "--------------------"
    echo "[*] Finished"
    echo "--------------------"
    echo "# to continue to build ffmpeg, run script below,"
    echo "sh compile-ffmpeg.sh "
}
echo "FF_TARGET $1"
#----------
case "$FF_TARGET" in
    "")
        echo_archs armeabi-v7a
        sh ./do-compile-fdk-aac.sh armeabi-v7a
    ;;
    armeabi|armeabi-v7a|arm64-v8a|x86|x86-64)
        echo_archs $FF_TARGET
        sh ./do-compile-fdk-aac.sh $FF_TARGET
        echo_nextstep_help
    ;;
    all32)
        echo_archs $FF_ACT_ARCHS_32
        for ARCH in $FF_ACT_ARCHS_32
        do
            sh ./do-compile-fdk-aac.sh $ARCH
        done
        echo_nextstep_help
    ;;
    all|all64)
        echo_archs $FF_ACT_ARCHS_64
        for ARCH in $FF_ACT_ARCHS_64
        do
            sh ./do-compile-fdk-aac.sh $ARCH
        done
        echo_nextstep_help
    ;;
    clean)
        echo_archs FF_ACT_ARCHS_64
        for ARCH in $FF_ACT_ARCHS_ALL
        do
            if [ -d fdk-aac-$ARCH ]; then
                cd fdk-aac-$ARCH && git clean -xdf && cd -
            fi
        done
        rm -rf ./build/fdk-aac-*
    ;;
    check)
        echo_archs FF_ACT_ARCHS_ALL
    ;;
    *)
        echo_usage
        exit 1
    ;;
esac
