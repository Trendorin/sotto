#include "output.h"
#include "log.h"
#include "config.h"
#include "portal.h"

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

TextOutput::TextOutput(QObject *parent)
    : QObject(parent)
{
}

TextOutput::~TextOutput() = default;

void TextOutput::deliver(const QString &text)
{
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

void TextOutput::copyAndPaste(const QString &text)
{
    if (!copyToClipboard(text)) {
        Q_EMIT failed(tr("Could not copy the text to the clipboard"));
        return;
    }
    if (!Config::autoPaste())
        return;

    const QString keys = Config::pasteKeys();
    QList<int> combo{LeftCtrl, V};
    if (keys == QLatin1String("ctrl+shift+v"))
        combo = {LeftCtrl, LeftShift, V};
    else if (keys == QLatin1String("shift+insert"))
        combo = {LeftShift, Insert};

    withSession([this, combo](bool ok) {
        qCInfo(lcSotto) << "paste via RemoteDesktop portal:" << (ok ? "ok" : "no access");
        if (!ok) {
            Q_EMIT failed(tr("No input access — the text is in the clipboard, paste it yourself"));
            return;
        }
        // Give the clipboard owner a moment to settle before pasting.
        QTimer::singleShot(60, this, [this, combo] { sendKeys(combo); });
    });
}

void TextOutput::prepare()
{
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

void TextOutput::sendKeys(const QList<int> &keycodes)
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
