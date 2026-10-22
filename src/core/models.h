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

// Which engine runs a model.
enum class Backend {
    Whisper, // whisper.cpp: one ggml-<id>.bin file, 99 languages, Vulkan/CUDA
    Sherpa,  // sherpa-onnx (ONNX Runtime): a folder <id>/ with an encoder, decoder, joiner and tokens
    Vad,     // the Silero voice activity model that cuts the speech into phrases
};

struct ModelFile {
    QString name;          // file name in the repository and on disk
    qint64 bytes = 0;      // exact size of the published file (0 = unknown, custom model)
    QByteArray sha256;     // hex SHA-256 of the published file (the Git LFS object id)
};

struct ModelInfo {
    QString id;
    QString title;
    Backend backend = Backend::Whisper;
    QString repo;          // Hugging Face repository, "owner/name"
    QList<ModelFile> files;
    QString note;
    QString language;      // the only language it understands ("ru"), empty = many
    bool punctuation = true;

    int sizeMb() const;
    bool custom() const { return files.isEmpty() || files.constFirst().bytes == 0; }
};

struct ModelSource {
    QString id;
    QString title;
    QString host;    // https://huggingface.co or a mirror with the same layout
};

namespace Models {
inline const QString VadId = QStringLiteral("silero-v5.1.2");

// The models Sotto knows, best first within each engine. Sherpa models only when it was built with sherpa-onnx.
const QList<ModelInfo> &catalog();
const QList<ModelSource> &sources();
ModelInfo find(const QString &id);  // also knows VadId and models dropped into the folder
QStringList customIds();            // extra ggml-*.bin files and sherpa-onnx folders in dir()
bool engineAvailable(Backend backend);

QString dir();
QString path(const QString &id);    // the ggml-<id>.bin file, or the folder of a sherpa-onnx model
QString filePath(const ModelInfo &model, const ModelFile &file);
QString url(const ModelInfo &model, const ModelFile &file, const QString &host);
QString host();                     // the configured server, empty if the custom one is not valid
QString pageUrl(const QString &id); // human-readable model page on the configured source
QString sourceTitle();              // name of the configured source
bool customSource();                // the "custom" source is selected
bool customUrlValid(const QString &url); // https://…, or http:// for localhost only
bool verifyChecksums();             // false only for a custom source the user chose not to verify
QString logFile();                  // download.log next to the models
bool isInstalled(const QString &id);
bool remove(const QString &id);
// The model to suggest: GigaAM for Russian, Whisper Turbo with a GPU, otherwise Parakeet (or Whisper Small).
QString recommended(bool haveGpu, const QString &language);
} // namespace Models

// Downloads a model (plus the small VAD model if it's missing) into Models::dir(), file by file.
class ModelDownloader : public QObject
{
    Q_OBJECT
public:
    explicit ModelDownloader(QObject *parent = nullptr);

    void start(const QString &id);
    void cancel();
    bool isRunning() const { return !m_queue.isEmpty(); }

Q_SIGNALS:
    void log(const QString &line);   // what is being downloaded, from where, and the result
    void progress(qint64 received, qint64 total);
    void finished(const QString &id, bool ok, const QString &error);

private:
    struct Item {
        ModelInfo model;
        ModelFile file;
    };

    void next();
    void done(bool ok, const QString &error);
    void note(const QString &line);

    QNetworkAccessManager *m_nam = nullptr;
    QPointer<QNetworkReply> m_reply;
    QFile m_file;
    QCryptographicHash m_hash{QCryptographicHash::Sha256};
    QString m_main;
    QList<Item> m_queue;
    qint64 m_doneBytes = 0;  // of the main model, in files already finished
    qint64 m_totalBytes = 0; // of the main model
    // What the download started with: changing the source or the checksum option in the settings
    // while it runs must not change where it comes from or how it is checked.
    QString m_host;
    QString m_sourceTitle;
    bool m_verify = true;
};
