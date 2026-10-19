#!/usr/bin/env bash
# Removes Sotto: the program, menu entry, autostart and KDE shortcut.
#   ./uninstall.sh          asks whether to delete downloaded models and settings too
#   ./uninstall.sh --purge  deletes them without asking
#   ./uninstall.sh --keep   keeps them
# The same thing is available in the app: Settings → "Remove Sotto…", or `sotto --uninstall`.
set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")"

DATA=${XDG_DATA_HOME:-$HOME/.local/share}
CONFIG=${XDG_CONFIG_HOME:-$HOME/.config}
ID=io.github.trendorin.Sotto

# A running copy: installed binary or an AppImage (its process is named after the file).
pkill -x sotto 2>/dev/null || true
pkill -f '/[S]otto[^/]*\.AppImage' 2>/dev/null || true

# Program
if command -v pacman >/dev/null && pacman -Qq sotto >/dev/null 2>&1; then
    echo "==> Removing pacman package"
    sudo pacman -R sotto
elif command -v dpkg-query >/dev/null && dpkg-query -W sotto >/dev/null 2>&1; then
    echo "==> Removing Debian package"
    sudo apt-get remove -y sotto
elif command -v rpm >/dev/null && rpm -q sotto >/dev/null 2>&1; then
    echo "==> Removing RPM package"
    sudo dnf remove -y sotto
elif [[ -f build/install_manifest.txt ]]; then
    echo "==> Removing files installed by cmake"
    xargs rm -fv < build/install_manifest.txt
fi

# AppImage menu entry (the .AppImage file itself is wherever you saved it)
if grep -qs 'X-AppImage-Integrated=true' "$DATA/applications/$ID.desktop"; then
    echo "==> Removing AppImage menu entry"
    sed -n 's/^TryExec=//p' "$DATA/applications/$ID.desktop" | while read -r f; do
        [[ -f $f && $f == *.AppImage ]] && rm -fv "$f"
    done
    rm -fv "$DATA/applications/$ID.desktop" "$DATA/icons/hicolor/scalable/apps/$ID.svg"
fi

rm -fv "$CONFIG/autostart/$ID.desktop"

# KDE global shortcut registered through the portal
if command -v qdbus6 >/dev/null; then
    qdbus6 org.kde.kglobalaccel /kglobalaccel org.kde.KGlobalAccel.unregister "$ID" dictate >/dev/null 2>&1 || true
fi

# Models and settings
purge=${1:-}
if [[ -z $purge && ( -d $DATA/sotto || -d $CONFIG/sotto ) ]]; then
    size=$(du -sh "$DATA/sotto" 2>/dev/null | cut -f1)
    read -rp "Delete the downloaded models (${size:-0}) and settings? [Y/n] " answer
    [[ ${answer,,} == n* ]] && purge=--keep || purge=--purge
fi
if [[ $purge == --purge ]]; then
    rm -rf "$DATA/sotto" "$CONFIG/sotto"
    echo "==> Models and settings removed"
fi

echo "Sotto removed."
