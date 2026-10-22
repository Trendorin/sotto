#pragma once

#include <QDBusArgument>
#include <QList>
#include <QObject>
#include <QVariantMap>

#include <functional>

// Minimal helpers for the xdg-desktop-portal request/response dance.
namespace Portal {

inline const QString Service = QStringLiteral("org.freedesktop.portal.Desktop");
inline const QString Path = QStringLiteral("/org/freedesktop/portal/desktop");

// response: 0 = success, 1 = cancelled by user, 2 = other error.
using Callback = std::function<void(uint response, const QVariantMap &results)>;

QString newToken();

// Calls `method` on `interface`. `options` (the trailing a{sv} argument) gets a
// handle_token added automatically; `cb` runs once the Response signal arrives.
void request(const QString &interface, const QString &method, QVariantList args,
             QVariantMap options, Callback cb);

// Plain method call without a Request object.
void call(const QString &interface, const QString &method, const QVariantList &args);

uint version(const QString &interface);

// Tells the portal which .desktop file we are (needed outside of Flatpak).
void registerApp();

QString objectPath(const QVariant &value);

} // namespace Portal

struct PortalShortcut {
    QString id;
    QVariantMap properties;
};
using PortalShortcutList = QList<PortalShortcut>;
Q_DECLARE_METATYPE(PortalShortcut)

QDBusArgument &operator<<(QDBusArgument &arg, const PortalShortcut &s);
const QDBusArgument &operator>>(const QDBusArgument &arg, PortalShortcut &s);

class PortalRequest : public QObject
{
    Q_OBJECT
public:
    PortalRequest(QString path, Portal::Callback cb);
    void finish(uint response, const QVariantMap &results);

public Q_SLOTS:
    void onResponse(uint response, const QVariantMap &results);

private:
    QString m_path;
    Portal::Callback m_cb;
    bool m_done = false;
};
