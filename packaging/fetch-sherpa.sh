#!/usr/bin/env bash
# Fetches the pinned sherpa-onnx release (its C API and the ONNX Runtime it brings) for the
# GigaAM and Parakeet models, checks it against the SHA-256 below and unpacks it into DEST:
#
#   DEST/include/sherpa-onnx/c-api/c-api.h
#   DEST/lib/libsherpa-onnx-c-api.so   DEST/lib/libonnxruntime.so
#
#   packaging/fetch-sherpa.sh DEST [ARCH]        ARCH: x86_64 (default: this machine) or aarch64
#
# Build Sotto with -DSOTTO_SHERPA_DIR=DEST and ship DEST/lib/*.so next to the private whisper.cpp
# (the binary's run path). SHERPA_ONNX_CACHE keeps the downloads between runs.
set -euo pipefail

SHERPA_VERSION=1.13.8
dest=${1:?destination folder}
arch=${2:-$(uname -m)}
base=https://github.com/k2-fsa/sherpa-onnx/releases/download/v$SHERPA_VERSION
header_url=https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/v$SHERPA_VERSION/sherpa-onnx/c-api/c-api.h
header_sha=2a1b95084be8fd1deb3228fcad2fd3f7f0258b64582f7402281ec174c7b7f4ce
case "$arch" in
    x86_64 | amd64)
        name=sherpa-onnx-v$SHERPA_VERSION-linux-x64-shared
        sha=c0bdb7907d3a74bba1d55d22bf4d9fa75586cf1530614ebe88a27b9118e015c4 ;;
    aarch64 | arm64)
        name=sherpa-onnx-v$SHERPA_VERSION-linux-aarch64-shared-cpu
        sha=4e3734f82bc1379fd91f219f5869c7e9d03b7a4f7561907d8abca4849c51a789 ;;
    *) echo "sherpa-onnx has no Linux build for $arch" >&2; exit 1 ;;
esac

cache=${SHERPA_ONNX_CACHE:-${dest%/}.cache}
mkdir -p "$cache" "$dest/include/sherpa-onnx/c-api" "$dest/lib"

# fetch URL SHA256 FILE — downloads unless FILE is already there with that checksum
fetch() {
    if [[ -f $3 ]] && echo "$2  $3" | sha256sum -c --status; then return; fi
    curl -fsSL --retry 3 -o "$3.part" "$1"
    if ! echo "$2  $3.part" | sha256sum -c --status; then
        echo "checksum mismatch for $1" >&2
        rm -f "$3.part"
        exit 1
    fi
    mv -f "$3.part" "$3"
}

fetch "$base/$name.tar.bz2" "$sha" "$cache/$name.tar.bz2"
fetch "$header_url" "$header_sha" "$cache/c-api-$SHERPA_VERSION.h"

tar -xjf "$cache/$name.tar.bz2" -C "$cache" "$name/lib/libsherpa-onnx-c-api.so" "$name/lib/libonnxruntime.so"
install -m755 "$cache/$name/lib/libsherpa-onnx-c-api.so" "$cache/$name/lib/libonnxruntime.so" "$dest/lib/"
install -m644 "$cache/c-api-$SHERPA_VERSION.h" "$dest/include/sherpa-onnx/c-api/c-api.h"
rm -rf "${cache:?}/$name"
echo "sherpa-onnx $SHERPA_VERSION → $dest"
