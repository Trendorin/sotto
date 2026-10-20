# Using Sotto

## The key

The default is **Meta+Space**. Change it with **Change…** in the settings; on Wayland the desktop
asks you to confirm it through the GlobalShortcuts portal.

- **Tap** records until the next tap.
- **Hold** records while the key is down.

Any key can also run `sotto --toggle`, set in your desktop's own shortcut settings.

## Pasting

When you stop, the text goes into the active window. On Wayland Sotto presses the paste keys
through the RemoteDesktop portal: the first time the desktop asks for "remote control"
permission. Tick **Remember**. Without the permission the text stays in the clipboard.

**Paste keys** in the settings switch between Ctrl+V and Ctrl+Shift+V (terminals). **Only copy to
the clipboard** turns pasting off.

## The indicator

While recording, a small indicator floats above the windows without taking focus. Drag it to
move it, click it to stop, right-click to cancel.

## Vocabulary

Names and terms you say often go into **Vocabulary** in the settings, separated by commas.
Whisper then spells them the way you wrote them.

## Command line

```sh
sotto               # start in the tray (or show the settings if it already runs)
sotto --toggle      # start or stop recording
sotto --start       # start recording
sotto --stop        # stop and paste
sotto --cancel      # stop and throw the recording away
sotto --settings    # open the settings
sotto --uninstall   # remove Sotto
sotto --quit        # quit the running copy
```

`SOTTO_LANG=en` or `SOTTO_LANG=ru` overrides the interface language.
