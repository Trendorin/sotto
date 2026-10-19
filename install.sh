#!/usr/bin/env bash
# Sotto installer: builds this source tree and installs it.
#   ./install.sh              build and install
#   ./install.sh --uninstall  same as ./uninstall.sh
#
# Arch-based systems get a real pacman package (dependencies resolved, `pacman -R sotto`
# removes it). Elsewhere Sotto goes to ~/.local without root. Ready-made packages for
# Debian and Fedora and an AppImage are on the releases page.
set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")"

if [[ ${1:-} == --uninstall ]]; then
    exec ./uninstall.sh "${@:2}"
fi

if command -v pacman >/dev/null && command -v makepkg >/dev/null; then
    (cd packaging/arch && makepkg -si --clean --cleanbuild)
    rm -f packaging/arch/*.pkg.tar.*
else
    echo "Needs: cmake, a C++ compiler, Qt 6 (Widgets, Multimedia, DBus, Network, Wayland), layer-shell-qt,"
    echo "whisper.cpp and wl-clipboard."
    cmake -B build -DCMAKE_INSTALL_PREFIX="$HOME/.local" -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j"$(nproc)"
    cmake --install build
fi

# Restart a running copy so the new version takes over.
if pkill -x sotto 2>/dev/null || pkill -f '/[S]otto[^/]*\.AppImage' 2>/dev/null; then
    sleep 1
    (setsid sotto >/dev/null 2>&1 &)
    echo "Sotto restarted."
fi
echo
echo "Done. Sotto is in the application menu; choose the shortcut in its settings."
