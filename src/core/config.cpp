#include "core/config.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

#include <algorithm>

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

QString uiLanguage() { return get("uiLanguage", QStringLiteral("auto")).toString(); }
void setUiLanguage(const QString &code) { set("uiLanguage", code); }

QString language() { return get("language", QStringLiteral("auto")).toString(); }
void setLanguage(const QString &code) { set("language", code); }

bool useGpu() { return get("gpu", true).toBool(); }
void setUseGpu(bool on) { set("gpu", on); }
QString gpuDevice() { return get("gpuDevice").toString(); }
void setGpuDevice(const QString &key) { set("gpuDevice", key); }

QByteArray inputDevice() { return get("inputDevice").toByteArray(); }
void setInputDevice(const QByteArray &id) { set("inputDevice", id); }

QString prompt() { return get("prompt").toString(); }
void setPrompt(const QString &text) { set("prompt", text); }

bool liveTyping() { return get("live", true).toBool(); }
void setLiveTyping(bool on) { set("live", on); }
int pauseMs() { return std::clamp(get("pauseMs", 600).toInt(), 200, 3000); }
void setPauseMs(int ms) { set("pauseMs", ms); }

bool autoPaste() { return get("output/paste", true).toBool(); }
void setAutoPaste(bool on) { set("output/paste", on); }

QString pasteKeys() { return get("output/keys", QStringLiteral("ctrl+v")).toString(); }
void setPasteKeys(const QString &keys) { set("output/keys", keys); }

QString inputMethod() { return get("output/method", QStringLiteral("auto")).toString(); }
void setInputMethod(const QString &method) { set("output/method", method); }

QPointF overlayPosition() { return get("overlay/position", QPointF(0.5, 0.9)).toPointF(); }
void setOverlayPosition(const QPointF &pos) { set("overlay/position", pos); }
void resetOverlayPosition() { store().remove(QStringLiteral("overlay/position")); }

QString modelSource() { return get("modelSource", QStringLiteral("huggingface")).toString(); }
void setModelSource(const QString &source) { set("modelSource", source); }
QString customSourceUrl() { return get("customSourceUrl").toString().trimmed(); }
void setCustomSourceUrl(const QString &url) { set("customSourceUrl", url.trimmed()); }
bool allowUnverified() { return get("allowUnverified", false).toBool(); }
void setAllowUnverified(bool on) { set("allowUnverified", on); }

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

// A path as one argument of a Desktop Entry Exec line: in double quotes, with the characters
// the specification names escaped (a path with spaces or a % must not be split or expanded).
static QString execQuoted(QString path)
{
    for (const QChar c : {QLatin1Char('\\'), QLatin1Char('"'), QLatin1Char('$'), QLatin1Char('`')})
        path.replace(c, QString(QLatin1Char('\\')) + c);
    path.replace(QLatin1Char('%'), QStringLiteral("%%"));
    return QLatin1Char('"') + path + QLatin1Char('"');
}

// Autostart (systemd-xdg-autostart-generator) and compositors look a bare name up in the system
// PATH, without the user's ~/.local/bin, so a bare `sotto` is only safe when that PATH finds us.
static QString systemSotto()
{
    return QStandardPaths::findExecutable(QStringLiteral("sotto"),
                                          {QStringLiteral("/usr/local/bin"), QStringLiteral("/usr/bin"), QStringLiteral("/bin")});
}

QString launchProgram()
{
    if (const QString image = appImage(); !image.isEmpty())
        return image;
    if (systemSotto() == QCoreApplication::applicationFilePath())
        return QStringLiteral("sotto");
    return QCoreApplication::applicationFilePath();
}

QString launchCommand()
{
    const QString program = launchProgram();
    return program == QLatin1String("sotto") ? program : execQuoted(program);
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

static QByteArray readAll(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

// The value of "Key=" in a desktop entry, empty if absent.
static QString entryValue(const QByteArray &entry, const char *key)
{
    const QByteArray prefix = QByteArray(key) + '=';
    for (const QByteArray &line : entry.split('\n'))
        if (line.startsWith(prefix))
            return QString::fromUtf8(line.mid(prefix.size()));
    return {};
}

// The program a desktop entry's Exec line starts, when it is a quoted absolute path.
static QString execPath(const QByteArray &entry)
{
    const QString exec = entryValue(entry, "Exec");
    if (!exec.startsWith(QLatin1String("\"/")))
        return {};
    QString path;
    for (qsizetype i = 1; i < exec.size() && exec[i] != QLatin1Char('"'); ++i) {
        if (exec[i] == QLatin1Char('\\') && i + 1 < exec.size())
            ++i;
        path += exec[i];
    }
    return path.replace(QStringLiteral("%%"), QStringLiteral("%"));
}

// A Sotto installed by install.sh lists its desktop entry in <prefix>/share/sotto/installed-files.
static bool installedHere(const QString &entryPath)
{
    const QString prefix = QFileInfo(QCoreApplication::applicationDirPath() + QStringLiteral("/..")).canonicalFilePath();
    const QByteArray manifest = readAll(prefix + QStringLiteral("/share/sotto/installed-files"));
    return manifest.split('\n').contains(entryPath.toUtf8());
}

// An AppImage that was run once and then deleted leaves its menu entry and autostart behind. If its
// entry took the place of this installation's one, the portals no longer know our app id and the
// shortcut stops working; an autostart entry would start a file that is gone. Sotto up to 2.0.0
// also wrote a bare `sotto` for ~/.local/bin, which the autostart never finds.
static void repairIntegration(const QString &entryPath)
{
    const QByteArray entry = readAll(entryPath);
    if (entry.contains("X-AppImage-Integrated=true") && !QFileInfo::exists(entryValue(entry, "TryExec"))) {
        QFile::remove(entryPath);
        if (installedHere(entryPath)) {
            QByteArray own = readAll(QStringLiteral(":/sotto.desktop"));
            own.replace("\nExec=sotto", "\nExec=" + execQuoted(QCoreApplication::applicationFilePath()).toUtf8());
            writeIfChanged(entryPath, own);
        }
    }
    if (autostart()) {
        const QByteArray autostartEntry = readAll(autostartFile());
        const QString target = execPath(autostartEntry);
        const bool bareNotFound = entryValue(autostartEntry, "Exec") == QLatin1String("sotto") && systemSotto().isEmpty();
        if ((!target.isEmpty() && !QFileInfo::exists(target)) || bareNotFound)
            setAutostart(true);
    }
}

void integrateAppImage()
{
    const QString data = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QString id = QLatin1String(AppId);
    const QString entryPath = data + QStringLiteral("/applications/") + id + QStringLiteral(".desktop");
    if (appImage().isEmpty()) {
        repairIntegration(entryPath);
        return;
    }
    const QString appDir = qEnvironmentVariable("APPDIR");

    // Don't replace the entry of a source installation in ~/.local: it names the app id already.
    if (const QByteArray existing = readAll(entryPath); !existing.isEmpty() && !existing.contains("X-AppImage-Integrated=true"))
        return;

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
    writeIfChanged(entryPath, entry);

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
