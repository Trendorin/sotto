#include "system/portal.h"
#include "core/config.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QDBusVariant>

namespace Portal {

static const QString RequestIface = QStringLiteral("org.freedesktop.portal.Request");

QString newToken()
{
    static uint counter = 0;
    return QStringLiteral("sotto%1").arg(++counter);
}

static QString requestPath(const QString &token)
{
    QString sender = QDBusConnection::sessionBus().baseService().mid(1);
    sender.replace(QLatin1Char('.'), QLatin1Char('_'));
    return QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, token);
}

void request(const QString &interface, const QString &method, QVariantList args,
             QVariantMap options, Callback cb)
{
    auto bus = QDBusConnection::sessionBus();
    const QString token = newToken();
    options.insert(QStringLiteral("handle_token"), token);
    args.append(options);

    // Subscribe before calling so a fast Response can't slip past us.
    const QString path = requestPath(token);
    auto *req = new PortalRequest(path, std::move(cb));
    bus.connect(Service, path, RequestIface, QStringLiteral("Response"), req,
                SLOT(onResponse(uint, QVariantMap)));

    auto msg = QDBusMessage::createMethodCall(Service, Path, interface, method);
    msg.setArguments(args);
    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(msg), req);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, req, [req](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        if (w->isError())
            req->finish(2, {{QStringLiteral("error"), w->error().message()}});
    });
}

void call(const QString &interface, const QString &method, const QVariantList &args)
{
    auto msg = QDBusMessage::createMethodCall(Service, Path, interface, method);
    msg.setArguments(args);
    QDBusConnection::sessionBus().asyncCall(msg);
}

uint version(const QString &interface)
{
    auto msg = QDBusMessage::createMethodCall(Service, Path, QStringLiteral("org.freedesktop.DBus.Properties"),
                                              QStringLiteral("Get"));
    msg.setArguments({interface, QStringLiteral("version")});
    QDBusReply<QDBusVariant> reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 3000);
    return reply.isValid() ? reply.value().variant().toUInt() : 0;
}

void registerApp()
{
    auto msg = QDBusMessage::createMethodCall(Service, Path, QStringLiteral("org.freedesktop.host.portal.Registry"),
                                              QStringLiteral("Register"));
    msg.setArguments({QString::fromLatin1(Config::AppId), QVariantMap()});
    // Fails harmlessly on old portals or once we are already registered.
    QDBusConnection::sessionBus().call(msg, QDBus::Block, 3000);
}

QString objectPath(const QVariant &value)
{
    if (value.canConvert<QDBusObjectPath>())
        return value.value<QDBusObjectPath>().path();
    return value.toString();
}

} // namespace Portal

QDBusArgument &operator<<(QDBusArgument &arg, const PortalShortcut &s)
{
    arg.beginStructure();
    arg << s.id << s.properties;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, PortalShortcut &s)
{
    arg.beginStructure();
    arg >> s.id >> s.properties;
    arg.endStructure();
    return arg;
}

PortalRequest::PortalRequest(QString path, Portal::Callback cb)
    : m_path(std::move(path))
    , m_cb(std::move(cb))
{
}

void PortalRequest::onResponse(uint response, const QVariantMap &results)
{
    finish(response, results);
}

void PortalRequest::finish(uint response, const QVariantMap &results)
{
    if (m_done)
        return;
    m_done = true;
    QDBusConnection::sessionBus().disconnect(Portal::Service, m_path, Portal::RequestIface, QStringLiteral("Response"),
                                             this, SLOT(onResponse(uint, QVariantMap)));
    if (m_cb)
        m_cb(response, results);
    deleteLater();
}
