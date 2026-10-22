#pragma once

#include "core/endpointer.h"

#include <QList>

struct whisper_vad_context;

// Turns a stream of 16 kHz audio into phrases: Silero VAD (through whisper.cpp) rates every
// 32 ms frame, Endpointer cuts at the pauses. Without the VAD model it falls back to loudness.
// Not thread-safe: Listener runs one on its thread, `sotto --transcribe` one on the main thread.
class Segmenter
{
public:
    explicit Segmenter(int pauseMs);
    ~Segmenter();
    Segmenter(const Segmenter &) = delete;
    Segmenter &operator=(const Segmenter &) = delete;

    QList<QList<float>> feed(const QList<float> &samples);
    QList<QList<float>> finish(); // the rest, including a phrase cut short by the end of the audio
    bool heardSpeech() const { return m_endpointer.heardSpeech(); }

private:
    Endpointer m_endpointer;
    EnergyDetector m_energy;
    whisper_vad_context *m_vad = nullptr;
    QList<float> m_pending; // samples short of a whole frame
};
