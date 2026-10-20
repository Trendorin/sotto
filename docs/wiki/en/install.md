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

`install.sh` builds the checkout with CMake and installs into `~/.local`. It never uses `sudo`.

| distribution | build dependencies |
|---|---|
| Arch | `base-devel cmake ninja qt6-base qt6-multimedia qt6-wayland layer-shell-qt whisper-cpp ggml-vulkan wl-clipboard` |
| Debian, Ubuntu | `build-essential cmake ninja-build git qt6-base-dev qt6-multimedia-dev qt6-wayland-dev liblayershellqtinterface-dev libvulkan-dev glslc wl-clipboard` |
| Fedora | `gcc-c++ cmake ninja-build git qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qtwayland-devel layer-shell-qt-devel vulkan-loader-devel glslc spirv-headers-devel wl-clipboard` |

```sh
./install.sh                  # interactive: asks where to install
./install.sh --yes            # no questions, defaults
./install.sh --prefix /opt/sotto
./install.sh --own-whisper    # build the pinned whisper.cpp even if the system has one
```

Without a system whisper.cpp the installer builds the pinned version into
`<prefix>/lib/sotto`, used by Sotto only. It takes a few minutes the first time; with the Vulkan
headers and `glslc` it includes the GPU backend, otherwise it is CPU only. If something else is
missing, the installer stops and prints the command that installs it on your distribution.

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
