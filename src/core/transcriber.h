#pragma once

#include "core/engine.h"

#include <QList>
#include <QObject>
#include <QThread>

#include <atomic>
#include <memory>

// Runs the recognition engine on a dedicated thread and keeps the model loaded between
// dictations (until unload() is called). Phrases are recognised one after another, in order.
class Transcriber : public QObject
{
    Q_OBJECT
public:
    struct Job {
        QString modelId;
        int gpu = -1; // Gpu::Device index, -1 = CPU only (Whisper; sherpa-onnx always runs on the CPU)
        Engine::Request request;
    };

    explicit Transcriber(QObject *parent = nullptr);
    ~Transcriber() override;

    void preload(const QString &modelId, int gpu);
    void transcribe(const QList<float> &pcm, const Job &job, quint64 session);
    // Abandons the running and queued phrases of `session` and every session before it.
    void cancel(quint64 session);
    void unload();

Q_SIGNALS:
    void finished(quint64 session, const QString &text);
    void failed(quint64 session, const QString &message);

private:
    bool ensureLoaded(const QString &modelId, int gpu, QString *error);

    QThread m_thread;
    QObject *m_worker = nullptr;
    std::unique_ptr<Engine> m_engine; // touched only on m_thread
    QString m_loadedId;
    int m_loadedGpu = -1;
    std::atomic<quint64> m_cancelled{0};
};
