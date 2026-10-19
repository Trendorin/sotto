#include "app.h"
#include "log.h"
#include "config.h"
#include "models.h"
#include "output.h"
#include "overlay.h"
#include "recorder.h"
#include "settingsdialog.h"
#include "shortcut.h"
#include "transcriber.h"
#include "uninstaller.h"

#include <QAction>
#include <QApplication>
#include <QMenu>

#include <cmath>

static constexpr qint64 HoldThresholdMs = 400;

static QIcon themedIcon(const char *name, const char *fallback)
{
    return QIcon::fromTheme(QString::fromLatin1(name), QIcon::fromTheme(QString::fromLatin1(fallback),
                                                                        QIcon(QStringLiteral(":/sotto.svg"))));
}

// Loudest 20 ms window, as RMS. Used to skip whisper on pure silence,
// where it tends to invent text.
static float peakRms(const std::vector<float> &pcm)
{
    constexpr size_t window = Recorder::SampleRate / 50;
    float peak = 0.f;
    for (size_t start = 0; start + window <= pcm.size(); start += window) {
        double sum = 0.0;
        for (size_t i = start; i < start + window; ++i)
            sum += double(pcm[i]) * pcm[i];
        peak = std::max(peak, float(std::sqrt(sum / window)));
    }
    return peak;
}

App::App(QObject *parent)
    : QObject(parent)
    , m_recorder(new Recorder(this))
    , m_transcriber(new Transcriber(this))
    , m_output(new TextOutput(this))
    , m_shortcut(new GlobalShortcut(this))
    , m_downloader(new ModelDownloader(this))
    , m_overlay(new Overlay)
    , m_tray(new QSystemTrayIcon(this))
{
    connect(m_recorder, &Recorder::level, m_overlay, &Overlay::pushLevel);
    connect(m_recorder, &Recorder::failed, this, &App::onError);
    connect(m_transcriber, &Transcriber::finished, this, &App::onTranscribed);
    connect(m_transcriber, &Transcriber::progress, m_overlay, &Overlay::setProgress);
    connect(m_transcriber, &Transcriber::failed, this, &App::onError);
    connect(m_output, &TextOutput::failed, this, [this](const QString &msg) { m_overlay->showMessage(msg, true); });
    connect(m_shortcut, &GlobalShortcut::pressed, this, &App::onShortcutPressed);
    connect(m_shortcut, &GlobalShortcut::released, this, &App::onShortcutReleased);
    connect(m_overlay, &Overlay::clicked, this, [this] {
        if (m_state == State::Recording)
            stopRecording();
        else if (m_state == State::Idle)
            m_overlay->dismiss();
    });
    connect(m_overlay, &Overlay::cancelRequested, this, &App::cancel);

    connect(m_downloader, &ModelDownloader::finished, this, [this](const QString &id, bool ok, const QString &) {
        if (!ok || id == Models::VadId)
            return;
        if (!Models::isInstalled(Config::model()))
            Config::setModel(id);
        const QString key = m_shortcut->trigger();
        m_tray->showMessage(tr("Model downloaded"),
                            key.isEmpty() ? tr("Ready for dictation.") : tr("Press %1 and speak.").arg(key),
                            QIcon(QStringLiteral(":/sotto.svg")));
    });

    m_unloadTimer.setSingleShot(true);
    connect(&m_unloadTimer, &QTimer::timeout, m_transcriber, &Transcriber::unload);

    auto *menu = new QMenu;
    m_toggleAction = menu->addAction(tr("Start dictation"), this, &App::toggle);
    m_cancelAction = menu->addAction(QIcon::fromTheme(QStringLiteral("dialog-cancel")), tr("Cancel"), this, &App::cancel);
    menu->addSeparator();
    menu->addAction(QIcon::fromTheme(QStringLiteral("configure")), tr("Settings…"), this, &App::showSettings);
    menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), tr("Quit"), qApp, &QApplication::quit);
    connect(m_output, &TextOutput::failed, this, [](const QString &msg) { qCWarning(lcSotto) << msg; });
    m_tray->setContextMenu(menu);
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger)
            showSettings();
        else if (reason == QSystemTrayIcon::MiddleClick)
            toggle();
    });
    setState(State::Idle);
}

App::~App()
{
    m_recorder->stop();
    delete m_overlay;
    delete m_tray->contextMenu();
}

void App::start()
{
    m_tray->show();
    m_shortcut->setup();
    if (!Models::isInstalled(Config::model())) {
        showSettings();
        return;
    }
    // Older installs have a model but no VAD yet: fetch it quietly (under 1 MB).
    if (Config::useVad() && !Models::isInstalled(Models::VadId))
        m_downloader->start(Models::VadId);
    // A saved permission restores silently, without a dialog.
    if (Config::autoPaste() && !Config::remoteDesktopToken().isEmpty())
        m_output->prepare();

    const QString model = Config::model();
    if (Transcriber::gpuName().isEmpty() && (model.startsWith(QLatin1String("large")) || model.startsWith(QLatin1String("medium"))))
        m_tray->showMessage(tr("The GPU is not used"),
                            tr("The %1 model is slow on a CPU. Install a Vulkan driver "
                               "or choose the Small model in Settings.").arg(Models::find(model).title),
                            QSystemTrayIcon::Warning, 10000);
}

void App::uninstall()
{
    cancel();
    if (Uninstaller::run(m_settings))
        qApp->quit();
}

void App::command(const QString &cmd)
{
    if (cmd == QLatin1String("toggle"))
        toggle();
    else if (cmd == QLatin1String("start") && m_state == State::Idle)
        startRecording();
    else if (cmd == QLatin1String("stop") && m_state == State::Recording)
        stopRecording();
    else if (cmd == QLatin1String("cancel"))
        cancel();
    else if (cmd == QLatin1String("settings"))
        showSettings();
    else if (cmd == QLatin1String("uninstall"))
        uninstall();
    else if (cmd == QLatin1String("quit"))
        qApp->quit();
}

void App::toggle()
{
    if (m_state == State::Idle)
        startRecording();
    else if (m_state == State::Recording)
        stopRecording();
}

void App::onShortcutPressed()
{
    qCInfo(lcSotto) << "shortcut pressed";
    if (m_seenRelease && m_keyDown)
        return; // key auto-repeat
    m_keyDown = true;
    m_pressClock.start();
    toggle();
}

void App::onShortcutReleased()
{
    qCInfo(lcSotto) << "shortcut released after" << (m_pressClock.isValid() ? m_pressClock.elapsed() : -1) << "ms";
    m_seenRelease = true;
    m_keyDown = false;
    // A long press means push-to-talk: releasing ends the recording.
    // A short tap leaves it running until the next tap.
    if (m_state == State::Recording && m_pressClock.isValid() && m_pressClock.elapsed() > HoldThresholdMs)
        stopRecording();
}

void App::startRecording()
{
    const QString model = Config::model();
    if (!Models::isInstalled(model)) {
        m_overlay->showMessage(tr("Download a model in Settings first"), true);
        showSettings();
        return;
    }
    if (!m_recorder->start(Config::inputDevice()))
        return;

    m_unloadTimer.stop();
    m_transcriber->preload(Models::path(model), Config::useGpu()); // warm up while the user talks
    m_overlay->showRecording();
    setState(State::Recording);
}

void App::stopRecording()
{
    if (m_state != State::Recording)
        return;
    std::vector<float> pcm = m_recorder->stop();
    m_pressClock.invalidate();

    if (pcm.size() < size_t(Recorder::SampleRate) / 4) {
        m_overlay->dismiss();
        setState(State::Idle);
        return;
    }
    const float peak = peakRms(pcm);
    qCInfo(lcSotto) << "peak level" << peak;
    if (peak < 0.004f) {
        m_overlay->showMessage(tr("No speech heard — check the microphone"), true);
        setState(State::Idle);
        return;
    }

    setState(State::Transcribing);
    m_overlay->showBusy();
    const bool vad = Config::useVad() && Models::isInstalled(Models::VadId);
    m_transcriber->transcribe(std::move(pcm), {Models::path(Config::model()), Config::useGpu(), Config::language(),
                                               Config::prompt(), vad ? Models::path(Models::VadId) : QString()});
}

void App::cancel()
{
    if (m_state == State::Recording)
        m_recorder->stop();
    else if (m_state == State::Transcribing)
        m_transcriber->cancel();
    m_overlay->dismiss();
    setState(State::Idle);
    scheduleUnload();
}

void App::onTranscribed(const QString &text)
{
    if (m_state != State::Transcribing)
        return; // cancelled meanwhile
    setState(State::Idle);
    scheduleUnload();

    qCInfo(lcSotto) << "delivering" << text.size() << "chars, paste:" << Config::autoPaste();
    if (text.isEmpty()) {
        m_overlay->showMessage(tr("Nothing recognized"));
        return;
    }
    if (Config::autoPaste())
        m_overlay->dismiss();
    else
        m_overlay->showMessage(tr("Copied: %1").arg(text));
    m_output->deliver(text);
}

void App::onError(const QString &message)
{
    qCWarning(lcSotto) << "error:" << message;
    if (m_state == State::Recording)
        m_recorder->stop();
    setState(State::Idle);
    m_overlay->showMessage(message, true);
}

void App::scheduleUnload()
{
    if (const int minutes = Config::unloadMinutes(); minutes > 0)
        m_unloadTimer.start(minutes * 60 * 1000);
}

void App::showSettings()
{
    if (!m_settings) {
        m_settings = new SettingsDialog(m_downloader, m_shortcut, m_output);
        connect(m_settings, &SettingsDialog::uninstallRequested, this, &App::uninstall);
        connect(m_settings, &SettingsDialog::resetOverlayRequested, m_overlay, &Overlay::resetPosition);
        connect(m_settings, &QDialog::accepted, this, &App::scheduleUnload);
    }
    m_settings->show();
    m_settings->raise();
    m_settings->activateWindow();
}

void App::setState(State state)
{
    m_state = state;
    const bool recording = state == State::Recording;
    m_tray->setIcon(recording ? themedIcon("media-record-symbolic", "media-record")
                              : themedIcon("audio-input-microphone-symbolic", "audio-input-microphone"));
    m_tray->setToolTip(recording ? tr("Sotto — recording")
                       : state == State::Transcribing ? tr("Sotto — transcribing…")
                                                      : tr("Sotto — voice typing"));
    m_toggleAction->setText(recording ? tr("Stop and paste") : tr("Start dictation"));
    m_toggleAction->setEnabled(state != State::Transcribing);
    m_cancelAction->setEnabled(state != State::Idle);
}
