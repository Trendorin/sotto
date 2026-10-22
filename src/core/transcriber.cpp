#include "core/transcriber.h"
#include "core/gpu.h"
#include "core/log.h"
#include "core/models.h"

#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>

Transcriber::Transcriber(QObject *parent)
    : QObject(parent)
    , m_worker(new QObject)
{
    m_thread.setObjectName(QStringLiteral("recognition"));
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread.start();
    QMetaObject::invokeMethod(m_worker, [] {
        // Recognition yields to the microphone and the desktop: a lower priority for this thread, which
        // the compute threads of whisper.cpp and ONNX Runtime inherit when they are created from it.
        // (QThread priorities do nothing for normal threads on Linux.)
        setpriority(PRIO_PROCESS, id_t(syscall(SYS_gettid)), 5);
        Gpu::loadBackends();
    });
}

Transcriber::~Transcriber()
{
    m_cancelled.store(~quint64(0));
    m_thread.quit();
    m_thread.wait();
    m_engine.reset();
}

void Transcriber::preload(const QString &modelId, int gpu)
{
    QMetaObject::invokeMethod(m_worker, [=, this] {
        QString error;
        ensureLoaded(modelId, gpu, &error);
    });
}

void Transcriber::cancel(quint64 session)
{
    quint64 current = m_cancelled.load();
    while (current < session && !m_cancelled.compare_exchange_weak(current, session)) {
    }
}

void Transcriber::unload()
{
    QMetaObject::invokeMethod(m_worker, [this] {
        m_engine.reset();
        m_loadedId.clear();
    });
}

bool Transcriber::ensureLoaded(const QString &modelId, int gpu, QString *error)
{
    const ModelInfo model = Models::find(modelId);
    if (model.backend != Backend::Whisper)
        gpu = -1;
    if (m_engine && m_loadedId == modelId && m_loadedGpu == gpu)
        return true;
    m_engine.reset();
    m_loadedId.clear();
    m_engine = Engine::load(model, gpu, error);
    if (!m_engine)
        return false;
    m_loadedId = modelId;
    m_loadedGpu = gpu;
    return true;
}

void Transcriber::transcribe(const QList<float> &pcm, const Job &job, quint64 session)
{
    QMetaObject::invokeMethod(m_worker, [this, pcm, job, session] {
        const auto aborted = [this, session] { return session <= m_cancelled.load(); };
        if (aborted())
            return;
        QString error;
        if (!ensureLoaded(job.modelId, job.gpu, &error)) {
            Q_EMIT failed(session, error);
            return;
        }
        QString text;
        const bool ok = m_engine->transcribe(pcm, job.request, aborted, &text, &error);
        if (aborted()) {
            qCInfo(lcSotto) << "phrase cancelled";
            return;
        }
        if (ok)
            Q_EMIT finished(session, text);
        else
            Q_EMIT failed(session, error);
    });
}
