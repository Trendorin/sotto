#pragma once

#include <QAudioFormat>
#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QTimer>

class QAudioSource;
class QIODevice;

// Captures the microphone as 16 kHz mono float, which is what the recognisers want, hands it
// out every 20 ms and reports a perceptual input level for the indicator.
class Recorder : public QObject
{
    Q_OBJECT
public:
    static constexpr int SampleRate = 16000;

    explicit Recorder(QObject *parent = nullptr);
    ~Recorder() override;

    bool start(const QByteArray &deviceId);
    void stop();

Q_SIGNALS:
    void audio(const QList<float> &samples);
    void level(float value); // 0..1
    void failed(const QString &message);

private:
    void readAvailable();
    void consume(const char *data, qint64 bytes);

    QAudioSource *m_source = nullptr;
    QIODevice *m_io = nullptr;
    QAudioFormat m_format;
    QTimer m_poll;
    QList<float> m_chunk;
    QElapsedTimer m_started; // until the first audio arrives: how long the device takes to open

    // Box-filter resampler state, used only if the device refuses 16 kHz.
    double m_step = 1.0;
    double m_phase = 0.0;
    double m_acc = 0.0;
    int m_accCount = 0;
};
