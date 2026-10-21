# Troubleshooting

## Logs

```sh
journalctl --user -t sotto -f     # microphone, length, GPU or not, speed
SOTTO_DEBUG=1 sotto               # whisper and ggml details
```

## Common problems

| problem | what to do |
|---|---|
| Recognition is slow | the log says `gpu: false`: install a Vulkan driver (on Arch also `ggml-vulkan`) or choose **Small** |
| A laptop uses the wrong GPU | Sotto takes the discrete one; choose another in **Settings → Acceleration**. The log lists every GPU it sees (`gpu 0 …`) |
| The NVIDIA card is not in the list | Vulkan does not see it, and the settings say why: the driver is not loaded or the card is switched off (choose hybrid mode in envycontrol, supergfxctl or optimus-manager), the driver was updated and needs a restart, or its Vulkan part is missing (`nvidia-utils` on Arch, `nvidia-vulkan-icd` on Debian). `vulkaninfo --summary` should list the card |
| Text is not pasted | **Settings → Input access → Allow…**; in terminals choose Ctrl+Shift+V |
| The key does nothing | check it with **Change…**, or bind any key to `sotto --toggle` |
| Names come out wrong | add them to **Vocabulary** |
| A model download fails | switch **Source** to the mirror; a file with a wrong checksum is deleted, try again |
| The indicator is a normal window | `layer-shell-qt` is missing (source installs) |
| GNOME: the indicator takes the focus, the text goes nowhere | Sotto runs through XWayland on GNOME so this cannot happen; it needs Qt's X11 plugin (`qt6-qpa-plugins` on Debian and Ubuntu) and XWayland. `QT_QPA_PLATFORM=wayland sotto` turns this off |
| Hotkey missing for the AppImage | start it once normally so it adds itself to the menu |
