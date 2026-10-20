#pragma once

#include "portal.h"

#include <QDBusObjectPath>
#include <QObject>

// Global hotkey through org.freedesktop.portal.GlobalShortcuts
// (KDE Plasma, GNOME 48+, Hyprland, ...).
class GlobalShortcut : public QObject
{
    Q_OBJECT
public:
    explicit GlobalShortcut(QObject *parent = nullptr);

    void setup();
    void configure();

    bool isAvailable() const { return m_bound; }
    QString trigger() const { return m_trigger; }
    QString error() const { return m_error; }

Q_SIGNALS:
    void pressed();
    void released();
    void changed(); // availability or trigger changed

private Q_SLOTS:
    void onActivated(const QDBusObjectPath &session, const QString &id, qulonglong timestamp, const QVariantMap &opts);
    void onDeactivated(const QDBusObjectPath &session, const QString &id, qulonglong timestamp, const QVariantMap &opts);
    void onShortcutsChanged(const QDBusObjectPath &session, const PortalShortcutList &shortcuts);

private:
    void bind();
    void updateTrigger(const PortalShortcutList &shortcuts);
    void fail(const QString &reason);

    QString m_session;
    QString m_trigger;
    QString m_error;
    uint m_version = 0;
    int m_attempts = 0;
    bool m_bound = false;
};
