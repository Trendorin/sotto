#include "models.h"
#include "config.h"
#include "log.h"

#include <QDir>
#include <QNetworkAccessManager>
#include <QStandardPaths>

namespace Models {

const QList<ModelInfo> &catalog()
{
    static const QList<ModelInfo> list{
        // Sizes and SHA-256 sums as published on huggingface.co/ggerganov/whisper.cpp. A download
        // (from Hugging Face or a mirror) is only kept if it matches both.
        {QStringLiteral("base-q5_1"), QStringLiteral("Base"), 57, QObject::tr("very fast, average accuracy"), 59707625,
         "422f1ae452ade6f30a004d7e5c6a43195e4433bc370bf23fac9cc591f01a8898"},
        {QStringLiteral("small-q5_1"), QStringLiteral("Small"), 181, QObject::tr("best choice without a GPU"), 190085487,
         "ae85e4a935d7a567bd102fe55afc16bb595bdb618e11b2fc7591bc08120411bb"},
        {QStringLiteral("medium-q5_0"), QStringLiteral("Medium"), 514, QObject::tr("more accurate, slow on a CPU"), 539212467,
         "19fea4b380c3a618ec4723c3eef2eb785ffba0d0538cf43f8f235e7b3b34220f"},
        {QStringLiteral("large-v3-turbo-q5_0"), QStringLiteral("Large v3 Turbo"), 547, QObject::tr("accurate and fast on a GPU"), 574041195,
         "394221709cd5ad1f40c46e6031ca61bce88931e6e088c188294c6d5a55ffa7e2"},
        {QStringLiteral("large-v3-turbo-q8_0"), QStringLiteral("Large v3 Turbo Q8"), 834, QObject::tr("slightly more accurate, more memory"), 874188075,
         "317eb69c11673c9de1e1f0d459b253999804ec71ac4c23c17ecf5fbe24e259a1"},
        {QStringLiteral("large-v3-q5_0"), QStringLiteral("Large v3"), 1080, QObject::tr("most accurate, slow"), 1081140203,
         "d75795ecff3f83b5faa89d1900604ad8c780abd5739fae406de19f23ecd98ad1"},
    };
    return list;
}

const QList<ModelSource> &sources()
{
    static const QList<ModelSource> list{
        {QStringLiteral("huggingface"), QStringLiteral("Hugging Face"), QStringLiteral("https://huggingface.co")},
        {QStringLiteral("hf-mirror"), QObject::tr("Mirror hf-mirror.com"), QStringLiteral("https://hf-mirror.com")},
    };
    return list;
}

static QString host()
{
    const QString id = Config::modelSource();
    for (const auto &s : sources())
        if (s.id == id)
            return s.host;
    return sources().constFirst().host;
}

ModelInfo find(const QString &id)
{
    if (id == VadId) // huggingface.co/ggml-org/whisper-vad
        return {id, QStringLiteral("Silero VAD"), 1, QString(), 885098, QByteArrayLiteral("29940d98d42b91fbd05ce489f3ecf7c72f0a42f027e4875919a28fb4c04ea2cf")};
    for (const auto &m : catalog())
        if (m.id == id)
            return m;
    return {id, id, 0, QObject::tr("custom model"), 0, {}};
}

QStringList customIds()
{
    QStringList ids;
    const auto files = QDir(dir()).entryList({QStringLiteral("ggml-*.bin")}, QDir::Files, QDir::Name);
    for (const QString &f : files) {
        const QString id = f.mid(5, f.size() - 9);
        if (id != VadId && std::none_of(catalog().begin(), catalog().end(), [&](const ModelInfo &m) { return m.id == id; }))
            ids.append(id);
    }
    return ids;
}

QString dir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/sotto/models");
}

QString path(const QString &id)
{
    return dir() + QStringLiteral("/ggml-") + id + QStringLiteral(".bin");
}

QString url(const QString &id)
{
    if (id == VadId)
        return host() + QStringLiteral("/ggml-org/whisper-vad/resolve/main/ggml-") + id + QStringLiteral(".bin");
    return host() + QStringLiteral("/ggerganov/whisper.cpp/resolve/main/ggml-") + id + QStringLiteral(".bin");
}

QString pageUrl()
{
    return host() + QStringLiteral("/ggerganov/whisper.cpp");
}

bool isInstalled(const QString &id)
{
    const qint64 minSize = id == VadId ? 100 * 1024 : 1024 * 1024;
    return !id.isEmpty() && QFileInfo(path(id)).size() > minSize;
}

QString recommended(bool haveGpu)
{
    return haveGpu ? QStringLiteral("large-v3-turbo-q5_0") : QStringLiteral("small-q5_1");
}

} // namespace Models

ModelDownloader::ModelDownloader(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void ModelDownloader::start(const QString &id)
{
    if (isRunning())
        return;
    m_main = id;
    m_queue = {id};
    if (id != Models::VadId && !Models::isInstalled(Models::VadId))
        m_queue.append(Models::VadId);
    next();
}

void ModelDownloader::next()
{
    const QString id = m_queue.constFirst();
    QDir().mkpath(Models::dir());
    m_file.setFileName(Models::path(id) + QStringLiteral(".part"));
    m_hash.reset();
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        done(false, m_file.errorString());
        return;
    }

    const QUrl url(Models::url(id));
    qCInfo(lcSotto) << "downloading" << url.toString();
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Sotto/" SOTTO_VERSION));
    m_reply = m_nam->get(req);

    connect(m_reply, &QNetworkReply::readyRead, this, [this] {
        const QByteArray chunk = m_reply->readAll();
        m_hash.addData(chunk);
        m_file.write(chunk);
    });
    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (m_queue.constFirst() == m_main) // the VAD model is tiny, don't flicker the bar
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

        const QString id = m_queue.constFirst();
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
        const ModelInfo info = Models::find(id);
        if (!info.sha256.isEmpty() && (m_file.size() != info.bytes || m_hash.result().toHex() != info.sha256)) {
            qCWarning(lcSotto) << "checksum mismatch for" << id << m_hash.result().toHex();
            m_file.remove();
            done(false, tr("The downloaded file does not match the published checksum, so it was deleted. "
                           "Try again or switch the source."));
            return;
        }
        QFile::remove(Models::path(id));
        if (!m_file.rename(Models::path(id))) {
            m_file.remove();
            done(false, tr("Could not save the model file"));
            return;
        }
        qCInfo(lcSotto) << "downloaded" << Models::path(id);
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
        qCWarning(lcSotto) << "download failed:" << error;
    m_queue.clear();
    Q_EMIT finished(m_main, ok, error);
}

void ModelDownloader::cancel()
{
    if (m_reply)
        m_reply->abort();
}
