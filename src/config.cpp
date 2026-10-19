#include "config.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

namespace Config {

static QSettings &store()
{
    static QSettings settings;
    return settings;
}

static QVariant get(const char *key, const QVariant &fallback = {})
{
    return store().value(QLatin1String(key), fallback);
}

static void set(const char *key, const QVariant &value)
{
    store().setValue(QLatin1String(key), value);
}

QString model() { return get("model").toString(); }
void setModel(const QString &id) { set("model", id); }

QString language() { return get("language", QStringLiteral("auto")).toString(); }
void setLanguage(const QString &code) { set("language", code); }

bool useGpu() { return get("gpu", true).toBool(); }
void setUseGpu(bool on) { set("gpu", on); }

QByteArray inputDevice() { return get("inputDevice").toByteArray(); }
void setInputDevice(const QByteArray &id) { set("inputDevice", id); }

QString prompt() { return get("prompt").toString(); }
void setPrompt(const QString &text) { set("prompt", text); }

bool autoPaste() { return get("output/paste", true).toBool(); }
void setAutoPaste(bool on) { set("output/paste", on); }

QString pasteKeys() { return get("output/keys", QStringLiteral("ctrl+v")).toString(); }
void setPasteKeys(const QString &keys) { set("output/keys", keys); }

QPointF overlayPosition() { return get("overlay/position", QPointF(0.5, 0.9)).toPointF(); }
void setOverlayPosition(const QPointF &pos) { set("overlay/position", pos); }
void resetOverlayPosition() { store().remove(QStringLiteral("overlay/position")); }

bool useVad() { return get("vad", true).toBool(); }
void setUseVad(bool on) { set("vad", on); }

QString modelSource() { return get("modelSource", QStringLiteral("huggingface")).toString(); }
void setModelSource(const QString &source) { set("modelSource", source); }

int unloadMinutes() { return get("unloadMinutes", 10).toInt(); }
void setUnloadMinutes(int minutes) { set("unloadMinutes", minutes); }

QString remoteDesktopToken() { return get("output/restoreToken").toString(); }
void setRemoteDesktopToken(const QString &token) { set("output/restoreToken", token); }

static QString autostartFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
         + QStringLiteral("/autostart/") + QLatin1String(AppId) + QStringLiteral(".desktop");
}

bool autostart() { return QFile::exists(autostartFile()); }

void setAutostart(bool on)
{
    const QString path = autostartFile();
    if (!on) {
        QFile::remove(path);
        return;
    }
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    const QString exec = launchCommand();
    file.write(QStringLiteral("[Desktop Entry]\n"
                              "Type=Application\n"
                              "Name=Sotto\n"
                              "Exec=%1\n"
                              "Icon=%2\n"
                              "X-GNOME-Autostart-enabled=true\n"
                              "NoDisplay=true\n")
                   .arg(exec, QLatin1String(AppId))
                   .toUtf8());
}

QString appImage()
{
    const QString image = qEnvironmentVariable("APPIMAGE");
    const QString appDir = qEnvironmentVariable("APPDIR");
    if (image.isEmpty() || appDir.isEmpty() || !QFileInfo(image).isFile())
        return {};
    const QString self = QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath();
    const QString root = QFileInfo(appDir).canonicalFilePath();
    return !root.isEmpty() && self.startsWith(root + QLatin1Char('/')) ? image : QString();
}

QString launchCommand()
{
    if (const QString image = appImage(); !image.isEmpty())
        return QLatin1Char('"') + image + QLatin1Char('"');
    if (QStandardPaths::findExecutable(QStringLiteral("sotto")) == QCoreApplication::applicationFilePath())
        return QStringLiteral("sotto");
    return QCoreApplication::applicationFilePath();
}

static bool writeIfChanged(const QString &path, const QByteArray &content)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly) && file.readAll() == content)
        return false;
    file.close();
    QDir().mkpath(QFileInfo(path).absolutePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    file.write(content);
    return true;
}

void integrateAppImage()
{
    if (appImage().isEmpty())
        return;
    const QString data = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QString appDir = qEnvironmentVariable("APPDIR");
    const QString id = QLatin1String(AppId);

    // Don't shadow a system-wide installation.
    for (const QString &dir : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
        if (dir != data && QFile::exists(dir + QStringLiteral("/applications/") + id + QStringLiteral(".desktop")))
            return;
    }

    QFile icon(appDir + QStringLiteral("/usr/share/icons/hicolor/scalable/apps/") + id + QStringLiteral(".svg"));
    if (icon.open(QIODevice::ReadOnly))
        writeIfChanged(data + QStringLiteral("/icons/hicolor/scalable/apps/") + id + QStringLiteral(".svg"), icon.readAll());

    QFile desktop(appDir + QStringLiteral("/usr/share/applications/") + id + QStringLiteral(".desktop"));
    if (!desktop.open(QIODevice::ReadOnly))
        return;
    QByteArray entry = desktop.readAll();
    entry.replace("\nExec=sotto", "\nExec=" + launchCommand().toUtf8());
    entry.append("TryExec=" + appImage().toUtf8() + "\n");
    entry.append("X-AppImage-Integrated=true\n");
    writeIfChanged(data + QStringLiteral("/applications/") + id + QStringLiteral(".desktop"), entry);

    // Keep an existing autostart entry pointing at the current file location.
    if (autostart())
        setAutostart(true);
}

void migrateLegacyFiles()
{
    const QString legacy = QStringLiteral("io.github.sotto");
    const QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/autostart/");
    if (QFile::remove(autostartDir + legacy + QStringLiteral(".desktop")))
        setAutostart(true);
    const QString data = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QFile old(data + QStringLiteral("/applications/") + legacy + QStringLiteral(".desktop"));
    if (old.open(QIODevice::ReadOnly) && old.readAll().contains("X-AppImage-Integrated=true")) {
        old.remove();
        QFile::remove(data + QStringLiteral("/icons/hicolor/scalable/apps/") + legacy + QStringLiteral(".svg"));
    }
}

void removeAppImageIntegration()
{
    const QString data = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QString id = QLatin1String(AppId);
    const QString desktop = data + QStringLiteral("/applications/") + id + QStringLiteral(".desktop");
    QFile file(desktop);
    if (file.open(QIODevice::ReadOnly) && file.readAll().contains("X-AppImage-Integrated=true")) {
        file.remove();
        QFile::remove(data + QStringLiteral("/icons/hicolor/scalable/apps/") + id + QStringLiteral(".svg"));
    }
}

void wipe()
{
    store().clear();
    store().sync();
    const QString file = store().fileName();
    QFile::remove(file);
    QDir().rmdir(QFileInfo(file).absolutePath());
}

} // namespace Config
