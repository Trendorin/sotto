#include "gui/app.h"
#include "core/config.h"
#include "core/gpu.h"
#include "core/listener.h"
#include "core/log.h"
#include "core/models.h"
#include "core/recorder.h"
#include "core/text.h"
#include "core/transcriber.h"
#include "gui/overlay.h"
#include "gui/settingsdialog.h"
#include "gui/uninstaller.h"
#include "system/desktop.h"
#include "system/output.h"
#include "system/shortcut.h"

#include <QAction>
#include <QApplication>
#include <QMenu>

// A press longer than this is push-to-talk: releasing the key ends the recording.
static constexpr qint64 HoldThresholdMs = 400;
// How much of the previous phrase Whisper gets as context for the next one.
static constexpr int ContextChars = 200;

static QIcon themedIcon(const char *name, const char *fallback)
{
    return QIcon::fromTheme(QString::fromLatin1(name), QIcon::fromTheme(QString::fromLatin1(fallback),
                                                                        QIcon(QStringLiteral(":/sotto.svg"))));
}

App::App(QObject *parent)
    : QObject(parent)
    , m_recorder(new Recorder(this))
    , m_listener(new Listener(this))
    , m_transcriber(new Transcriber(this))
    , m_output(new TextOutput(this))
    , m_shortcut(new GlobalShortcut(this))
    , m_downloader(new ModelDownloader(this))
    , m_overlay(new Overlay)
    , m_tray(new QSystemTrayIcon(this))
{
    connect(m_recorder, &Recorder::audio, m_listener, &Listener::feed);
    connect(m_recorder, &Recorder::level, m_overlay, &Overlay::pushLevel);
    connect(m_recorder, &Recorder::failed, this, &App::onError);
    connect(m_listener, &Listener::phrase, this, &App::onPhrase);
    connect(m_listener, &Listener::finished, this, &App::onListenerFinished);
    connect(m_transcriber, &Transcriber::finished, this, &App::onTranscribed);
    connect(m_transcriber, &Transcriber::failed, this, &App::onTranscriptionFailed);
    connect(m_output, &TextOutput::failed, this, [this](const QString &msg) {
        qCWarning(lcSotto) << msg;
        m_overlay->showMessage(msg, true);
    });
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
        notify(tr("Model downloaded"), key.isEmpty() ? tr("Ready for dictation.") : tr("Press %1 and speak.").arg(key));
    });

    m_unloadTimer.setSingleShot(true);
    connect(&m_unloadTimer, &QTimer::timeout, m_transcriber, &Transcriber::unload);

    auto *menu = new QMenu;
    m_toggleAction = menu->addAction(tr("Start dictation"), this, &App::toggle);
    m_cancelAction = menu->addAction(QIcon::fromTheme(QStringLiteral("dialog-cancel")), tr("Cancel"), this, &App::cancel);
    menu->addSeparator();
    menu->addAction(QIcon::fromTheme(QStringLiteral("configure")), tr("Settings…"), this, &App::showSettings);
    menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), tr("Quit"), qApp, &QApplication::quit);
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
    qCInfo(lcSotto) << "desktop:" << Desktop::name() << "tray:" << QSystemTrayIcon::isSystemTrayAvailable()
                    << "paste with:" << TextOutput::effectiveMethod();
    if (!Models::isInstalled(Config::model())) {
        showSettings();
        return;
    }
    // Phrases are cut with Silero VAD (under 1 MB); fetch it quietly if an older install has none.
    if (!Models::isInstalled(Models::VadId))
        m_downloader->start(Models::VadId);
    // Load the model now, while nothing else happens, so the first dictation does not wait for it.
    m_transcriber->preload(Config::model(), Gpu::configured());
    scheduleUnload();
    // A saved permission restores silently, without a dialog.
    if (Config::autoPaste() && !Config::remoteDesktopToken().isEmpty())
        m_output->prepare();

    const ModelInfo model = Models::find(Config::model());
    const bool heavy = model.backend == Backend::Whisper
                    && (model.id.startsWith(QLatin1String("large")) || model.id.startsWith(QLatin1String("medium")));
    const QString hint = Gpu::missingHint();
    if (heavy && Gpu::devices().isEmpty())
        notify(tr("The GPU is not used"),
               hint.isEmpty() ? tr("%1 is slow on a CPU. Install a Vulkan driver or choose GigaAM or Parakeet in Settings.").arg(model.title)
                              : hint,
               true);
    else if (heavy && !hint.isEmpty())
        notify(tr("The discrete GPU is not used"), hint, true);
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
    m_model = Config::model();
    if (!Models::isInstalled(m_model)) {
        m_overlay->showMessage(tr("Download a model in Settings first"), true);
        showSettings();
        return;
    }
    if (!m_recorder->start(Config::inputDevice()))
        return;

    m_unloadTimer.stop();
    m_gpu = Gpu::configured();
    m_transcriber->preload(m_model, m_gpu); // warm up while the user talks
    m_session = m_listener->start(Config::pauseMs());
    // A window that takes the focus (no layer-shell) would get the keys, so it pastes once at the end.
    m_live = Config::liveTyping() && Config::autoPaste() && !m_overlay->takesFocus();
    m_listening = true;
    m_heardSpeech = false;
    m_pending = 0;
    m_phrases = 0;
    m_typed.clear();
    m_overlay->showRecording();
    setState(State::Recording);
}

void App::stopRecording()
{
    if (m_state != State::Recording)
        return;
    m_recorder->stop();
    m_pressClock.invalidate();
    m_listener->finish();
    setState(State::Finishing);
    m_overlay->showBusy();
}

void App::cancel()
{
    if (m_state != State::Idle) {
        m_recorder->stop();
        m_listener->cancel();
        m_transcriber->cancel(m_session);
    }
    m_overlay->dismiss();
    setState(State::Idle);
    scheduleUnload();
}

void App::onPhrase(quint64 session, const QList<float> &pcm)
{
    if (session != m_session || m_state == State::Idle)
        return;
    Transcriber::Job job{m_model, m_gpu, {Config::language(), Config::prompt()}};
    if (!m_typed.isEmpty()) // Whisper keeps spelling and punctuation consistent with what came before
        job.request.prompt = (job.request.prompt + QLatin1Char(' ') + m_typed.right(ContextChars)).trimmed();
    ++m_pending;
    qCInfo(lcSotto) << "phrase of" << pcm.size() / double(Recorder::SampleRate) << "s";
    m_transcriber->transcribe(pcm, job, session);
}

void App::onListenerFinished(quint64 session, bool heardSpeech)
{
    if (session != m_session)
        return;
    m_listening = false;
    m_heardSpeech = heardSpeech;
    finishIfDone();
}

void App::onTranscribed(quint64 session, const QString &text)
{
    if (session != m_session || m_state == State::Idle)
        return; // cancelled meanwhile
    --m_pending;
    if (!text.isEmpty()) {
        const QString phrase = Text::continuing(m_typed, text);
        if (m_live)
            m_output->deliver(m_typed.isEmpty() ? phrase : Text::separatorBefore(phrase) + phrase);
        Text::append(m_typed, text);
        ++m_phrases;
    }
    finishIfDone();
}

// A phrase that cannot be recognised means the model is not usable (it did not load, the GPU failed):
// the next phrases would fail too, so the dictation stops and says why.
void App::onTranscriptionFailed(quint64 session, const QString &message)
{
    if (session != m_session || m_state == State::Idle)
        return;
    onError(message);
}

void App::finishIfDone()
{
    if (m_state != State::Finishing || m_listening || m_pending > 0)
        return;
    setState(State::Idle);
    scheduleUnload();

    const QString text = m_typed;
    qCInfo(lcSotto) << "dictation done:" << m_phrases << "phrases," << text.size() << "chars, live:" << m_live
                    << "paste:" << Config::autoPaste();
    if (text.isEmpty())
        m_overlay->showMessage(m_heardSpeech ? tr("Nothing recognized") : tr("No speech heard — check the microphone"), !m_heardSpeech);
    else if (m_live)
        m_overlay->dismiss();
    else
        deliver(text);
}

void App::deliver(const QString &text)
{
    if (!Config::autoPaste()) {
        m_overlay->showMessage(tr("Copied: %1").arg(text));
        m_output->deliver(text);
    } else if (m_overlay->takesFocus() && m_overlay->isVisible()) {
        // The compositor hands the focus back to the user's window only once the indicator is unmapped.
        m_overlay->hideNow();
        QTimer::singleShot(250, m_output, [this, text] { m_output->deliver(text); });
    } else {
        m_overlay->dismiss();
        m_output->deliver(text);
    }
}

void App::onError(const QString &message)
{
    qCWarning(lcSotto) << "error:" << message;
    if (m_state != State::Idle) {
        m_recorder->stop();
        m_listener->cancel();
        m_transcriber->cancel(m_session);
    }
    setState(State::Idle);
    scheduleUnload();
    m_overlay->showMessage(message, true);
}

void App::notify(const QString &title, const QString &body, bool warning)
{
    if (QSystemTrayIcon::isSystemTrayAvailable() && m_tray->isVisible())
        m_tray->showMessage(title, body, warning ? QSystemTrayIcon::Warning : QSystemTrayIcon::Information, warning ? 10000 : 5000);
    else
        Desktop::notify(title, body, warning);
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
                       : state == State::Finishing ? tr("Sotto — transcribing…")
                                                   : tr("Sotto — voice typing"));
    m_toggleAction->setText(recording ? tr("Stop and paste") : tr("Start dictation"));
    m_toggleAction->setEnabled(state != State::Finishing);
    m_cancelAction->setEnabled(state != State::Idle);
}
