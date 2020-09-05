set -e

UNAME_S=$(uname -s)
UNAME_SM=$(uname -sm)
echo "build on $UNAME_SM"

echo "ANDROID_SDK=$ANDROID_SDK"
echo "ANDROID_NDK=$ANDROID_NDK"

if [ -z "$ANDROID_NDK" -o -z "$ANDROID_SDK" ]; then
    echo "You must define ANDROID_NDK, ANDROID_SDK before starting."
    echo "They must point to your NDK and SDK directories."
    echo ""
    exit 1
fi

# try to detect NDK version
export GCC_VER=4.9
export GCC_64_VER=4.9
export MAKE_TOOLCHAIN_FLAGS=
export MAKE_FLAG=
export NDK_REL=$(grep -o '^r[0-9]*.*' $ANDROID_NDK/RELEASE.TXT 2>/dev/null|cut -b2-)
case "$NDK_REL" in
    10e*)
        # we don't use 4.4.3 because it doesn't handle threads correctly.
        if test -d ${ANDROID_NDK}/toolchains/arm-linux-androideabi-4.8
        # if gcc 4.8 is present, it's there for all the archs (x86, mips, arm)
        then
            case "$UNAME_S" in
                Darwin)
                    export MAKE_TOOLCHAIN_FLAGS="$MAKE_TOOLCHAIN_FLAGS --system=darwin-x86_64"
                ;;
                CYGWIN_NT-*)
                    export MAKE_TOOLCHAIN_FLAGS="$MAKE_TOOLCHAIN_FLAGS --system=windows-x86_64"
                ;;
            esac
        else
            echo "You need the NDKr10e or later"
            exit 1
        fi
    ;;
    *)
        NDK_REL=$(grep -o '^Pkg\.Revision.*=[0-9]*.*' $ANDROID_NDK/source.properties 2>/dev/null | sed 's/[[:space:]]*//g' | cut -d "=" -f 2)
        echo "NDK_REL=$NDK_REL"
        case "$NDK_REL" in
            11*|12*|13*|14*|15*|16*|17*|18*)
                if test -d ${ANDROID_NDK}/toolchains/arm-linux-androideabi-4.9
                then
                    echo "NDKr$NDK_REL detected"
                else
                    echo "You need the NDKr10e or later"
                    exit 1
                fi
            ;;
            *)
                echo "You need the NDKr10e or later"
                exit 1
            ;;
        esac
    ;;
esac


case "$UNAME_S" in
    Darwin)
        export MAKE_FLAG=-j`sysctl -n machdep.cpu.thread_count`
    ;;
    CYGWIN_NT-*)
        WIN_TEMP="$(cygpath -am /tmp)"
        export TEMPDIR=$WIN_TEMP/

        echo "Cygwin temp prefix=$WIN_TEMP/"
    ;;
esac
