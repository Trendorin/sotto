#include "cli.h"
#include "core/config.h"
#include "core/engine.h"
#include "core/gpu.h"
#include "core/models.h"
#include "core/segmenter.h"
#include "core/text.h"
#include "system/desktop.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QtEndian>

#include <cmath>
#include <cstdio>

namespace Cli {

static QString tr(const char *text)
{
    return QCoreApplication::translate("Cli", text);
}

static void print(FILE *to, const QString &text)
{
    const QByteArray utf8 = text.toUtf8();
    fwrite(utf8.constData(), 1, size_t(utf8.size()), to);
    fputc('\n', to);
    fflush(to);
}

// RIFF/WAVE with PCM 16/24/32-bit integer or 32-bit float samples, mixed down to 16 kHz mono.
static bool readWav(const QString &path, QList<float> *out, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = file.errorString();
        return false;
    }
    const QByteArray data = file.readAll();
    // Header fields byte by byte; reads past the end give 0, so a truncated header is rejected below.
    const auto byte = [&](qsizetype at) { return at < data.size() ? quint32(quint8(data.at(at))) : 0u; };
    const auto u16 = [&](qsizetype at) { return quint16(byte(at) | byte(at + 1) << 8); };
    const auto u32 = [&](qsizetype at) { return quint32(u16(at)) | quint32(u16(at + 2)) << 16; };
    if (data.size() < 12 || !data.startsWith("RIFF") || data.mid(8, 4) != "WAVE") {
        *error = tr("not a WAV file");
        return false;
    }
    int format = 0, channels = 0, rate = 0, bits = 0;
    qsizetype pcmAt = -1, pcmBytes = 0;
    for (qsizetype at = 12; at + 8 <= data.size();) {
        const QByteArray id = data.mid(at, 4);
        const qsizetype size = u32(at + 4);
        if (id == "fmt " && size >= 16 && at + 8 + 16 <= data.size()) {
            format = u16(at + 8);
            channels = u16(at + 10);
            rate = int(u32(at + 12));
            bits = u16(at + 22);
            if (format == 0xFFFE && size >= 40) // WAVE_FORMAT_EXTENSIBLE: the real format is in the sub-format GUID
                format = u16(at + 8 + 24);
        } else if (id == "data") {
            pcmAt = at + 8;
            pcmBytes = std::min<qsizetype>(size, data.size() - pcmAt);
            break;
        }
        at += 8 + size + (size & 1);
    }
    const bool integer = format == 1 && (bits == 16 || bits == 24 || bits == 32);
    const bool floating = format == 3 && bits == 32;
    if (pcmAt < 0 || channels < 1 || rate < 8000 || !(integer || floating)) {
        *error = tr("unsupported WAV format (use 16-bit PCM or 32-bit float)");
        return false;
    }

    const int bytes = bits / 8;
    const qsizetype frames = pcmBytes / (qsizetype(bytes) * channels);
    QList<float> mono(frames);
    const char *p = data.constData() + pcmAt;
    for (qsizetype i = 0; i < frames; ++i) {
        float sum = 0.f;
        for (int c = 0; c < channels; ++c, p += bytes) {
            if (floating)
                sum += qFromLittleEndian<float>(p);
            else if (bits == 16)
                sum += qFromLittleEndian<qint16>(p) / 32768.f;
            else if (bits == 24)
                sum += float(qint32(quint32(quint8(p[0])) << 8 | quint32(quint8(p[1])) << 16 | quint32(quint8(p[2])) << 24) >> 8) / 8388608.f;
            else
                sum += float(qFromLittleEndian<qint32>(p) / 2147483648.0);
        }
        mono[i] = sum / float(channels);
    }
    if (rate == 16000) {
        *out = mono;
        return true;
    }
    // Linear interpolation is enough for speech recognition.
    const double step = double(rate) / 16000.0;
    const qsizetype n = qsizetype(std::floor(double(frames) / step));
    out->resize(n);
    for (qsizetype i = 0; i < n; ++i) {
        const double x = double(i) * step;
        const qsizetype j = qsizetype(x);
        const float frac = float(x - double(j));
        const float a = mono[j];
        const float b = j + 1 < frames ? mono[j + 1] : a;
        (*out)[i] = a + (b - a) * frac;
    }
    return true;
}

int transcribe(const QString &path, const QString &modelId)
{
    const QString id = modelId.isEmpty() ? Config::model() : modelId;
    if (!Models::isInstalled(id)) {
        print(stderr, tr("The model “%1” is not downloaded: choose one in the settings or with --model (see --models).").arg(id));
        return 1;
    }
    QList<float> pcm;
    QString error;
    if (!readWav(path, &pcm, &error)) {
        print(stderr, QStringLiteral("%1: %2").arg(path, error));
        return 1;
    }

    const ModelInfo model = Models::find(id);
    std::unique_ptr<Engine> engine = Engine::load(model, model.backend == Backend::Whisper ? Gpu::configured() : -1, &error);
    if (!engine) {
        print(stderr, error);
        return 1;
    }

    // The same pipeline as a dictation: phrases cut at the pauses, recognised one by one.
    QElapsedTimer timer;
    timer.start();
    Segmenter segmenter(Config::pauseMs());
    QList<QList<float>> phrases = segmenter.feed(pcm);
    phrases += segmenter.finish();
    QString text;
    const auto never = [] { return false; };
    for (const QList<float> &phrase : std::as_const(phrases)) {
        Engine::Request request{Config::language(), Config::prompt()};
        if (!text.isEmpty() && model.backend == Backend::Whisper)
            request.prompt = (request.prompt + QLatin1Char(' ') + text.right(200)).trimmed();
        QString part;
        if (!engine->transcribe(phrase, request, never, &part, &error)) {
            print(stderr, error);
            return 1;
        }
        Text::append(text, part);
    }
    print(stdout, text);
    print(stderr, tr("%1 s of audio, %2 phrases, %3 ms (%4)")
                      .arg(pcm.size() / 16000.0, 0, 'f', 1)
                      .arg(phrases.size())
                      .arg(timer.elapsed())
                      .arg(model.title));
    return 0;
}

int listModels()
{
    const auto line = [](const ModelInfo &m) {
        const QString engine = m.backend == Backend::Sherpa ? QStringLiteral("sherpa-onnx") : QStringLiteral("whisper.cpp");
        print(stdout, QStringLiteral("%1 %2  %3  %4  %5 MB  %6")
                          .arg(Models::isInstalled(m.id) ? QStringLiteral("*") : QStringLiteral(" "), m.id.leftJustified(20))
                          .arg(m.title.leftJustified(26), engine.leftJustified(11))
                          .arg(m.sizeMb(), 5)
                          .arg(m.note));
    };
    for (const ModelInfo &m : Models::catalog())
        line(m);
    for (const QString &id : Models::customIds())
        line(Models::find(id));
    print(stdout, tr("* downloaded, in %1").arg(Models::dir()));
    return 0;
}

int keybind()
{
    const Desktop::Binding binding = Desktop::binding();
    print(stdout, tr("On %1 add to %2:").arg(Desktop::name(), binding.where));
    print(stdout, QString());
    print(stdout, binding.snippet);
    return 0;
}

} // namespace Cli
