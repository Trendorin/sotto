#include "recorder.h"
#include "log.h"

#include <QAudioDevice>
#include <QAudioSource>
#include <QMediaDevices>

#include <cmath>

static constexpr size_t MaxSamples = size_t(Recorder::SampleRate) * 60 * 15; // 15 minutes

Recorder::Recorder(QObject *parent)
    : QObject(parent)
{
    // Drain the device on a steady clock instead of relying on readyRead alone,
    // so a busy moment on the GUI thread never drops audio.
    m_poll.setInterval(20);
    m_poll.setTimerType(Qt::PreciseTimer);
    connect(&m_poll, &QTimer::timeout, this, &Recorder::readAvailable);
    // The audio backend initialises lazily (~0.5 s); do it now, not when the user starts talking.
    QTimer::singleShot(0, this, [] { QMediaDevices::defaultAudioInput(); });
}

Recorder::~Recorder()
{
    stop();
}

bool Recorder::start(const QByteArray &deviceId)
{
    stop();

    QAudioDevice device = QMediaDevices::defaultAudioInput();
    if (!deviceId.isEmpty()) {
        for (const QAudioDevice &d : QMediaDevices::audioInputs()) {
            if (d.id() == deviceId) {
                device = d;
                break;
            }
        }
    }
    if (device.isNull()) {
        Q_EMIT failed(tr("No microphone found"));
        return false;
    }

    QAudioFormat format;
    format.setSampleRate(SampleRate);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Float);
    if (!device.isFormatSupported(format))
        format = device.preferredFormat();

    m_format = format;
    m_step = double(format.sampleRate()) / SampleRate;
    m_phase = m_acc = 0.0;
    m_accCount = 0;
    m_pcm.clear();
    m_pcm.reserve(SampleRate * 30);

    qCInfo(lcSotto) << "recording from" << device.description() << device.id() << "format" << format.sampleRate()
                    << "Hz" << format.channelCount() << "ch" << format.sampleFormat();
    m_source = new QAudioSource(device, format, this);
    m_source->setBufferSize(format.bytesForDuration(500'000)); // headroom; we read every 20 ms anyway
    m_io = m_source->start();
    if (!m_io || m_source->error() != QAudio::NoError) {
        qCWarning(lcSotto) << "cannot open microphone, error" << m_source->error();
        delete m_source;
        m_source = nullptr;
        m_io = nullptr;
        Q_EMIT failed(tr("Could not open the microphone"));
        return false;
    }
    connect(m_io, &QIODevice::readyRead, this, &Recorder::readAvailable);
    m_poll.start();
    connect(m_source, &QAudioSource::stateChanged, this, [this](QAudio::State state) {
        if (state == QAudio::StoppedState && m_source && m_source->error() != QAudio::NoError)
            Q_EMIT failed(tr("The microphone was disconnected"));
    });
    return true;
}

std::vector<float> Recorder::stop()
{
    if (!m_source)
        return {};
    m_poll.stop();
    readAvailable();
    m_source->disconnect(this);
    m_source->stop();
    m_source->deleteLater();
    m_source = nullptr;
    m_io = nullptr;
    qCInfo(lcSotto) << "recorded" << m_pcm.size() / double(SampleRate) << "s";
    return std::exchange(m_pcm, {});
}

void Recorder::readAvailable()
{
    if (!m_io)
        return;
    const QByteArray data = m_io->readAll();
    if (!data.isEmpty())
        consume(data.constData(), data.size());
}

void Recorder::consume(const char *data, qint64 bytes)
{
    const int channels = m_format.channelCount();
    const int bytesPerSample = m_format.bytesPerSample();
    const qint64 frames = bytes / (qint64(bytesPerSample) * channels);
    if (frames <= 0)
        return;

    double sumSquares = 0.0;
    for (qint64 i = 0; i < frames; ++i) {
        float sample = 0.f;
        for (int c = 0; c < channels; ++c)
            sample += m_format.normalizedSampleValue(data + (i * channels + c) * bytesPerSample);
        sample /= float(channels);
        sumSquares += double(sample) * sample;

        if (m_pcm.size() >= MaxSamples)
            continue;
        if (m_step == 1.0) {
            m_pcm.push_back(sample);
        } else if (m_step > 1.0) {
            m_acc += sample;
            ++m_accCount;
            m_phase += 1.0;
            if (m_phase >= m_step) {
                m_pcm.push_back(float(m_acc / m_accCount));
                m_acc = 0.0;
                m_accCount = 0;
                m_phase -= m_step;
            }
        } else {
            for (; m_phase < 1.0; m_phase += m_step)
                m_pcm.push_back(sample);
            m_phase -= 1.0;
        }
    }

    // Map RMS from roughly -60..-12 dBFS onto 0..1 so quiet speech still moves.
    const double rms = std::sqrt(sumSquares / double(frames));
    const double db = 20.0 * std::log10(rms + 1e-9);
    Q_EMIT level(float(std::clamp((db + 60.0) / 48.0, 0.0, 1.0)));
}
