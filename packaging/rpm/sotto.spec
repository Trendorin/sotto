%global whisper_version 1.9.5
%global priv %{_libdir}/sotto
# The bundled whisper.cpp is private: keep its libraries out of Provides/Requires.
%global __provides_exclude_from ^%{priv}/.*$
%global __requires_exclude ^lib(whisper|ggml|parakeet)
# Keep the run path to the private libraries (Fedora's %%cmake drops it by default).
%global _cmake_skip_rpath %{nil}
# ggml builds one CPU backend per instruction set; link-time optimisation only costs time here.
%global _lto_cflags %{nil}

Name:           sotto
Version:        1.1.0
Release:        1%{?dist}
Summary:        Voice typing for Linux with whisper.cpp
License:        MIT
URL:            https://github.com/Trendorin/sotto
Source0:        %{url}/releases/download/v%{version}/%{name}-%{version}.tar.gz
Source1:        https://github.com/ggml-org/whisper.cpp/archive/v%{whisper_version}/whisper.cpp-%{whisper_version}.tar.gz

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
Requires:       qt6-qtwayland
Requires:       layer-shell-qt
Recommends:     wl-clipboard
Recommends:     xdg-desktop-portal
Requires:       hicolor-icon-theme
Provides:       bundled(whisper-cpp) = %{whisper_version}
Provides:       bundled(ggml)

%description
Press a key, speak, release it: Sotto transcribes your speech locally with
whisper.cpp and pastes the text where the cursor is. It uses Vulkan when a GPU
is available, cuts silence with Silero VAD, and on Wayland gets its shortcut and
pastes through the desktop portals. Models are downloaded once and checked
against their published SHA-256 sums.

%prep
%autosetup
tar -xf %{SOURCE1}

%build
packaging/build-whisper.sh %{priv} "$PWD/whisper-root" "$PWD/whisper.cpp-%{whisper_version}"
%cmake -G Ninja -DCMAKE_PREFIX_PATH="$PWD/whisper-root%{priv}" -DCMAKE_INSTALL_RPATH=%{priv} \
    -DSOTTO_GGML_BACKEND_DIR=%{priv}/ggml
%cmake_build

%install
%cmake_install
install -d %{buildroot}%{priv}/ggml
cp -a whisper-root%{priv}/lib/lib*.so* %{buildroot}%{priv}/
cp -a whisper-root%{priv}/ggml/*.so %{buildroot}%{priv}/ggml/
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
* Tue Oct 20 2026 Trendorin <193045225+Trendorin@users.noreply.github.com> - 1.1.0-1
- Download log and custom model server, security fixes, smooth dragging of the indicator

* Tue Oct 20 2026 Trendorin <193045225+Trendorin@users.noreply.github.com> - 1.0.0-1
- First stable release
