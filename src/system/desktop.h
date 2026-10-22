#pragma once

#include <QString>

// The desktop or compositor Sotto runs on, and what differs between them.
namespace Desktop {

enum class Kind { Kde, Gnome, Cosmic, Hyprland, Niri, Sway, River, Wayfire, Labwc, Other };

Kind current();
QString name();

// How to bind dictation to a key by hand, for desktops without the GlobalShortcuts portal or where
// the portal leaves the key to the compositor's configuration (Hyprland).
struct Binding {
    QString where;   // a config file to add `snippet` to, or the settings page to open
    QString snippet; // config lines, or the command to enter there
};
Binding binding();

// A desktop notification. Without a tray (niri, sway, Hyprland without a bar) the tray icon
// cannot show messages, so this goes straight to org.freedesktop.Notifications.
void notify(const QString &title, const QString &body, bool warning = false);

} // namespace Desktop
