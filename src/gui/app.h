#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QTimer>

class GlobalShortcut;
class Listener;
class ModelDownloader;
class Overlay;
class Recorder;
class SettingsDialog;
class TextOutput;
class Transcriber;
class QAction;

// The dictation controller. A dictation runs as a pipeline: Recorder → Listener (cuts phrases at
// pauses) → Transcriber (recognises each phrase while the user goes on talking) → TextOutput. With
// live typing every phrase is pasted as soon as it is recognised; otherwise all of them at the end.
class App : public QObject
{
    Q_OBJECT
public:
    explicit App(QObject *parent = nullptr);
    ~App() override;

    void start();
    void command(const QString &cmd); // "toggle", "start", "stop", "cancel", "settings", "uninstall", "quit"

private:
    enum class State { Idle, Recording, Finishing };

    void toggle();
    void startRecording();
    void stopRecording();
    void cancel();
    void showSettings();
    void uninstall();

    void setState(State state);
    void onShortcutPressed();
    void onShortcutReleased();
    void onPhrase(quint64 session, const QList<float> &pcm);
    void onListenerFinished(quint64 session, bool heardSpeech);
    void onTranscribed(quint64 session, const QString &text);
    void onTranscriptionFailed(quint64 session, const QString &message);
    void finishIfDone();
    void deliver(const QString &text);
    void onError(const QString &message);
    void notify(const QString &title, const QString &body, bool warning = false);
    void scheduleUnload();

    State m_state = State::Idle;

    Recorder *m_recorder;
    Listener *m_listener;
    Transcriber *m_transcriber;
    TextOutput *m_output;
    GlobalShortcut *m_shortcut;
    ModelDownloader *m_downloader;
    Overlay *m_overlay;
    QSystemTrayIcon *m_tray;
    QAction *m_toggleAction;
    QAction *m_cancelAction;
    QPointer<SettingsDialog> m_settings;

    // The dictation in progress.
    quint64 m_session = 0;
    QString m_model;
    int m_gpu = -1;
    bool m_live = false;      // paste each phrase as it is recognised
    bool m_listening = false; // the Listener has not reported its last phrase yet
    bool m_heardSpeech = false;
    int m_pending = 0;        // phrases with the Transcriber
    int m_phrases = 0;        // recognised, not empty
    QString m_typed;          // the phrases joined into one text, as typed (Whisper's context too)

    QTimer m_unloadTimer;
    QElapsedTimer m_pressClock;
    bool m_keyDown = false;
    bool m_seenRelease = false; // compositor reports key release => hold-to-talk works
};
