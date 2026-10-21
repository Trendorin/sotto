#pragma once

#include <QObject>

#include <functional>
#include <memory>

class QWindow;

// Puts recognised text where the user is typing: clipboard + a paste key
// combination sent through org.freedesktop.portal.RemoteDesktop.
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

Q_SIGNALS:
    void failed(const QString &message);
    void accessChanged(bool granted);

private Q_SLOTS:
    void onSessionClosed();

private:
    void copyAndPaste(const QString &text);
    bool copyToClipboard(const QString &text);
    void withSession(std::function<void(bool)> ready);
    void sendKeys(const QList<int> &keycodes);

    QString m_session;
    bool m_ready = false;
    bool m_starting = false;
    QList<std::function<void(bool)>> m_waiting;
    std::unique_ptr<QWindow> m_stamp; // X11 only: see deliver()
};
