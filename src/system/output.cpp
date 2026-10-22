#include "system/output.h"
#include "core/config.h"
#include "core/log.h"
#include "system/portal.h"

#include <QClipboard>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QGuiApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#include <QWindow>

static const QString Iface = QStringLiteral("org.freedesktop.portal.RemoteDesktop");

// Linux evdev key codes (linux/input-event-codes.h).
enum Key { LeftCtrl = 29, LeftShift = 42, V = 47, Insert = 110 };

// How long the target window gets to read the clipboard before the next text replaces it.
static constexpr int SettleMs = 250;

static QList<int> pasteCombo()
{
    const QString keys = Config::pasteKeys();
    if (keys == QLatin1String("ctrl+shift+v"))
        return {LeftCtrl, LeftShift, V};
    if (keys == QLatin1String("shift+insert"))
        return {LeftShift, Insert};
    return {LeftCtrl, V};
}

// The command line that presses `keys` with a tool: modifiers down, the key, modifiers up.
static QStringList toolArguments(const QString &tool, const QList<int> &keys)
{
    const auto name = [](int code) -> QString {
        switch (code) {
        case LeftCtrl: return QStringLiteral("ctrl");
        case LeftShift: return QStringLiteral("shift");
        case Insert: return QStringLiteral("Insert");
        default: return QStringLiteral("v");
        }
    };
    QStringList args;
    if (tool == QLatin1String("wtype")) {
        for (int i = 0; i < keys.size() - 1; ++i)
            args << QStringLiteral("-M") << name(keys[i]);
        args << QStringLiteral("-k") << name(keys.constLast());
        for (int i = int(keys.size()) - 2; i >= 0; --i)
            args << QStringLiteral("-m") << name(keys[i]);
    } else if (tool == QLatin1String("ydotool")) {
        args << QStringLiteral("key");
        for (int code : keys)
            args << QStringLiteral("%1:1").arg(code);
        for (auto it = keys.rbegin(); it != keys.rend(); ++it)
            args << QStringLiteral("%1:0").arg(*it);
    } else { // xdotool
        QStringList names;
        for (int code : keys)
            names << name(code);
        args << QStringLiteral("key") << QStringLiteral("--clearmodifiers") << names.join(QLatin1Char('+'));
    }
    return args;
}

TextOutput::TextOutput(QObject *parent)
    : QObject(parent)
{
}

TextOutput::~TextOutput() = default;

QStringList TextOutput::available()
{
    static const QStringList methods = [] {
        QStringList m;
        // Answered by the portal backend of the desktop: KDE, GNOME and COSMIC have one.
        if (Portal::version(Iface) > 0)
            m << QStringLiteral("portal");
        const bool wayland = qEnvironmentVariableIsSet("WAYLAND_DISPLAY");
        const auto have = [](const char *tool) { return !QStandardPaths::findExecutable(QLatin1String(tool)).isEmpty(); };
        if (wayland && have("wtype"))
            m << QStringLiteral("wtype"); // the virtual-keyboard protocol: Hyprland, niri, sway, river, labwc, wayfire
        if (!wayland && have("xdotool"))
            m << QStringLiteral("xdotool");
        if (have("ydotool"))
            m << QStringLiteral("ydotool"); // uinput, any compositor, needs ydotoold running
        qCInfo(lcSotto) << "ways to press keys:" << m;
        return m;
    }();
    return methods;
}

QString TextOutput::effectiveMethod()
{
    const QString method = Config::inputMethod();
    return method == QLatin1String("auto") ? available().value(0) : method;
}

void TextOutput::deliver(const QString &text)
{
    m_queue.append(text);
    pump();
}

void TextOutput::pump()
{
    if (m_busy || m_queue.isEmpty())
        return;
    m_busy = true;
    const QString text = m_queue.takeFirst();
    if (QGuiApplication::platformName() == QLatin1String("xcb")) {
        // X ignores a new selection owner stamped earlier than the last change, and Qt stamps the
        // clipboard with the time of the last X event it saw, which may be older than a copy made since
        // in another program. Change a property of a window of ours so the server sends a fresh time.
        if (!m_stamp) {
            m_stamp = std::make_unique<QWindow>();
            m_stamp->create();
        }
        m_stamp->setTitle(m_stamp->title() == QLatin1String("a") ? QStringLiteral("b") : QStringLiteral("a"));
        QTimer::singleShot(30, this, [this, text] { copyAndPaste(text); });
        return;
    }
    copyAndPaste(text);
}

void TextOutput::finishItem()
{
    QTimer::singleShot(SettleMs, this, [this] {
        m_busy = false;
        pump();
    });
}

void TextOutput::copyAndPaste(const QString &text)
{
    if (!copyToClipboard(text)) {
        Q_EMIT failed(tr("Could not copy the text to the clipboard"));
        finishItem();
        return;
    }
    if (!Config::autoPaste()) {
        finishItem();
        return;
    }
    const QString method = Config::inputMethod();
    const QStringList methods = method == QLatin1String("auto") ? available() : QStringList{method};
    // Give the clipboard owner a moment to settle before pasting.
    QTimer::singleShot(60, this, [this, methods] { paste(methods, pasteCombo()); });
}

void TextOutput::paste(QStringList methods, QList<int> keys)
{
    if (methods.isEmpty()) {
        Q_EMIT failed(available().isEmpty() ? tr("Nothing can press keys here: install wtype or ydotool — the text is in the clipboard")
                                            : tr("No input access — the text is in the clipboard, paste it yourself"));
        finishItem();
        return;
    }
    const QString method = methods.takeFirst();
    if (method == QLatin1String("portal")) {
        withSession([this, methods, keys](bool ok) {
            qCInfo(lcSotto) << "paste via RemoteDesktop portal:" << (ok ? "ok" : "no access");
            if (!ok)
                return paste(methods, keys);
            sendPortalKeys(keys);
            finishItem();
        });
        return;
    }
    auto *process = new QProcess(this);
    const auto done = [this, process, method, methods, keys](bool ok) {
        process->disconnect(this);
        process->deleteLater();
        qCInfo(lcSotto) << "paste via" << method << (ok ? "ok" : "failed:") << process->readAllStandardError().trimmed();
        if (ok)
            finishItem();
        else
            paste(methods, keys);
    };
    connect(process, &QProcess::finished, this, [done](int code, QProcess::ExitStatus status) {
        done(status == QProcess::NormalExit && code == 0);
    });
    connect(process, &QProcess::errorOccurred, this, [done](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            done(false);
    });
    QTimer::singleShot(3000, process, &QProcess::kill);
    process->start(method, toolArguments(method, keys));
}

void TextOutput::prepare()
{
    if (available().contains(QStringLiteral("portal")))
        withSession([](bool) {});
}

bool TextOutput::copyToClipboard(const QString &text)
{
    // On Wayland a background app can't own the clipboard through Qt without
    // focus, but wl-copy can (wlr/ext data-control).
    if (QGuiApplication::platformName() == QLatin1String("wayland")
        && !QStandardPaths::findExecutable(QStringLiteral("wl-copy")).isEmpty()) {
        QProcess p;
        p.start(QStringLiteral("wl-copy"), {QStringLiteral("--type"), QStringLiteral("text/plain;charset=utf-8")});
        if (p.waitForStarted(1000)) {
            p.write(text.toUtf8());
            p.closeWriteChannel();
            if (p.waitForFinished(2000) && p.exitCode() == 0)
                return true;
            qCWarning(lcSotto) << "wl-copy failed:" << p.readAllStandardError().trimmed();
        }
    }
    QGuiApplication::clipboard()->setText(text);
    return true;
}

void TextOutput::withSession(std::function<void(bool)> ready)
{
    if (m_ready) {
        ready(true);
        return;
    }
    m_waiting.append(std::move(ready));
    if (m_starting)
        return;
    m_starting = true;

    auto fail = [this](const char *stage, uint response, const QVariantMap &res) {
        qCWarning(lcSotto) << "RemoteDesktop" << stage << "failed, response" << response << res.value(QStringLiteral("error")).toString();
        m_starting = false;
        m_ready = false;
        m_session.clear();
        const auto waiting = std::exchange(m_waiting, {});
        for (const auto &cb : waiting)
            cb(false);
        Q_EMIT accessChanged(false);
    };

    Portal::registerApp();
    QVariantMap opts{{QStringLiteral("session_handle_token"), Portal::newToken()}};
    Portal::request(Iface, QStringLiteral("CreateSession"), {}, opts, [this, fail](uint response, const QVariantMap &res) {
        if (response != 0)
            return fail("CreateSession", response, res);
        m_session = Portal::objectPath(res.value(QStringLiteral("session_handle")));
        QDBusConnection::sessionBus().connect(Portal::Service, m_session, QStringLiteral("org.freedesktop.portal.Session"),
                                              QStringLiteral("Closed"), this, SLOT(onSessionClosed()));

        const QVariantList session{QVariant::fromValue(QDBusObjectPath(m_session))};
        QVariantMap select{
            {QStringLiteral("types"), 1u},        // keyboard
            {QStringLiteral("persist_mode"), 2u}, // until revoked
        };
        if (const QString token = Config::remoteDesktopToken(); !token.isEmpty())
            select.insert(QStringLiteral("restore_token"), token);

        Portal::request(Iface, QStringLiteral("SelectDevices"), session, select, [this, fail, session](uint response, const QVariantMap &res) {
            if (response != 0)
                return fail("SelectDevices", response, res);
            Portal::request(Iface, QStringLiteral("Start"), session + QVariantList{QString()}, {},
                            [this, fail](uint response, const QVariantMap &res) {
                                if (response != 0)
                                    return fail("Start", response, res);
                                if (const QString token = res.value(QStringLiteral("restore_token")).toString(); !token.isEmpty())
                                    Config::setRemoteDesktopToken(token);
                                m_starting = false;
                                m_ready = true;
                                Q_EMIT accessChanged(true);
                                const auto waiting = std::exchange(m_waiting, {});
                                for (const auto &cb : waiting)
                                    cb(true);
                            });
        });
    });
}

void TextOutput::sendPortalKeys(const QList<int> &keycodes)
{
    if (!m_ready)
        return;
    // Press in order, release in reverse, with a short gap between events.
    QList<QPair<int, uint>> events;
    for (int code : keycodes)
        events.append({code, 1u});
    for (auto it = keycodes.rbegin(); it != keycodes.rend(); ++it)
        events.append({*it, 0u});

    const QString session = m_session;
    for (int i = 0; i < events.size(); ++i) {
        QTimer::singleShot(i * 12, this, [session, ev = events[i]] {
            Portal::call(Iface, QStringLiteral("NotifyKeyboardKeycode"),
                         {QVariant::fromValue(QDBusObjectPath(session)), QVariantMap(), ev.first, ev.second});
        });
    }
}

void TextOutput::onSessionClosed()
{
    qCInfo(lcSotto) << "RemoteDesktop session closed";
    m_ready = false;
    m_session.clear();
    Q_EMIT accessChanged(false);
}
