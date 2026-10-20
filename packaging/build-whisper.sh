#!/usr/bin/env bash
# Builds the pinned whisper.cpp that the Debian and Fedora packages carry privately in
# PRIVATE_DIR: the packages ship its shared libraries in PRIVATE_DIR (their run path)
# and the ggml backends (every CPU variant plus Vulkan, picked at run time) in
# PRIVATE_DIR/ggml. Under DESTDIR it also leaves headers and CMake files in
# PRIVATE_DIR/{include,lib} to build Sotto against.
#
#   packaging/build-whisper.sh PRIVATE_DIR DESTDIR [SOURCE_DIR]
#
# Without SOURCE_DIR the release tag is cloned next to DESTDIR. WHISPER_VULKAN=OFF builds
# the CPU backends only. Needs cmake, ninja,
# a C++ compiler, the Vulkan headers and loader, and glslc.
set -euo pipefail

WHISPER_VERSION=1.9.5
prefix=${1:?prefix} dest=${2:?destdir} src=${3:-}
work=${dest%/}.build

if [[ -z $src ]]; then
    src=$work/whisper.cpp-$WHISPER_VERSION
    [[ -d $src ]] || git clone --quiet --depth 1 --branch "v$WHISPER_VERSION" https://github.com/ggml-org/whisper.cpp.git "$src"
fi

cmake -S "$src" -B "$work/build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$prefix" -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_INSTALL_RPATH="$prefix" \
    -DBUILD_SHARED_LIBS=ON -DGGML_NATIVE=OFF -DGGML_BACKEND_DL=ON -DGGML_CPU_ALL_VARIANTS=ON \
    -DGGML_OPENMP=OFF -DGGML_VULKAN="${WHISPER_VULKAN:-ON}" -DGGML_BACKEND_DIR="$prefix/ggml" \
    -DWHISPER_BUILD_EXAMPLES=OFF -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_SERVER=OFF -DWHISPER_FFMPEG=OFF
cmake --build "$work/build"
DESTDIR=$dest cmake --install "$work/build"
