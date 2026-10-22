#include "cli.h"
#include "core/config.h"
#include "core/i18n.h"
#include "core/log.h"
#include "gui/app.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>

#include <cstring>
#include <memory>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

Q_LOGGING_CATEGORY(lcSotto, "sotto", QtInfoMsg)

// `sotto --toggle` bound to a key in niri, Hyprland or sway starts a whole process per key press.
// When Sotto already runs, hand the command over through its socket before Qt starts up at all:
// the key press then costs a millisecond instead of a Qt start.
static bool forwardFast(int argc, char *argv[])
{
    static const char *const commands[] = {"--toggle", "--start", "--stop", "--cancel", "--settings", "--quit"};
    const char *runtime = getenv("XDG_RUNTIME_DIR");
    if (argc != 2 || !runtime || !*runtime)
        return false;
    const char *command = nullptr;
    for (const char *c : commands)
        if (std::strcmp(argv[1], c) == 0)
            command = c + 2;
    if (!command)
        return false;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    const int n = snprintf(addr.sun_path, sizeof addr.sun_path, "%s/sotto.socket", runtime);
    if (n <= 0 || size_t(n) >= sizeof addr.sun_path)
        return false;
    const int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0)
        return false;
    bool sent = false;
    if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof addr) == 0) {
        const size_t len = std::strlen(command);
        sent = write(fd, command, len) == ssize_t(len);
    }
    close(fd);
    return sent;
}

// Commands that only print something need no display: they run on a QCoreApplication, so they
// also work over SSH and in CI.
static bool printsOnly(int argc, char *argv[])
{
    static const char *const options[] = {"--transcribe", "--models", "--keybind", "--help", "-h", "--help-all", "--version", "-v"};
    for (int i = 1; i < argc; ++i)
        for (const char *o : options)
            if (std::strcmp(argv[i], o) == 0 || (std::strncmp(argv[i], o, std::strlen(o)) == 0 && argv[i][std::strlen(o)] == '='))
                return true;
    return false;
}

int main(int argc, char *argv[])
{
    if (forwardFast(argc, argv))
        return 0;

    // GNOME has no layer-shell, so on Wayland the indicator would be an ordinary window: mutter gives it
    // the focus, and wl-copy has to focus a window of its own to set the clipboard, so the paste keys
    // land there instead of in the user's window. Through XWayland the indicator is an override-redirect
    // window that never takes the focus and the clipboard is set without one. QT_QPA_PLATFORM still wins;
    // without the xcb plugin Qt falls back to Wayland.
    if (qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains(QLatin1String("GNOME")) && qEnvironmentVariableIsSet("WAYLAND_DISPLAY")
        && qEnvironmentVariableIsSet("DISPLAY") && !qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "xcb;wayland");

    QCoreApplication::setOrganizationName(QStringLiteral("sotto"));
    QCoreApplication::setApplicationName(QStringLiteral("sotto"));
    QCoreApplication::setApplicationVersion(QStringLiteral(SOTTO_VERSION));
    QGuiApplication::setDesktopFileName(QString::fromLatin1(Config::AppId));
    const std::unique_ptr<QCoreApplication> app = printsOnly(argc, argv) ? std::make_unique<QCoreApplication>(argc, argv)
                                                                          : std::make_unique<QApplication>(argc, argv);
    I18n::install(*app);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Sotto — voice typing for Linux"));
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
    const QCommandLineOption transcribe(QStringLiteral("transcribe"),
                                        QCoreApplication::translate("main", "Print the text of a WAV file and exit"),
                                        QStringLiteral("file.wav"));
    const QCommandLineOption model(QStringLiteral("model"),
                                   QCoreApplication::translate("main", "Model for --transcribe (default: the one in the settings)"),
                                   QStringLiteral("id"));
    const QCommandLineOption models(QStringLiteral("models"), QCoreApplication::translate("main", "List the models and exit"));
    const QCommandLineOption keybind(QStringLiteral("keybind"),
                                     QCoreApplication::translate("main", "Show how to bind dictation to a key on this desktop"));
    parser.addOptions({transcribe, model, models, keybind});
    parser.process(*app);

    // Commands that print something and exit; they never touch a running Sotto.
    if (parser.isSet(transcribe))
        return Cli::transcribe(parser.value(transcribe), parser.value(model));
    if (parser.isSet(models))
        return Cli::listModels();
    if (parser.isSet(keybind))
        return Cli::keybind();

    QGuiApplication::setApplicationDisplayName(QStringLiteral("Sotto"));
    // Before any portal call: the portal looks our app id up in the .desktop files.
    Config::migrateLegacyFiles();
    Config::integrateAppImage();
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication::setWindowIcon(QIcon::fromTheme(QString::fromLatin1(Config::AppId), QIcon(QStringLiteral(":/sotto.svg"))));

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
    return app->exec();
}
