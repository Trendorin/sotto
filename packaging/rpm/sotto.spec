%global whisper_version 1.9.5
%global sherpa_version 1.13.8
%global priv %{_libdir}/sotto
# The bundled whisper.cpp and sherpa-onnx are private: keep their libraries out of Provides/Requires.
%global __provides_exclude_from ^%{priv}/.*$
%global __requires_exclude ^lib(whisper|ggml|parakeet|sherpa-onnx|onnxruntime)
# Keep the run path to the private libraries (Fedora's %%cmake drops it by default).
%global _cmake_skip_rpath %{nil}
# ggml builds one CPU backend per instruction set; link-time optimisation only costs time here.
%global _lto_cflags %{nil}

Name:           sotto
Version:        2.0.0
Release:        1%{?dist}
Summary:        Voice typing for Linux: the text appears while you speak
License:        MIT
URL:            https://github.com/Trendorin/sotto
Source0:        %{url}/releases/download/v%{version}/%{name}-%{version}.tar.gz
Source1:        https://github.com/ggml-org/whisper.cpp/archive/v%{whisper_version}/whisper.cpp-%{whisper_version}.tar.gz
# Prebuilt by upstream with the ONNX Runtime it needs (not packaged in Fedora).
Source2:        https://github.com/k2-fsa/sherpa-onnx/releases/download/v%{sherpa_version}/sherpa-onnx-v%{sherpa_version}-linux-x64-shared.tar.bz2

BuildRequires:  cmake
BuildRequires:  ninja-build
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtmultimedia-devel
BuildRequires:  qt6-qtwayland-devel
BuildRequires:  layer-shell-qt-devel
BuildRequires:  vulkan-loader-devel
BuildRequires:  glslc
BuildRequires:  spirv-headers-devel
BuildRequires:  python3
Requires:       qt6-qtwayland
Requires:       qt6-qtsvg
Requires:       layer-shell-qt
Recommends:     wl-clipboard
Recommends:     xdg-desktop-portal
Suggests:       wtype
Suggests:       ydotool
Requires:       hicolor-icon-theme
Provides:       bundled(whisper-cpp) = %{whisper_version}
Provides:       bundled(ggml)
Provides:       bundled(sherpa-onnx) = %{sherpa_version}
Provides:       bundled(onnxruntime)

%description
Press a key and speak: Sotto recognises your speech locally and types each
phrase where the cursor is as soon as you pause. GigaAM v3 (the most accurate
open model for Russian) and Parakeet TDT v3 (25 languages) run fast on the
processor through sherpa-onnx; Whisper runs on the GPU with Vulkan. On Wayland
it gets its shortcut and pastes through the desktop portals, or through wtype on
Hyprland, niri and sway. Models are downloaded once and checked against their
published SHA-256 sums.

%prep
%autosetup
tar -xf %{SOURCE1}
tar -xf %{SOURCE2}

%build
packaging/build-whisper.sh %{priv} "$PWD/whisper-root" "$PWD/whisper.cpp-%{whisper_version}"
%cmake -G Ninja -DCMAKE_PREFIX_PATH="$PWD/whisper-root%{priv}" -DCMAKE_INSTALL_RPATH=%{priv} \
    -DSOTTO_GGML_BACKEND_DIR=%{priv}/ggml -DSOTTO_SHERPA_DIR="$PWD/sherpa-onnx-v%{sherpa_version}-linux-x64-shared" \
    -DSOTTO_REQUIRE_SHERPA=ON -DSOTTO_BUILD_TESTS=ON
%cmake_build

%check
%ctest

%install
%cmake_install
install -d %{buildroot}%{priv}/ggml
cp -a whisper-root%{priv}/lib/lib*.so* %{buildroot}%{priv}/
cp -a whisper-root%{priv}/ggml/*.so %{buildroot}%{priv}/ggml/
# Without the execute bit: find-debuginfo and brp-strip skip them. Their strip calls the prebuilt
# libonnxruntime.so ill-formed and the build stops on its missing build-id; ld.so does not need the bit.
install -m644 sherpa-onnx-v%{sherpa_version}-linux-x64-shared/lib/libsherpa-onnx-c-api.so \
    sherpa-onnx-v%{sherpa_version}-linux-x64-shared/lib/libonnxruntime.so %{buildroot}%{priv}/
rm -rf %{buildroot}%{_datadir}/licenses

%files
%license LICENSE
%doc README.md CHANGELOG.md
%{_bindir}/sotto
%{priv}/
%{_datadir}/applications/io.github.trendorin.Sotto.desktop
%{_datadir}/metainfo/io.github.trendorin.Sotto.metainfo.xml
%{_datadir}/icons/hicolor/scalable/apps/io.github.trendorin.Sotto.svg

%changelog
* Thu Oct 22 2026 Trendorin <193045225+Trendorin@users.noreply.github.com> - 2.0.0-1
- GigaAM v3 and Parakeet TDT v3 through sherpa-onnx, next to Whisper
- Live typing: every phrase appears as soon as you pause
- niri, Hyprland, sway, COSMIC: key binding help, pasting with wtype or ydotool, notifications without a tray

* Wed Oct 21 2026 Trendorin <193045225+Trendorin@users.noreply.github.com> - 1.2.5-1
- install.sh installs missing packages and touches only its own prefix

* Wed Oct 21 2026 Trendorin <193045225+Trendorin@users.noreply.github.com> - 1.2.4-1
- Repair the menu and autostart entries left by a deleted AppImage

* Wed Oct 21 2026 Trendorin <193045225+Trendorin@users.noreply.github.com> - 1.2.3-1
- Choice of GPU and a hint when a graphics card is not usable
- GNOME: the recording indicator no longer takes the focus

* Wed Oct 21 2026 Trendorin <193045225+Trendorin@users.noreply.github.com> - 1.2.2-1
- Use the discrete GPU on laptops that also have an integrated one

* Tue Oct 20 2026 Trendorin <193045225+Trendorin@users.noreply.github.com> - 1.2.1-1
- AppImage fix: Qt Wayland shell plugins

* Tue Oct 20 2026 Trendorin <193045225+Trendorin@users.noreply.github.com> - 1.2.0-1
- Interface language setting

* Tue Oct 20 2026 Trendorin <193045225+Trendorin@users.noreply.github.com> - 1.1.0-1
- Download log and custom model server, security fixes, smooth dragging of the indicator

* Tue Oct 20 2026 Trendorin <193045225+Trendorin@users.noreply.github.com> - 1.0.0-1
- First stable release
