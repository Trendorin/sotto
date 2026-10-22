# Troubleshooting

## Logs

```sh
journalctl --user -t sotto -f     # desktop, how keys are pressed, phrases, engine, speed
SOTTO_DEBUG=1 sotto               # whisper.cpp, ggml and sherpa-onnx details
```

Every dictation logs its phrases (`phrase of 3.2 s`), how long each took (`sherpa-onnx: 3.2 s in
310 ms`) and how the text was pasted (`paste via wtype ok`).

## Common problems

| problem | what to do |
|---|---|
| Recognition is slow | with Whisper the log says `gpu: off`: install a Vulkan driver (on Arch also `ggml-vulkan`), or take GigaAM or Parakeet, which are fast on the processor |
| Phrases are typed too early, sentences break up | raise **Pause that ends a phrase** (800–1000 ms) |
| Phrases are typed too late | lower it (400 ms) |
| A laptop uses the wrong GPU | Sotto takes the discrete one; choose another in **Settings → Acceleration**. The log lists every GPU it sees (`gpu 0 …`) |
| The NVIDIA card is not in the list | Vulkan does not see it, and the settings say why: the driver is not loaded or the card is switched off (choose hybrid mode in envycontrol, supergfxctl or optimus-manager), the driver was updated and needs a restart, or its Vulkan part is missing (`nvidia-utils` on Arch, `nvidia-vulkan-icd` on Debian). `vulkaninfo --summary` should list the card |
| Text is not pasted on KDE, GNOME or COSMIC | **Settings → Input access → Allow…**; in terminals choose Ctrl+Shift+V |
| Text is not pasted on Hyprland, niri or sway | install `wtype`; the log says `ways to press keys: ("wtype")` once it is found |
| "Nothing can press keys here" | none of the portal, wtype, ydotool or xdotool is there: install `wtype` (Wayland) or `xdotool` (X11) |
| ydotool does nothing | it needs the `ydotoold` service running and access to `/dev/uinput` |
| The key does nothing | check it with **Change…**; on tiling compositors add the binding from the settings or `sotto --keybind` |
| No tray icon | the compositor has no tray (add one to your bar); open the settings with `sotto --settings` |
| Names come out wrong | add them to **Vocabulary** (Whisper) |
| GigaAM writes English words in Cyrillic | it only knows Russian; take Parakeet for mixed speech |
| A model download fails | switch **Source** to the mirror; a file with a wrong checksum is deleted, try again |
| GigaAM and Parakeet are missing from the list | this build has no sherpa-onnx (`--no-sherpa`, or a source build without it): run `./install.sh` again |
| The indicator is a normal window | `layer-shell-qt` is missing (source installs) |
| GNOME: the indicator takes the focus, the text goes nowhere | Sotto runs through XWayland on GNOME so this cannot happen; it needs Qt's X11 plugin (`qt6-qpa-plugins` on Debian and Ubuntu) and XWayland. `QT_QPA_PLATFORM=wayland sotto` turns this off |
| COSMIC: the text is not in the clipboard | start the session with `COSMIC_DATA_CONTROL_ENABLED=1` so `wl-copy` can set it |
| Hotkey missing for the AppImage | start it once normally so it adds itself to the menu |
| Icons are blank | the Qt SVG plugin is missing: `qt6-svg` (Arch), `qt6-svg-plugins` (Debian, Ubuntu), `qt6-qtsvg` (Fedora) |
