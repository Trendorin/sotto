# Contributing to Sotto

Thanks for helping. Bug reports, translations, documentation and code are all welcome. For
anything larger than a small fix, open an issue first so we can agree on the approach.

## Build

```sh
packaging/fetch-sherpa.sh build/sherpa          # sherpa-onnx for GigaAM and Parakeet (optional)
cmake -S . -B build -G Ninja -DSOTTO_WERROR=ON -DSOTTO_BUILD_TESTS=ON -DSOTTO_SHERPA_DIR=build/sherpa
cmake --build build
ctest --test-dir build --output-on-failure      # phrase detection, translations
SOTTO_LANG=ru build/sotto                       # try the Russian interface
build/sotto --transcribe some.wav               # try a model on a recording
```

You need Qt 6 (Widgets, Multimedia, DBus, Network), layer-shell-qt and whisper.cpp (the
`whisper-cpp` package on Arch, or `packaging/build-whisper.sh`). Without `SOTTO_SHERPA_DIR` Sotto
builds with the Whisper models only.

## Layout

| | |
|---|---|
| `src/core/` | audio, phrase detection (`endpointer`, `segmenter`, `listener`), the engines (`engine`, `whisperengine`, `sherpaengine`), `transcriber`, models, settings, translations |
| `src/system/` | desktop detection and key bindings, pasting, the xdg-desktop-portal helpers, the global shortcut |
| `src/gui/` | the controller (`app`), the indicator, the settings, the uninstaller |
| `src/cli.cpp` | `--transcribe`, `--models`, `--keybind` |
| `packaging/` | desktop entry, AppStream metadata, icon; Arch, Debian, Fedora and AppImage builds; the pinned whisper.cpp and sherpa-onnx |
| `tests/`, `tools/` | ctest programs; the translation check and the README art |

A new engine is a class derived from `Engine` (`src/core/engine.h`), created in `Engine::load` and
given models in `Models::catalog()`.

## Guidelines

- Interface text is English in the code (`tr("...")`); add its Russian line to `src/core/i18n.cpp`.
  `tools/check-translations.py` (a ctest) fails on a missing or unused entry.
- Nothing may record without the user starting it, write audio to disk, or use the network
  apart from the model download. New models need the size and SHA-256 of every file in
  `src/core/models.cpp`.
- Use the desktop portals on Wayland where the desktop has them; no root helpers.
- Note user-visible changes in `CHANGELOG.md` and both READMEs.

By contributing you agree that your work is licensed under the MIT license.
