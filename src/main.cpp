#include "app.h"
#include "config.h"
#include "i18n.h"
#include "log.h"
#include "uninstaller.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>

Q_LOGGING_CATEGORY(lcSotto, "sotto", QtInfoMsg)

int main(int argc, char *argv[])
{
    // GNOME has no layer-shell, so on Wayland the indicator would be an ordinary window: mutter gives it
    // the focus, and wl-copy has to focus a window of its own to set the clipboard, so the paste keys
    // land there instead of in the user's window. Through XWayland the indicator is an override-redirect
    // window that never takes the focus and the clipboard is set without one. QT_QPA_PLATFORM still wins;
    // without the xcb plugin Qt falls back to Wayland.
    if (qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains(QLatin1String("GNOME")) && qEnvironmentVariableIsSet("WAYLAND_DISPLAY")
        && qEnvironmentVariableIsSet("DISPLAY") && !qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "xcb;wayland");

    QApplication::setOrganizationName(QStringLiteral("sotto"));
    QApplication::setApplicationName(QStringLiteral("sotto"));
    QApplication::setApplicationVersion(QStringLiteral(SOTTO_VERSION));
    QApplication::setDesktopFileName(QString::fromLatin1(Config::AppId));
    QApplication app(argc, argv);
    I18n::install(app);
    // Before any portal call: the portal looks our app id up in the .desktop files.
    Config::migrateLegacyFiles();
    Config::integrateAppImage();
    QApplication::setApplicationDisplayName(QStringLiteral("Sotto"));
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication::setWindowIcon(QIcon::fromTheme(QString::fromLatin1(Config::AppId), QIcon(QStringLiteral(":/sotto.svg"))));

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Sotto — voice typing with whisper.cpp"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QList<QCommandLineOption> commands{
        {QStringLiteral("toggle"), QCoreApplication::translate("main", "Start or stop dictation")},
        {QStringLiteral("start"), QCoreApplication::translate("main", "Start dictation")},
        {QStringLiteral("stop"), QCoreApplication::translate("main", "Stop dictation and paste the text")},
        {QStringLiteral("cancel"), QCoreApplication::translate("main", "Cancel dictation")},
        {QStringLiteral("settings"), QCoreApplication::translate("main", "Open settings")},
        {QStringLiteral("uninstall"), QCoreApplication::translate("main", "Remove Sotto from this computer")},
        {QStringLiteral("quit"), QCoreApplication::translate("main", "Quit the running Sotto")},
    };
    parser.addOptions(commands);
    parser.process(app);

    QString command;
    for (const auto &opt : commands)
        if (parser.isSet(opt))
            command = opt.names().constFirst();

    // Single instance: forward the command to the running copy.
    const QString socketPath = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) + QStringLiteral("/sotto.socket");
    {
        QLocalSocket probe;
        probe.connectToServer(socketPath);
        if (probe.waitForConnected(500)) {
            probe.write((command.isEmpty() ? QStringLiteral("settings") : command).toUtf8());
            probe.flush();
            probe.waitForBytesWritten(1000);
            probe.disconnectFromServer();
            return 0;
        }
    }

    App controller;

    QLocalServer::removeServer(socketPath);
    QLocalServer server;
    server.setSocketOptions(QLocalServer::UserAccessOption);
    if (!server.listen(socketPath))
        qWarning("Cannot listen on %s: %s", qPrintable(socketPath), qPrintable(server.errorString()));
    QObject::connect(&server, &QLocalServer::newConnection, &controller, [&] {
        while (QLocalSocket *client = server.nextPendingConnection()) {
            auto handle = [client, &controller] {
                const QString cmd = QString::fromUtf8(client->readAll()).trimmed();
                if (cmd.isEmpty())
                    return;
                controller.command(cmd);
                client->disconnectFromServer();
            };
            QObject::connect(client, &QLocalSocket::readyRead, client, handle);
            QObject::connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
            if (client->bytesAvailable())
                handle();
        }
    });

    if (command == QLatin1String("quit"))
        return 0; // nothing was running
    controller.start();
    if (!command.isEmpty())
        controller.command(command);
    return app.exec();
}
