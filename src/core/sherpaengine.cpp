#include "core/sherpaengine.h"
#include "core/log.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QThread>

#include <sherpa-onnx/c-api/c-api.h>

#include <algorithm>
#include <cstring>

// The file of a role ("encoder", "decoder", "joiner"): the int8 one when both are there.
static QString pick(const QDir &dir, const QString &role)
{
    const QStringList found = dir.entryList({role + QStringLiteral("*.onnx")}, QDir::Files, QDir::Name);
    for (const QString &f : found)
        if (f.contains(QLatin1String(".int8.")))
            return dir.filePath(f);
    return found.isEmpty() ? QString() : dir.filePath(found.constFirst());
}

std::unique_ptr<Engine> SherpaEngine::load(const QString &folder, QString *error)
{
    const QDir dir(folder);
    const QByteArray encoder = pick(dir, QStringLiteral("encoder")).toLocal8Bit();
    const QByteArray decoder = pick(dir, QStringLiteral("decoder")).toLocal8Bit();
    const QByteArray joiner = pick(dir, QStringLiteral("joiner")).toLocal8Bit();
    const QByteArray tokens = dir.filePath(QStringLiteral("tokens.txt")).toLocal8Bit();
    if (encoder.isEmpty() || decoder.isEmpty() || joiner.isEmpty() || !QFileInfo::exists(QString::fromLocal8Bit(tokens))) {
        *error = QCoreApplication::translate("Engine", "Could not load the model");
        return nullptr;
    }

    SherpaOnnxOfflineRecognizerConfig config;
    std::memset(&config, 0, sizeof config);
    config.feat_config.sample_rate = 16000;
    config.feat_config.feature_dim = 80; // NeMo models read their own from the model metadata
    config.model_config.transducer.encoder = encoder.constData();
    config.model_config.transducer.decoder = decoder.constData();
    config.model_config.transducer.joiner = joiner.constData();
    config.model_config.tokens = tokens.constData();
    config.model_config.model_type = "nemo_transducer";
    config.model_config.provider = "cpu";
    // Leave a core for the audio and the desktop.
    const int cores = QThread::idealThreadCount();
    config.model_config.num_threads = std::clamp(cores - std::max(1, cores / 4), 1, 8);
    config.model_config.debug = qEnvironmentVariableIsSet("SOTTO_DEBUG") ? 1 : 0;
    config.decoding_method = "greedy_search";

    QElapsedTimer timer;
    timer.start();
    const SherpaOnnxOfflineRecognizer *recognizer = SherpaOnnxCreateOfflineRecognizer(&config);
    qCInfo(lcSotto) << "sherpa-onnx" << SherpaOnnxGetVersionStr() << "model" << folder << (recognizer ? "loaded in" : "FAILED after")
                    << timer.elapsed() << "ms, threads:" << config.model_config.num_threads;
    if (!recognizer) {
        *error = QCoreApplication::translate("Engine", "Could not load the model");
        return nullptr;
    }
    return std::unique_ptr<Engine>(new SherpaEngine(recognizer));
}

SherpaEngine::SherpaEngine(const SherpaOnnxOfflineRecognizer *recognizer)
    : m_recognizer(recognizer)
{
}

SherpaEngine::~SherpaEngine()
{
    SherpaOnnxDestroyOfflineRecognizer(m_recognizer);
}

bool SherpaEngine::transcribe(const QList<float> &pcm, const Request &, const std::function<bool()> &aborted, QString *text,
                              QString *)
{
    if (aborted())
        return true;
    QElapsedTimer timer;
    timer.start();
    const SherpaOnnxOfflineStream *stream = SherpaOnnxCreateOfflineStream(m_recognizer);
    SherpaOnnxAcceptWaveformOffline(stream, 16000, pcm.constData(), int32_t(pcm.size()));
    SherpaOnnxDecodeOfflineStream(m_recognizer, stream);
    const SherpaOnnxOfflineRecognizerResult *result = SherpaOnnxGetOfflineStreamResult(stream);
    *text = result && result->text ? QString::fromUtf8(result->text).simplified() : QString();
    SherpaOnnxDestroyOfflineRecognizerResult(result);
    SherpaOnnxDestroyOfflineStream(stream);
    qCInfo(lcSotto) << "sherpa-onnx:" << pcm.size() / 16000.0 << "s in" << timer.elapsed() << "ms ->" << text->size() << "chars";
    return true;
}
