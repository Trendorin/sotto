#!/usr/bin/env bash
# Builds dist/sotto_<version>-1_<arch>.deb from this source tree on Debian 13 or newer.
# whisper.cpp and sherpa-onnx are not in Debian, so the pinned versions are built (whisper.cpp)
# or fetched and checked (sherpa-onnx) and kept in /usr/lib/sotto.
#
#   sudo apt-get install build-essential cmake ninja-build dpkg-dev git curl bzip2 python3 qt6-base-dev qt6-qpa-plugins \
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
packaging/fetch-sherpa.sh "$BUILD/sherpa"
cmake -S . -B "$BUILD/sotto" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_PREFIX_PATH="$BUILD/whisper$PRIV" -DCMAKE_INSTALL_RPATH="$PRIV" -DSOTTO_GGML_BACKEND_DIR="$PRIV/ggml" \
    -DSOTTO_SHERPA_DIR="$BUILD/sherpa" -DSOTTO_REQUIRE_SHERPA=ON -DSOTTO_WERROR=ON -DSOTTO_BUILD_TESTS=ON
cmake --build "$BUILD/sotto"
ctest --test-dir "$BUILD/sotto" --output-on-failure
DESTDIR=$PKG cmake --install "$BUILD/sotto"

# The private whisper.cpp (libraries and backends only) and sherpa-onnx with its ONNX Runtime.
install -d "$PKG$PRIV/ggml"
cp -a "$BUILD/whisper$PRIV"/lib/lib*.so* "$PKG$PRIV/"
cp -a "$BUILD/whisper$PRIV"/ggml/*.so "$PKG$PRIV/ggml/"
cp -a "$BUILD/sherpa"/lib/*.so "$PKG$PRIV/"

rm -rf "$PKG/usr/share/licenses"
install -Dm644 LICENSE "$PKG/usr/share/doc/sotto/copyright"

mkdir -p "$STAGE/debian"
printf 'Source: sotto\n\nPackage: sotto\nArchitecture: any\n' > "$STAGE/debian/control"
SHLIBS=$(cd "$STAGE" && dpkg-shlibdeps -O --ignore-missing-info -l"sotto$PRIV" \
    -e sotto/usr/bin/sotto $(cd "$STAGE" && ls sotto$PRIV/*.so* sotto$PRIV/ggml/*.so | sed 's/^/-e /') \
    | sed -n 's/^shlibs:Depends=//p')

install -d "$PKG/DEBIAN"
cat > "$PKG/DEBIAN/control" <<CONTROL
Package: sotto
Version: $VERSION-1
Architecture: $ARCH
Maintainer: Trendorin <193045225+Trendorin@users.noreply.github.com>
Installed-Size: $(du -sk "$PKG" | cut -f1)
Depends: $SHLIBS, qt6-wayland, qt6-qpa-plugins, qt6-svg-plugins, layer-shell-qt
Recommends: wl-clipboard, xdg-desktop-portal, pkexec
Suggests: wtype, ydotool
Section: utils
Priority: optional
Homepage: https://github.com/Trendorin/sotto
Description: voice typing for Linux: the text appears while you speak
 Press a key and speak: Sotto recognises your speech locally and types each
 phrase where the cursor is as soon as you pause. GigaAM v3 (the most accurate
 open model for Russian) and Parakeet TDT v3 (25 languages) run fast on the
 processor through sherpa-onnx; Whisper runs on the GPU with Vulkan. On Wayland
 it gets its shortcut and pastes through the desktop portals, or through wtype
 on Hyprland, niri and sway. Models are downloaded once and checked against
 their published SHA-256 sums.
CONTROL

mkdir -p dist
OUT=dist/sotto_${VERSION}-1_${ARCH}.deb
dpkg-deb --root-owner-group -Zxz --build "$PKG" "$OUT"
echo "$OUT"
