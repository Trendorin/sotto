#!/usr/bin/env bash
# Builds a portable AppImage: dist/Sotto-<version>-x86_64.AppImage
#
# Everything is compiled inside an Ubuntu 22.04 root filesystem, entered without
# root via user namespaces, so the result runs on any distro with glibc >= 2.35
# (Ubuntu 22.04+, Debian 12+, Fedora 36+, Arch, ...) and on CPUs without AVX2.
#
# The build cache (~4 GB) lives in $SOTTO_APPIMAGE_WORK, default ~/.cache/sotto-appimage.
# Delete it when you no longer need fast rebuilds.
set -euo pipefail

HERE=$(cd "$(dirname "$(readlink -f "$0")")" && pwd)
SRC=$(cd "$HERE/../.." && pwd)
WORK=${SOTTO_APPIMAGE_WORK:-${XDG_CACHE_HOME:-$HOME/.cache}/sotto-appimage}
ROOT=$WORK/rootfs
BASE_URL=https://cdimage.ubuntu.com/ubuntu-base/releases/22.04/release/ubuntu-base-22.04.5-base-amd64.tar.gz

mkdir -p "$WORK" "$SRC/dist"

if [[ ! -x $ROOT/bin/bash ]]; then
    echo "==> Downloading Ubuntu 22.04 base"
    curl -fL --retry 3 -o "$WORK/base.tar.gz" "$BASE_URL"
    mkdir -p "$ROOT"
    unshare --map-root-user tar -xzf "$WORK/base.tar.gz" -C "$ROOT" --no-same-owner --exclude='./dev/*'
    rm "$WORK/base.tar.gz"
fi

rm -f "$ROOT/etc/resolv.conf"
cp -L /etc/resolv.conf "$ROOT/etc/resolv.conf"
mkdir -p "$ROOT/src" "$ROOT/work" "$ROOT/usr/local/lib"
gcc -shared -fPIC -O2 -nostdlib -o "$ROOT/usr/local/lib/fakechown.so" "$HERE/fakechown.c"

unshare --user --map-root-user --mount --pid --fork --kill-child bash -c '
    set -e
    R=$1
    mount --rbind /dev "$R/dev"
    mount -t proc proc "$R/proc"
    mount -t tmpfs tmpfs "$R/tmp"
    mount --bind "$2" "$R/src"
    exec chroot "$R" /usr/bin/env -i HOME=/root TERM=xterm LANG=C.UTF-8 \
        PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
        /bin/bash /src/packaging/appimage/inside.sh
' _ "$ROOT" "$SRC"

ls -lh "$SRC"/dist/*.AppImage
