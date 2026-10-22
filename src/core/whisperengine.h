#pragma once

#include "core/engine.h"

struct whisper_context;

// whisper.cpp: 99 languages, runs on Vulkan or CUDA when ggml finds a GPU.
class WhisperEngine : public Engine
{
public:
    static std::unique_ptr<Engine> load(const QString &modelPath, int gpu, QString *error);
    ~WhisperEngine() override;

    bool transcribe(const QList<float> &pcm, const Request &request, const std::function<bool()> &aborted,
                    QString *text, QString *error) override;

private:
    WhisperEngine(whisper_context *ctx, int gpu);

    whisper_context *m_ctx;
    int m_gpu;
};
