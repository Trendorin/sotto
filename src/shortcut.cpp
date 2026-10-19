#include "shortcut.h"

#include <QDBusConnection>
#include <QDBusMetaType>
#include <QProcess>
#include <QTimer>

#include <algorithm>

static const QString Iface = QStringLiteral("org.freedesktop.portal.GlobalShortcuts");
static const QString ShortcutId = QStringLiteral("dictate");

static PortalShortcutList shortcutsFrom(const QVariant &value)
{
    if (value.canConvert<QDBusArgument>())
        return qdbus_cast<PortalShortcutList>(value.value<QDBusArgument>());
    return value.value<PortalShortcutList>();
}

GlobalShortcut::GlobalShortcut(QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<PortalShortcut>();
    qDBusRegisterMetaType<PortalShortcutList>();
}

void GlobalShortcut::setup()
{
    Portal::registerApp();
    m_version = Portal::version(Iface);
    if (m_version == 0) {
        fail(tr("the GlobalShortcuts portal is unavailable"));
        return;
    }

    auto bus = QDBusConnection::sessionBus();
    bus.connect(Portal::Service, Portal::Path, Iface, QStringLiteral("Activated"), this,
                SLOT(onActivated(QDBusObjectPath, QString, qulonglong, QVariantMap)));
    bus.connect(Portal::Service, Portal::Path, Iface, QStringLiteral("Deactivated"), this,
                SLOT(onDeactivated(QDBusObjectPath, QString, qulonglong, QVariantMap)));
    bus.connect(Portal::Service, Portal::Path, Iface, QStringLiteral("ShortcutsChanged"), this,
                SLOT(onShortcutsChanged(QDBusObjectPath, PortalShortcutList)));

    QVariantMap opts{{QStringLiteral("session_handle_token"), Portal::newToken()}};
    Portal::request(Iface, QStringLiteral("CreateSession"), {}, opts, [this](uint response, const QVariantMap &res) {
        if (response != 0) {
            const QString error = res.value(QStringLiteral("error"), tr("the portal refused")).toString();
            // Right after the AppImage wrote its .desktop file the portal may not know us yet.
            if (error.contains(QLatin1String("app id")) && ++m_attempts < 4) {
                QTimer::singleShot(1500, this, &GlobalShortcut::setup);
                return;
            }
            fail(error);
            return;
        }
        m_session = Portal::objectPath(res.value(QStringLiteral("session_handle")));
        // ListShortcuts can return bindings remembered from an earlier session.
        // They only become active for this session after BindShortcuts is called.
        bind();
    });
}

void GlobalShortcut::bind()
{
    const PortalShortcutList shortcuts{{
        ShortcutId,
        {
            {QStringLiteral("description"), tr("Voice typing")},
            {QStringLiteral("preferred_trigger"), QStringLiteral("LOGO+space")},
        },
    }};
    const QVariantList args{QVariant::fromValue(QDBusObjectPath(m_session)), QVariant::fromValue(shortcuts), QString()};
    Portal::request(Iface, QStringLiteral("BindShortcuts"), args, {}, [this](uint response, const QVariantMap &res) {
        if (response != 0) {
            fail(res.value(QStringLiteral("error"), tr("the portal rejected the shortcut")).toString());
            return;
        }
        const auto bound = shortcutsFrom(res.value(QStringLiteral("shortcuts")));
        m_bound = std::any_of(bound.begin(), bound.end(), [](const PortalShortcut &s) { return s.id == ShortcutId; });
        if (!m_bound) {
            fail(tr("the portal did not bind the shortcut"));
            return;
        }
        m_error.clear();
        updateTrigger(bound);
    });
}

void GlobalShortcut::configure()
{
    if (m_session.isEmpty())
        return;
    if (m_version >= 2) {
        Portal::call(Iface, QStringLiteral("ConfigureShortcuts"),
                     {QVariant::fromValue(QDBusObjectPath(m_session)), QString(), QVariantMap()});
    } else if (qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains(QLatin1String("KDE"))) {
        QProcess::startDetached(QStringLiteral("systemsettings"), {QStringLiteral("kcm_keys")});
    } else {
        bind();
    }
}

void GlobalShortcut::updateTrigger(const PortalShortcutList &shortcuts)
{
    for (const auto &s : shortcuts) {
        if (s.id != ShortcutId)
            continue;
        m_trigger = s.properties.value(QStringLiteral("trigger_description")).toString();
        Q_EMIT changed();
    }
}

void GlobalShortcut::fail(const QString &reason)
{
    m_bound = false;
    m_trigger.clear();
    m_error = reason;
    qWarning("Global shortcut unavailable: %s", qPrintable(reason));
    Q_EMIT changed();
}

void GlobalShortcut::onActivated(const QDBusObjectPath &session, const QString &id, qulonglong, const QVariantMap &)
{
    if (session.path() == m_session && id == ShortcutId)
        Q_EMIT pressed();
}

void GlobalShortcut::onDeactivated(const QDBusObjectPath &session, const QString &id, qulonglong, const QVariantMap &)
{
    if (session.path() == m_session && id == ShortcutId)
        Q_EMIT released();
}

void GlobalShortcut::onShortcutsChanged(const QDBusObjectPath &session, const PortalShortcutList &shortcuts)
{
    if (session.path() == m_session)
        updateTrigger(shortcuts);
}
