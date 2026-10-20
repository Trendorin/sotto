# Changelog

All notable changes to Sotto. The format follows [Keep a Changelog](https://keepachangelog.com/),
and versions follow [Semantic Versioning](https://semver.org/).

## [Unreleased]

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

[Unreleased]: https://github.com/Trendorin/sotto/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/Trendorin/sotto/releases/tag/v1.0.0
