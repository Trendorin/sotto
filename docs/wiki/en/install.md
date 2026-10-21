# Installation

## Packages

The [latest release](https://github.com/Trendorin/sotto/releases/latest) has a package for
Arch Linux, Debian 13+ and Fedora 44+, and an AppImage for any distribution with glibc 2.35+.

```sh
sudo pacman -U ./sotto-*-x86_64.pkg.tar.zst     # Arch, CachyOS, Manjaro
sudo apt install ./sotto_*_amd64.deb             # Debian, Ubuntu
sudo dnf install ./sotto-*.x86_64.rpm            # Fedora
```

The Arch package uses the distribution's `whisper-cpp` and `ggml-vulkan`. The Debian and Fedora
packages carry their own whisper.cpp 1.9.5 with Vulkan in `/usr/lib/sotto`. The AppImage carries
Qt and whisper.cpp; for the GPU it needs the system's Vulkan loader. On first start it adds itself
to the menu, because the portals grant a hotkey only to applications they know.

Check a download against the release's `SHA256SUMS`:

```sh
sha256sum --ignore-missing -c SHA256SUMS
```

## From source

`install.sh` builds the checkout with CMake and installs into `~/.local`. Sotto itself goes in
without `sudo`; the installer asks for it only to install missing distribution packages. It needs
Qt 6.5 or newer (Debian 13, Ubuntu 24.10, Fedora 39 and later); on older systems use the AppImage.

| distribution | packages the installer offers to install |
|---|---|
| Arch | `base-devel cmake ninja qt6-base qt6-multimedia whisper-cpp ggml-vulkan qt6-wayland layer-shell-qt wl-clipboard vulkan-headers shaderc` |
| Debian, Ubuntu | `build-essential pkg-config cmake ninja-build git ca-certificates qt6-base-dev qt6-multimedia-dev qt6-wayland-dev qt6-qpa-plugins liblayershellqtinterface-dev libvulkan-dev glslc spirv-headers wl-clipboard` |
| Fedora | `gcc-c++ pkgconf-pkg-config cmake ninja-build git qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qtwayland-devel layer-shell-qt-devel vulkan-loader-devel glslc spirv-headers-devel wl-clipboard` |
| openSUSE | `gcc-c++ pkgconf-pkg-config cmake ninja git ca-certificates-mozilla qt6-base-devel qt6-multimedia-devel qt6-wayland-devel layer-shell-qt6-devel vulkan-devel vulkan-headers shaderc spirv-headers wl-clipboard` |

```sh
./install.sh                  # interactive: asks where to install
./install.sh --yes            # no questions, defaults
./install.sh --prefix /opt/sotto
./install.sh --own-whisper    # build the pinned whisper.cpp even if the system has one
./install.sh --no-packages    # never install packages, only name the missing ones
```

Without a system whisper.cpp the installer builds the pinned version into
`<prefix>/lib/sotto`, used by Sotto only. It takes a few minutes the first time; with the Vulkan
headers and `glslc` it includes the GPU backend, otherwise it is CPU only.

If something is missing, the installer names it and offers to install it (the default answer is
yes, also with `--yes`). Without a terminal and without password-less `sudo` it prints the command
instead. The build tools are required; without the rest Sotto still works, but the indicator is a
normal window on Wayland, whisper.cpp runs on the CPU and pasting goes through the Qt clipboard.

A running Sotto is restarted after an update.

### Your own AppImage

```sh
packaging/appimage/build.sh     # rootless, no docker or podman
```

## Updating

Pull the new version and run `./install.sh` again. It sees the installed version and offers to
update it in place.

## Removal

From the app: **Settings → Remove Sotto…**, or `sotto --uninstall`. From the source folder:

```sh
./install.sh --uninstall           # removes exactly the files it installed
./install.sh --uninstall --purge   # also models and settings, without asking
```

Both also remove the menu entry, autostart and the KDE shortcut. Packages are removed with your
package manager; the AppImage file stays wherever you saved it.

## Files

| what | where |
|---|---|
| settings | `~/.config/sotto/` |
| models | `~/.local/share/sotto/models/` |
| autostart, if enabled | `~/.config/autostart/io.github.trendorin.Sotto.desktop` |
| installed files (source install) | `<prefix>/share/sotto/installed-files` |
