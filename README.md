<div align="center">

<img src=".github/assets/logo.svg" width="88" alt="">

# Sotto

Voice typing for Linux, local and fast.

[![Release](https://img.shields.io/badge/release-v2.0.0-e0475e)](https://github.com/Trendorin/sotto/releases/latest)
[![License](https://img.shields.io/badge/license-MIT-e0475e)](LICENSE)
![sherpa-onnx](https://img.shields.io/badge/sherpa--onnx-1.13.8-e0475e)
![whisper.cpp](https://img.shields.io/badge/whisper.cpp-1.9.5-e0475e)
![Platform](https://img.shields.io/badge/platform-Linux-e0475e)

[Download](https://github.com/Trendorin/sotto/releases/latest) · [Install](#install) · [Wiki](docs/wiki/en/README.md) · [Changelog](CHANGELOG.md) · [Русский](README.ru.md)

</div>

<p align="center"><img src=".github/assets/screenshot.png" width="620" alt="Sotto settings: GigaAM v3 downloaded and ready, its source and the download log"></p>

Press a key, speak, and the text appears where the cursor is — phrase by phrase, while you are
still talking. Sotto (as in *sotto voce*) lives in the tray, listens only while you hold or toggle
its key, recognises speech on your computer and types the result into the active window.

## Features

- **Live typing**: every phrase is recognised and typed as soon as you pause, not after the
  recording. Or, if you prefer, everything at once when you stop.
- **Modern models**: GigaAM v3, the most accurate open model for Russian, and Parakeet TDT v3 for
  25 European languages, both with punctuation and fast on an ordinary processor (sherpa-onnx).
  Whisper for 99 languages, on the GPU through Vulkan (whisper.cpp).
- Tap the key to start and stop, or hold it while you speak.
- Recognition runs locally. Audio stays in memory and is never written to disk or sent anywhere.
- Made for Wayland: on KDE, GNOME and COSMIC the hotkey and pasting go through the desktop portals,
  with no root. On **niri, Hyprland, sway** and other tiling compositors Sotto shows the binding
  to add and pastes with `wtype`. X11 works too.
- Silero VAD cuts the speech into phrases, so recognition never sees silence and Whisper does not
  invent text on a quiet recording.
- Models are downloaded from Hugging Face, a mirror or your own server, with a visible download log,
  and every file is kept only if its size and SHA-256 match.
- C++ and Qt 6 widgets in your system theme. A few megabytes when idle; the model is unloaded
  after a while without use.
- English and Russian interface, picked from the system language.

## Install

| your system | file from the [latest release](https://github.com/Trendorin/sotto/releases/latest) |
|---|---|
| Arch Linux, CachyOS, Manjaro | `sotto-2.0.0-1-x86_64.pkg.tar.zst` |
| Debian 13 and newer | `sotto_2.0.0-1_amd64.deb` |
| Fedora 44 and newer | `sotto-2.0.0-1.fc44.x86_64.rpm` |
| any distribution with glibc 2.35+ | `Sotto-2.0.0-x86_64.AppImage` |

```sh
sudo pacman -U ./sotto-2.0.0-1-x86_64.pkg.tar.zst    # Arch
sudo apt install ./sotto_2.0.0-1_amd64.deb            # Debian
sudo dnf install ./sotto-2.0.0-1.fc44.x86_64.rpm      # Fedora
chmod +x Sotto-2.0.0-x86_64.AppImage && ./Sotto-2.0.0-x86_64.AppImage
```

Every package is built in a clean container of its distribution, then installed and started
there before it is published. Check the files with `sha256sum --ignore-missing -c SHA256SUMS`.
The AppImage carries Qt, whisper.cpp with Vulkan and CPU builds for any processor, and sherpa-onnx;
on first start it adds itself to the menu, which the portals need to grant a hotkey.

### From source

`install.sh` checks for CMake, a C++ compiler and Qt 6.5+ with Multimedia, and offers to install
whatever is missing with pacman, apt, dnf or zypper. If your distribution has whisper.cpp
(Arch: `whisper-cpp`), Sotto uses it; otherwise the installer builds the pinned version for Sotto
alone, with Vulkan. sherpa-onnx comes from its release page, checked against a pinned SHA-256.

```sh
git clone https://github.com/Trendorin/sotto && cd sotto
./install.sh                 # asks a few questions, builds, installs into ~/.local
./install.sh --uninstall     # removes exactly what it installed
```

`./install.sh --help` lists the options (`--yes`, `--prefix`, `--own-whisper`, `--no-sherpa`,
`--no-packages`).

## Getting started

1. Start **Sotto** from the menu. The first time, the settings open: download a model
   (**GigaAM v3** for Russian, **Parakeet TDT v3** for other European languages, **Whisper Large
   v3 Turbo** with a GPU).
2. The default key is **Meta+Space**; change it with **Change…** in the settings. On niri,
   Hyprland or sway the settings show the line to add to your config instead.
3. Speak. Each phrase is typed into the active window as soon as you pause.

On KDE, GNOME and COSMIC the first paste asks for "remote control" permission: that is the portal
Sotto uses to press Ctrl+V. Tick **Remember**; without it the text stays in the clipboard. Drag the
indicator to move it, click it to stop, right-click to cancel.

```sh
sotto --toggle    # start or stop; bind it to any key in your desktop or compositor
sotto --start | --stop | --cancel | --settings | --uninstall | --quit
sotto --keybind   # the binding to add on this desktop
sotto --transcribe recording.wav    # the text of a recording
```

## Models

| model | engine | size | for |
|---|---|---|---|
| **GigaAM v3** | sherpa-onnx | 221 MB | Russian: the most accurate, with punctuation, fast on a CPU |
| **Parakeet TDT v3** | sherpa-onnx | 639 MB | 25 European languages, punctuation, fast on a CPU |
| **Whisper Large v3 Turbo** | whisper.cpp | 547 MB | 99 languages, accurate and fast on a GPU |
| Whisper Large v3 Turbo Q8 | whisper.cpp | 834 MB | slightly more accurate, more memory |
| Whisper Large v3 | whisper.cpp | 1031 MB | the most accurate Whisper, slow |
| Whisper Medium / Small / Base | whisper.cpp | 514 / 181 / 57 MB | smaller and faster, less accurate |

GigaAM v3 ([Sber](https://github.com/salute-developers/GigaAM), MIT) and Parakeet TDT v3
([NVIDIA](https://huggingface.co/nvidia/parakeet-tdt-0.6b-v3), CC-BY-4.0) come in the sherpa-onnx
format from [huggingface.co/csukuangfj](https://huggingface.co/csukuangfj), Whisper from
[huggingface.co/ggerganov/whisper.cpp](https://huggingface.co/ggerganov/whisper.cpp) and Silero VAD
from [huggingface.co/ggml-org/whisper-vad](https://huggingface.co/ggml-org/whisper-vad) (MIT). If
Hugging Face is unreachable, switch **Source** to the [hf-mirror.com](https://hf-mirror.com) mirror,
or to your own server (**Custom server…**). The settings show a download log with the exact URL
and where the file really comes from. You can also put your own `ggml-*.bin` or sherpa-onnx model
folder into `~/.local/share/sotto/models/`.

## How it works

<picture><source media="(prefers-color-scheme: dark)" srcset=".github/assets/flow-dark.svg"><img src=".github/assets/flow-light.svg" width="100%" alt="Hotkey, recording in memory, Silero VAD cuts phrases at the pauses, GigaAM, Parakeet or Whisper recognises each one, the text is typed while you speak"></picture>

Audio is recorded into memory at 16 kHz. Silero VAD rates every 32 ms and ends a phrase at a pause;
the phrase goes to the recogniser on its own thread while the microphone keeps listening, and its
text is typed as soon as it is ready. GigaAM and Parakeet run on ONNX Runtime (sherpa-onnx),
Whisper on the GPU (Vulkan) or the best CPU variant. The model starts loading while you are still
talking.

## Privacy

Your voice never leaves the computer and is never written to disk. The only network access is
the model download you start yourself. Settings live in `~/.config/sotto/`, models in
`~/.local/share/sotto/models/`.

To remove Sotto, use **Settings → Remove Sotto…** or `sotto --uninstall`: it removes the program,
the menu entry, autostart and the hotkey, and optionally the models and settings.

## Documentation

| | |
|---|---|
| [Installation](docs/wiki/en/install.md) | packages, the source installer, updating, removal |
| [Using Sotto](docs/wiki/en/usage.md) | the key, live typing, pasting, the indicator, command line |
| [Models](docs/wiki/en/models.md) | GigaAM, Parakeet or Whisper, sources, checksums, your own files |
| [Tiling compositors](docs/wiki/en/desktops.md) | niri, Hyprland, sway, river, COSMIC: key bindings and pasting |
| [Troubleshooting](docs/wiki/en/troubleshooting.md) | logs and common problems |

[Contributing](CONTRIBUTING.md) · [Security policy](SECURITY.md) · [Changelog](CHANGELOG.md)

<div align="center"><sub>MIT · <a href="https://github.com/Trendorin">Trendorin</a></sub></div>
