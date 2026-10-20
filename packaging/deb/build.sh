#!/usr/bin/env bash
# Builds dist/sotto_<version>-1_<arch>.deb from this source tree on Debian 13 or newer.
# whisper.cpp is not in Debian, so the pinned version is built and kept in /usr/lib/sotto.
#
#   sudo apt-get install build-essential cmake ninja-build dpkg-dev git qt6-base-dev \
#       qt6-multimedia-dev qt6-wayland-dev liblayershellqtinterface-dev libvulkan-dev glslc spirv-headers
#   packaging/deb/build.sh
#   sudo apt install ./dist/sotto_*.deb
set -euo pipefail

ROOT=$(cd "$(dirname "$(readlink -f "$0")")/../.." && pwd)
cd "$ROOT"
VERSION=$(sed -n 's/^project(sotto VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
ARCH=$(dpkg --print-architecture)
PRIV=/usr/lib/sotto
BUILD=$ROOT/build-deb
STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT
PKG=$STAGE/sotto

packaging/build-whisper.sh "$PRIV" "$BUILD/whisper"
cmake -S . -B "$BUILD/sotto" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_PREFIX_PATH="$BUILD/whisper$PRIV" -DCMAKE_INSTALL_RPATH="$PRIV" -DSOTTO_GGML_BACKEND_DIR="$PRIV/ggml" \
    -DSOTTO_WERROR=ON
cmake --build "$BUILD/sotto"
DESTDIR=$PKG cmake --install "$BUILD/sotto"

# The private whisper.cpp: libraries and backends only.
install -d "$PKG$PRIV/ggml"
cp -a "$BUILD/whisper$PRIV"/lib/lib*.so* "$PKG$PRIV/"
cp -a "$BUILD/whisper$PRIV"/ggml/*.so "$PKG$PRIV/ggml/"

rm -rf "$PKG/usr/share/licenses"
install -Dm644 LICENSE "$PKG/usr/share/doc/sotto/copyright"

mkdir -p "$STAGE/debian"
printf 'Source: sotto\n\nPackage: sotto\nArchitecture: any\n' > "$STAGE/debian/control"
SHLIBS=$(cd "$STAGE" && dpkg-shlibdeps -O --ignore-missing-info -l"sotto$PRIV" \
    -e sotto/usr/bin/sotto $(cd "$STAGE" && ls sotto$PRIV/*.so.* sotto$PRIV/ggml/*.so | sed 's/^/-e /') \
    | sed -n 's/^shlibs:Depends=//p')

install -d "$PKG/DEBIAN"
cat > "$PKG/DEBIAN/control" <<CONTROL
Package: sotto
Version: $VERSION-1
Architecture: $ARCH
Maintainer: Trendorin <193045225+Trendorin@users.noreply.github.com>
Installed-Size: $(du -sk "$PKG" | cut -f1)
Depends: $SHLIBS, qt6-wayland, layer-shell-qt
Recommends: wl-clipboard, xdg-desktop-portal, pkexec
Section: utils
Priority: optional
Homepage: https://github.com/Trendorin/sotto
Description: voice typing for Linux with whisper.cpp
 Press a key, speak, release it: Sotto transcribes your speech locally with
 whisper.cpp and pastes the text where the cursor is. It uses Vulkan when a GPU
 is available, cuts silence with Silero VAD, and on Wayland gets its shortcut
 and pastes through the desktop portals. Models are downloaded once and checked
 against their published SHA-256 sums.
CONTROL

mkdir -p dist
OUT=dist/sotto_${VERSION}-1_${ARCH}.deb
dpkg-deb --root-owner-group -Zxz --build "$PKG" "$OUT"
echo "$OUT"
