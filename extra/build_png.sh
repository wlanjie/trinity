#! /usr/bin/env bash
set -e
set -x

ARCHS_32="armeabi armeabi-v7a x86"
ARCHS_64="armeabi armeabi-v7a arm64-v8a x86 x86-64"

case "$1" in
  "")
    sh ./build_png_platform.sh armeabi-v7a
  ;;
  armeabi|armeabi-v7a|arm64-v8a|x86|x86-64)
    sh ./build_png_platform.sh $1
  ;;
  all32)
    for ARCH in $ARCHS_32
    do
      sh ./build_png_platform.sh $ARCH
    done
  ;;
  all|all64)
    for ARCH in $ARCHS_64
    do
      sh ./build_png_platform.sh $ARCH
    done
  ;;
  clean)
    for ARCH in $ARCHS_64
    do
      echo $ARCH
    done
  ;;

esac
