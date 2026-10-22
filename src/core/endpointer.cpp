#include "core/endpointer.h"

#include <algorithm>
#include <cmath>

Endpointer::Endpointer(const Params &params)
    : m_p(params)
{
}

QList<float> Endpointer::push(const float *frame, float p)
{
    QList<float> samples(frame, frame + FrameSamples);
    if (!m_active) {
        // Keep the last moments before speech: the first syllable often starts below the threshold.
        m_preroll.push_back(std::move(samples));
        while (int(m_preroll.size()) > frames(m_p.padMs) + 1)
            m_preroll.erase(m_preroll.begin());
        if (p < m_p.onThreshold)
            return {};
        m_active = true;
        m_phrase.clear();
        m_probs.clear();
        for (const QList<float> &f : m_preroll) {
            m_phrase += f;
            m_probs.push_back(0.f);
        }
        m_probs.back() = p;
        m_preroll.clear();
        m_speechFrames = 1;
        m_silentFrames = 0;
        return {};
    }

    m_phrase += samples;
    m_probs.push_back(p);
    if (p >= m_p.offThreshold) {
        ++m_speechFrames;
        m_silentFrames = 0;
    } else {
        ++m_silentFrames;
    }

    if (m_silentFrames >= frames(m_p.pauseMs))
        return finish(int(m_probs.size()) - std::max(0, m_silentFrames - frames(m_p.padMs)));

    if (int(m_probs.size()) >= frames(m_p.maxPhraseMs)) {
        // Too long without a pause: cut at the quietest frame of the last three seconds and go on.
        const int n = int(m_probs.size());
        const int from = std::max(n / 2, n - frames(3000));
        const int cut = int(std::min_element(m_probs.begin() + from, m_probs.end()) - m_probs.begin()) + 1;
        QList<float> out = m_phrase.first(qsizetype(cut) * FrameSamples);
        const int speech = int(std::count_if(m_probs.begin(), m_probs.begin() + cut, [&](float x) { return x >= m_p.offThreshold; }));
        m_phrase = m_phrase.sliced(qsizetype(cut) * FrameSamples);
        m_probs.erase(m_probs.begin(), m_probs.begin() + cut);
        m_speechFrames = int(std::count_if(m_probs.begin(), m_probs.end(), [&](float x) { return x >= m_p.offThreshold; }));
        m_silentFrames = std::min(m_silentFrames, int(m_probs.size()));
        if (speech < frames(m_p.minSpeechMs))
            return {};
        m_heard = true;
        return out;
    }
    return {};
}

QList<float> Endpointer::flush()
{
    if (!m_active)
        return {};
    return finish(int(m_probs.size()) - std::max(0, m_silentFrames - frames(m_p.padMs)));
}

QList<float> Endpointer::finish(int keepFrames)
{
    keepFrames = std::clamp(keepFrames, 0, int(m_probs.size()));
    const bool enough = m_speechFrames >= frames(m_p.minSpeechMs);
    QList<float> out = enough ? m_phrase.first(qsizetype(keepFrames) * FrameSamples) : QList<float>();
    m_active = false;
    m_phrase.clear();
    m_probs.clear();
    m_preroll.clear();
    m_speechFrames = m_silentFrames = 0;
    m_heard |= enough;
    return out;
}

float EnergyDetector::probability(const float *frame, int samples)
{
    double sum = 0.0;
    for (int i = 0; i < samples; ++i)
        sum += double(frame[i]) * frame[i];
    const double db = 20.0 * std::log10(std::sqrt(sum / std::max(samples, 1)) + 1e-9);
    // The background level follows quiet moments quickly and loud ones slowly (about 0.6 dB/s).
    m_floorDb = db < m_floorDb ? 0.7 * m_floorDb + 0.3 * db : m_floorDb + 0.02;
    m_floorDb = std::clamp(m_floorDb, -90.0, -25.0);
    if (db < -55.0)
        return 0.f;
    return float(std::clamp((db - m_floorDb - 6.0) / 10.0, 0.0, 1.0));
}
