#pragma once

#include <QList>

#include <vector>

// How Endpointer decides; the defaults suit dictation.
struct EndpointerParams {
    float onThreshold = 0.5f;   // a frame this likely to be speech starts a phrase
    float offThreshold = 0.35f; // below this inside a phrase the frame counts as silence
    int minSpeechMs = 250;      // shorter bursts (a click, a cough) are dropped
    int pauseMs = 600;          // this much silence ends a phrase
    int maxPhraseMs = 20000;    // longer phrases are cut at their quietest moment
    int padMs = 200;            // audio kept before and after the speech
};

// Cuts a stream of audio frames into phrases at the pauses between them, from a speech
// probability per frame (Silero VAD, or EnergyDetector when there is no VAD model). Pure logic,
// no threads or Qt objects, so the tests drive it directly.
class Endpointer
{
public:
    static constexpr int SampleRate = 16000;
    static constexpr int FrameSamples = 512; // 32 ms, the window Silero VAD works on
    static constexpr int FrameMs = FrameSamples * 1000 / SampleRate;

    using Params = EndpointerParams;

    explicit Endpointer(const Params &params = Params());

    // Adds one frame of FrameSamples samples. Returns a finished phrase, or an empty list.
    QList<float> push(const float *frame, float speechProbability);
    // The phrase in progress, if it has enough speech; then starts over.
    QList<float> flush();

    bool inPhrase() const { return m_active; }
    bool heardSpeech() const { return m_heard; } // some phrase had enough speech

private:
    QList<float> finish(int keepFrames);
    int frames(int ms) const { return (ms + FrameMs - 1) / FrameMs; }

    Params m_p;
    bool m_active = false;
    bool m_heard = false;
    QList<float> m_phrase;          // samples of the phrase in progress, preroll included
    std::vector<float> m_probs;     // one per frame of m_phrase
    std::vector<QList<float>> m_preroll;
    int m_speechFrames = 0;
    int m_silentFrames = 0;         // trailing
};

// A speech probability from loudness alone: how far a frame is above the background noise,
// which is tracked as it changes. Good enough in a quiet room; Silero is better everywhere else.
class EnergyDetector
{
public:
    float probability(const float *frame, int samples);

private:
    double m_floorDb = -60.0;
};
