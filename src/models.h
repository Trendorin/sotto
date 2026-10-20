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
QString url(const QString &id, const QString &host); // the same on a given server (a download keeps the one it started with)
QString baseUrl();                  // the configured server, empty if the custom one is not valid
QString pageUrl();                  // human-readable model page on the configured source
QString sourceTitle();              // name of the configured source
bool customSource();                // the "custom" source is selected
bool customUrlValid(const QString &url); // https://…, or http:// for localhost only
bool verifyChecksums();             // false only for a custom source the user chose not to verify
QString logFile();                  // download.log next to the models
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
    void log(const QString &line);   // what is being downloaded, from where, and the result
    void progress(qint64 received, qint64 total);
    void finished(const QString &id, bool ok, const QString &error);

private:
    void next();
    void done(bool ok, const QString &error);
    void note(const QString &line);

    QNetworkAccessManager *m_nam = nullptr;
    QPointer<QNetworkReply> m_reply;
    QFile m_file;
    QCryptographicHash m_hash{QCryptographicHash::Sha256};
    QString m_main;
    QStringList m_queue;
    // What the download started with: changing the source or the checksum option in the settings
    // while it runs must not change where it comes from or how it is checked.
    QString m_host;
    QString m_sourceTitle;
    bool m_verify = true;
};
