# Using Sotto

## The key

The default is **Meta+Space**. Change it with **Change…** in the settings; on Wayland the desktop
asks you to confirm it through the GlobalShortcuts portal.

- **Tap** records until the next tap.
- **Hold** records while the key is down.

On niri, Hyprland, sway and other compositors that bind keys in their own config, the settings
show the lines to add instead; see [Tiling compositors](desktops.md). Any key can also run
`sotto --toggle`, set in your desktop's own shortcut settings.

## Live typing

Sotto cuts what you say into phrases at your pauses and types each phrase as soon as it is
recognised, while you go on talking. **Pause that ends a phrase** (600 ms by default) sets how long
you have to stop for a phrase to be typed: shorter is quicker, longer keeps long sentences in one
piece, which helps punctuation.

Turn **Type each phrase as soon as I pause** off to get everything in one paste when the recording
stops. The phrases are still recognised while you speak, so even then you only wait for the last
one.

## Pasting

The text goes into the active window through the clipboard and the paste keys.

**Press keys with** chooses how the keys are pressed:

| | where | notes |
|---|---|---|
| Desktop portal | KDE, GNOME, COSMIC | the first time the desktop asks for "remote control" permission: tick **Remember** |
| wtype | Hyprland, niri, sway, river, Wayfire, labwc | the virtual-keyboard protocol; install the `wtype` package |
| ydotool | any desktop | needs the `ydotoold` service and access to `/dev/uinput` |
| xdotool | X11 sessions | |

**Automatic** takes the first one that is there, in this order, and falls back to the next one if
it fails. Without any of them the text stays in the clipboard.

**Paste keys** switch between Ctrl+V, Ctrl+Shift+V (terminals) and Shift+Insert. **Only copy to the
clipboard** turns pasting off; the whole text is then copied when the recording stops.

## The indicator

While recording, a small indicator floats above the windows without taking focus (a layer-shell
surface on Wayland, also on tiling compositors). Drag it to move it, click it to stop,
right-click to cancel.

## Vocabulary

Names and terms you say often go into **Vocabulary** in the settings, separated by commas.
Whisper then spells them the way you wrote them; it also gets the end of the previous phrase, so
spelling and punctuation stay consistent from phrase to phrase. GigaAM and Parakeet do not use it.

## Command line

```sh
sotto               # start in the tray (or show the settings if it already runs)
sotto --toggle      # start or stop recording
sotto --start       # start recording
sotto --stop        # stop and paste
sotto --cancel      # stop and throw the recording away
sotto --settings    # open the settings
sotto --keybind     # print the key binding to add on this desktop
sotto --models      # list the models, * = downloaded
sotto --transcribe file.wav [--model ID]   # print the text of a recording
sotto --uninstall   # remove Sotto
sotto --quit        # quit the running copy
```

`--toggle`, `--start`, `--stop`, `--cancel`, `--settings` and `--quit` reach a running Sotto through
its socket in `$XDG_RUNTIME_DIR` before Qt even starts, so a key bound to them reacts at once.

The interface language is chosen in **Settings → General → Interface language** (English, Russian or
follow the system; it applies after a restart). `SOTTO_LANG=en` or `SOTTO_LANG=ru` overrides it for one run.
