#include "transcriber.h"
#include "log.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QRegularExpression>

#include <ggml-backend.h>
#include <whisper.h>

#include <algorithm>
#include <mutex>

static void quietLog(ggml_log_level level, const char *text, void *)
{
    if (level == GGML_LOG_LEVEL_ERROR || qEnvironmentVariableIsSet("SOTTO_DEBUG"))
        fputs(text, stderr);
}

static void loadBackends()
{
    static std::once_flag once;
    std::call_once(once, [] {
        ggml_log_set(quietLog, nullptr);
        whisper_log_set(quietLog, nullptr);
        // Packages that carry their own whisper.cpp name the backend folder at build time;
        // otherwise backends live in <prefix>/lib/ggml, both in /usr and inside the AppImage.
#ifdef SOTTO_GGML_BACKEND_DIR
        const QString dir = QStringLiteral(SOTTO_GGML_BACKEND_DIR);
#else
        const QString dir = QCoreApplication::applicationDirPath() + QStringLiteral("/../lib/ggml");
#endif
        if (QDir(dir).exists())
            ggml_backend_load_all_from_path(QDir(dir).canonicalPath().toLocal8Bit().constData());
        else
            ggml_backend_load_all();
    });
}

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

Transcriber::Transcriber(QObject *parent)
    : QObject(parent)
    , m_worker(new QObject)
{
    m_thread.setObjectName(QStringLiteral("whisper"));
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread.start(QThread::LowPriority);
    QMetaObject::invokeMethod(m_worker, [] { loadBackends(); });
}

Transcriber::~Transcriber()
{
    m_abort = true;
    m_thread.quit();
    m_thread.wait();
    if (m_ctx)
        whisper_free(m_ctx);
}

QString Transcriber::gpuName()
{
    loadBackends();
    for (size_t i = 0; i < ggml_backend_dev_count(); ++i) {
        ggml_backend_dev_t dev = ggml_backend_dev_get(i);
        const auto type = ggml_backend_dev_type(dev);
        if (type == GGML_BACKEND_DEVICE_TYPE_GPU || type == GGML_BACKEND_DEVICE_TYPE_IGPU)
            return QString::fromUtf8(ggml_backend_dev_description(dev));
    }
    return {};
}

void Transcriber::preload(const QString &modelPath, bool gpu)
{
    QMetaObject::invokeMethod(m_worker, [=, this] { ensureLoaded(modelPath, gpu, nullptr); });
}

void Transcriber::transcribe(std::vector<float> pcm, const Job &job)
{
    m_abort = false;
    QMetaObject::invokeMethod(m_worker, [this, pcm = std::move(pcm), job]() mutable { run(std::move(pcm), job); });
}

void Transcriber::unload()
{
    QMetaObject::invokeMethod(m_worker, [this] {
        if (m_ctx)
            whisper_free(m_ctx);
        m_ctx = nullptr;
        m_loadedPath.clear();
    });
}

bool Transcriber::ensureLoaded(const QString &modelPath, bool gpu, QString *error)
{
    if (m_ctx && m_loadedPath == modelPath && m_loadedGpu == gpu)
        return true;
    if (m_ctx)
        whisper_free(m_ctx);
    m_ctx = nullptr;
    m_loadedPath.clear();

    loadBackends();
    whisper_context_params params = whisper_context_default_params();
    params.use_gpu = gpu;
    QElapsedTimer timer;
    timer.start();
    m_ctx = whisper_init_from_file_with_params(modelPath.toLocal8Bit().constData(), params);
    qCInfo(lcSotto) << "model" << modelPath << (m_ctx ? "loaded in" : "FAILED after") << timer.elapsed() << "ms, gpu:"
                    << (gpu ? gpuName() : QStringLiteral("off"));
    if (!m_ctx) {
        if (error)
            *error = tr("Could not load the model");
        return false;
    }
    m_loadedPath = modelPath;
    m_loadedGpu = gpu;
    return true;
}

void Transcriber::run(std::vector<float> pcm, const Job &job)
{
    QString error;
    if (!ensureLoaded(job.modelPath, job.gpu, &error)) {
        Q_EMIT failed(error);
        return;
    }
    if (m_abort)
        return;

    // whisper rejects clips shorter than one second.
    if (pcm.size() < 17600)
        pcm.resize(17600, 0.f);

    const bool gpu = job.gpu && !gpuName().isEmpty();
    whisper_full_params p = whisper_full_default_params(gpu ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY);
    // On the GPU the CPU only feeds it; without one, use most cores but leave the desktop responsive.
    const int cores = QThread::idealThreadCount();
    p.n_threads = gpu ? std::clamp(cores / 2, 2, 4) : std::clamp(cores - cores / 4, 2, 16);
    p.no_context = true;
    p.no_timestamps = true;
    p.print_progress = false;
    p.print_realtime = false;
    p.print_special = false;
    p.print_timestamps = false;
    p.suppress_nst = true;
    p.beam_search.beam_size = 5;
    p.greedy.best_of = 3;

    const QByteArray language = job.language.isEmpty() ? QByteArrayLiteral("auto") : job.language.toUtf8();
    const QByteArray prompt = job.prompt.trimmed().toUtf8();
    p.language = language.constData();
    p.initial_prompt = prompt.isEmpty() ? nullptr : prompt.constData();
    if (!gpu) {
        // The encoder always works on a 30 s window; shrink it to the clip (+margin) on the CPU.
        // 50 frames per second of audio, 1500 = full window.
        const int frames = int(pcm.size() * 50 / 16000);
        p.audio_ctx = std::clamp(frames + 128, 384, 1500);
    }

    const QByteArray vadModel = job.vadModelPath.toLocal8Bit();
    if (!vadModel.isEmpty()) {
        // Decode only the parts with speech: faster, and no invented text in silence.
        p.vad = true;
        p.vad_model_path = vadModel.constData();
        p.vad_params = whisper_vad_default_params();
        p.vad_params.speech_pad_ms = 200;
        p.vad_params.min_silence_duration_ms = 300;
    }
    p.progress_callback = [](whisper_context *, whisper_state *, int percent, void *self) {
        Q_EMIT static_cast<Transcriber *>(self)->progress(percent);
    };
    p.progress_callback_user_data = this;
    p.abort_callback = [](void *flag) { return static_cast<std::atomic_bool *>(flag)->load(); };
    p.abort_callback_user_data = &m_abort;

    QElapsedTimer timer;
    timer.start();
    const int rc = whisper_full(m_ctx, p, pcm.data(), int(pcm.size()));
    if (m_abort) {
        qCInfo(lcSotto) << "transcription cancelled";
        return;
    }
    if (rc != 0) {
        qCWarning(lcSotto) << "whisper_full failed:" << rc;
        Q_EMIT failed(tr("Recognition error (%1)").arg(rc));
        return;
    }

    QString text;
    const int segments = whisper_full_n_segments(m_ctx);
    for (int i = 0; i < segments; ++i)
        text += QString::fromUtf8(whisper_full_get_segment_text(m_ctx, i));
    const QString result = cleanup(text);
    qCInfo(lcSotto) << "transcribed" << pcm.size() / 16000.0 << "s of audio in" << timer.elapsed() << "ms, gpu:" << gpu
                    << "vad:" << !vadModel.isEmpty() << "threads:" << p.n_threads << "audio_ctx:" << p.audio_ctx << "->" << result.size() << "chars";
    Q_EMIT finished(result);
}
