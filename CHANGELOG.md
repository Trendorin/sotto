# Changelog

All notable changes to Sotto. The format follows [Keep a Changelog](https://keepachangelog.com/),
and versions follow [Semantic Versioning](https://semver.org/).

## [Unreleased]

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

[Unreleased]: https://github.com/Trendorin/sotto/compare/v1.2.0...HEAD
[1.2.0]: https://github.com/Trendorin/sotto/releases/tag/v1.2.0
[1.1.0]: https://github.com/Trendorin/sotto/releases/tag/v1.1.0
[1.0.0]: https://github.com/Trendorin/sotto/releases/tag/v1.0.0
