#include "core/segmenter.h"
#include "core/gpu.h"
#include "core/log.h"
#include "core/models.h"

#include <whisper.h>

static Endpointer::Params paramsFor(int pauseMs)
{
    Endpointer::Params p;
    p.pauseMs = pauseMs;
    return p;
}

Segmenter::Segmenter(int pauseMs)
    : m_endpointer(paramsFor(pauseMs))
{
    // Silero runs on the CPU: a 32 ms frame takes well under a millisecond.
    if (Models::isInstalled(Models::VadId) && Gpu::haveCpuBackend()) {
        whisper_vad_context_params params = whisper_vad_default_context_params();
        params.n_threads = 1;
        params.use_gpu = false;
        m_vad = whisper_vad_init_from_file_with_params(Models::path(Models::VadId).toLocal8Bit().constData(), params);
        if (m_vad)
            whisper_vad_reset_state(m_vad); // a new context does not start from a zero LSTM state
    }
    qCInfo(lcSotto) << "phrases are cut by" << (m_vad ? "Silero VAD" : "loudness") << "at pauses of" << pauseMs << "ms";
}

Segmenter::~Segmenter()
{
    if (m_vad)
        whisper_vad_free(m_vad);
}

QList<QList<float>> Segmenter::feed(const QList<float> &samples)
{
    QList<QList<float>> phrases;
    m_pending += samples;
    const qsizetype frames = m_pending.size() / Endpointer::FrameSamples;
    if (frames == 0)
        return phrases;
    const float *data = m_pending.constData();
    const float *probs = nullptr;
    if (m_vad && whisper_vad_detect_speech_no_reset(m_vad, data, int(frames * Endpointer::FrameSamples))
        && whisper_vad_n_probs(m_vad) >= frames)
        probs = whisper_vad_probs(m_vad);
    for (qsizetype i = 0; i < frames; ++i) {
        const float *frame = data + i * Endpointer::FrameSamples;
        const float p = probs ? probs[i] : m_energy.probability(frame, Endpointer::FrameSamples);
        if (QList<float> pcm = m_endpointer.push(frame, p); !pcm.isEmpty())
            phrases.append(pcm);
    }
    m_pending.remove(0, frames * Endpointer::FrameSamples);
    return phrases;
}

QList<QList<float>> Segmenter::finish()
{
    QList<QList<float>> phrases;
    // The last partial frame, padded with silence, so the end of the last word is kept.
    if (!m_pending.isEmpty()) {
        m_pending.resize(Endpointer::FrameSamples, 0.f);
        const float p = m_endpointer.inPhrase() ? 1.f : 0.f;
        if (QList<float> pcm = m_endpointer.push(m_pending.constData(), p); !pcm.isEmpty())
            phrases.append(pcm);
        m_pending.clear();
    }
    if (QList<float> pcm = m_endpointer.flush(); !pcm.isEmpty())
        phrases.append(pcm);
    return phrases;
}
