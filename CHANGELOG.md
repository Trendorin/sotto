# Changelog

All notable changes to Sotto. The format follows [Keep a Changelog](https://keepachangelog.com/),
and versions follow [Semantic Versioning](https://semver.org/).

## [Unreleased]

## [2.0.0] — 2026-10-22

### Added
- **New recognition engine: sherpa-onnx**, next to whisper.cpp, with two models that are more
  accurate than Whisper and fast on an ordinary processor:
  - **GigaAM v3** (Sber, MIT): the most accurate open model for Russian, with punctuation and
    capital letters. 221 MB; an 11-second phrase takes about a tenth of its length to recognise
    on two CPU cores.
  - **Parakeet TDT v3** (NVIDIA, CC-BY-4.0): 25 European languages including Russian, Ukrainian
    and English, finds the language itself, punctuation and capitals. 639 MB.
  Both are downloaded file by file from Hugging Face, the mirror or your own server and kept only
  if every file matches its published size and SHA-256, like the Whisper models. Whisper stays
  for its 99 languages and the GPU.
- **Live typing**: the recording is cut into phrases at your pauses (Silero VAD), each phrase is
  recognised while you go on talking and typed as soon as it is ready, so the text appears while
  you speak instead of after the recording. **Pause that ends a phrase** sets how long a pause is
  (600 ms by default). Turning **Type each phrase as soon as I pause** off pastes everything at
  the end, still recognised phrase by phrase, so even then only the last phrase is left to wait for.
- **Tiling compositors** (niri, Hyprland, sway, river, Wayfire, labwc) and **COSMIC**:
  - where no portal gives Sotto a key, the settings and `sotto --keybind` show the exact lines for
    the compositor's config (`config.kdl`, `hyprland.conf`, ...), with the path to Sotto filled in
    and a hold-to-talk variant;
  - `sotto --toggle` bound in the compositor hands the command to the running Sotto in about a
    millisecond, before Qt even starts;
  - pasting works without the RemoteDesktop portal: through `wtype` (Hyprland, niri, sway, ...),
    `ydotool` (any desktop) or `xdotool` (X11), chosen automatically or in **Press keys with**,
    falling back from one to the next;
  - without a tray, messages go to the notification daemon.
- `sotto --transcribe file.wav [--model ID]` prints the text of a recording (the same phrase
  pipeline as a dictation), `sotto --models` lists the models.
- Tests: phrase detection and a check that every interface string has its Russian translation
  (`ctest`, in CI and in the Arch, Debian and Fedora package builds).

### Changed
- The silence filter is no longer an option: Silero VAD now finds the phrases, and recognition
  only ever sees speech. Without the VAD model (an old install that has not fetched it yet) Sotto
  falls back to loudness.
- The source tree follows the other Trendorin projects: `src/core` (audio, engines, models),
  `src/system` (portals, pasting, desktop detection), `src/gui`; desktop files in `packaging/`,
  resources in `resources/`, tests in `tests/`, `.editorconfig`.
- Packages need the Qt SVG plugin (the icons are SVG): `qt6-svg`, `qt6-svg-plugins`, `qt6-qtsvg`.
- Recognition runs at a lower priority (and its compute threads with it) and leaves a core free, so
  the microphone and the desktop stay responsive while a phrase is recognised; the model is loaded
  when Sotto starts, not during the first dictation.
- Phrases after a pause inside a sentence no longer start with a capital letter; after a full stop
  they always do.
- A phrase that cannot be recognised stops the dictation with the reason instead of failing on
  every following phrase.
- The interface strings around the old single-pass recognition are gone: the transcription
  percentage, the silence filter switch, `ModelDownloader::current()` and `Recorder::isActive()`
  (unused).
- Settings: with GigaAM or Parakeet the Whisper-only rows (language, GPU, vocabulary) are hidden,
  and the download log takes the free room.

### Fixed
- Silero VAD gave different results for the same recording: a new VAD context does not start
  from a zero state in whisper.cpp; Sotto now resets it.
- Without ggml's CPU backend (a broken whisper.cpp install) Sotto stopped with an assertion; now
  Whisper reports it and phrases fall back to loudness detection.
- The Cancel button of the settings was not translated into Russian.

## [1.2.5] — 2026-10-21

### Added
- `install.sh` offers to install missing build tools and libraries with pacman, apt, dnf or zypper
  (openSUSE is new) instead of stopping with a command to copy, and then the optional parts too:
  layer-shell-qt, the Vulkan headers and `glslc`, `wl-clipboard`. `--no-packages` turns that off.
  It now says up front when Qt is older than 6.5 and points to the AppImage.
- CI installs Sotto with `install.sh` on bare Arch, Debian 13, Ubuntu 26.04, Fedora 44 and
  openSUSE Tumbleweed images, runs it and uninstalls it.

### Fixed
- `install.sh --prefix DIR` and `install.sh --uninstall --prefix DIR` stopped or restarted the
  Sotto running from another place, and the uninstall also removed its autostart entry and KDE
  shortcut. Now only the Sotto of that prefix is stopped, and autostart, shortcut, models and
  settings stay as long as another Sotto is installed.

## [1.2.4] — 2026-10-21

### Fixed
- After the AppImage was run once and deleted, the shortcut of an installed Sotto stopped working
  ("An app id is required"): the AppImage had written its menu entry over the installation's one, and
  the portals ignore an entry whose program is gone. The AppImage no longer replaces another
  installation's entry, and Sotto repairs what a deleted AppImage left behind: its menu entry and an
  autostart entry that pointed at it.

## [1.2.3] — 2026-10-21

### Added
- **Choice of GPU** (Settings → Acceleration): automatic (the discrete one), any GPU ggml sees, or the
  processor only.
- When a graphics card in the computer is missing from that list, the settings say why: the NVIDIA driver
  is not loaded or the card is switched off (integrated-only mode), the driver was updated and needs a
  restart, its Vulkan part is not installed, the card runs on nouveau, or ggml's GPU part is missing.

### Fixed
- GNOME: the recording indicator took the focus, so the text was pasted into it instead of your window
  and you had to click the window first. GNOME has no layer-shell, so on Wayland the indicator was an
  ordinary window that mutter focuses, and wl-copy focuses a window of its own to set the clipboard.
  On GNOME Sotto now runs through XWayland: the indicator never takes the focus, can be dragged anywhere
  and remembers its place, and the clipboard is set without a window. `QT_QPA_PLATFORM=wayland` turns
  this off. Without XWayland the indicator is hidden before the text is pasted.
- On X11 the indicator is now kept out of the window manager's hands, so no window manager can focus it.
- The Debian package and the installer pull in Qt's X11 plugin (`qt6-qpa-plugins`); the release
  workflow checks that the AppImage carries it.

## [1.2.2] — 2026-10-21

### Fixed
- On laptops with both an integrated and a discrete GPU, recognition ran on the integrated one: whisper.cpp
  takes the first GPU in the list, and the integrated one usually comes first. Sotto now picks the discrete
  GPU (among several of the same kind, the one with more memory), and the settings name the GPU it uses.

## [1.2.1] — 2026-10-20

### Fixed
- The AppImage did not start on Wayland: it lacked Qt's Wayland shell plugins (the build tool that packs it
  was pinned to a release that does not deploy them). The release workflow now checks that they are inside.
  The distribution packages were not affected.


## [1.2.0] — 2026-10-20

### Added
- **Interface language** (Settings → General): follow the system, English or Russian; applies after a
  restart. `SOTTO_LANG=en` or `ru` still overrides it for one run.


## [1.1.0] — 2026-10-20

New: a visible, replaceable model source and a download log. Security fixes after an independent review.

### Added
- **Download log** in the settings: which server a model comes from, redirects (the host the file
  really arrives from), the checksum result and where it was saved. The same lines go to
  `~/.local/share/sotto/download.log` and the journal.
- **Custom server** as a model source: any server with the same layout as huggingface.co, `https://`
  (or `http://localhost`). The checksum of official models is always checked; for your own files
  there is an option to skip it, and you can still drop any `ggml-*.bin` into the models folder.

### Security
- A download keeps the server and the checksum policy it started with; changing them in the
  settings while it runs cannot weaken it.
- Removal through the package manager uses the system's own `pkexec` and package tools, never the
  first copy found in `PATH`, and refuses manifest entries that leave the prefix.
- The installer no longer interpolates the prefix into `sed` programs, quotes the desktop entry path
  and refuses uninstall manifest entries that leave the prefix. Autostart quotes the program path.
- Release builds pin every GitHub Action to a commit and check the AppImage tools against a SHA-256.

### Fixed
- Cancelling a recording and starting another could revive the cancelled one and paste its text;
  every recording is now its own job.
- First start of the indicator on X11, GNOME Wayland or without layer-shell dereferenced a null window.
- A rejected recording (too short, silence) left the model loaded forever instead of unloading it.
- Dragging the recording indicator no longer jerks. On Wayland with layer-shell the compositor applies a
  move a moment after it is requested, and every pointer event was measured against a surface that had not
  moved yet. While you drag, the surface now covers the screen and the indicator is drawn where the
  pointer is; when you let go it shrinks back to its new place.

## [1.0.0] — 2026-10-20

First public release. Compared with the private 0.x builds:

### Added
- English interface. Russian is chosen automatically when the system language is Russian
  (`SOTTO_LANG=en` or `SOTTO_LANG=ru` to override).
- Every model download is checked against the size and SHA-256 published on Hugging Face, from
  either source; a file that does not match is deleted.
- Packages for Debian 13 and Fedora 44 (with their own Vulkan-enabled whisper.cpp 1.9.5 in
  `/usr/lib/sotto`), next to the Arch package and the AppImage. Each one is built in a clean
  container of its distribution, installed and started there before a release.
- Removal from the settings window and `sotto --uninstall` also work for the Debian and Fedora
  packages.
- Interactive installer (`install.sh`) for source checkouts, into `~/.local` without root. It
  uses the system's whisper.cpp or builds the pinned one for Sotto alone (with Vulkan when the
  headers are there), and `--uninstall` removes exactly the files it installed.

### Changed
- The application id is now `io.github.trendorin.Sotto`. The autostart entry of an older
  version moves over by itself; set the shortcut once more in the settings.
- New icon.

### Fixed
- The AppImage menu entry and the uninstaller trusted `$APPIMAGE` alone. Programs started from
  any other AppImage inherit it, so a packaged Sotto could point its menu entry at, or remove,
  someone else's AppImage. Sotto now checks that it really runs from inside that AppImage.

[Unreleased]: https://github.com/Trendorin/sotto/compare/v2.0.0...HEAD
[2.0.0]: https://github.com/Trendorin/sotto/releases/tag/v2.0.0
[1.2.5]: https://github.com/Trendorin/sotto/releases/tag/v1.2.5
[1.2.4]: https://github.com/Trendorin/sotto/releases/tag/v1.2.4
[1.2.3]: https://github.com/Trendorin/sotto/releases/tag/v1.2.3
[1.2.2]: https://github.com/Trendorin/sotto/releases/tag/v1.2.2
[1.2.1]: https://github.com/Trendorin/sotto/releases/tag/v1.2.1
[1.2.0]: https://github.com/Trendorin/sotto/releases/tag/v1.2.0
[1.1.0]: https://github.com/Trendorin/sotto/releases/tag/v1.1.0
[1.0.0]: https://github.com/Trendorin/sotto/releases/tag/v1.0.0
