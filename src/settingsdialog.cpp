#include "settingsdialog.h"
#include "config.h"
#include "models.h"
#include "output.h"
#include "shortcut.h"
#include "transcriber.h"

#include <QAudioDevice>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMediaDevices>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>

static QLabel *hint(const QString &text)
{
    auto *label = new QLabel(text);
    label->setEnabled(false); // themed "secondary text" look
    return label;
}

SettingsDialog::SettingsDialog(ModelDownloader *downloader, GlobalShortcut *shortcut, TextOutput *output, QWidget *parent)
    : QDialog(parent)
    , m_downloader(downloader)
    , m_shortcut(shortcut)
    , m_output(output)
{
    setWindowTitle(tr("Settings"));
    setWindowIcon(QIcon::fromTheme(QString::fromLatin1(Config::AppId), QIcon(QStringLiteral(":/sotto.svg"))));
    setAttribute(Qt::WA_DeleteOnClose);

    const QString gpu = Transcriber::gpuName();
    m_haveGpu = !gpu.isEmpty();

    // Recognition
    auto *recognition = new QGroupBox(tr("Recognition"));
    auto *recForm = new QFormLayout(recognition);

    m_model = new QComboBox;
    m_modelAction = new QPushButton;
    auto *modelRow = new QHBoxLayout;
    modelRow->addWidget(m_model, 1);
    modelRow->addWidget(m_modelAction);
    auto *openFolder = new QPushButton(QIcon::fromTheme(QStringLiteral("folder-open")), QString());
    openFolder->setToolTip(tr("Open the models folder"));
    modelRow->addWidget(openFolder);
    recForm->addRow(tr("Model:"), modelRow);

    m_progress = new QProgressBar;
    m_progress->setVisible(false);
    recForm->addRow(QString(), m_progress);
    m_modelNote = hint(QString());
    recForm->addRow(QString(), m_modelNote);

    m_source = new QComboBox;
    for (const auto &s : Models::sources())
        m_source->addItem(s.title, s.id);
    m_sourceLink = new QLabel;
    m_sourceLink->setOpenExternalLinks(true);
    m_sourceLink->setTextInteractionFlags(Qt::TextBrowserInteraction);
    auto *sourceRow = new QHBoxLayout;
    sourceRow->addWidget(m_source);
    sourceRow->addWidget(m_sourceLink, 1);
    recForm->addRow(tr("Source:"), sourceRow);

    m_language = new QComboBox;
    m_language->addItem(tr("Detect automatically"), QStringLiteral("auto"));
    for (const char *code : {"ru", "en", "uk", "be", "kk", "de", "fr", "es", "it", "pt", "pl", "tr", "zh", "ja", "ko"}) {
        QString name = QLocale(QString::fromLatin1(code)).nativeLanguageName();
        if (!name.isEmpty())
            name[0] = name[0].toUpper();
        m_language->addItem(name, QString::fromLatin1(code));
    }
    recForm->addRow(tr("Language:"), m_language);

    m_gpu = new QCheckBox(m_haveGpu ? tr("Use the GPU (%1)").arg(gpu) : tr("Use the GPU"));
    m_gpu->setEnabled(m_haveGpu);
    recForm->addRow(tr("Acceleration:"), m_gpu);
    if (!m_haveGpu) {
        auto *warning = new QLabel(tr("⚠ No GPU in use — Large models are slow on a CPU.<br>"
                                      "Install a Vulkan driver (on Arch also <b>ggml-vulkan</b>) or choose Small."));
        recForm->addRow(QString(), warning);
    }

    m_vad = new QCheckBox(tr("Cut silence and noise (Silero VAD)"));
    m_vad->setToolTip(tr("Only speech is recognized: faster, and no invented phrases in silence"));
    recForm->addRow(tr("Filter:"), m_vad);

    m_prompt = new QLineEdit;
    m_prompt->setPlaceholderText(tr("For example: Kubernetes, Grafana, Jane Doe"));
    m_prompt->setClearButtonEnabled(true);
    recForm->addRow(tr("Vocabulary:"), m_prompt);
    recForm->addRow(QString(), hint(tr("Names and terms you often say —\nwhisper will spell them correctly.")));

    // Recording
    auto *recording = new QGroupBox(tr("Recording"));
    auto *recordForm = new QFormLayout(recording);
    m_microphone = new QComboBox;
    m_microphone->addItem(tr("System default"), QByteArray());
    for (const QAudioDevice &d : QMediaDevices::audioInputs())
        m_microphone->addItem(d.description(), d.id());
    recordForm->addRow(tr("Microphone:"), m_microphone);

    m_shortcutLabel = new QLabel;
    m_shortcutLabel->setWordWrap(true);
    m_shortcutButton = new QPushButton(tr("Change…"));
    auto *shortcutRow = new QHBoxLayout;
    shortcutRow->addWidget(m_shortcutLabel, 1);
    shortcutRow->addWidget(m_shortcutButton);
    recordForm->addRow(tr("Shortcut:"), shortcutRow);
    recordForm->addRow(QString(), hint(tr("Tap: records until the next tap.\n"
                                          "Hold: records while the key is held.\n"
                                          "Any key can also run the command “sotto --toggle”.")));

    // Output
    auto *pasteGroup = new QGroupBox(tr("Pasting"));
    auto *outForm = new QFormLayout(pasteGroup);
    m_paste = new QRadioButton(tr("Paste into the active window"));
    m_copyOnly = new QRadioButton(tr("Only copy to the clipboard"));
    outForm->addRow(tr("Result:"), m_paste);
    outForm->addRow(QString(), m_copyOnly);
    m_pasteKeys = new QComboBox;
    m_pasteKeys->addItem(QStringLiteral("Ctrl+V"), QStringLiteral("ctrl+v"));
    m_pasteKeys->addItem(tr("Ctrl+Shift+V (terminals)"), QStringLiteral("ctrl+shift+v"));
    m_pasteKeys->addItem(QStringLiteral("Shift+Insert"), QStringLiteral("shift+insert"));
    outForm->addRow(tr("Paste keys:"), m_pasteKeys);
    connect(m_paste, &QRadioButton::toggled, m_pasteKeys, &QWidget::setEnabled);

    m_pasteAccess = new QLabel;
    m_pasteAccessButton = new QPushButton(tr("Allow…"));
    auto *accessRow = new QHBoxLayout;
    accessRow->addWidget(m_pasteAccess, 1);
    accessRow->addWidget(m_pasteAccessButton);
    outForm->addRow(tr("Input access:"), accessRow);
    outForm->addRow(QString(), hint(tr("The system asks once for “remote control” permission:\n"
                                       "Sotto uses it to press the paste keys. Tick “Remember”.")));
    connect(m_pasteAccessButton, &QPushButton::clicked, m_output, &TextOutput::prepare);
    connect(m_output, &TextOutput::accessChanged, this, &SettingsDialog::updatePasteAccess);

    // General
    auto *general = new QGroupBox(tr("General"));
    auto *genForm = new QFormLayout(general);
    m_autostart = new QCheckBox(tr("Start at login"));
    genForm->addRow(QString(), m_autostart);
    m_unload = new QSpinBox;
    m_unload->setRange(0, 240);
    m_unload->setSuffix(tr(" min"));
    m_unload->setSpecialValueText(tr("never"));
    genForm->addRow(tr("Unload the model after:"), m_unload);
    auto *uninstall = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-delete")), tr("Remove Sotto…"));
    genForm->addRow(tr("Removal:"), uninstall);
    connect(uninstall, &QPushButton::clicked, this, &SettingsDialog::uninstallRequested);
    auto *resetPos = new QPushButton(tr("Reset position"));
    genForm->addRow(tr("Indicator:"), resetPos);
    genForm->addRow(QString(), hint(tr("Drag the indicator with the mouse.\nClick to stop recording, right-click to cancel.")));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    // Groups become tab pages: no frame, no duplicate title.
    auto page = [](std::initializer_list<QGroupBox *> groups) {
        auto *w = new QWidget;
        auto *l = new QVBoxLayout(w);
        for (QGroupBox *g : groups) {
            if (groups.size() == 1) {
                g->setTitle(QString());
                g->setFlat(true);
            }
            l->addWidget(g);
        }
        l->addStretch();
        return w;
    };
    auto *tabs = new QTabWidget;
    tabs->setDocumentMode(true);
    tabs->addTab(page({recognition}), tr("Recognition"));
    tabs->addTab(page({recording, pasteGroup}), tr("Recording and pasting"));
    tabs->addTab(page({general}), tr("General"));

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_shortcutButton, &QPushButton::clicked, m_shortcut, &GlobalShortcut::configure);
    connect(m_shortcut, &GlobalShortcut::changed, this, &SettingsDialog::updateShortcutLabel);
    connect(resetPos, &QPushButton::clicked, this, [this] {
        Config::resetOverlayPosition();
        Q_EMIT resetOverlayRequested();
    });
    connect(openFolder, &QPushButton::clicked, this, [] {
        QDir().mkpath(Models::dir());
        QDesktopServices::openUrl(QUrl::fromLocalFile(Models::dir()));
    });

    connect(m_model, &QComboBox::currentIndexChanged, this, &SettingsDialog::updateModelRow);
    connect(m_source, &QComboBox::currentIndexChanged, this, [this] {
        Config::setModelSource(m_source->currentData().toString()); // used by the downloader right away
        updateSourceLink();
    });
    connect(m_modelAction, &QPushButton::clicked, this, [this] {
        const QString id = selectedModel();
        if (m_downloader->isRunning()) {
            m_downloader->cancel();
        } else if (Models::isInstalled(id)) {
            if (QMessageBox::question(this, tr("Delete model"), tr("Delete the “%1” model from disk?").arg(Models::find(id).title))
                == QMessageBox::Yes) {
                QFile::remove(Models::path(id));
                fillModels();
            }
        } else {
            m_downloader->start(id);
            updateModelRow();
        }
    });
    connect(m_downloader, &ModelDownloader::progress, this, [this](qint64 received, qint64 total) {
        m_progress->setRange(0, total > 0 ? 1000 : 0);
        if (total > 0)
            m_progress->setValue(int(received * 1000 / total));
        m_progress->setFormat(tr("%1 of %2 MB").arg(received >> 20).arg(total >> 20));
    });
    connect(m_downloader, &ModelDownloader::finished, this, [this](const QString &, bool ok, const QString &error) {
        if (!ok && !error.isEmpty())
            QMessageBox::warning(this, tr("Model download"), tr("Could not download the model:\n%1").arg(error));
        fillModels();
    });

    load();
    resize(sizeHint().expandedTo(QSize(520, 0)));
}

void SettingsDialog::load()
{
    m_source->setCurrentIndex(std::max(0, m_source->findData(Config::modelSource())));
    updateSourceLink();
    m_vad->setChecked(Config::useVad());
    updatePasteAccess();
    fillModels();
    m_language->setCurrentIndex(std::max(0, m_language->findData(Config::language())));
    m_gpu->setChecked(m_haveGpu && Config::useGpu());
    m_prompt->setText(Config::prompt());
    m_microphone->setCurrentIndex(std::max(0, m_microphone->findData(Config::inputDevice())));
    updateShortcutLabel();
    (Config::autoPaste() ? m_paste : m_copyOnly)->setChecked(true);
    m_pasteKeys->setEnabled(Config::autoPaste());
    m_pasteKeys->setCurrentIndex(std::max(0, m_pasteKeys->findData(Config::pasteKeys())));
    m_autostart->setChecked(Config::autostart());
    m_unload->setValue(Config::unloadMinutes());
}

void SettingsDialog::fillModels()
{
    QString current = m_model->count() ? selectedModel() : Config::model();
    if (current.isEmpty())
        current = Models::recommended(m_haveGpu);

    const QSignalBlocker blocker(m_model);
    m_model->clear();
    const QString recommended = Models::recommended(m_haveGpu);
    auto add = [&](const ModelInfo &m) {
        QString text = m.sizeMb ? tr("%1 — %2 MB").arg(m.title).arg(m.sizeMb) : m.title;
        if (m.id == recommended)
            text += tr(" (recommended)");
        const QIcon icon = Models::isInstalled(m.id) ? QIcon::fromTheme(QStringLiteral("emblem-ok-symbolic"),
                                                                       QIcon::fromTheme(QStringLiteral("dialog-ok")))
                                                     : QIcon();
        m_model->addItem(icon, text, m.id);
    };
    for (const auto &m : Models::catalog())
        add(m);
    for (const QString &id : Models::customIds())
        add(Models::find(id));
    m_model->setCurrentIndex(std::max(0, m_model->findData(current)));
    updateModelRow();
}

void SettingsDialog::updateModelRow()
{
    const QString id = selectedModel();
    const bool downloading = m_downloader->isRunning();
    const bool installed = Models::isInstalled(id);

    m_progress->setVisible(downloading);
    m_model->setEnabled(!downloading);
    if (downloading)
        m_modelAction->setText(tr("Cancel"));
    else
        m_modelAction->setText(installed ? tr("Delete") : tr("Download"));

    const ModelInfo info = Models::find(id);
    m_modelNote->setText(installed ? tr("Downloaded — %1").arg(info.note) : tr("Not downloaded — %1").arg(info.note));
}

void SettingsDialog::updateShortcutLabel()
{
    m_shortcutButton->setEnabled(m_shortcut->isAvailable());
    if (!m_shortcut->isAvailable() && !m_shortcut->error().isEmpty())
        m_shortcutLabel->setText(tr("unavailable: %1").arg(m_shortcut->error()));
    else if (!m_shortcut->isAvailable())
        m_shortcutLabel->setText(tr("connecting…"));
    else if (m_shortcut->trigger().isEmpty())
        m_shortcutLabel->setText(tr("not assigned"));
    else
        m_shortcutLabel->setText(QStringLiteral("<b>%1</b>").arg(m_shortcut->trigger().toHtmlEscaped()));
}

void SettingsDialog::updateSourceLink()
{
    const QString url = Models::pageUrl();
    m_sourceLink->setText(QStringLiteral("<a href=\"%1\">%2</a>").arg(url, QUrl(url).host() + QUrl(url).path()));
    m_sourceLink->setToolTip(tr("whisper.cpp models: %1\nVAD: %2").arg(url, Models::url(Models::VadId)));
}

void SettingsDialog::updatePasteAccess()
{
    const bool granted = m_output->hasAccess();
    m_pasteAccess->setText(granted ? tr("granted") : tr("not requested yet"));
    m_pasteAccessButton->setVisible(!granted);
}

QString SettingsDialog::selectedModel() const
{
    return m_model->currentData().toString();
}

void SettingsDialog::accept()
{
    Config::setModel(selectedModel());
    Config::setLanguage(m_language->currentData().toString());
    if (m_haveGpu)
        Config::setUseGpu(m_gpu->isChecked());
    Config::setPrompt(m_prompt->text().trimmed());
    Config::setInputDevice(m_microphone->currentData().toByteArray());
    Config::setAutoPaste(m_paste->isChecked());
    Config::setPasteKeys(m_pasteKeys->currentData().toString());
    if (m_autostart->isChecked() != Config::autostart())
        Config::setAutostart(m_autostart->isChecked());
    Config::setUnloadMinutes(m_unload->value());
    Config::setUseVad(m_vad->isChecked());
    Config::setModelSource(m_source->currentData().toString());
    QDialog::accept();
}
