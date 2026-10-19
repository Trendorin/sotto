#pragma once

#include <QAudioFormat>
#include <QObject>
#include <QTimer>

#include <vector>

class QAudioSource;
class QIODevice;

// Captures the microphone as 16 kHz mono float, which is what whisper wants,
// and reports a perceptual input level for the indicator.
class Recorder : public QObject
{
    Q_OBJECT
public:
    static constexpr int SampleRate = 16000;

    explicit Recorder(QObject *parent = nullptr);
    ~Recorder() override;

    bool start(const QByteArray &deviceId);
    std::vector<float> stop();
    bool isActive() const { return m_source != nullptr; }

Q_SIGNALS:
    void level(float value); // 0..1
    void failed(const QString &message);

private:
    void readAvailable();
    void consume(const char *data, qint64 bytes);

    QAudioSource *m_source = nullptr;
    QIODevice *m_io = nullptr;
    QAudioFormat m_format;
    QTimer m_poll;
    std::vector<float> m_pcm;

    // Box-filter resampler state, used only if the device refuses 16 kHz.
    double m_step = 1.0;
    double m_phase = 0.0;
    double m_acc = 0.0;
    int m_accCount = 0;
};
