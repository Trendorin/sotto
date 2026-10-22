#include "gui/uninstaller.h"
#include "core/config.h"
#include "core/log.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>

namespace Uninstaller {

static qint64 dirSize(const QString &path)
{
    qint64 total = 0;
    QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        total += it.nextFileInfo().size();
    return total;
}

static void forgetKdeShortcut()
{
    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kglobalaccel"), QStringLiteral("/kglobalaccel"),
                                              QStringLiteral("org.kde.KGlobalAccel"), QStringLiteral("unregister"));
    msg.setArguments({QString::fromLatin1(Config::AppId), QStringLiteral("dictate")});
    QDBusConnection::sessionBus().call(msg, QDBus::Block, 2000);
}

// System tools only from the system's own directories, never from PATH: a copy of `pacman` in
// ~/.local/bin must not be the one that pkexec later runs as root.
static QString systemTool(const QString &name)
{
    for (const char *dir : {"/usr/bin", "/bin", "/usr/sbin", "/sbin"}) {
        const QString path = QLatin1String(dir) + QLatin1Char('/') + name;
        if (QFileInfo(path).isExecutable())
            return path;
    }
    return {};
}

static bool succeeds(const QString &program, const QStringList &args)
{
    const QString tool = systemTool(program);
    if (tool.isEmpty())
        return false;
    QProcess p;
    p.start(tool, args);
    return p.waitForFinished(5000) && p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

// The package manager that owns `file`, as the command that removes the sotto package.
static QStringList packageRemoval(const QString &file)
{
    if (succeeds(QStringLiteral("pacman"), {QStringLiteral("-Qqo"), file}))
        return {systemTool(QStringLiteral("pacman")), QStringLiteral("-R"), QStringLiteral("--noconfirm"), QStringLiteral("sotto")};
    if (succeeds(QStringLiteral("dpkg-query"), {QStringLiteral("-S"), file}))
        return {systemTool(QStringLiteral("apt-get")), QStringLiteral("remove"), QStringLiteral("-y"), QStringLiteral("sotto")};
    if (succeeds(QStringLiteral("rpm"), {QStringLiteral("-qf"), file}))
        return {systemTool(QStringLiteral("rpm")), QStringLiteral("-e"), QStringLiteral("sotto")};
    return {};
}

bool run(QWidget *parent)
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/sotto");
    const qint64 dataMb = dirSize(dataDir) >> 20;

    QMessageBox box(QMessageBox::Question, QObject::tr("Remove Sotto"),
                    QObject::tr("Remove Sotto from this computer?\n\n"
                                "The program, its menu entry, autostart and shortcut will be removed."),
                    QMessageBox::Yes | QMessageBox::Cancel, parent);
    box.button(QMessageBox::Yes)->setText(QObject::tr("Delete"));
    auto *purge = new QCheckBox(QObject::tr("Also delete downloaded models and settings (%1 MB)").arg(dataMb));
    purge->setChecked(true);
    box.setCheckBox(purge);
    if (box.exec() != QMessageBox::Yes)
        return false;

    qCInfo(lcSotto) << "uninstalling, purge data:" << purge->isChecked();
    Config::setAutostart(false);
    Config::removeAppImageIntegration();
    forgetKdeShortcut();
    if (purge->isChecked()) {
        QDir(dataDir).removeRecursively();
        Config::wipe();
    }

    const QString binary = QCoreApplication::applicationFilePath();
    const QString appImage = Config::appImage();
    const QStringList removal = appImage.isEmpty() ? packageRemoval(binary) : QStringList();
    const QString home = QDir::homePath();
    QString error;

    if (!appImage.isEmpty()) {
        if (!QFile::remove(appImage))
            error = QObject::tr("Could not delete %1").arg(appImage);
    } else if (!removal.isEmpty()) {
        // pkexec shows the system password dialog.
        const QString pkexec = systemTool(QStringLiteral("pkexec"));
        const int rc = (pkexec.isEmpty() || removal.constFirst().isEmpty()) ? -1 : QProcess::execute(pkexec, removal);
        if (rc != 0)
            error = QObject::tr("The package was not removed. Run it yourself:\n%1").arg(QStringLiteral("sudo ") + removal.join(QLatin1Char(' ')));
    } else if (binary.startsWith(home)) {
        // install.sh lists everything it installed, the private whisper.cpp included.
        const QString prefix = QDir::cleanPath(QFileInfo(binary).absolutePath() + QStringLiteral("/.."));
        QFile manifest(prefix + QStringLiteral("/share/sotto/installed-files"));
        if (manifest.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString root = QFileInfo(prefix).canonicalFilePath();
            while (!manifest.atEnd()) {
                const QString path = QString::fromLocal8Bit(manifest.readLine()).trimmed();
                if (path.isEmpty() || !QDir::isAbsolutePath(path))
                    continue;
                // Resolve the directory first: ".." and symlinked parents must not lead out of the prefix.
                const QFileInfo info(path);
                const QString dir = QFileInfo(info.absolutePath()).canonicalFilePath();
                if (!root.isEmpty() && dir.startsWith(root + QLatin1Char('/')))
                    QFile::remove(dir + QLatin1Char('/') + info.fileName());
            }
            manifest.close();
            manifest.remove();
            for (const char *dir : {"/lib/sotto/ggml", "/lib/sotto", "/share/sotto", "/share/licenses/sotto"})
                QDir().rmdir(prefix + QLatin1String(dir));
        } else {
            const QString data = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
            const QString id = QString::fromLatin1(Config::AppId);
            QFile::remove(data + QStringLiteral("/applications/") + id + QStringLiteral(".desktop"));
            QFile::remove(data + QStringLiteral("/icons/hicolor/scalable/apps/") + id + QStringLiteral(".svg"));
            QFile::remove(binary);
        }
    } else {
        error = QObject::tr("Sotto is installed in %1. Remove it with your distribution's tools.").arg(binary);
    }

    if (error.isEmpty())
        QMessageBox::information(parent, QObject::tr("Remove Sotto"), QObject::tr("Sotto was removed."));
    else
        QMessageBox::warning(parent, QObject::tr("Remove Sotto"), error);
    return true;
}

} // namespace Uninstaller
