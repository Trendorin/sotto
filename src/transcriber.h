#pragma once

#include <QObject>
#include <QThread>

#include <atomic>
#include <vector>

struct whisper_context;

// Runs whisper.cpp on a dedicated thread and keeps the model loaded between
// dictations (until unload() is called).
class Transcriber : public QObject
{
    Q_OBJECT
public:
    struct Job {
        QString modelPath;
        int gpu = -1; // Gpu::index, -1 = CPU only
        QString language;
        QString prompt;
        QString vadModelPath; // empty = no voice activity detection
    };

    explicit Transcriber(QObject *parent = nullptr);
    ~Transcriber() override;

    struct Gpu {
        int index = -1;  // among ggml's GPUs: the numbering of whisper_context_params::gpu_device
        QString key;     // "<backend>/<description>", what the settings store
        QString name;
        bool discrete = false;
        quint64 memory = 0; // bytes
    };

    // GPUs ggml can use, best first (loads ggml backends on the first call).
    static const QList<Gpu> &gpus();
    // The GPU for a stored key: that one, or the best one when the key is empty or the GPU is gone.
    // index is -1 when there is no GPU.
    static Gpu gpu(const QString &key);
    // Why a graphics card in the computer is not among gpus(); empty when nothing is missing.
    static QString missingGpuHint();

    void preload(const QString &modelPath, int gpu);
    void transcribe(std::vector<float> pcm, const Job &job);
    void cancel() { ++m_generation; } // abandons the running and any queued job
    void unload();

Q_SIGNALS:
    void progress(int percent);
    void finished(const QString &text);
    void failed(const QString &message);

private:
    static QString missingGpuHint(const QList<Gpu> &found);
    bool ensureLoaded(const QString &modelPath, int gpu, QString *error);
    void run(std::vector<float> pcm, const Job &job, quint64 generation);

    QThread m_thread;
    QObject *m_worker = nullptr;
    whisper_context *m_ctx = nullptr; // touched only on m_thread
    QString m_loadedPath;
    int m_loadedGpu = -1;
    // Every job gets a number; cancelling or starting another job changes the current one, so a
    // cancelled job can never be revived by the next recording and never reports its result.
    std::atomic<quint64> m_generation{0};
};
