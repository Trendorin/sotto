#include "transcriber.h"
#include "log.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
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

// Packages that carry their own whisper.cpp name the backend folder at build time;
// otherwise backends live in <prefix>/lib/ggml, both in /usr and inside the AppImage.
static QString backendDir()
{
#ifdef SOTTO_GGML_BACKEND_DIR
    return QStringLiteral(SOTTO_GGML_BACKEND_DIR);
#else
    return QCoreApplication::applicationDirPath() + QStringLiteral("/../lib/ggml");
#endif
}

static void loadBackends()
{
    static std::once_flag once;
    std::call_once(once, [] {
        ggml_log_set(quietLog, nullptr);
        whisper_log_set(quietLog, nullptr);
        const QString dir = backendDir();
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
    ++m_generation;
    m_thread.quit();
    m_thread.wait();
    if (m_ctx)
        whisper_free(m_ctx);
}

// whisper.cpp takes the first GPU ggml lists, and on laptops with two the integrated one usually
// comes first. List discrete GPUs first, then the ones with more memory.
const QList<Transcriber::Gpu> &Transcriber::gpus()
{
    static const QList<Gpu> list = [] {
        loadBackends();
        QList<Gpu> found;
        for (size_t i = 0; i < ggml_backend_dev_count(); ++i) {
            ggml_backend_dev_t dev = ggml_backend_dev_get(i);
            const auto type = ggml_backend_dev_type(dev);
            if (type != GGML_BACKEND_DEVICE_TYPE_GPU && type != GGML_BACKEND_DEVICE_TYPE_IGPU)
                continue;
            size_t free = 0, total = 0;
            ggml_backend_dev_memory(dev, &free, &total);
            Gpu gpu;
            gpu.index = int(found.size());
            gpu.name = QString::fromUtf8(ggml_backend_dev_description(dev));
            gpu.key = QString::fromUtf8(ggml_backend_reg_name(ggml_backend_dev_backend_reg(dev))) + QLatin1Char('/') + gpu.name;
            gpu.discrete = type == GGML_BACKEND_DEVICE_TYPE_GPU;
            gpu.memory = total;
            qCInfo(lcSotto) << "gpu" << gpu.index << gpu.key << (gpu.discrete ? "discrete" : "integrated") << total / (1024 * 1024) << "MiB";
            found.append(gpu);
        }
        std::stable_sort(found.begin(), found.end(), [](const Gpu &a, const Gpu &b) {
            return a.discrete != b.discrete ? a.discrete : a.memory > b.memory;
        });
        if (const QString hint = missingGpuHint(found); !hint.isEmpty())
            qCWarning(lcSotto) << "missing gpu:" << hint;
        return found;
    }();
    return list;
}

Transcriber::Gpu Transcriber::gpu(const QString &key)
{
    const QList<Gpu> &list = gpus();
    for (const Gpu &g : list)
        if (g.key == key)
            return g;
    return list.isEmpty() ? Gpu() : list.constFirst();
}

static QString readSysfs(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? QString::fromLatin1(f.readAll()).trimmed() : QString();
}

// Versions of the NVIDIA user-space driver installed, from its library names (libnvidia-glcore.so.580.95.05).
static QStringList nvidiaLibraryVersions()
{
    QStringList versions;
    for (const char *dir : {"/usr/lib", "/usr/lib64", "/usr/lib/x86_64-linux-gnu"})
        for (const QString &file : QDir(QString::fromLatin1(dir)).entryList({QStringLiteral("libnvidia-glcore.so.*")}, QDir::Files))
            versions.append(file.mid(int(qstrlen("libnvidia-glcore.so."))));
    return versions;
}

static bool haveVulkanIcd(const QString &pattern)
{
    for (const char *dir : {"/usr/share/vulkan/icd.d", "/etc/vulkan/icd.d", "/usr/local/share/vulkan/icd.d"})
        if (!QDir(QString::fromLatin1(dir)).entryList({pattern}, QDir::Files).isEmpty())
            return true;
    return false;
}

QString Transcriber::missingGpuHint()
{
    return missingGpuHint(gpus());
}

// A discrete card ggml does not list is the usual reason recognition runs on the integrated GPU or the
// CPU. Find the graphics cards on the PCI bus and say why the best one is missing.
QString Transcriber::missingGpuHint(const QList<Gpu> &found)
{
    if (std::any_of(found.begin(), found.end(), [](const Gpu &g) { return g.discrete; }))
        return {};
    const auto listed = [&](std::initializer_list<const char *> words) {
        return std::any_of(found.begin(), found.end(), [&](const Gpu &g) {
            return std::any_of(words.begin(), words.end(), [&](const char *w) { return g.name.contains(QLatin1String(w), Qt::CaseInsensitive); });
        });
    };
    bool nvidia = false, amd = false, intel = false;
    const QDir pci(QStringLiteral("/sys/bus/pci/devices"));
    for (const QString &dev : pci.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!readSysfs(pci.filePath(dev + QStringLiteral("/class"))).startsWith(QLatin1String("0x03")))
            continue; // not a display controller
        const QString vendor = readSysfs(pci.filePath(dev + QStringLiteral("/vendor")));
        nvidia |= vendor == QLatin1String("0x10de");
        amd |= vendor == QLatin1String("0x1002");
        intel |= vendor == QLatin1String("0x8086");
    }

    // A backend that finds no device is not loaded at all, so look for its library instead.
    const QDir backends(backendDir());
    if ((nvidia || amd || intel) && backends.exists()
        && backends.entryList({QStringLiteral("libggml-vulkan.so"), QStringLiteral("libggml-cuda.so")}, QDir::Files).isEmpty())
        return tr("The GPU part of ggml is not installed: install ggml-vulkan on Arch.");
    if (nvidia) { // a card ggml lists would be discrete
        const QString kernel = readSysfs(QStringLiteral("/sys/module/nvidia/version"));
        if (kernel.isEmpty() && QDir(QStringLiteral("/sys/module/nouveau")).exists())
            return tr("The NVIDIA card runs on the nouveau driver, which gives it no Vulkan here. "
                      "Install the NVIDIA driver, or Mesa's NVK (vulkan-nouveau) for recent cards.");
        if (kernel.isEmpty())
            return tr("The NVIDIA card has no driver loaded. It may be switched off (integrated-only mode in "
                      "envycontrol, supergfxctl or optimus-manager: choose hybrid), or the NVIDIA driver is not installed.");
        const QStringList libraries = nvidiaLibraryVersions();
        if (!libraries.isEmpty() && !libraries.contains(kernel))
            return tr("The NVIDIA driver was updated (kernel module %1, libraries %2): restart the computer.")
                .arg(kernel, libraries.join(QStringLiteral(", ")));
        if (!haveVulkanIcd(QStringLiteral("nvidia*.json")))
            return tr("The Vulkan part of the NVIDIA driver is not installed: nvidia-utils on Arch, "
                      "nvidia-vulkan-icd on Debian, libnvidia-gl on Ubuntu.");
        return tr("The NVIDIA driver is installed but Vulkan does not see the card. "
                  "Run “vulkaninfo --summary” in a terminal to see why; on Arch, ggml-cuda is another way to use it.");
    }
    if (amd && !listed({"amd", "radeon"}))
        return tr("The AMD graphics have no Vulkan driver: install vulkan-radeon on Arch, mesa-vulkan-drivers on Debian, Ubuntu or Fedora.");
    if (intel && found.isEmpty())
        return tr("The Intel graphics have no Vulkan driver: install vulkan-intel on Arch, mesa-vulkan-drivers on Debian, Ubuntu or Fedora.");
    return {};
}

void Transcriber::preload(const QString &modelPath, int gpu)
{
    QMetaObject::invokeMethod(m_worker, [=, this] { ensureLoaded(modelPath, gpu, nullptr); });
}

void Transcriber::transcribe(std::vector<float> pcm, const Job &job)
{
    const quint64 generation = ++m_generation;
    QMetaObject::invokeMethod(m_worker, [this, pcm = std::move(pcm), job, generation]() mutable { run(std::move(pcm), job, generation); });
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

static QString gpuNamed(int index)
{
    for (const Transcriber::Gpu &g : Transcriber::gpus())
        if (g.index == index)
            return g.name;
    return {};
}

bool Transcriber::ensureLoaded(const QString &modelPath, int gpu, QString *error)
{
    if (m_ctx && m_loadedPath == modelPath && m_loadedGpu == gpu)
        return true;
    if (m_ctx)
        whisper_free(m_ctx);
    m_ctx = nullptr;
    m_loadedPath.clear();

    loadBackends();
    whisper_context_params params = whisper_context_default_params();
    params.use_gpu = gpu >= 0;
    params.gpu_device = std::max(gpu, 0);
    QElapsedTimer timer;
    timer.start();
    m_ctx = whisper_init_from_file_with_params(modelPath.toLocal8Bit().constData(), params);
    qCInfo(lcSotto) << "model" << modelPath << (m_ctx ? "loaded in" : "FAILED after") << timer.elapsed() << "ms, gpu:"
                    << (gpu >= 0 ? gpuNamed(gpu) : QStringLiteral("off"));
    if (!m_ctx) {
        if (error)
            *error = tr("Could not load the model");
        return false;
    }
    m_loadedPath = modelPath;
    m_loadedGpu = gpu;
    return true;
}

void Transcriber::run(std::vector<float> pcm, const Job &job, quint64 generation)
{
    const auto cancelled = [&] { return m_generation.load() != generation; };
    QString error;
    if (!ensureLoaded(job.modelPath, job.gpu, &error)) {
        Q_EMIT failed(error);
        return;
    }
    if (cancelled())
        return;

    // whisper rejects clips shorter than one second.
    if (pcm.size() < 17600)
        pcm.resize(17600, 0.f);

    const bool gpu = job.gpu >= 0;
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
    struct Abort {
        const std::atomic<quint64> *current;
        quint64 generation;
    } abort{&m_generation, generation};
    p.abort_callback = [](void *data) {
        const auto *a = static_cast<const Abort *>(data);
        return a->current->load() != a->generation;
    };
    p.abort_callback_user_data = &abort;

    QElapsedTimer timer;
    timer.start();
    const int rc = whisper_full(m_ctx, p, pcm.data(), int(pcm.size()));
    if (cancelled()) {
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
