#pragma once

#include <QCryptographicHash>
#include <QFile>
#include <QList>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>

class QNetworkAccessManager;

struct ModelInfo {
    QString id;      // file is ggml-<id>.bin
    QString title;
    int sizeMb;
    QString note;
    qint64 bytes = 0;      // exact size of the published file (0 = unknown, custom model)
    QByteArray sha256;     // hex SHA-256 of the published file (Git LFS object id)
};

struct ModelSource {
    QString id;
    QString title;
    QString host;    // https://huggingface.co or a mirror with the same layout
};

namespace Models {
// Whisper models: https://huggingface.co/ggerganov/whisper.cpp
// Silero VAD:     https://huggingface.co/ggml-org/whisper-vad
inline const QString VadId = QStringLiteral("silero-v5.1.2");

const QList<ModelInfo> &catalog();
const QList<ModelSource> &sources();
ModelInfo find(const QString &id); // falls back to a generic entry for custom files; knows VadId
QStringList customIds();            // extra ggml-*.bin files dropped into the folder
QString dir();
QString path(const QString &id);    // also accepts VadId
QString url(const QString &id);     // download URL from the configured source
QString pageUrl();                  // human-readable model page on the configured source
bool isInstalled(const QString &id);
QString recommended(bool haveGpu);
} // namespace Models

// Downloads a model (plus the small VAD model if it's missing) into Models::dir().
class ModelDownloader : public QObject
{
    Q_OBJECT
public:
    explicit ModelDownloader(QObject *parent = nullptr);

    void start(const QString &id);
    void cancel();
    bool isRunning() const { return !m_queue.isEmpty(); }
    QString current() const { return m_main; }

Q_SIGNALS:
    void progress(qint64 received, qint64 total);
    void finished(const QString &id, bool ok, const QString &error);

private:
    void next();
    void done(bool ok, const QString &error);

    QNetworkAccessManager *m_nam = nullptr;
    QPointer<QNetworkReply> m_reply;
    QFile m_file;
    QCryptographicHash m_hash{QCryptographicHash::Sha256};
    QString m_main;
    QStringList m_queue;
};
