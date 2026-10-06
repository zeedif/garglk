#!/bin/sh
# Builds Gargoyle for Kindle firmware 5.16.3+ (kindlehf) into build-kindlehf/dist.
# Requires koxtoolchain's kindlehf toolchain with kindle-sdk installed on top of it.

set -e

TOP=$(CDPATH='' cd "$(dirname "$0")/../.." && pwd -P)
BUILD="${BUILD_DIR:-${TOP}/build-kindlehf}"
DEPS="${BUILD}/deps"
TOOLCHAIN="${TOP}/support/kindle/kindlehf.cmake"
JPEG_VERSION=3.1.0
FONTCONFIG_VERSION=2.15.0

# Several bundled interpreters predate the C99 rules that GCC 14 turned into errors.
LEGACY_C_FLAGS="-Wno-error=implicit-function-declaration -Wno-error=implicit-int -Wno-error=int-conversion -Wno-error=incompatible-pointer-types"

# The firmware libstdc++ (GCC 10) is older than the toolchain one.
STATIC_RUNTIME_FLAGS="-static-libstdc++ -static-libgcc"

mkdir -p "${BUILD}/src"

# The firmware ships libjpeg without headers and its soname is not stable across
# releases, so a static PIC libjpeg-turbo is linked into libgarglk instead.
if [ ! -f "${DEPS}/lib/libjpeg.a" ]; then
    JPEG_SRC="${BUILD}/src/libjpeg-turbo-${JPEG_VERSION}"
    if [ ! -d "${JPEG_SRC}" ]; then
        curl -fsSL "https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/${JPEG_VERSION}/libjpeg-turbo-${JPEG_VERSION}.tar.gz" |
            tar -xz -C "${BUILD}/src"
    fi
    cmake -S "${JPEG_SRC}" -B "${BUILD}/jpeg" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${DEPS}" \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DENABLE_SHARED=OFF \
        -DWITH_TURBOJPEG=OFF \
        -DWITH_TOOLS=OFF \
        -DWITH_TESTS=OFF
    cmake --build "${BUILD}/jpeg"
    cmake --install "${BUILD}/jpeg"
fi

# kindle-sdk provides libfontconfig from the firmware but not its headers.
if [ ! -f "${DEPS}/include/fontconfig/fontconfig.h" ]; then
    curl -fsSL "https://www.freedesktop.org/software/fontconfig/release/fontconfig-${FONTCONFIG_VERSION}.tar.xz" |
        tar -xJ -C "${BUILD}/src"
    mkdir -p "${DEPS}/include/fontconfig"
    cp "${BUILD}/src/fontconfig-${FONTCONFIG_VERSION}/fontconfig/"*.h "${DEPS}/include/fontconfig/"
fi

cmake -S "${TOP}" -B "${BUILD}/garglk" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
    -DKINDLE_DEPS_PREFIX="${DEPS}" \
    -DCMAKE_C_FLAGS="-I${DEPS}/include ${LEGACY_C_FLAGS}" \
    -DCMAKE_EXE_LINKER_FLAGS="-Wl,--as-needed ${STATIC_RUNTIME_FLAGS}" \
    -DCMAKE_SHARED_LINKER_FLAGS="-Wl,--as-needed" \
    -DCMAKE_INSTALL_RPATH='$ORIGIN' \
    -DJPEG_INCLUDE_DIR="${DEPS}/include" \
    -DJPEG_LIBRARY="${DEPS}/lib/libjpeg.a" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${BUILD}/dist" \
    -DCOMPILE_FOR_KINDLE=ON \
    -DWITH_SDL=OFF
cmake --build "${BUILD}/garglk"
cmake --install "${BUILD}/garglk"
