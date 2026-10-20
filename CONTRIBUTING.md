# Contributing to Sotto

Thanks for helping. Bug reports, translations, documentation and code are all welcome. For
anything larger than a small fix, open an issue first so we can agree on the approach.

## Build

```sh
cmake -S . -B build -G Ninja -DSOTTO_WERROR=ON
cmake --build build
SOTTO_LANG=ru build/sotto      # try the Russian interface
```

You need Qt 6 (Widgets, Multimedia, DBus, Network), layer-shell-qt and whisper.cpp (the
`whisper-cpp` package on Arch, or `packaging/build-whisper.sh`).

## Guidelines

- Interface text is English in the code (`tr("...")`); add its Russian line to `src/i18n.cpp`.
- Nothing may record without the user starting it, write audio to disk, or use the network
  apart from the model download. New models need their size and SHA-256 in `src/models.cpp`.
- Use the desktop portals on Wayland; no root helpers.
- Note user-visible changes in `CHANGELOG.md` and both READMEs.

By contributing you agree that your work is licensed under the MIT license.
