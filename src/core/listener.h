#pragma once

#include <QList>
#include <QObject>
#include <QThread>

#include <atomic>

class Segmenter;

// Listens to the microphone stream on its own thread and hands out each phrase as soon as the
// speaker pauses, so recognition runs while the user is still talking. Uses Silero VAD when its
// model is downloaded, loudness otherwise.
class Listener : public QObject
{
    Q_OBJECT
public:
    explicit Listener(QObject *parent = nullptr);
    ~Listener() override;

    // Starts a new session and returns its number; phrases of older sessions are never reported.
    quint64 start(int pauseMs);
    void feed(const QList<float> &samples);
    void finish(); // reports the last phrase, then finished()
    void cancel();

Q_SIGNALS:
    void phrase(quint64 session, const QList<float> &pcm);
    void finished(quint64 session, bool heardSpeech);

private:
    bool current(quint64 session) const;

    QThread m_thread;
    QObject *m_worker = nullptr;
    Segmenter *m_segmenter = nullptr; // touched only on m_thread
    quint64 m_segmenterSession = 0;
    std::atomic<quint64> m_session{0};
};
