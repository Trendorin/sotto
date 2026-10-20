#pragma once

class QWidget;

namespace Uninstaller {
// Asks for confirmation, then removes the program (pacman package, ~/.local install
// or the AppImage), its menu entry, autostart, the KDE shortcut and, optionally, models
// and settings. Returns true if the app should quit.
bool run(QWidget *parent = nullptr);
} // namespace Uninstaller
