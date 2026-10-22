#!/usr/bin/env bash
# Takes the README and wiki screenshots of the settings window in English and Russian:
#
#   .github/assets/screenshot.png         .github/assets/screenshot-ru.png          (Recognition)
#   .github/assets/screenshot-typing.png  .github/assets/screenshot-typing-ru.png   (Recording and pasting, niri)
#
#   tools/screenshots.sh [SOTTO]      SOTTO: the binary (default build/sotto/sotto)
#
# Everything runs in a hidden KWin (kwin_wayland --virtual) on a private D-Bus session with a
# scratch HOME and Breeze Dark, so the desktop you work in is never touched. The models come from
# SOTTO_MODELS (default ~/.local/share/sotto/models, linked read-only); GigaAM v3 has to be there.
# Needs kwin_wayland, spectacle and dbus-run-session (KDE Plasma).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOTTO="$(realpath "${1:-$ROOT/build/sotto/sotto}")"
MODELS="${SOTTO_MODELS:-${XDG_DATA_HOME:-$HOME/.local/share}/sotto/models}"
OUT="$ROOT/.github/assets"

for tool in kwin_wayland spectacle dbus-run-session; do
    command -v "$tool" >/dev/null || { echo "$tool is needed" >&2; exit 1; }
done
[ -x "$SOTTO" ] || { echo "no Sotto binary at $SOTTO (build it first)" >&2; exit 1; }
[ -d "$MODELS/gigaam-v3-ru" ] || { echo "GigaAM v3 is not in $MODELS (download it in the settings)" >&2; exit 1; }

# Short paths: a Wayland socket path must fit in 108 bytes.
WORK="$(mktemp -d /tmp/sotto-shots.XXXXXX)"
trap 'rm -rf "$WORK"' EXIT
mkdir -m 700 "$WORK/run"
mkdir -p "$WORK/home/.config/sotto" "$WORK/home/.local/share/sotto/models"
for m in "$MODELS"/*; do ln -s "$m" "$WORK/home/.local/share/sotto/models/"; done
# Sotto on PATH, so the key binding reads "sotto" instead of the build path, and a wtype that
# presses nothing, so "Press keys with" shows what it does on niri.
mkdir "$WORK/bin"
cp "$SOTTO" "$WORK/bin/sotto"
printf '#!/bin/sh\nexit 0\n' > "$WORK/bin/wtype"
chmod +x "$WORK/bin/wtype"
SOTTO="$WORK/bin/sotto"

# Breeze Dark for Qt (kdeglobals = the colour scheme plus the widget style and icons).
{
    cat /usr/share/color-schemes/BreezeDark.colors
    printf '\n[KDE]\nwidgetStyle=Breeze\n\n[Icons]\nTheme=breeze-dark\n'
} > "$WORK/home/.config/kdeglobals"

# A session bus that starts no services: no portal can open a dialog over the window.
cat > "$WORK/bus.conf" <<'EOF'
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <type>session</type>
  <listen>unix:tmpdir=/tmp</listen>
  <policy context="default">
    <allow send_destination="*" eavesdrop="true"/>
    <allow eavesdrop="true"/>
    <allow own="*"/>
  </policy>
</busconfig>
EOF

cat > "$WORK/inside.sh" <<'EOF'
set -euo pipefail
kwin_wayland --virtual --no-lockscreen --no-global-shortcuts --socket wayland-shot --width 1280 --height 960 \
    >"$WORK/kwin.log" 2>&1 &
kwin=$!
for _ in $(seq 50); do [ -S "$XDG_RUNTIME_DIR/wayland-shot" ] && break; sleep 0.1; done
export WAYLAND_DISPLAY=wayland-shot QT_QPA_PLATFORM=wayland QT_QPA_PLATFORMTHEME=kde

# shoot LANG TAB DESKTOP FILE
shoot() {
    cat > "$HOME/.config/sotto/sotto.conf" <<CONF
[General]
model=gigaam-v3-ru
language=ru
uiLanguage=$1
live=true
pauseMs=600
modelSource=huggingface

[output]
keys=ctrl+v
method=auto
paste=true
CONF
    XDG_CURRENT_DESKTOP=$3 SOTTO_SETTINGS_TAB=$2 "$SOTTO" >>"$WORK/sotto.log" 2>&1 &
    local app=$!
    sleep 3
    "$SOTTO" --settings
    sleep 2
    spectacle -b -n -a -o "$OUT/$4" >/dev/null 2>&1
    sleep 2
    "$SOTTO" --quit
    wait "$app" || true
    echo "$OUT/$4"
}
shoot en 0 KDE screenshot.png
shoot ru 0 KDE screenshot-ru.png
shoot en 1 niri screenshot-typing.png
shoot ru 1 niri screenshot-typing-ru.png
kill "$kwin"
wait "$kwin" 2>/dev/null || true
EOF

unset DISPLAY WAYLAND_DISPLAY
HOME="$WORK/home" XDG_CONFIG_HOME="$WORK/home/.config" XDG_DATA_HOME="$WORK/home/.local/share" \
    XDG_CACHE_HOME="$WORK/home/.cache" XDG_RUNTIME_DIR="$WORK/run" PATH="$WORK/bin:$PATH" WORK="$WORK" SOTTO="$SOTTO" OUT="$OUT" \
    dbus-run-session --config-file="$WORK/bus.conf" -- bash "$WORK/inside.sh"
