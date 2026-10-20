#pragma once

#include <QByteArray>
#include <QPointF>
#include <QString>
#include <QVariant>

// Thin typed wrapper over QSettings (~/.config/sotto/sotto.conf).
namespace Config {

inline constexpr auto AppId = "io.github.trendorin.Sotto";

QString model();               // empty = not chosen yet
void setModel(const QString &id);

QString uiLanguage();          // interface language: "auto", "en" or "ru" (applies at the next start)
void setUiLanguage(const QString &code);
QString language();            // speech recognition language: "auto" or ISO code
void setLanguage(const QString &code);

bool useGpu();
void setUseGpu(bool on);

QByteArray inputDevice();      // empty = system default
void setInputDevice(const QByteArray &id);

QString prompt();              // optional hint for the decoder (names, terms)
void setPrompt(const QString &text);

bool autoPaste();              // false = copy to clipboard only
void setAutoPaste(bool on);

QString pasteKeys();           // "ctrl+v" | "ctrl+shift+v" | "shift+insert"
void setPasteKeys(const QString &keys);

QPointF overlayPosition();     // centre of the indicator, as a fraction of the screen
void setOverlayPosition(const QPointF &pos);
void resetOverlayPosition();

bool useVad();                 // cut silence with Silero VAD before decoding
void setUseVad(bool on);

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

// Command that launches us from outside: the .AppImage itself, `sotto`, or our binary path.
QString launchCommand();

// When running as an AppImage, install a .desktop entry and icon into ~/.local/share
// so the desktop portals can identify the app (and it shows up in the menu).
void integrateAppImage();

// Sotto up to 0.2 used the app id "io.github.sotto": move its autostart entry over
// and drop its AppImage menu entry.
void migrateLegacyFiles();

// Removes the AppImage menu entry/icon written by integrateAppImage().
void removeAppImageIntegration();

// Deletes all settings (used by the uninstaller).
void wipe();

} // namespace Config
