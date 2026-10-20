#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QSystemTrayIcon>
#include <QTimer>

class GlobalShortcut;
class ModelDownloader;
class Overlay;
class Recorder;
class SettingsDialog;
class TextOutput;
class Transcriber;
class QAction;

class App : public QObject
{
    Q_OBJECT
public:
    explicit App(QObject *parent = nullptr);
    ~App() override;

    void start();
    void command(const QString &cmd); // "toggle", "start", "stop", "cancel", "settings"

    void toggle();
    void startRecording();
    void stopRecording();
    void cancel();
    void showSettings();
    void uninstall();

private:
    enum class State { Idle, Recording, Transcribing };

    void setState(State state);
    void onShortcutPressed();
    void onShortcutReleased();
    void onTranscribed(const QString &text);
    void onError(const QString &message);
    void scheduleUnload();

    State m_state = State::Idle;

    Recorder *m_recorder;
    Transcriber *m_transcriber;
    TextOutput *m_output;
    GlobalShortcut *m_shortcut;
    ModelDownloader *m_downloader;
    Overlay *m_overlay;
    QSystemTrayIcon *m_tray;
    QAction *m_toggleAction;
    QAction *m_cancelAction;
    QPointer<SettingsDialog> m_settings;

    QTimer m_unloadTimer;
    QElapsedTimer m_pressClock;
    bool m_keyDown = false;
    bool m_seenRelease = false; // compositor reports key release => hold-to-talk works
};
