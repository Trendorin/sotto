#include "core/whisperengine.h"
#include "core/gpu.h"
#include "core/log.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QThread>

#include <whisper.h>

#include <algorithm>

// Whisper likes to "hear" these in silence or noise; drop them when they are the whole result.
static QString cleanup(QString text)
{
    static const QRegularExpression annotations(QStringLiteral(R"(\[[^\]]{0,40}\]|\*[^*]{0,40}\*)"));
    text.remove(annotations);
    text = text.simplified();

    static const QStringList hallucinations{
        QStringLiteral("Продолжение следует..."),
        QStringLiteral("Спасибо за просмотр!"),
        QStringLiteral("Thank you for watching!"),
        QStringLiteral("Thanks for watching!"),
        QStringLiteral("Thank you."),
        QStringLiteral("you"),
    };
    for (const QString &h : hallucinations)
        if (text.compare(h, Qt::CaseInsensitive) == 0)
            return {};
    if (text.contains(QLatin1String("DimaTorzok")) || text.contains(QLatin1String("А.Синецкая")))
        return {};
    return text;
}

static QString gpuName(int index)
{
    for (const Gpu::Device &d : Gpu::devices())
        if (d.index == index)
            return d.name;
    return {};
}

std::unique_ptr<Engine> WhisperEngine::load(const QString &modelPath, int gpu, QString *error)
{
    if (!Gpu::haveCpuBackend()) {
        *error = QCoreApplication::translate("Engine", "whisper.cpp is incomplete: its ggml backends are missing. Reinstall Sotto.");
        return nullptr;
    }
    whisper_context_params params = whisper_context_default_params();
    params.use_gpu = gpu >= 0;
    params.gpu_device = std::max(gpu, 0);
    QElapsedTimer timer;
    timer.start();
    whisper_context *ctx = whisper_init_from_file_with_params(modelPath.toLocal8Bit().constData(), params);
    qCInfo(lcSotto) << "whisper model" << modelPath << (ctx ? "loaded in" : "FAILED after") << timer.elapsed() << "ms, gpu:"
                    << (gpu >= 0 ? gpuName(gpu) : QStringLiteral("off"));
    if (!ctx) {
        *error = QCoreApplication::translate("Engine", "Could not load the model");
        return nullptr;
    }
    return std::unique_ptr<Engine>(new WhisperEngine(ctx, gpu));
}

WhisperEngine::WhisperEngine(whisper_context *ctx, int gpu)
    : m_ctx(ctx)
    , m_gpu(gpu)
{
}

WhisperEngine::~WhisperEngine()
{
    whisper_free(m_ctx);
}

bool WhisperEngine::transcribe(const QList<float> &input, const Request &request, const std::function<bool()> &aborted,
                               QString *text, QString *error)
{
    QList<float> pcm = input;
    // whisper rejects clips shorter than one second.
    if (pcm.size() < 17600)
        pcm.resize(17600, 0.f);

    const bool gpu = m_gpu >= 0;
    whisper_full_params p = whisper_full_default_params(gpu ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY);
    // On the GPU the CPU only feeds it; without one, use most cores but leave one for the audio and the desktop.
    const int cores = QThread::idealThreadCount();
    p.n_threads = gpu ? std::clamp(cores / 2, 1, 4) : std::clamp(cores - std::max(1, cores / 4), 1, 16);
    p.no_context = true;
    p.no_timestamps = true;
    p.print_progress = false;
    p.print_realtime = false;
    p.print_special = false;
    p.print_timestamps = false;
    p.suppress_nst = true;
    p.beam_search.beam_size = 5;
    p.greedy.best_of = 3;

    const QByteArray language = request.language.isEmpty() ? QByteArrayLiteral("auto") : request.language.toUtf8();
    const QByteArray prompt = request.prompt.trimmed().toUtf8();
    p.language = language.constData();
    p.initial_prompt = prompt.isEmpty() ? nullptr : prompt.constData();
    if (!gpu) {
        // The encoder always works on a 30 s window; shrink it to the clip (+margin) on the CPU.
        // 50 frames per second of audio, 1500 = full window.
        const int frames = int(pcm.size() * 50 / 16000);
        p.audio_ctx = std::clamp(frames + 128, 384, 1500);
    }
    p.abort_callback = [](void *data) { return (*static_cast<const std::function<bool()> *>(data))(); };
    p.abort_callback_user_data = const_cast<std::function<bool()> *>(&aborted);

    QElapsedTimer timer;
    timer.start();
    const int rc = whisper_full(m_ctx, p, pcm.constData(), int(pcm.size()));
    if (aborted())
        return true;
    if (rc != 0) {
        qCWarning(lcSotto) << "whisper_full failed:" << rc;
        *error = QCoreApplication::translate("Engine", "Recognition error (%1)").arg(rc);
        return false;
    }

    QString raw;
    const int segments = whisper_full_n_segments(m_ctx);
    for (int i = 0; i < segments; ++i)
        raw += QString::fromUtf8(whisper_full_get_segment_text(m_ctx, i));
    *text = cleanup(raw);
    qCInfo(lcSotto) << "whisper:" << pcm.size() / 16000.0 << "s in" << timer.elapsed() << "ms, threads:" << p.n_threads
                    << "audio_ctx:" << p.audio_ctx << "->" << text->size() << "chars";
    return true;
}
