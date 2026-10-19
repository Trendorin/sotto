#!/bin/bash
# Builds the AppImage on Ubuntu 22.04, as root. build.sh runs it inside a rootless
# Ubuntu 22.04 root (/src = the source tree, /work = cache); the release workflow runs
# it directly on GitHub's ubuntu-22.04 runner with SOTTO_SRC and SOTTO_WORK set.
# Finished steps are cached in the work folder.
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive

QT_VERSION=6.8.3
ECM_TAG=v6.14.0
LAYERSHELL_TAG=v6.4.5
WHISPER_TAG=v1.9.5
VULKAN_SDK=1.4.363.0

SRC=${SOTTO_SRC:-/src}
W=${SOTTO_WORK:-/work}
mkdir -p "$W"
QTDIR=/opt/qt/$QT_VERSION/gcc_64
JOBS=$(nproc)
export CC=gcc-12 CXX=g++-12

step() {
    local name=$1
    shift
    [[ -f $W/.done-$name ]] && return
    echo "==> $name"
    "$@"
    touch "$W/.done-$name"
}

fetch() { # repo tag dir
    rm -rf "$3"
    git clone --quiet --depth 1 --branch "$2" "$1" "$3"
}

deps() {
    local preload=
    if [[ -f /usr/local/lib/fakechown.so ]]; then # the rootless build root (see build.sh)
        echo 'APT::Sandbox::User "root";' >/etc/apt/apt.conf.d/99sandbox
        preload=/usr/local/lib/fakechown.so
    fi
    apt-get update -q
    LD_PRELOAD=$preload apt-get install -yq --no-install-recommends \
        build-essential gcc-12 g++-12 git curl ca-certificates python3-pip pkg-config patchelf file \
        libgl-dev libegl-dev libxkbcommon-dev libxkbcommon-x11-0 libfontconfig1 libfreetype6 libdbus-1-3 libglib2.0-0 \
        libwayland-dev libwayland-bin wayland-protocols \
        libpulse0 libx11-xcb1 libxcb-cursor0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 \
        libxcb-render-util0 libxcb-shape0 libxcb-xinerama0 libxcb-xkb1 libxcb-xfixes0 libxrender1 libsm6 libice6 \
        libxrandr2 libxext6 libxfixes3 libxi6 libxtst6 libxcomposite1 libxdamage1 libdrm2 \
        libva2 libva-drm2 libva-x11-2 libvdpau1 \
        libvulkan-dev xz-utils
    pip3 install --quiet --no-cache-dir aqtinstall 'cmake>=3.28' ninja
}

qt() {
    aqt install-qt linux desktop "$QT_VERSION" linux_gcc_64 -m qtmultimedia -O /opt/qt
}

kde() {
    fetch https://invent.kde.org/frameworks/extra-cmake-modules.git "$ECM_TAG" $W/ecm
    cmake -S $W/ecm -B $W/ecm/build -G Ninja -DCMAKE_INSTALL_PREFIX=/opt/kde -DBUILD_TESTING=OFF \
        -DBUILD_HTML_DOCS=OFF -DBUILD_MAN_DOCS=OFF -DBUILD_QTHELP_DOCS=OFF
    cmake --install $W/ecm/build

    fetch https://invent.kde.org/plasma/layer-shell-qt.git "$LAYERSHELL_TAG" $W/layer-shell-qt
    cmake -S $W/layer-shell-qt -B $W/layer-shell-qt/build -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/kde -DKDE_INSTALL_LIBDIR=lib -DBUILD_TESTING=OFF \
        -DCMAKE_PREFIX_PATH="$QTDIR;/opt/kde"
    cmake --build $W/layer-shell-qt/build -j"$JOBS"
    cmake --install $W/layer-shell-qt/build
}

vulkan() {
    # Ubuntu 22.04 has no glslc; LunarG's SDK ships one built for old glibc, plus current headers.
    curl -fL --retry 3 -o /tmp/vulkansdk.tar.xz \
        "https://sdk.lunarg.com/sdk/download/$VULKAN_SDK/linux/vulkansdk-linux-x86_64-$VULKAN_SDK.tar.xz"
    rm -rf /opt/vulkan && mkdir -p /opt/vulkan
    tar -xJf /tmp/vulkansdk.tar.xz -C /opt/vulkan --strip-components=2 --wildcards \
        "$VULKAN_SDK/x86_64/bin/glslc" "$VULKAN_SDK/x86_64/include/vulkan/*" "$VULKAN_SDK/x86_64/include/vk_video/*"
    rm /tmp/vulkansdk.tar.xz
    /opt/vulkan/bin/glslc --version

    fetch https://github.com/KhronosGroup/SPIRV-Headers.git "vulkan-sdk-$VULKAN_SDK" $W/spirv-headers
    cmake -S $W/spirv-headers -B $W/spirv-headers/build -DCMAKE_INSTALL_PREFIX=/opt/vulkan -DSPIRV_HEADERS_ENABLE_TESTS=OFF
    cmake --install $W/spirv-headers/build
}

whisper() {
    fetch https://github.com/ggml-org/whisper.cpp.git "$WHISPER_TAG" $W/whisper.cpp
    # Every CPU flavour is built and the best one is picked at runtime; Vulkan is loaded
    # only if the system has a Vulkan driver.
    cmake -S $W/whisper.cpp -B $W/whisper.cpp/build -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/whisper -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_PREFIX_PATH=/opt/vulkan \
        -DBUILD_SHARED_LIBS=ON -DGGML_NATIVE=OFF -DGGML_BACKEND_DL=ON -DGGML_CPU_ALL_VARIANTS=ON \
        -DGGML_OPENMP=OFF -DGGML_VULKAN=ON -DGGML_BACKEND_DIR=/opt/whisper/lib/ggml \
        -DVulkan_INCLUDE_DIR=/opt/vulkan/include -DVulkan_GLSLC_EXECUTABLE=/opt/vulkan/bin/glslc \
        -DWHISPER_BUILD_EXAMPLES=OFF -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_SERVER=OFF -DWHISPER_FFMPEG=OFF
    cmake --build $W/whisper.cpp/build -j"$JOBS"
    cmake --install $W/whisper.cpp/build
}

tools() {
    mkdir -p $W/tools
    for t in linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage \
             linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage; do
        curl -fsSL --retry 3 -o "$W/tools/${t##*/}" "https://github.com/$t"
        chmod +x "$W/tools/${t##*/}"
    done
}

step deps deps
step qt qt
step kde kde
step vulkan vulkan
step whisper whisper
step tools tools

echo "==> sotto"
VERSION=$(sed -n 's/^project(sotto VERSION \([0-9.]*\).*/\1/p' "$SRC/CMakeLists.txt")
rm -rf $W/sotto-build $W/AppDir
cmake -S "$SRC" -B $W/sotto-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_PREFIX_PATH="$QTDIR;/opt/kde;/opt/whisper"
cmake --build $W/sotto-build -j"$JOBS"
DESTDIR=$W/AppDir cmake --install $W/sotto-build

echo "==> AppDir"
A=$W/AppDir
mkdir -p $A/usr/lib/ggml $A/usr/plugins/wayland-shell-integration
cp /opt/whisper/lib/ggml/*.so $A/usr/lib/ggml/
cp "$(find /opt/kde -name liblayer-shell.so | head -1)" $A/usr/plugins/wayland-shell-integration/

export APPIMAGE_EXTRACT_AND_RUN=1 NO_STRIP=0
export QMAKE=$QTDIR/bin/qmake
export LD_LIBRARY_PATH=$QTDIR/lib:/opt/kde/lib:/opt/whisper/lib
export EXTRA_PLATFORM_PLUGINS="libqwayland-generic.so"
# Audio goes through PulseAudio/PipeWire inside QtMultimedia; the media playback backends aren't needed.
export EXCLUDE_LIBRARIES="libgstreamermediaplugin.so"

cd $W
$W/tools/linuxdeploy-x86_64.AppImage --appdir $A \
    --executable $A/usr/bin/sotto \
    --desktop-file $A/usr/share/applications/io.github.trendorin.Sotto.desktop \
    --icon-file $A/usr/share/icons/hicolor/scalable/apps/io.github.trendorin.Sotto.svg \
    --deploy-deps-only $A/usr/lib/ggml \
    --deploy-deps-only $A/usr/plugins/wayland-shell-integration/liblayer-shell.so \
    --plugin qt

# Keep loading libraries relative to their own location.
for f in $A/usr/lib/ggml/*.so; do patchelf --set-rpath '$ORIGIN/..' "$f"; done
patchelf --set-rpath '$ORIGIN/../../lib' $A/usr/plugins/wayland-shell-integration/liblayer-shell.so

# Must come from the host: GPU drivers load into our process and need the host's C++ runtime.
rm -f $A/usr/lib/libstdc++.so* $A/usr/lib/libgcc_s.so* $A/usr/lib/libvulkan.so* \
      $A/usr/lib/libGL*.so* $A/usr/lib/libEGL*.so*

echo "==> AppImage"
mkdir -p "$SRC/dist"
export LDAI_OUTPUT=$SRC/dist/Sotto-$VERSION-x86_64.AppImage LDAI_COMP=zstd
rm -f "$LDAI_OUTPUT"
$W/tools/linuxdeploy-x86_64.AppImage --appdir $A --output appimage
