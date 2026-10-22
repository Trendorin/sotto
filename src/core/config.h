#pragma once

#include <QByteArray>
#include <QPointF>
#include <QString>

// Thin typed wrapper over QSettings (~/.config/sotto/sotto.conf). Every default lives in config.cpp.
namespace Config {

inline constexpr auto AppId = "io.github.trendorin.Sotto";

QString model();               // empty = not chosen yet
void setModel(const QString &id);

QString uiLanguage();          // interface language: "auto", "en" or "ru" (applies at the next start)
void setUiLanguage(const QString &code);
QString language();            // speech recognition language for Whisper: "auto" or ISO code
void setLanguage(const QString &code);

bool useGpu();
void setUseGpu(bool on);
QString gpuDevice();           // Gpu::Device::key, empty = pick the best one
void setGpuDevice(const QString &key);

QByteArray inputDevice();      // empty = system default
void setInputDevice(const QByteArray &id);

QString prompt();              // optional hint for Whisper (names, terms)
void setPrompt(const QString &text);

bool liveTyping();             // type every phrase as soon as it is recognised, not all at the end
void setLiveTyping(bool on);
int pauseMs();                 // a pause this long ends a phrase
void setPauseMs(int ms);

bool autoPaste();              // false = copy to clipboard only
void setAutoPaste(bool on);

QString pasteKeys();           // "ctrl+v" | "ctrl+shift+v" | "shift+insert"
void setPasteKeys(const QString &keys);

QString inputMethod();         // how keys are sent: "auto" | "portal" | "wtype" | "ydotool" | "xdotool"
void setInputMethod(const QString &method);

QPointF overlayPosition();     // centre of the indicator, as a fraction of the screen
void setOverlayPosition(const QPointF &pos);
void resetOverlayPosition();

QString modelSource();         // "huggingface" | "hf-mirror" | "custom"
void setModelSource(const QString &source);
QString customSourceUrl();     // base URL of a server with the Hugging Face layout, used when the source is "custom"
void setCustomSourceUrl(const QString &url);
bool allowUnverified();        // accept models from the custom source without the published checksum
void setAllowUnverified(bool on);

int unloadMinutes();           // 0 = keep model loaded
void setUnloadMinutes(int minutes);

QString remoteDesktopToken();
void setRemoteDesktopToken(const QString &token);

bool autostart();
void setAutostart(bool on);

// Path of the AppImage we run from, or empty. $APPIMAGE alone is not enough: every
// program started from *any* AppImage inherits it, so our own binary must also be
// inside the mounted $APPDIR.
QString appImage();

// The program that launches us from outside: the .AppImage itself, `sotto`, or our binary path.
QString launchProgram();
// The same as one argument of a Desktop Entry Exec line (quoted when it is a path).
QString launchCommand();

// When running as an AppImage, install a .desktop entry and icon into ~/.local/share
// so the desktop portals can identify the app (and it shows up in the menu). Otherwise repair
// what a deleted AppImage left behind: its menu entry and an autostart entry pointing at it.
void integrateAppImage();

// Sotto up to 0.2 used the app id "io.github.sotto": move its autostart entry over
// and drop its AppImage menu entry.
void migrateLegacyFiles();

// Removes the AppImage menu entry/icon written by integrateAppImage().
void removeAppImageIntegration();

// Deletes all settings (used by the uninstaller).
void wipe();

} // namespace Config
