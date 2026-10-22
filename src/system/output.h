#pragma once

#include <QObject>
#include <QStringList>

#include <functional>
#include <memory>

class QWindow;

// Puts recognised text where the user is typing: the clipboard plus a paste key combination,
// pressed through the RemoteDesktop portal (KDE, GNOME, COSMIC) or, where the compositor has no
// such portal (Hyprland, niri, sway, ...), through wtype, ydotool or xdotool. Texts handed in
// quickly one after another (live typing) are pasted in order, never over each other.
class TextOutput : public QObject
{
    Q_OBJECT
public:
    explicit TextOutput(QObject *parent = nullptr);
    ~TextOutput() override;

    void deliver(const QString &text);

    // Asks for keyboard access now (shows the system permission dialog once),
    // so it doesn't pop up in the middle of the first dictation.
    void prepare();
    bool hasAccess() const { return m_ready; }

    // The ways to press keys here, best first: "portal", "wtype", "ydotool", "xdotool".
    static QStringList available();
    // What the configured method means here ("auto" resolved), empty when there is none.
    static QString effectiveMethod();

Q_SIGNALS:
    void failed(const QString &message);
    void accessChanged(bool granted);

private Q_SLOTS:
    void onSessionClosed();

private:
    void pump();
    void copyAndPaste(const QString &text);
    bool copyToClipboard(const QString &text);
    void paste(QStringList methods, QList<int> keys);
    void finishItem();
    void withSession(std::function<void(bool)> ready);
    void sendPortalKeys(const QList<int> &keycodes);

    QStringList m_queue;
    bool m_busy = false;
    QString m_session;
    bool m_ready = false;
    bool m_starting = false;
    QList<std::function<void(bool)>> m_waiting;
    std::unique_ptr<QWindow> m_stamp; // X11 only: see deliver()
};
