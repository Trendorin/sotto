// SPDX-License-Identifier: MIT
//
// Phrases must end at pauses (and only there), keep the audio just before and after the speech,
// drop clicks that are too short to be words, survive very long speech without a pause, and be
// joined back into text without stray spaces or capitals in the middle of a sentence.

#include "core/endpointer.h"
#include "core/text.h"

#include <QList>

#include <cstdio>
#include <vector>

namespace {

int g_failed = 0;

void check(bool ok, const char *what)
{
    std::printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        ++g_failed;
}

constexpr int Frame = Endpointer::FrameSamples;
constexpr int FrameMs = Endpointer::FrameMs;

// Drives the endpointer with a probability per frame; each frame's samples carry its index so the
// tests can tell which frames ended up in a phrase.
struct Run {
    QList<QList<float>> phrases;
    Endpointer e;

    explicit Run(Endpointer::Params p = Endpointer::Params())
        : e(p)
    {
    }

    void frames(int count, float probability)
    {
        static int index = 0;
        std::vector<float> f(Frame);
        for (int i = 0; i < count; ++i) {
            std::fill(f.begin(), f.end(), float(++index));
            if (QList<float> phrase = e.push(f.data(), probability); !phrase.isEmpty())
                phrases.append(phrase);
        }
    }
    void ms(int duration, float probability) { frames(duration / FrameMs, probability); }
    void flush()
    {
        if (QList<float> phrase = e.flush(); !phrase.isEmpty())
            phrases.append(phrase);
    }
};

int framesIn(const QList<float> &phrase) { return int(phrase.size() / Frame); }

} // namespace

int main()
{
    {
        Run r;
        r.ms(1000, 0.0f);
        r.ms(1500, 0.9f);
        check(r.phrases.isEmpty(), "no phrase while the speaker is still talking");
        r.ms(1000, 0.0f);
        check(r.phrases.size() == 1, "a pause ends the phrase");
        const int speech = 1500 / FrameMs, pad = (200 + FrameMs - 1) / FrameMs;
        check(r.phrases.size() == 1 && framesIn(r.phrases[0]) == speech + 2 * pad,
              "the phrase keeps 200 ms before and after the speech");
        check(r.e.heardSpeech(), "speech was heard");
    }
    {
        Run r;
        r.ms(1000, 0.9f);
        r.ms(300, 0.1f); // shorter than the 600 ms pause
        r.ms(1000, 0.9f);
        r.ms(1000, 0.0f);
        check(r.phrases.size() == 1, "a short breath does not split a phrase");
    }
    {
        Endpointer::Params p;
        p.pauseMs = 300;
        Run r(p);
        r.ms(1000, 0.9f);
        r.ms(400, 0.1f);
        r.ms(1000, 0.9f);
        r.ms(400, 0.1f);
        check(r.phrases.size() == 2, "the pause length is configurable");
    }
    {
        Run r;
        r.ms(1000, 0.0f);
        r.ms(96, 0.9f); // a click
        r.ms(1000, 0.0f);
        r.flush();
        check(r.phrases.isEmpty(), "a click is not a phrase");
        check(!r.e.heardSpeech(), "a click is not speech");
    }
    {
        Run r;
        r.ms(800, 0.9f);
        r.flush();
        check(r.phrases.size() == 1, "stopping mid-phrase keeps the phrase");
        r.flush();
        check(r.phrases.size() == 1, "flushing twice gives nothing more");
    }
    {
        Endpointer::Params p;
        p.maxPhraseMs = 5000;
        Run r(p);
        r.ms(4000, 0.9f);
        r.frames(1, 0.4f); // the quietest moment
        r.ms(3000, 0.9f);
        check(r.phrases.size() == 1, "speech without a pause is cut at the length limit");
        check(r.phrases.size() == 1 && framesIn(r.phrases[0]) <= 5000 / FrameMs + 1, "the cut phrase is not longer than the limit");
        r.ms(1000, 0.0f);
        check(r.phrases.size() == 2, "the rest becomes the next phrase");
    }
    {
        Run r;
        r.ms(1000, 0.9f);
        r.ms(1000, 0.0f);
        r.ms(1000, 0.9f);
        r.ms(1000, 0.0f);
        r.ms(1000, 0.9f);
        r.flush();
        check(r.phrases.size() == 3, "three phrases with two pauses and a stop");
    }

    QString text;
    Text::append(text, QStringLiteral("Привет"));
    Text::append(text, QStringLiteral(", как дела?"));
    Text::append(text, QString());
    Text::append(text, QStringLiteral("Всё хорошо."));
    check(text == QStringLiteral("Привет, как дела? Всё хорошо."), "phrases are joined with single spaces, none before punctuation");

    QString sentence;
    Text::append(sentence, QStringLiteral("Ничьих не требуя похвал,"));
    Text::append(sentence, QStringLiteral("Счастлив уж я надеждой сладкой"));
    Text::append(sentence, QStringLiteral("Я думаю."));
    Text::append(sentence, QStringLiteral("новая мысль"));
    Text::append(sentence, QStringLiteral("NASA и ГОСТ"));
    check(sentence == QStringLiteral("Ничьих не требуя похвал, счастлив уж я надеждой сладкой я думаю. Новая мысль NASA и ГОСТ"),
          "a phrase inside a sentence loses its capital, a new sentence gets one, acronyms stay");

    EnergyDetector energy;
    std::vector<float> silence(Frame, 0.0005f), voice(Frame);
    for (int i = 0; i < Frame; ++i)
        voice[i] = (i % 16 < 8 ? 0.2f : -0.2f);
    float quiet = 0.f;
    for (int i = 0; i < 30; ++i)
        quiet = energy.probability(silence.data(), Frame);
    const float loud = energy.probability(voice.data(), Frame);
    check(quiet < 0.1f && loud > 0.9f, "loudness tells speech from background noise");

    std::printf("%s\n", g_failed ? "FAILED" : "all passed");
    return g_failed ? 1 : 0;
}
