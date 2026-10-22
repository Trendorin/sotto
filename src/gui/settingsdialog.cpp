#include "gui/settingsdialog.h"
#include "core/config.h"
#include "core/gpu.h"
#include "core/models.h"
#include "system/desktop.h"
#include "system/output.h"
#include "system/shortcut.h"

#include <QAudioDevice>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMediaDevices>
#include <QMessageBox>
#include <QPlainTextEdit>
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
    label->setWordWrap(true);
    return label;
}

// A tab page: the form, then a stretch so it stays at the top.
static QWidget *page(QFormLayout *form)
{
    auto *w = new QWidget;
    auto *l = new QVBoxLayout(w);
    l->addLayout(form);
    l->addStretch();
    return w;
}

SettingsDialog::SettingsDialog(ModelDownloader *downloader, GlobalShortcut *shortcut, TextOutput *output, QWidget *parent)
    : QDialog(parent)
    , m_downloader(downloader)
    , m_shortcut(shortcut)
    , m_output(output)
    , m_haveGpu(!Gpu::devices().isEmpty())
{
    setWindowTitle(tr("Settings"));
    setWindowIcon(QIcon::fromTheme(QString::fromLatin1(Config::AppId), QIcon(QStringLiteral(":/sotto.svg"))));
    setAttribute(Qt::WA_DeleteOnClose);

    auto *tabs = new QTabWidget;
    tabs->setDocumentMode(true);
    tabs->addTab(recognitionPage(), tr("Recognition"));
    tabs->addTab(recordingPage(), tr("Recording and pasting"));
    tabs->addTab(generalPage(), tr("General"));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    layout->addWidget(buttons);

    load();
    resize(sizeHint().expandedTo(QSize(640, 0)));
}

QWidget *SettingsDialog::recognitionPage()
{
    auto *form = new QFormLayout;

    m_model = new QComboBox;
    m_model->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_modelAction = new QPushButton;
    auto *openFolder = new QPushButton(QIcon::fromTheme(QStringLiteral("folder-open")), QString());
    openFolder->setToolTip(tr("Open the models folder"));
    auto *modelRow = new QHBoxLayout;
    modelRow->addWidget(m_model, 1);
    modelRow->addWidget(m_modelAction);
    modelRow->addWidget(openFolder);
    form->addRow(tr("Model:"), modelRow);

    m_progress = new QProgressBar;
    m_progress->setVisible(false);
    form->addRow(QString(), m_progress);
    m_modelNote = hint(QString());
    form->addRow(QString(), m_modelNote);

    m_source = new QComboBox;
    for (const auto &s : Models::sources())
        m_source->addItem(s.title, s.id);
    m_sourceLink = new QLabel;
    m_sourceLink->setOpenExternalLinks(true);
    m_sourceLink->setTextInteractionFlags(Qt::TextBrowserInteraction);
    auto *sourceRow = new QHBoxLayout;
    sourceRow->addWidget(m_source);
    sourceRow->addWidget(m_sourceLink, 1);
    form->addRow(tr("Source:"), sourceRow);

    m_customUrl = new QLineEdit;
    m_customUrl->setPlaceholderText(tr("https://your-server.example/ (same layout as huggingface.co)"));
    m_customUrl->setClearButtonEnabled(true);
    m_customUrl->setToolTip(tr("Files are fetched from ADDRESS/REPOSITORY/resolve/main/FILE, as on Hugging Face"));
    m_customUrlLabel = new QLabel(tr("Server:"));
    form->addRow(m_customUrlLabel, m_customUrl);
    m_unverified = new QCheckBox(tr("Do not check the published SHA-256 (my own models)"));
    m_unverified->setToolTip(tr("Only for a custom server that hosts files other than the official ones. "
                                "Official models are always verified."));
    form->addRow(QString(), m_unverified);

    m_downloadLog = new QPlainTextEdit;
    m_downloadLog->setReadOnly(true);
    m_downloadLog->setMaximumHeight(96);
    m_downloadLog->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_downloadLog->setPlaceholderText(tr("The download log appears here: server, redirects, checksum."));
    m_downloadLog->setToolTip(tr("Also saved to %1").arg(Models::logFile()));
    form->addRow(tr("Download log:"), m_downloadLog);

    m_language = new QComboBox;
    m_language->addItem(tr("Detect automatically"), QStringLiteral("auto"));
    for (const char *code : {"ru", "en", "uk", "be", "kk", "de", "fr", "es", "it", "pt", "pl", "tr", "zh", "ja", "ko"}) {
        QString name = QLocale(QString::fromLatin1(code)).nativeLanguageName();
        if (!name.isEmpty())
            name[0] = name[0].toUpper();
        m_language->addItem(name, QString::fromLatin1(code));
    }
    form->addRow(tr("Language:"), m_language);

    const QList<Gpu::Device> &gpus = Gpu::devices();
    m_gpu = new QComboBox;
    if (m_haveGpu)
        m_gpu->addItem(tr("Automatic — %1").arg(gpus.constFirst().name), QString());
    for (const Gpu::Device &g : gpus)
        m_gpu->addItem(QStringLiteral("%1 (%2)").arg(g.name, g.key.section(QLatin1Char('/'), 0, 0)), g.key);
    m_gpu->addItem(tr("Processor only"), QStringLiteral("cpu"));
    form->addRow(tr("Acceleration:"), m_gpu);
    if (!m_haveGpu)
        form->addRow(QString(), hint(tr("No GPU for Whisper: install a Vulkan driver (on Arch also ggml-vulkan). "
                                        "GigaAM and Parakeet are fast on the processor anyway.")));
    if (const QString missing = Gpu::missingHint(); !missing.isEmpty()) {
        auto *warning = new QLabel(QStringLiteral("⚠ ") + missing);
        warning->setTextFormat(Qt::PlainText);
        warning->setWordWrap(true);
        form->addRow(QString(), warning);
    }

    m_prompt = new QLineEdit;
    m_prompt->setPlaceholderText(tr("For example: Kubernetes, Grafana, Jane Doe"));
    m_prompt->setClearButtonEnabled(true);
    form->addRow(tr("Vocabulary:"), m_prompt);
    form->addRow(QString(), hint(tr("Names and terms you often say — Whisper will spell them correctly.")));

    connect(openFolder, &QPushButton::clicked, this, [] {
        QDir().mkpath(Models::dir());
        QDesktopServices::openUrl(QUrl::fromLocalFile(Models::dir()));
    });
    connect(m_model, &QComboBox::currentIndexChanged, this, &SettingsDialog::updateModelRow);
    connect(m_source, &QComboBox::currentIndexChanged, this, [this] {
        Config::setModelSource(m_source->currentData().toString()); // used by the downloader right away
        updateSourceLink();
    });
    connect(m_customUrl, &QLineEdit::textChanged, this, [this] {
        Config::setCustomSourceUrl(m_customUrl->text());
        updateSourceLink();
    });
    connect(m_unverified, &QCheckBox::toggled, this, [](bool on) { Config::setAllowUnverified(on); });
    connect(m_downloader, &ModelDownloader::log, this, [this](const QString &line) {
        m_downloadLog->appendPlainText(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")) + QLatin1Char(' ') + line);
    });
    connect(m_modelAction, &QPushButton::clicked, this, [this] {
        const QString id = selectedModel();
        if (m_downloader->isRunning()) {
            m_downloader->cancel();
        } else if (Models::isInstalled(id)) {
            if (QMessageBox::question(this, tr("Delete model"), tr("Delete the “%1” model from disk?").arg(Models::find(id).title))
                == QMessageBox::Yes) {
                Models::remove(id);
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
    return page(form);
}

QWidget *SettingsDialog::recordingPage()
{
    auto *form = new QFormLayout;
    m_microphone = new QComboBox;
    m_microphone->addItem(tr("System default"), QByteArray());
    for (const QAudioDevice &d : QMediaDevices::audioInputs())
        m_microphone->addItem(d.description(), d.id());
    form->addRow(tr("Microphone:"), m_microphone);

    m_shortcutLabel = new QLabel;
    m_shortcutLabel->setWordWrap(true);
    m_shortcutButton = new QPushButton(tr("Change…"));
    auto *shortcutRow = new QHBoxLayout;
    shortcutRow->addWidget(m_shortcutLabel, 1);
    shortcutRow->addWidget(m_shortcutButton);
    form->addRow(tr("Shortcut:"), shortcutRow);

    // For compositors that bind keys in their own configuration (niri, Hyprland, sway, ...).
    const Desktop::Binding binding = Desktop::binding();
    m_binding = new QWidget;
    auto *bindingLayout = new QVBoxLayout(m_binding);
    bindingLayout->setContentsMargins(0, 0, 0, 0);
    m_bindingWhere = new QLabel(tr("On %1 bind a key yourself — add to %2:").arg(Desktop::name(), binding.where));
    m_bindingWhere->setWordWrap(true);
    m_bindingWhere->setTextFormat(Qt::PlainText);
    m_bindingSnippet = new QPlainTextEdit(binding.snippet);
    m_bindingSnippet->setReadOnly(true);
    m_bindingSnippet->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_bindingSnippet->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_bindingSnippet->setFixedHeight(m_bindingSnippet->fontMetrics().lineSpacing() * 5 + 12);
    auto *copy = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-copy")), tr("Copy"));
    auto *copyRow = new QHBoxLayout;
    copyRow->addStretch();
    copyRow->addWidget(copy);
    bindingLayout->addWidget(m_bindingWhere);
    bindingLayout->addWidget(m_bindingSnippet);
    bindingLayout->addLayout(copyRow);
    form->addRow(m_binding); // the whole width: config lines are long
    connect(copy, &QPushButton::clicked, this, [this] { QGuiApplication::clipboard()->setText(m_bindingSnippet->toPlainText()); });

    form->addRow(QString(), hint(tr("Tap: records until the next tap. Hold: records while the key is held. "
                                    "Any key can also run the command “sotto --toggle”.")));

    m_live = new QCheckBox(tr("Type each phrase as soon as I pause"));
    m_live->setToolTip(tr("Phrases are recognised while you go on talking. Off: everything is pasted when the recording stops."));
    form->addRow(tr("Typing:"), m_live);
    m_pause = new QSpinBox;
    m_pause->setRange(200, 3000);
    m_pause->setSingleStep(100);
    m_pause->setSuffix(tr(" ms"));
    form->addRow(tr("Pause that ends a phrase:"), m_pause);

    m_paste = new QRadioButton(tr("Paste into the active window"));
    m_copyOnly = new QRadioButton(tr("Only copy to the clipboard"));
    form->addRow(tr("Result:"), m_paste);
    form->addRow(QString(), m_copyOnly);
    m_pasteKeys = new QComboBox;
    m_pasteKeys->addItem(QStringLiteral("Ctrl+V"), QStringLiteral("ctrl+v"));
    m_pasteKeys->addItem(tr("Ctrl+Shift+V (terminals)"), QStringLiteral("ctrl+shift+v"));
    m_pasteKeys->addItem(QStringLiteral("Shift+Insert"), QStringLiteral("shift+insert"));
    form->addRow(tr("Paste keys:"), m_pasteKeys);

    const QStringList available = TextOutput::available();
    m_inputMethod = new QComboBox;
    m_inputMethod->addItem(available.isEmpty() ? tr("Automatic — nothing found") : tr("Automatic — %1").arg(available.constFirst()),
                           QStringLiteral("auto"));
    m_inputMethod->addItem(tr("Desktop portal (KDE, GNOME, COSMIC)"), QStringLiteral("portal"));
    m_inputMethod->addItem(tr("wtype (Hyprland, niri, sway, river)"), QStringLiteral("wtype"));
    m_inputMethod->addItem(tr("ydotool (any desktop, needs ydotoold)"), QStringLiteral("ydotool"));
    m_inputMethod->addItem(tr("xdotool (X11)"), QStringLiteral("xdotool"));
    form->addRow(tr("Press keys with:"), m_inputMethod);

    // Only the portal asks for permission; wtype, ydotool and xdotool just work.
    m_pasteAccess = new QLabel;
    m_pasteAccessButton = new QPushButton(tr("Allow…"));
    m_portalAccess = new QWidget;
    auto *accessLayout = new QVBoxLayout(m_portalAccess);
    accessLayout->setContentsMargins(0, 0, 0, 0);
    auto *accessRow = new QHBoxLayout;
    accessRow->addWidget(m_pasteAccess, 1);
    accessRow->addWidget(m_pasteAccessButton);
    accessLayout->addLayout(accessRow);
    accessLayout->addWidget(hint(tr("The system asks once for “remote control” permission: "
                                    "Sotto uses it to press the paste keys. Tick “Remember”.")));
    m_portalAccessLabel = new QLabel(tr("Input access:"));
    form->addRow(m_portalAccessLabel, m_portalAccess);

    connect(m_paste, &QRadioButton::toggled, this, [this](bool on) {
        m_pasteKeys->setEnabled(on);
        m_inputMethod->setEnabled(on);
        m_live->setEnabled(on);
    });
    connect(m_inputMethod, &QComboBox::currentIndexChanged, this, &SettingsDialog::updatePasteAccess);
    connect(m_pasteAccessButton, &QPushButton::clicked, m_output, &TextOutput::prepare);
    connect(m_output, &TextOutput::accessChanged, this, &SettingsDialog::updatePasteAccess);
    connect(m_shortcutButton, &QPushButton::clicked, m_shortcut, &GlobalShortcut::configure);
    connect(m_shortcut, &GlobalShortcut::changed, this, &SettingsDialog::updateShortcutLabel);
    return page(form);
}

QWidget *SettingsDialog::generalPage()
{
    auto *form = new QFormLayout;
    m_uiLanguage = new QComboBox;
    m_uiLanguage->addItem(tr("Follow system"), QStringLiteral("auto"));
    m_uiLanguage->addItem(QStringLiteral("English"), QStringLiteral("en"));
    m_uiLanguage->addItem(QStringLiteral("Русский"), QStringLiteral("ru"));
    form->addRow(tr("Interface language:"), m_uiLanguage);
    m_autostart = new QCheckBox(tr("Start at login"));
    form->addRow(QString(), m_autostart);
    m_unload = new QSpinBox;
    m_unload->setRange(0, 240);
    m_unload->setSuffix(tr(" min"));
    m_unload->setSpecialValueText(tr("never"));
    form->addRow(tr("Unload the model after:"), m_unload);
    auto *uninstall = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-delete")), tr("Remove Sotto…"));
    form->addRow(tr("Removal:"), uninstall);
    auto *resetPos = new QPushButton(tr("Reset position"));
    form->addRow(tr("Indicator:"), resetPos);
    form->addRow(QString(), hint(tr("Drag the indicator with the mouse. Click to stop recording, right-click to cancel.")));

    connect(uninstall, &QPushButton::clicked, this, &SettingsDialog::uninstallRequested);
    connect(resetPos, &QPushButton::clicked, this, [this] {
        Config::resetOverlayPosition();
        Q_EMIT resetOverlayRequested();
    });
    return page(form);
}

void SettingsDialog::load()
{
    m_source->setCurrentIndex(std::max(0, m_source->findData(Config::modelSource())));
    m_customUrl->setText(Config::customSourceUrl());
    m_unverified->setChecked(Config::allowUnverified());
    updateSourceLink();
    fillModels();
    m_language->setCurrentIndex(std::max(0, m_language->findData(Config::language())));
    const QString gpu = !m_haveGpu || !Config::useGpu() ? QStringLiteral("cpu") : Config::gpuDevice();
    if (m_gpu->findData(gpu) < 0) // chosen earlier, missing now: keep the choice
        m_gpu->insertItem(m_gpu->count() - 1, tr("%1 (not found)").arg(gpu.section(QLatin1Char('/'), 1)), gpu);
    m_gpu->setCurrentIndex(m_gpu->findData(gpu));
    m_prompt->setText(Config::prompt());
    m_microphone->setCurrentIndex(std::max(0, m_microphone->findData(Config::inputDevice())));
    updateShortcutLabel();
    m_live->setChecked(Config::liveTyping());
    m_pause->setValue(Config::pauseMs());
    (Config::autoPaste() ? m_paste : m_copyOnly)->setChecked(true);
    m_pasteKeys->setEnabled(Config::autoPaste());
    m_inputMethod->setEnabled(Config::autoPaste());
    m_live->setEnabled(Config::autoPaste());
    m_pasteKeys->setCurrentIndex(std::max(0, m_pasteKeys->findData(Config::pasteKeys())));
    m_inputMethod->setCurrentIndex(std::max(0, m_inputMethod->findData(Config::inputMethod())));
    updatePasteAccess();
    m_uiLanguage->setCurrentIndex(std::max(0, m_uiLanguage->findData(Config::uiLanguage())));
    m_autostart->setChecked(Config::autostart());
    m_unload->setValue(Config::unloadMinutes());
}

void SettingsDialog::fillModels()
{
    QString current = m_model->count() ? selectedModel() : Config::model();
    const QString language = Config::language() == QLatin1String("auto") ? QLocale().name() : Config::language();
    const QString recommended = Models::recommended(m_haveGpu, language);
    if (current.isEmpty())
        current = recommended;

    const QSignalBlocker blocker(m_model);
    m_model->clear();
    auto add = [&](const ModelInfo &m) {
        QString text = m.sizeMb() ? tr("%1 — %2 MB").arg(m.title).arg(m.sizeMb()) : m.title;
        if (m.id == recommended)
            text += tr(" (recommended)");
        const QIcon icon = Models::isInstalled(m.id) ? QIcon::fromTheme(QStringLiteral("emblem-ok-symbolic"),
                                                                       QIcon::fromTheme(QStringLiteral("dialog-ok")))
                                                     : QIcon();
        m_model->addItem(icon, text, m.id);
    };
    // Engines in groups: sherpa-onnx models first, then Whisper, then the user's own files.
    Backend group = Models::catalog().constFirst().backend;
    for (const auto &m : Models::catalog()) {
        if (m.backend != group) {
            m_model->insertSeparator(m_model->count());
            group = m.backend;
        }
        add(m);
    }
    if (const QStringList custom = Models::customIds(); !custom.isEmpty()) {
        m_model->insertSeparator(m_model->count());
        for (const QString &id : custom)
            add(Models::find(id));
    }
    m_model->setCurrentIndex(std::max(0, m_model->findData(current)));
    updateModelRow();
}

void SettingsDialog::updateModelRow()
{
    const bool downloading = m_downloader->isRunning();
    m_source->setEnabled(!downloading);
    m_customUrl->setEnabled(!downloading);
    m_unverified->setEnabled(!downloading);
    m_progress->setVisible(downloading);
    m_model->setEnabled(!downloading);

    const QString id = selectedModel();
    const bool installed = Models::isInstalled(id);
    if (downloading)
        m_modelAction->setText(tr("Cancel"));
    else
        m_modelAction->setText(installed ? tr("Delete") : tr("Download"));

    const ModelInfo info = Models::find(id);
    QString note = installed ? tr("Downloaded — %1").arg(info.note) : tr("Not downloaded — %1").arg(info.note);
    const bool whisper = info.backend == Backend::Whisper;
    if (!whisper)
        note += QLatin1Char('\n') + (info.language == QLatin1String("ru") ? tr("Recognises Russian; runs on the processor.")
                                                                          : tr("Finds the language itself; runs on the processor."));
    m_modelNote->setText(note);
    // Language, GPU and vocabulary are Whisper settings.
    m_language->setEnabled(whisper);
    m_gpu->setEnabled(whisper && m_haveGpu);
    m_prompt->setEnabled(whisper);
    updateSourceLink();
}

void SettingsDialog::updateShortcutLabel()
{
    m_shortcutButton->setEnabled(m_shortcut->isAvailable());
    m_shortcutButton->setVisible(m_shortcut->isAvailable() || m_shortcut->error().isEmpty());
    if (!m_shortcut->isAvailable() && !m_shortcut->hasPortal() && !m_shortcut->error().isEmpty())
        m_shortcutLabel->setText(tr("%1 has no shortcut portal").arg(Desktop::name()));
    else if (!m_shortcut->isAvailable() && !m_shortcut->error().isEmpty())
        m_shortcutLabel->setText(tr("unavailable: %1").arg(m_shortcut->error()));
    else if (!m_shortcut->isAvailable())
        m_shortcutLabel->setText(tr("connecting…"));
    else if (m_shortcut->trigger().isEmpty())
        m_shortcutLabel->setText(tr("not assigned"));
    else
        m_shortcutLabel->setText(QStringLiteral("<b>%1</b>").arg(m_shortcut->trigger().toHtmlEscaped()));
    // Without a key from the portal, show how to bind one in the compositor.
    m_binding->setVisible(!m_shortcut->error().isEmpty() || (m_shortcut->isAvailable() && m_shortcut->trigger().isEmpty()));
}

void SettingsDialog::updateSourceLink()
{
    const bool custom = Models::customSource();
    m_customUrl->setVisible(custom);
    m_unverified->setVisible(custom);
    m_customUrlLabel->setVisible(custom);
    const QString url = Models::pageUrl(selectedModel());
    if (url.isEmpty()) {
        if (!custom)
            m_sourceLink->clear();
        else
            m_sourceLink->setText(m_customUrl->text().trimmed().isEmpty() ? tr("enter the server address below")
                                                                          : tr("not a valid address (https://…)"));
        m_sourceLink->setToolTip({});
        return;
    }
    // The host is what tells sources apart; the full address of the model page is in the tooltip.
    m_sourceLink->setText(QStringLiteral("<a href=\"%1\">%2</a>").arg(url.toHtmlEscaped(), tr("%1 — model page").arg(QUrl(url).host()).toHtmlEscaped()));
    m_sourceLink->setToolTip(url);
}

void SettingsDialog::updatePasteAccess()
{
    const QString method = m_inputMethod->currentData().toString();
    const bool portal = method == QLatin1String("portal")
                     || (method == QLatin1String("auto") && TextOutput::available().value(0) == QLatin1String("portal"));
    const bool granted = m_output->hasAccess();
    m_pasteAccess->setText(granted ? tr("granted") : tr("not requested yet"));
    m_pasteAccessButton->setVisible(!granted);
    m_portalAccess->setVisible(portal);
    m_portalAccessLabel->setVisible(portal);
}

QString SettingsDialog::selectedModel() const
{
    return m_model->currentData().toString();
}

void SettingsDialog::accept()
{
    Config::setModel(selectedModel());
    Config::setLanguage(m_language->currentData().toString());
    if (m_haveGpu) {
        const QString gpu = m_gpu->currentData().toString();
        Config::setUseGpu(gpu != QLatin1String("cpu"));
        if (gpu != QLatin1String("cpu"))
            Config::setGpuDevice(gpu);
    }
    Config::setPrompt(m_prompt->text().trimmed());
    Config::setInputDevice(m_microphone->currentData().toByteArray());
    Config::setLiveTyping(m_live->isChecked());
    Config::setPauseMs(m_pause->value());
    Config::setAutoPaste(m_paste->isChecked());
    Config::setPasteKeys(m_pasteKeys->currentData().toString());
    Config::setInputMethod(m_inputMethod->currentData().toString());
    if (m_autostart->isChecked() != Config::autostart())
        Config::setAutostart(m_autostart->isChecked());
    if (m_uiLanguage->currentData().toString() != Config::uiLanguage()) {
        Config::setUiLanguage(m_uiLanguage->currentData().toString());
        QMessageBox::information(this, tr("Interface language"), tr("Restart Sotto to apply the language."));
    }
    Config::setUnloadMinutes(m_unload->value());
    Config::setModelSource(m_source->currentData().toString());
    QDialog::accept();
}
