#include "core/models.h"
#include "core/config.h"
#include "core/log.h"

#include <QDateTime>
#include <QDir>
#include <QNetworkAccessManager>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <numeric>

int ModelInfo::sizeMb() const
{
    const qint64 bytes = std::accumulate(files.begin(), files.end(), qint64(0), [](qint64 sum, const ModelFile &f) { return sum + f.bytes; });
    return int((bytes + (1 << 19)) >> 20);
}

namespace Models {

static ModelInfo whisper(const char *id, const char *title, const char *note, qint64 bytes, const char *sha256)
{
    const QString name = QStringLiteral("ggml-%1.bin").arg(QLatin1String(id));
    return {QLatin1String(id), QLatin1String(title), Backend::Whisper, QStringLiteral("ggerganov/whisper.cpp"),
            {{name, bytes, sha256}}, QObject::tr(note), QString(), true};
}

const QList<ModelInfo> &catalog()
{
    // Sizes and SHA-256 sums as published on Hugging Face. A download (from Hugging Face or a mirror)
    // is only kept if every file matches both.
    static const QList<ModelInfo> list = [] {
        QList<ModelInfo> l;
#ifdef SOTTO_HAVE_SHERPA
        l.append({QStringLiteral("gigaam-v3-ru"), QStringLiteral("GigaAM v3"), Backend::Sherpa,
                  QStringLiteral("csukuangfj/sherpa-onnx-nemo-transducer-punct-giga-am-v3-russian-2025-12-16"),
                  {
                      {QStringLiteral("encoder.int8.onnx"), 224570820, "369f35a71bf288d3b8e0391fabd8dba5f2314088d440bca474056b7b4b6e66bf"},
                      {QStringLiteral("decoder.onnx"), 4600132, "38fc7475443ea2a26f63211ca350f73ac50fff824ab7a3876ee2bd610c53bbc4"},
                      {QStringLiteral("joiner.onnx"), 2712896, "602ff7017a93311aad34df1437c8d7f49911353c13d6eae7a6ee7b041339465c"},
                      {QStringLiteral("tokens.txt"), 13354, "39abae20e692998290c574e606f11a9edef2902a1995463fcff63d1490cf22b7"},
                  },
                  QObject::tr("the most accurate for Russian, with punctuation, fast on a CPU"), QStringLiteral("ru"), true});
        l.append({QStringLiteral("parakeet-v3"), QStringLiteral("Parakeet TDT v3"), Backend::Sherpa,
                  QStringLiteral("csukuangfj/sherpa-onnx-nemo-parakeet-tdt-0.6b-v3-int8"),
                  {
                      {QStringLiteral("encoder.int8.onnx"), 652184281, "acfc2b4456377e15d04f0243af540b7fe7c992f8d898d751cf134c3a55fd2247"},
                      {QStringLiteral("decoder.int8.onnx"), 11845275, "179e50c43d1a9de79c8a24149a2f9bac6eb5981823f2a2ed88d655b24248db4e"},
                      {QStringLiteral("joiner.int8.onnx"), 6355277, "3164c13fc2821009440d20fcb5fdc78bff28b4db2f8d0f0b329101719c0948b3"},
                      {QStringLiteral("tokens.txt"), 93939, "d58544679ea4bc6ac563d1f545eb7d474bd6cfa467f0a6e2c1dc1c7d37e3c35d"},
                  },
                  QObject::tr("25 European languages with Russian, punctuation, fast on a CPU"), QString(), true});
#endif
        l.append(whisper("large-v3-turbo-q5_0", "Whisper Large v3 Turbo", QT_TR_NOOP("99 languages, accurate and fast on a GPU"), 574041195,
                         "394221709cd5ad1f40c46e6031ca61bce88931e6e088c188294c6d5a55ffa7e2"));
        l.append(whisper("large-v3-turbo-q8_0", "Whisper Large v3 Turbo Q8", QT_TR_NOOP("slightly more accurate, more memory"), 874188075,
                         "317eb69c11673c9de1e1f0d459b253999804ec71ac4c23c17ecf5fbe24e259a1"));
        l.append(whisper("large-v3-q5_0", "Whisper Large v3", QT_TR_NOOP("most accurate Whisper, slow"), 1081140203,
                         "d75795ecff3f83b5faa89d1900604ad8c780abd5739fae406de19f23ecd98ad1"));
        l.append(whisper("medium-q5_0", "Whisper Medium", QT_TR_NOOP("more accurate, slow on a CPU"), 539212467,
                         "19fea4b380c3a618ec4723c3eef2eb785ffba0d0538cf43f8f235e7b3b34220f"));
        l.append(whisper("small-q5_1", "Whisper Small", QT_TR_NOOP("Whisper for a CPU"), 190085487,
                         "ae85e4a935d7a567bd102fe55afc16bb595bdb618e11b2fc7591bc08120411bb"));
        l.append(whisper("base-q5_1", "Whisper Base", QT_TR_NOOP("very fast, average accuracy"), 59707625,
                         "422f1ae452ade6f30a004d7e5c6a43195e4433bc370bf23fac9cc591f01a8898"));
        return l;
    }();
    return list;
}

static const ModelInfo &vad()
{
    static const ModelInfo info{VadId, QStringLiteral("Silero VAD"), Backend::Vad, QStringLiteral("ggml-org/whisper-vad"),
                                {{QStringLiteral("ggml-silero-v5.1.2.bin"), 885098, "29940d98d42b91fbd05ce489f3ecf7c72f0a42f027e4875919a28fb4c04ea2cf"}},
                                QString(), QString(), false};
    return info;
}

const QList<ModelSource> &sources()
{
    static const QList<ModelSource> list{
        {QStringLiteral("huggingface"), QStringLiteral("Hugging Face"), QStringLiteral("https://huggingface.co")},
        {QStringLiteral("hf-mirror"), QObject::tr("Mirror hf-mirror.com"), QStringLiteral("https://hf-mirror.com")},
        {QStringLiteral("custom"), QObject::tr("Custom server…"), QString()},
    };
    return list;
}

bool engineAvailable(Backend backend)
{
#ifdef SOTTO_HAVE_SHERPA
    Q_UNUSED(backend)
    return true;
#else
    return backend != Backend::Sherpa;
#endif
}

bool customUrlValid(const QString &url)
{
    const QUrl u(url, QUrl::StrictMode);
    if (!u.isValid() || u.host().isEmpty() || !u.userInfo().isEmpty() || !u.query().isEmpty() || !u.fragment().isEmpty())
        return false;
    if (u.scheme() == QLatin1String("https"))
        return true;
    const QString h = u.host();
    return u.scheme() == QLatin1String("http") && (h == QLatin1String("localhost") || h == QLatin1String("127.0.0.1") || h == QLatin1String("::1"));
}

bool customSource() { return Config::modelSource() == QLatin1String("custom"); }

bool verifyChecksums() { return !(customSource() && Config::allowUnverified()); }

QString host()
{
    const QString id = Config::modelSource();
    if (id == QLatin1String("custom")) {
        QString u = Config::customSourceUrl();
        while (u.endsWith(QLatin1Char('/')))
            u.chop(1);
        return customUrlValid(u) ? u : QString();
    }
    for (const auto &s : sources())
        if (s.id == id)
            return s.host;
    return sources().constFirst().host;
}

QString sourceTitle()
{
    for (const auto &s : sources())
        if (s.id == Config::modelSource())
            return s.title;
    return sources().constFirst().title;
}

QString logFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/sotto/download.log");
}

QString dir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/sotto/models");
}

static bool isSherpaFolder(const QDir &d)
{
    const auto has = [&](const char *pattern) { return !d.entryList({QLatin1String(pattern)}, QDir::Files).isEmpty(); };
    return has("tokens.txt") && has("encoder*.onnx") && has("decoder*.onnx") && has("joiner*.onnx");
}

static bool inCatalog(const QString &id)
{
    return std::any_of(catalog().begin(), catalog().end(), [&](const ModelInfo &m) { return m.id == id; });
}

ModelInfo find(const QString &id)
{
    if (id == VadId)
        return vad();
    for (const auto &m : catalog())
        if (m.id == id)
            return m;
    if (QFileInfo(dir() + QLatin1Char('/') + id).isDir())
        return {id, id, Backend::Sherpa, QString(), {}, QObject::tr("custom model"), QString(), true};
    return {id, id, Backend::Whisper, QString(), {{QStringLiteral("ggml-%1.bin").arg(id), 0, {}}}, QObject::tr("custom model"), QString(), true};
}

QStringList customIds()
{
    QStringList ids;
    const QDir models(dir());
    for (const QString &f : models.entryList({QStringLiteral("ggml-*.bin")}, QDir::Files, QDir::Name)) {
        const QString id = f.mid(5, f.size() - 9);
        if (id != VadId && !inCatalog(id))
            ids.append(id);
    }
#ifdef SOTTO_HAVE_SHERPA
    for (const QString &d : models.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
        if (!inCatalog(d) && isSherpaFolder(QDir(models.filePath(d))))
            ids.append(d);
#endif
    return ids;
}

QString path(const QString &id)
{
    const ModelInfo info = find(id);
    if (info.backend == Backend::Sherpa)
        return dir() + QLatin1Char('/') + id;
    return dir() + QLatin1Char('/') + info.files.constFirst().name;
}

QString filePath(const ModelInfo &model, const ModelFile &file)
{
    if (model.backend == Backend::Sherpa)
        return dir() + QLatin1Char('/') + model.id + QLatin1Char('/') + file.name;
    return dir() + QLatin1Char('/') + file.name;
}

QString url(const ModelInfo &model, const ModelFile &file, const QString &base)
{
    if (base.isEmpty() || model.repo.isEmpty())
        return {};
    return base + QLatin1Char('/') + model.repo + QStringLiteral("/resolve/main/") + file.name;
}

QString pageUrl(const QString &id)
{
    const ModelInfo info = find(id);
    if (host().isEmpty() || info.repo.isEmpty())
        return {};
    return host() + QLatin1Char('/') + info.repo;
}

bool isInstalled(const QString &id)
{
    if (id.isEmpty())
        return false;
    const ModelInfo info = find(id);
    if (!engineAvailable(info.backend))
        return false;
    if (info.custom()) {
        if (info.backend == Backend::Sherpa)
            return isSherpaFolder(QDir(path(id)));
        return QFileInfo(path(id)).size() > 1024 * 1024;
    }
    return std::all_of(info.files.begin(), info.files.end(),
                       [&](const ModelFile &f) { return QFileInfo(filePath(info, f)).size() == f.bytes; });
}

bool remove(const QString &id)
{
    const QString p = path(id);
    return find(id).backend == Backend::Sherpa ? QDir(p).removeRecursively() : QFile::remove(p);
}

QString recommended(bool haveGpu, const QString &language)
{
    const bool sherpa = engineAvailable(Backend::Sherpa);
    if (sherpa && language.startsWith(QLatin1String("ru")))
        return QStringLiteral("gigaam-v3-ru");
    if (haveGpu)
        return QStringLiteral("large-v3-turbo-q5_0");
    return sherpa ? QStringLiteral("parakeet-v3") : QStringLiteral("small-q5_1");
}

} // namespace Models

ModelDownloader::ModelDownloader(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void ModelDownloader::note(const QString &line)
{
    qCInfo(lcSotto).noquote() << line;
    Q_EMIT log(line);
    QDir().mkpath(QFileInfo(Models::logFile()).absolutePath());
    QFile f(Models::logFile());
    if (f.open(QIODevice::Append | QIODevice::Text))
        f.write((QDateTime::currentDateTime().toString(Qt::ISODate) + QLatin1Char(' ') + line + QLatin1Char('\n')).toUtf8());
}

void ModelDownloader::start(const QString &id)
{
    if (isRunning())
        return;
    m_main = id;
    m_host = Models::host();
    m_sourceTitle = Models::sourceTitle();
    m_verify = Models::verifyChecksums();
    m_doneBytes = m_totalBytes = 0;

    // Files already on disk with the published size were checked when they were saved.
    const auto enqueue = [this](const ModelInfo &model) {
        for (const ModelFile &f : model.files) {
            if (QFileInfo(Models::filePath(model, f)).size() == f.bytes && f.bytes > 0)
                continue;
            m_queue.append({model, f});
            if (model.id == m_main)
                m_totalBytes += f.bytes;
        }
    };
    enqueue(Models::find(id));
    if (id != Models::VadId)
        enqueue(Models::find(Models::VadId));
    if (m_queue.isEmpty()) {
        QTimer::singleShot(0, this, [this] { done(true, {}); });
        return;
    }
    note(tr("Source: %1").arg(m_sourceTitle));
    next();
}

void ModelDownloader::next()
{
    const Item item = m_queue.constFirst();
    const QString target = Models::filePath(item.model, item.file);
    QDir().mkpath(QFileInfo(target).absolutePath());
    m_file.setFileName(target + QStringLiteral(".part"));
    m_hash.reset();
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        done(false, m_file.errorString());
        return;
    }

    const QUrl url(Models::url(item.model, item.file, m_host));
    if (!url.isValid() || url.isEmpty()) {
        m_file.close();
        m_file.remove();
        done(false, tr("The custom server address is missing or not valid (use https://…)."));
        return;
    }
    note(tr("Downloading %1").arg(url.toString()));
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Sotto/" SOTTO_VERSION));
    m_reply = m_nam->get(req);

    connect(m_reply, &QNetworkReply::redirected, this, [this](const QUrl &to) {
        note(tr("Redirected to %1").arg(to.host() + to.path()));
    });
    connect(m_reply, &QNetworkReply::readyRead, this, [this] {
        const QByteArray chunk = m_reply->readAll();
        m_hash.addData(chunk);
        m_file.write(chunk);
    });
    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (m_queue.constFirst().model.id != m_main) // the VAD model is tiny, don't flicker the bar
            return;
        if (m_totalBytes > 0)
            Q_EMIT progress(m_doneBytes + received, m_totalBytes);
        else
            Q_EMIT progress(received, total);
    });
    connect(m_reply, &QNetworkReply::finished, this, [this] {
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        reply->deleteLater();
        const QByteArray tail = reply->readAll();
        m_hash.addData(tail);
        m_file.write(tail);
        m_file.close();

        const Item item = m_queue.constFirst();
        const qint64 expected = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
        if (reply->error() != QNetworkReply::NoError) {
            m_file.remove();
            const bool cancelled = reply->error() == QNetworkReply::OperationCanceledError;
            done(false, cancelled ? QString() : reply->errorString());
            return;
        }
        if (const qint64 got = m_file.size(); expected > 0 && got != expected) {
            m_file.remove();
            done(false, tr("The download is incomplete (%1 of %2 bytes)").arg(got).arg(expected));
            return;
        }
        const QString sum = QString::fromLatin1(m_hash.result().toHex());
        if (!item.file.sha256.isEmpty() && m_verify) {
            if (m_file.size() != item.file.bytes || sum != QString::fromLatin1(item.file.sha256)) {
                note(tr("Checksum does not match: got %1, expected %2").arg(sum, QString::fromLatin1(item.file.sha256)));
                m_file.remove();
                done(false, tr("The downloaded file does not match the published checksum, so it was deleted. "
                               "Try again or switch the source."));
                return;
            }
            note(tr("SHA-256 matches the published value (%1)").arg(sum.left(16) + QStringLiteral("…")));
        } else {
            note(tr("Checksum not verified (SHA-256 %1)").arg(sum));
        }
        const QString target = Models::filePath(item.model, item.file);
        QFile::remove(target);
        if (!m_file.rename(target)) {
            m_file.remove();
            done(false, tr("Could not save the model file"));
            return;
        }
        note(tr("Saved %1").arg(target));
        if (item.model.id == m_main)
            m_doneBytes += item.file.bytes;
        m_queue.removeFirst();
        if (m_queue.isEmpty())
            done(true, {});
        else
            next();
    });
}

void ModelDownloader::done(bool ok, const QString &error)
{
    if (!ok && !error.isEmpty())
        note(tr("Failed: %1").arg(error));
    m_queue.clear();
    Q_EMIT finished(m_main, ok, error);
}

void ModelDownloader::cancel()
{
    if (m_reply)
        m_reply->abort();
}
