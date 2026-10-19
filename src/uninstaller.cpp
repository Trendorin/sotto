#include "uninstaller.h"
#include "config.h"
#include "log.h"
#include "models.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QDirIterator>
#include <QFile>
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

static bool succeeds(const QString &program, const QStringList &args)
{
    if (QStandardPaths::findExecutable(program).isEmpty())
        return false;
    QProcess p;
    p.start(program, args);
    return p.waitForFinished(5000) && p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

// The package manager that owns `file`, as the command that removes the sotto package.
static QStringList packageRemoval(const QString &file)
{
    if (succeeds(QStringLiteral("pacman"), {QStringLiteral("-Qqo"), file}))
        return {QStringLiteral("pacman"), QStringLiteral("-R"), QStringLiteral("--noconfirm"), QStringLiteral("sotto")};
    if (succeeds(QStringLiteral("dpkg-query"), {QStringLiteral("-S"), file}))
        return {QStringLiteral("apt-get"), QStringLiteral("remove"), QStringLiteral("-y"), QStringLiteral("sotto")};
    if (succeeds(QStringLiteral("rpm"), {QStringLiteral("-qf"), file}))
        return {QStringLiteral("rpm"), QStringLiteral("-e"), QStringLiteral("sotto")};
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
        const int rc = QProcess::execute(QStringLiteral("pkexec"), removal);
        if (rc != 0)
            error = QObject::tr("The package was not removed. Run it yourself:\n%1").arg(QStringLiteral("sudo ") + removal.join(QLatin1Char(' ')));
    } else if (binary.startsWith(home)) {
        const QString data = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        const QString id = QString::fromLatin1(Config::AppId);
        QFile::remove(data + QStringLiteral("/applications/") + id + QStringLiteral(".desktop"));
        QFile::remove(data + QStringLiteral("/icons/hicolor/scalable/apps/") + id + QStringLiteral(".svg"));
        QFile::remove(binary);
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
