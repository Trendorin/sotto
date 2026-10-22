#include "system/desktop.h"
#include "core/config.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QRegularExpression>
#include <QStringList>
#include <QVariantMap>

namespace Desktop {

static QString tr(const char *text)
{
    return QCoreApplication::translate("Desktop", text);
}

Kind current()
{
    static const Kind kind = [] {
        const QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP").toLower();
        const QStringList parts = desktop.split(QLatin1Char(':'), Qt::SkipEmptyParts);
        const auto is = [&](const char *name) { return parts.contains(QLatin1String(name)); };
        if (is("kde"))
            return Kind::Kde;
        if (is("cosmic"))
            return Kind::Cosmic;
        if (is("hyprland") || qEnvironmentVariableIsSet("HYPRLAND_INSTANCE_SIGNATURE"))
            return Kind::Hyprland;
        if (is("niri") || qEnvironmentVariableIsSet("NIRI_SOCKET"))
            return Kind::Niri;
        if (is("sway") || qEnvironmentVariableIsSet("SWAYSOCK"))
            return Kind::Sway;
        if (is("river"))
            return Kind::River;
        if (is("wayfire"))
            return Kind::Wayfire;
        if (is("labwc"))
            return Kind::Labwc;
        if (is("gnome"))
            return Kind::Gnome;
        return Kind::Other;
    }();
    return kind;
}

QString name()
{
    switch (current()) {
    case Kind::Kde: return QStringLiteral("KDE Plasma");
    case Kind::Gnome: return QStringLiteral("GNOME");
    case Kind::Cosmic: return QStringLiteral("COSMIC");
    case Kind::Hyprland: return QStringLiteral("Hyprland");
    case Kind::Niri: return QStringLiteral("niri");
    case Kind::Sway: return QStringLiteral("sway");
    case Kind::River: return QStringLiteral("river");
    case Kind::Wayfire: return QStringLiteral("Wayfire");
    case Kind::Labwc: return QStringLiteral("labwc");
    case Kind::Other: break;
    }
    const QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    return desktop.isEmpty() ? tr("this desktop") : desktop;
}

// One word for a POSIX shell (compositors run exec lines through sh -c).
static QString shellQuoted(const QString &word)
{
    static const QRegularExpression safe(QStringLiteral(R"(^[A-Za-z0-9_./+-]+$)"));
    if (safe.match(word).hasMatch())
        return word;
    QString quoted = word;
    quoted.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + quoted + QLatin1Char('\'');
}

static QString kdlString(QString text)
{
    text.replace(QLatin1Char('\\'), QStringLiteral("\\\\")).replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return QLatin1Char('"') + text + QLatin1Char('"');
}

Binding binding()
{
    const QString program = Config::launchProgram();
    const QString command = shellQuoted(program) + QStringLiteral(" --toggle");
    const QString start = shellQuoted(program) + QStringLiteral(" --start");
    const QString stop = shellQuoted(program) + QStringLiteral(" --stop");
    switch (current()) {
    case Kind::Niri:
        return {QStringLiteral("~/.config/niri/config.kdl"),
                QStringLiteral("binds {\n    Mod+Space repeat=false { spawn %1 \"--toggle\"; }\n}").arg(kdlString(program))};
    case Kind::Hyprland:
        return {QStringLiteral("~/.config/hypr/hyprland.conf"),
                QStringLiteral("bind = SUPER, SPACE, exec, %1\n"
                               "# %4\n"
                               "# bind = SUPER, SPACE, exec, %2\n"
                               "# bindr = SUPER, SPACE, exec, %3")
                    .arg(command, start, stop, tr("or hold to talk:"))};
    case Kind::Sway:
        return {QStringLiteral("~/.config/sway/config"),
                QStringLiteral("bindsym $mod+space exec %1\n"
                               "# %4\n"
                               "# bindsym $mod+space exec %2\n"
                               "# bindsym --release $mod+space exec %3")
                    .arg(command, start, stop, tr("or hold to talk:"))};
    case Kind::River:
        return {QStringLiteral("~/.config/river/init"),
                QStringLiteral("riverctl map normal Super Space spawn %1").arg(shellQuoted(command))};
    case Kind::Wayfire:
        return {QStringLiteral("~/.config/wayfire.ini"),
                QStringLiteral("[command]\nbinding_sotto = <super> KEY_SPACE\ncommand_sotto = %1").arg(command)};
    case Kind::Labwc:
        return {QStringLiteral("~/.config/labwc/rc.xml"),
                QStringLiteral("<keybind key=\"W-space\">\n  <action name=\"Execute\" command=\"%1\"/>\n</keybind>")
                    .arg(command.toHtmlEscaped())};
    case Kind::Cosmic:
        return {tr("Settings → Keyboard → Keyboard Shortcuts → Custom Shortcuts → Add Shortcut"), command};
    case Kind::Gnome:
        return {tr("Settings → Keyboard → View and Customize Shortcuts → Custom Shortcuts"), command};
    case Kind::Kde:
        return {tr("System Settings → Keyboard → Shortcuts → Add New → Command or Script"), command};
    case Kind::Other:
        break;
    }
    return {tr("the keyboard shortcut settings of your desktop"), command};
}

void notify(const QString &title, const QString &body, bool warning)
{
    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.Notifications"), QStringLiteral("/org/freedesktop/Notifications"),
                                              QStringLiteral("org.freedesktop.Notifications"), QStringLiteral("Notify"));
    const QVariantMap hints{
        {QStringLiteral("desktop-entry"), QString::fromLatin1(Config::AppId)},
        {QStringLiteral("urgency"), QVariant::fromValue(uchar(warning ? 1 : 0))},
    };
    msg.setArguments({QStringLiteral("Sotto"), 0u, QString::fromLatin1(Config::AppId), title, body, QStringList(), hints,
                      warning ? 10000 : 5000});
    QDBusConnection::sessionBus().send(msg);
}

} // namespace Desktop
