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
| A laptop uses the wrong GPU | Sotto takes the discrete one; the log lists every GPU it sees (`gpu 0 …`). To force one, start it with `GGML_VK_VISIBLE_DEVICES=<n>`, the number from `vulkaninfo --summary` |
| Text is not pasted | **Settings → Input access → Allow…**; in terminals choose Ctrl+Shift+V |
| The key does nothing | check it with **Change…**, or bind any key to `sotto --toggle` |
| Names come out wrong | add them to **Vocabulary** |
| A model download fails | switch **Source** to the mirror; a file with a wrong checksum is deleted, try again |
| The indicator is a normal window | `layer-shell-qt` is missing (source installs) |
| Hotkey missing for the AppImage | start it once normally so it adds itself to the menu |
