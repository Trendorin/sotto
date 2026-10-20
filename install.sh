#!/usr/bin/env bash
# Sotto installer.
#
#   ./install.sh                 interactive install or update (asks a few questions)
#   ./install.sh --yes           no questions: defaults plus the flags below
#   ./install.sh --prefix DIR    install into DIR/bin and DIR/share (default ~/.local)
#   ./install.sh --own-whisper   build the pinned whisper.cpp even if the system has one
#   ./install.sh --uninstall     remove Sotto (add --purge to delete models and settings too)
#
# Builds from this source checkout with CMake. Without a system whisper.cpp it builds
# the pinned version into PREFIX/lib/sotto (Vulkan when the headers and glslc are there).
# Prebuilt packages (Arch, Debian, Fedora, AppImage) are on the releases page. Never
# uses sudo for the install.

set -euo pipefail

# ---- options ------------------------------------------------------------------------

PREFIX="$HOME/.local"
OWN_WHISPER=0
ASSUME_YES=0
ACTION=install
PURGE=0

usage() { sed -n '2,14p' "$0" | sed 's/^# \{0,1\}//'; }

while [ $# -gt 0 ]; do
    case "$1" in
        --prefix) PREFIX="${2:?--prefix needs a directory}"; shift 2 ;;
        --prefix=*) PREFIX="${1#--prefix=}"; shift ;;
        --own-whisper) OWN_WHISPER=1; shift ;;
        -y | --yes) ASSUME_YES=1; shift ;;
        --uninstall) ACTION=uninstall; shift ;;
        --purge) PURGE=1; shift ;;
        -h | --help) usage; exit 0 ;;
        *) printf 'unknown option: %s\n\n' "$1" >&2; usage >&2; exit 2 ;;
    esac
done

INTERACTIVE=0
if [ "$ASSUME_YES" = 0 ] && [ -t 0 ] && [ -t 1 ]; then INTERACTIVE=1; fi

NAME=Sotto SLUG=sotto TAGLINE="voice typing · whisper.cpp · Linux"
ACCENT_RGB="224;71;94" ACCENT_256=168

# ---- look ---------------------------------------------------------------------------
# Shared by the installers of Engram, PkgScope and Sotto; only the name, tagline,
# accent colour and the build progress line differ.

if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
    R=$'\e[0m' B=$'\e[1m' D=$'\e[2m' RED=$'\e[31m' GRN=$'\e[32m' YEL=$'\e[33m' CLR=$'\e[K'
    case "${COLORTERM:-}" in
        truecolor | 24bit) ACC=$'\e[38;2;'"${ACCENT_RGB}m" ;;
        *) ACC=$'\e[38;5;'"${ACCENT_256}m" ;;
    esac
else
    R='' B='' D='' RED='' GRN='' YEL='' CLR='' ACC=''
fi
case "${LC_ALL:-${LC_CTYPE:-${LANG:-}}}" in *UTF-8* | *utf8*) UTF=1 ;; *) UTF=0 ;; esac

step() { printf '\n%s%s▸ %s%s\n' "$B" "$ACC" "$1" "$R"; }
ok() { printf '  %s✓%s %s\n' "$GRN" "$R" "$*"; }
warn() { printf '  %s!%s %s\n' "$YEL" "$R" "$*"; }
info() { printf '    %s%s%s\n' "$D" "$*" "$R"; }
die() { printf '\n  %s✗ %s%s\n\n' "$RED" "$*" "$R" >&2; exit 1; }
tilde() { case "$1" in "$HOME"/*) printf '~/%s' "${1#"$HOME"/}" ;; "$HOME") printf '~' ;; *) printf '%s' "$1" ;; esac; }

# Closed input is never taken as an answer: stop before doing anything else.
read_answer() {
    if ! read -r REPLY; then
        printf '\n\n  %sNo answer (input closed) — stopping.%s\n\n' "$YEL" "$R" >&2
        exit 1
    fi
}

# ask_yn "question" y|n → exit status 0 for yes
ask_yn() {
    local q="$1" def="$2" hint ans
    if [ "$INTERACTIVE" = 0 ]; then [ "$def" = y ]; return; fi
    if [ "$def" = y ]; then hint="Y/n"; else hint="y/N"; fi
    printf '  %s?%s %s %s[%s]%s ' "$ACC" "$R" "$q" "$D" "$hint" "$R"
    read_answer
    ans="$(printf '%s' "$REPLY" | tr '[:upper:]' '[:lower:]')"
    case "$ans" in
        "") [ "$def" = y ] ;;
        y* | д*) return 0 ;;
        *) return 1 ;;
    esac
}

# ask_text "question" default → prints the answer
ask_text() {
    local q="$1" def="$2" ans
    if [ "$INTERACTIVE" = 0 ]; then printf '%s' "$def"; return; fi
    printf '  %s?%s %s %s[%s]%s ' "$ACC" "$R" "$q" "$D" "$(tilde "$def")" "$R" >&2
    read_answer
    ans="$REPLY"
    case "$ans" in "~"/*) ans="$HOME/${ans#"~/"}" ;; esac
    printf '%s' "${ans:-$def}"
}

# choose "question" option... → prints the chosen number (1-based)
choose() {
    local q="$1" i=1 ans
    shift
    printf '  %s?%s %s\n' "$ACC" "$R" "$q" >&2
    for opt in "$@"; do printf '    %s%d)%s %s\n' "$B" "$i" "$R" "$opt" >&2; i=$((i + 1)); done
    while true; do
        printf '    %s>%s ' "$ACC" "$R" >&2
        read_answer
        ans="${REPLY:-1}"
        if [ "$ans" -ge 1 ] 2>/dev/null && [ "$ans" -le $# ]; then printf '%s' "$ans"; return; fi
    done
}

banner() {
    local bar='▍' w=48 title="$NAME $1"
    [ "$UTF" = 1 ] || bar='|'
    printf '\n  %s╭%s╮%s\n' "$D" "$(printf '─%.0s' $(seq $w))" "$R"
    printf '  %s│%s  %s%s%s %s%s%s %s%s%s%*s%s│%s\n' "$D" "$R" "$ACC" "$bar" "$R" "$B" "$NAME" "$R" "$D" "$1" "$R" \
        $((w - 4 - ${#title})) "" "$D" "$R"
    printf '  %s│%s  %s%s%s %s%*s%s│%s\n' "$D" "$R" "$ACC" "$bar" "$R" "$TAGLINE" $((w - 4 - ${#TAGLINE})) "" "$D" "$R"
    printf '  %s╰%s╯%s\n' "$D" "$(printf '─%.0s' $(seq $w))" "$R"
}

# spin "label" command... — runs the command with a progress line; output goes to a log.
# progress LOG (defined per project) prints what the build is doing right now.
spin() {
    local label="$1" log status frames='⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏' i=0 start=$SECONDS now
    shift
    [ "$UTF" = 1 ] || frames='|/-\|/-\||'
    log="$(mktemp "${TMPDIR:-/tmp}/$SLUG-install.XXXXXX")"
    "$@" >"$log" 2>&1 &
    local pid=$!
    if [ -t 1 ]; then
        while kill -0 "$pid" 2>/dev/null; do
            now="$(progress "$log" 2>/dev/null || true)"
            printf '\r  %s%s%s %s %s%s%ss%s%s' "$ACC" "${frames:i++%10:1}" "$R" "$label" "$D" "${now:+$now · }" "$((SECONDS - start))" "$R" "$CLR"
            sleep 0.12
        done
        printf '\r%s' "$CLR"
    fi
    status=0
    wait "$pid" || status=$?
    if [ "$status" -ne 0 ]; then
        printf '  %s✗%s %s failed. Last lines of the log:\n\n' "$RED" "$R" "$label"
        tail -n 25 "$log" | sed 's/^/    /'
        printf '\n    Full log: %s\n\n' "$log"
        exit 1
    fi
    rm -f "$log"
    local t=$((SECONDS - start))
    ok "$label done in $((t / 60))m $((t % 60))s"
}

# progress for CMake + Ninja/Make builds: "[12/40]" or "[ 30%]"
cmake_progress() {
    grep -oE '^\[ *[0-9]+(/[0-9]+|%)\]' "$1" | tail -1 | tr -d '[] '
}

# Distribution family for package hints: arch, debian, fedora or other.
distro() {
    local id like
    id="$(. /etc/os-release 2>/dev/null && printf '%s' "${ID:-}")"
    like="$(. /etc/os-release 2>/dev/null && printf '%s' "${ID_LIKE:-}")"
    case " $id $like " in
        *" arch "*) printf arch ;;
        *" debian "* | *" ubuntu "*) printf debian ;;
        *" fedora "* | *" rhel "*) printf fedora ;;
        *) printf other ;;
    esac
}

# need_packages "what" arch-pkgs debian-pkgs fedora-pkgs → stops with the right install command
need_packages() {
    local what="$1" cmd
    case "$(distro)" in
        arch) cmd="sudo pacman -S --needed $2" ;;
        debian) cmd="sudo apt install $3" ;;
        fedora) cmd="sudo dnf install $4" ;;
        *) cmd="" ;;
    esac
    printf '\n  %s✗ %s%s\n' "$RED" "$what" "$R" >&2
    [ -n "$cmd" ] && printf '\n    Install them with:\n      %s%s%s\n' "$B" "$cmd" "$R" >&2
    printf '\n    Then run ./install.sh again.\n\n' >&2
    exit 1
}

progress() { cmake_progress "$1"; }

# ---- where things are -----------------------------------------------------------

SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
[ -f "$SRC_DIR/CMakeLists.txt" ] && [ -f "$SRC_DIR/src/app.cpp" ] || die "run install.sh from the Sotto source folder"
VERSION="$(sed -n 's/^project(sotto VERSION \([0-9.]*\).*/\1/p' "$SRC_DIR/CMakeLists.txt")"
BUILD_DIR="$SRC_DIR/build"
APP_ID=io.github.trendorin.Sotto
DATA="${XDG_DATA_HOME:-$HOME/.local/share}"
CONFIG="${XDG_CONFIG_HOME:-$HOME/.config}"

set_paths() {
    case "$PREFIX" in /*) ;; *) PREFIX="$PWD/$PREFIX" ;; esac
    PREFIX="${PREFIX%/}"
    BIN_DIR="$PREFIX/bin"
    PRIV="$PREFIX/lib/sotto"
    MANIFEST="$PREFIX/share/sotto/installed-files"
}
set_paths

installed_version() {
    if [ -x "$BIN_DIR/sotto" ]; then "$BIN_DIR/sotto" --version 2>/dev/null | awk '{print $2}'; fi
}

refresh_menus() {
    command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database -q "$PREFIX/share/applications" 2>/dev/null || true
    command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -qtf "$PREFIX/share/icons/hicolor" 2>/dev/null || true
}

stop_running() {
    pkill -x sotto 2>/dev/null || true
    pkill -f '/[S]otto[^/]*\.AppImage' 2>/dev/null || true
}

# ---- uninstall --------------------------------------------------------------------------

packaged() {
    if command -v pacman >/dev/null 2>&1 && pacman -Qq sotto >/dev/null 2>&1; then printf 'sudo pacman -R sotto'
    elif command -v dpkg-query >/dev/null 2>&1 && dpkg-query -W sotto >/dev/null 2>&1; then printf 'sudo apt remove sotto'
    elif command -v rpm >/dev/null 2>&1 && rpm -q sotto >/dev/null 2>&1; then printf 'sudo dnf remove sotto'
    fi
}

uninstall() {
    step "Uninstall"
    local f d pkg found=0
    pkg="$(packaged || true)"
    [ -f "$MANIFEST" ] && found=1
    grep -qs 'X-AppImage-Integrated=true' "$DATA/applications/$APP_ID.desktop" && found=1
    if [ "$found" = 0 ] && [ -z "$pkg" ]; then
        ok "Sotto is not installed in $(tilde "$PREFIX")"
        exit 0
    fi
    ask_yn "Remove Sotto?" y || { info "Nothing changed."; exit 0; }
    stop_running
    if [ -f "$MANIFEST" ]; then
        while IFS= read -r f; do
            [ -n "$f" ] || continue
            case "$f" in "$PREFIX"/*) ;; *) continue ;; esac
            if [ -e "$f" ] || [ -L "$f" ]; then rm -f "$f"; ok "removed $(tilde "$f")"; fi
        done <"$MANIFEST"
        rm -f "$MANIFEST"
        for d in "$PRIV/ggml" "$PRIV" "$PREFIX/share/sotto" "$PREFIX/share/licenses/sotto"; do
            [ -d "$d" ] && rmdir --ignore-fail-on-non-empty "$d" 2>/dev/null || true
        done
        refresh_menus
    fi
    # AppImage menu entry (the .AppImage file itself stays wherever you saved it)
    if grep -qs 'X-AppImage-Integrated=true' "$DATA/applications/$APP_ID.desktop"; then
        rm -f "$DATA/applications/$APP_ID.desktop" "$DATA/icons/hicolor/scalable/apps/$APP_ID.svg"
        ok "removed the AppImage menu entry"
    fi
    rm -f "$CONFIG/autostart/$APP_ID.desktop"
    if command -v qdbus6 >/dev/null 2>&1; then
        qdbus6 org.kde.kglobalaccel /kglobalaccel org.kde.KGlobalAccel.unregister "$APP_ID" dictate >/dev/null 2>&1 || true
    fi
    ok "autostart and shortcut"
    if [ -d "$DATA/sotto" ] || [ -d "$CONFIG/sotto" ]; then
        local size
        size="$(du -sh "$DATA/sotto" 2>/dev/null | cut -f1)"
        if [ "$PURGE" = 1 ] || { [ "$ASSUME_YES" = 0 ] && ask_yn "Also delete downloaded models (${size:-0}) and settings?" y; }; then
            rm -rf "$DATA/sotto" "$CONFIG/sotto"
            ok "removed models and settings"
        fi
    fi
    if [ -n "$pkg" ]; then
        warn "the Sotto package is still installed — remove it with: $pkg"
    else
        ok "Sotto is uninstalled."
    fi
}

# ---- checks --------------------------------------------------------------------------------

SYSTEM_WHISPER=0
VULKAN=OFF
check_system() {
    step "Checking your system"
    [ "$(uname -s)" = Linux ] || die "Sotto runs on Linux only"
    ok "Linux $(uname -m)"
    local missing=0 q="" cand
    command -v cmake >/dev/null 2>&1 || missing=1
    command -v c++ >/dev/null 2>&1 || command -v g++ >/dev/null 2>&1 || command -v clang++ >/dev/null 2>&1 || missing=1
    for cand in qmake6 /usr/lib/qt6/bin/qmake /usr/lib64/qt6/bin/qmake qmake-qt6 qmake; do
        if command -v "$cand" >/dev/null 2>&1 && "$cand" -query QT_VERSION 2>/dev/null | grep -q '^6\.'; then q="$cand"; break; fi
    done
    if [ -n "$q" ]; then
        [ -d "$("$q" -query QT_INSTALL_LIBS)/cmake/Qt6Multimedia" ] || missing=1
    else
        missing=1
    fi
    if [ "$OWN_WHISPER" = 0 ] && pkg-config --exists whisper 2>/dev/null; then SYSTEM_WHISPER=1; fi
    if [ "$SYSTEM_WHISPER" = 0 ]; then
        command -v git >/dev/null 2>&1 || missing=1
        command -v ninja >/dev/null 2>&1 || missing=1
    fi
    if [ "$missing" = 1 ]; then
        need_packages "CMake, a C++ compiler and Qt 6 with Multimedia are needed to build Sotto." \
            "base-devel cmake ninja qt6-base qt6-multimedia qt6-wayland layer-shell-qt whisper-cpp ggml-vulkan wl-clipboard" \
            "build-essential cmake ninja-build git qt6-base-dev qt6-multimedia-dev qt6-wayland-dev liblayershellqtinterface-dev libvulkan-dev glslc wl-clipboard" \
            "gcc-c++ cmake ninja-build git qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qtwayland-devel layer-shell-qt-devel vulkan-loader-devel glslc spirv-headers-devel wl-clipboard"
    fi
    ok "CMake $(cmake --version | awk 'NR==1{print $3}') · Qt $("$q" -query QT_VERSION)"
    if [ "$SYSTEM_WHISPER" = 1 ]; then
        ok "whisper.cpp $(pkg-config --modversion whisper) from the system"
    else
        if command -v glslc >/dev/null 2>&1 && [ -e /usr/include/vulkan/vulkan.h ]; then
            VULKAN=ON
            ok "whisper.cpp will be built for Sotto, with Vulkan"
        else
            ok "whisper.cpp will be built for Sotto, CPU only"
            info "for the GPU install the Vulkan headers and glslc, then run this again"
        fi
    fi
    [ -d "$("$q" -query QT_INSTALL_LIBS)/cmake/LayerShellQt" ] ||
        warn "layer-shell-qt not found — the indicator becomes a normal window on Wayland"
    command -v wl-copy >/dev/null 2>&1 || warn "wl-clipboard not found — pasting falls back to the Qt clipboard"
}

# ---- install --------------------------------------------------------------------------------

build() {
    step "Building"
    local args=(-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX")
    # Earlier versions built straight into build/.
    [ -f "$BUILD_DIR/CMakeCache.txt" ] && rm -rf "$BUILD_DIR"
    if [ "$SYSTEM_WHISPER" = 0 ]; then
        info "whisper.cpp takes a few minutes the first time"
        spin "whisper.cpp" env WHISPER_VULKAN="$VULKAN" "$SRC_DIR/packaging/build-whisper.sh" "$PRIV" "$BUILD_DIR/whisper"
        args+=(-DCMAKE_PREFIX_PATH="$BUILD_DIR/whisper$PRIV" -DCMAKE_INSTALL_RPATH="$PRIV" -DSOTTO_GGML_BACKEND_DIR="$PRIV/ggml")
    fi
    [ -f "$BUILD_DIR/sotto/CMakeCache.txt" ] && ! grep -q "^CMAKE_INSTALL_PREFIX:PATH=$PREFIX\$" "$BUILD_DIR/sotto/CMakeCache.txt" &&
        rm -rf "$BUILD_DIR/sotto"
    local gen=()
    command -v ninja >/dev/null 2>&1 && gen=(-G Ninja)
    spin "Configure" cmake -S "$SRC_DIR" -B "$BUILD_DIR/sotto" "${gen[@]}" "${args[@]}"
    spin "Release build" cmake --build "$BUILD_DIR/sotto" -j "$(nproc 2>/dev/null || echo 2)"
}

install_files() {
    step "Installing into $(tilde "$PREFIX")"
    mkdir -p "$BIN_DIR" "$PREFIX/share/sotto" || die "cannot create $BIN_DIR"
    [ -w "$BIN_DIR" ] || die "$(tilde "$BIN_DIR") is not writable — choose another location"
    local was_running=0 files=()
    pgrep -x sotto >/dev/null 2>&1 && was_running=1
    cmake --install "$BUILD_DIR/sotto" >/dev/null || die "cmake --install failed"
    mapfile -t files <"$BUILD_DIR/sotto/install_manifest.txt"
    if [ "$SYSTEM_WHISPER" = 0 ]; then
        local f
        mkdir -p "$PRIV/ggml"
        for f in "$BUILD_DIR/whisper$PRIV"/lib/lib*.so*; do cp -a "$f" "$PRIV/"; files+=("$PRIV/$(basename "$f")"); done
        for f in "$BUILD_DIR/whisper$PRIV"/ggml/*.so; do cp -a "$f" "$PRIV/ggml/"; files+=("$PRIV/ggml/$(basename "$f")"); done
        ok "whisper.cpp → $(tilde "$PRIV")"
    fi
    local desktop="$PREFIX/share/applications/$APP_ID.desktop"
    [ -f "$desktop" ] && sed -i "s|^Exec=sotto|Exec=$BIN_DIR/sotto|" "$desktop"
    files+=("$MANIFEST")
    printf '%s\n' "${files[@]}" >"$MANIFEST"
    refresh_menus
    ok "sotto → $(tilde "$BIN_DIR")"
    ok "menu entry and icon"
    ok "uninstall manifest"
    if [ "$was_running" = 1 ]; then
        stop_running
        sleep 1
        (setsid "$BIN_DIR/sotto" >/dev/null 2>&1 &)
        ok "restarted the running Sotto"
    fi
}

summary() {
    local on_path=1 s="sotto"
    case ":$PATH:" in *":$BIN_DIR:"*) ;; *) on_path=0 ;; esac
    [ "$on_path" = 1 ] || s="$(tilde "$BIN_DIR")/sotto"
    printf '\n  %s%s✓ Sotto %s is installed%s\n\n' "$B" "$GRN" "$VERSION" "$R"
    printf '    %-14s %s  %s(or "Sotto" in your app menu)%s\n' "Open" "$s" "$D" "$R"
    printf '    %-14s %s --toggle  %s(bind it to a key, or use the shortcut in the settings)%s\n' "Dictate" "$s" "$D" "$R"
    printf '    %-14s %s --settings\n' "Settings" "$s"
    printf '    %-14s %s --uninstall\n' "Uninstall" "$s"
    printf '\n    %sThe first start opens the settings: download a model there.%s\n' "$D" "$R"
    if [ "$on_path" = 0 ]; then
        printf '\n    %s%s is not on your PATH.%s Add it in your shell profile, e.g.\n' "$YEL" "$(tilde "$BIN_DIR")" "$R"
        printf '      export PATH="%s:$PATH"\n' "$BIN_DIR"
    fi
    printf '\n'
}

# ---- main --------------------------------------------------------------------------------------

banner "$VERSION"

if [ "$ACTION" = uninstall ]; then
    uninstall
    exit 0
fi

current="$(installed_version || true)"
if [ -n "$current" ] && [ "$INTERACTIVE" = 1 ]; then
    printf '\n  Sotto %s is installed in %s.\n' "$current" "$(tilde "$PREFIX")"
    pick="$(choose "What do you want to do?" "Update to $VERSION" "Uninstall" "Quit")"
    case "$pick" in
        2) uninstall; exit 0 ;;
        3) exit 0 ;;
    esac
fi

pkg="$(packaged || true)"
[ -z "$pkg" ] || warn "a Sotto package is installed too; the copy first on PATH wins (remove the package with: $pkg)"

if [ "$INTERACTIVE" = 1 ] && [ -z "$current" ]; then
    step "A few questions"
    PREFIX="$(ask_text "Install location" "$PREFIX")"
    set_paths
fi

check_system
build
install_files
summary
