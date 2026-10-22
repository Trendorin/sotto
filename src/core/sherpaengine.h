#pragma once

#include "core/engine.h"

struct SherpaOnnxOfflineRecognizer;

// sherpa-onnx on ONNX Runtime: NeMo transducers such as GigaAM v3 and Parakeet TDT v3. CPU only,
// and fast there: a phrase takes a fraction of its length to recognise.
class SherpaEngine : public Engine
{
public:
    // folder: a model folder with encoder*.onnx, decoder*.onnx, joiner*.onnx and tokens.txt
    static std::unique_ptr<Engine> load(const QString &folder, QString *error);
    ~SherpaEngine() override;

    bool transcribe(const QList<float> &pcm, const Request &request, const std::function<bool()> &aborted,
                    QString *text, QString *error) override;

private:
    explicit SherpaEngine(const SherpaOnnxOfflineRecognizer *recognizer);

    const SherpaOnnxOfflineRecognizer *m_recognizer;
};
