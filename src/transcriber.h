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
        bool gpu = true;
        QString language;
        QString prompt;
        QString vadModelPath; // empty = no voice activity detection
    };

    explicit Transcriber(QObject *parent = nullptr);
    ~Transcriber() override;

    // Name of the GPU ggml can use, empty if none (loads ggml backends on first call).
    static QString gpuName();

    void preload(const QString &modelPath, bool gpu);
    void transcribe(std::vector<float> pcm, const Job &job);
    void cancel() { m_abort = true; }
    void unload();

Q_SIGNALS:
    void progress(int percent);
    void finished(const QString &text);
    void failed(const QString &message);

private:
    bool ensureLoaded(const QString &modelPath, bool gpu, QString *error);
    void run(std::vector<float> pcm, const Job &job);

    QThread m_thread;
    QObject *m_worker = nullptr;
    whisper_context *m_ctx = nullptr; // touched only on m_thread
    QString m_loadedPath;
    bool m_loadedGpu = false;
    std::atomic_bool m_abort{false};
};
