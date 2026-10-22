#pragma once

#include <QList>
#include <QString>

#include <functional>
#include <memory>

struct ModelInfo;

// One speech recogniser with its model loaded. Engines are created, used and destroyed on the
// transcriber's thread only.
class Engine
{
public:
    struct Request {
        QString language; // "auto" or an ISO code (Whisper only; the other models know their languages)
        QString prompt;   // names and terms, plus the end of the previous phrase (Whisper only)
    };

    virtual ~Engine() = default;

    // 16 kHz mono speech in, text out. `aborted` is polled where the engine allows it.
    // Returns false with *error set when recognition failed.
    virtual bool transcribe(const QList<float> &pcm, const Request &request, const std::function<bool()> &aborted,
                            QString *text, QString *error) = 0;

    // Loads the model for its engine; gpu is a Gpu::Device index or -1 for the CPU.
    static std::unique_ptr<Engine> load(const ModelInfo &model, int gpu, QString *error);
};
