#include "gui/overlay.h"
#include "core/config.h"

#include <QGuiApplication>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QWindow>

#ifdef SOTTO_HAVE_LAYERSHELL
#include <LayerShellQt/Window>
#endif

#include <cmath>

static constexpr int ShadowMargin = 10;
static constexpr QSize PillSize(232, 44);

Overlay::Overlay(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedSize(pillWindowSize());
    setWindowTitle(QStringLiteral("Sotto"));

    const bool wayland = QGuiApplication::platformName() == QLatin1String("wayland");
    // On X11 (and XWayland) keep the window manager out entirely: an override-redirect window is
    // never given the focus, while some window managers still focus a Tool window on map.
    if (QGuiApplication::platformName() == QLatin1String("xcb"))
        setWindowFlag(Qt::X11BypassWindowManagerHint);
#ifdef SOTTO_HAVE_LAYERSHELL
    const bool gnome = qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains(QLatin1String("GNOME"));
    if (wayland && !gnome) {
        winId(); // create the QWindow so layer-shell can claim it before the first show
        if ((m_layer = LayerShellQt::Window::get(windowHandle()))) {
            m_layer->setLayer(LayerShellQt::Window::LayerOverlay);
            m_layer->setAnchors({LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorLeft});
            m_layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
            m_layer->setExclusiveZone(-1);
            m_layer->setScope(QStringLiteral("sotto"));
        }
    }
#endif
    m_canPosition = m_layer || !wayland;
    m_takesFocus = wayland && !m_layer;

    m_frame.setTimerType(Qt::PreciseTimer);
    m_frame.setInterval(16);
    connect(&m_frame, &QTimer::timeout, this, &Overlay::tick);

    m_messageTimer.setSingleShot(true);
    connect(&m_messageTimer, &QTimer::timeout, this, &Overlay::dismiss);

    m_fade.setEasingCurve(QEasingCurve::OutCubic);
    connect(&m_fade, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_opacity = v.toReal();
        update();
    });
    connect(&m_fade, &QVariantAnimation::finished, this, [this] {
        if (m_opacity <= 0.0) {
            endGrab();
            hide();
            m_frame.stop();
        }
    });
    m_clock.start();
}

void Overlay::showRecording()
{
    m_mode = Mode::Recording;
    m_bars.fill(0.f);
    m_pending = 0.f;
    m_lastBar = m_clock.elapsed();
    m_recording.start();
    m_messageTimer.stop();
    appear();
}

void Overlay::showBusy()
{
    m_mode = Mode::Busy;
    m_messageTimer.stop();
    appear();
}

void Overlay::showMessage(const QString &text, bool error)
{
    m_mode = Mode::Message;
    m_message = text;
    m_error = error;
    appear();
    m_messageTimer.start(error ? 3500 : 2200);
}

void Overlay::dismiss()
{
    m_messageTimer.stop();
    if (!isVisible() || (m_fade.state() == QAbstractAnimation::Running && m_fade.endValue().toReal() == 0.0))
        return;
    m_fade.stop();
    m_fade.setDuration(180);
    m_fade.setStartValue(m_opacity);
    m_fade.setEndValue(0.0);
    m_fade.start();
}

void Overlay::appear()
{
    if (!isVisible()) {
        if (!windowHandle())
            winId(); // creates the native window; only some platforms do it in the constructor
        if (windowHandle())
            windowHandle()->setScreen(targetScreen());
        place(topLeftFor(Config::overlayPosition()));
        m_opacity = 0.0;
        show();
    }
    raise(); // an override-redirect window stays under windows raised since it was mapped
    if (!m_frame.isActive())
        m_frame.start();
    m_fade.stop();
    m_fade.setDuration(140);
    m_fade.setStartValue(m_opacity);
    m_fade.setEndValue(1.0);
    m_fade.start();
    update();
}

void Overlay::hideNow()
{
    m_messageTimer.stop();
    m_fade.stop();
    m_opacity = 0.0;
    endGrab();
    hide();
    m_frame.stop();
}

void Overlay::pushLevel(float level)
{
    m_pending = std::max(m_pending, level);
}

void Overlay::resetPosition()
{
    if (isVisible())
        place(topLeftFor(Config::overlayPosition()));
}

void Overlay::tick()
{
    const qint64 now = m_clock.elapsed();
    if (m_mode == Mode::Recording) {
        // Scroll at a constant pace regardless of how the audio arrives.
        while (now - m_lastBar >= BarPeriodMs) {
            std::move_backward(m_bars.begin(), m_bars.end() - 1, m_bars.end());
            m_bars[0] = m_pending;
            m_pending *= 0.35f;
            m_lastBar += BarPeriodMs;
        }
    } else {
        for (float &b : m_bars)
            b *= 0.88f;
    }
    update();
}

QScreen *Overlay::targetScreen() const
{
    return QGuiApplication::primaryScreen();
}

QSize Overlay::pillWindowSize() const
{
    return PillSize + QSize(2 * ShadowMargin, 2 * ShadowMargin);
}

QPoint Overlay::topLeftFor(QPointF fraction) const
{
    const QSize screen = targetScreen()->geometry().size();
    const QSize own = pillWindowSize();
    const int x = qRound(fraction.x() * screen.width() - own.width() / 2.0);
    const int y = qRound(fraction.y() * screen.height() - own.height() / 2.0);
    return {std::clamp(x, 0, std::max(0, screen.width() - own.width())),
            std::clamp(y, 0, std::max(0, screen.height() - own.height()))};
}

void Overlay::beginGrab()
{
#ifdef SOTTO_HAVE_LAYERSHELL
    if (!m_layer || m_grabbing)
        return;
    m_grabbing = true;
    m_grabClock.start();
    m_layer->setMargins(QMargins());
    setFixedSize(targetScreen()->geometry().size());
#endif
}

void Overlay::endGrab()
{
#ifdef SOTTO_HAVE_LAYERSHELL
    if (!m_grabbing)
        return;
    m_grabbing = false;
    m_layer->setMargins(QMargins(m_topLeft.x(), m_topLeft.y(), 0, 0));
    setFixedSize(pillWindowSize());
#endif
}

void Overlay::place(QPoint topLeft)
{
    m_topLeft = topLeft;
#ifdef SOTTO_HAVE_LAYERSHELL
    if (m_layer) {
        if (m_grabbing)
            update(); // the pill is painted at m_topLeft inside the big surface
        else
            m_layer->setMargins(QMargins(topLeft.x(), topLeft.y(), 0, 0));
        return;
    }
#endif
    move(targetScreen()->geometry().topLeft() + topLeft);
}

void Overlay::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setOpacity(m_opacity);

    const QPalette pal = palette();
    const QRectF own(m_grabbing ? QPointF(m_topLeft) : QPointF(0, 0), QSizeF(pillWindowSize()));
    const QRectF pill = own.adjusted(ShadowMargin, ShadowMargin, -ShadowMargin, -ShadowMargin);
    const qreal radius = pill.height() / 2;

    // Soft shadow: stacked translucent rounded rects.
    p.setPen(Qt::NoPen);
    for (int i = ShadowMargin - 2; i >= 1; --i) {
        p.setBrush(QColor(0, 0, 0, 7));
        p.drawRoundedRect(pill.adjusted(-i, -i + 2, i, i + 2), radius + i, radius + i);
    }

    QColor bg = pal.color(QPalette::Window);
    bg.setAlpha(248);
    QColor border = pal.color(QPalette::WindowText);
    border.setAlpha(28);
    p.setBrush(bg);
    p.setPen(QPen(border, 1));
    p.drawRoundedRect(pill.adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);

    const QColor accent = pal.color(QPalette::Highlight);
    QColor text = pal.color(QPalette::WindowText);
    const qreal t = m_clock.elapsed() / 1000.0;
    const QPointF badge(pill.left() + 22, pill.center().y());

    if (m_mode == Mode::Message) {
        const QIcon icon = QIcon::fromTheme(m_error ? QStringLiteral("dialog-warning") : QStringLiteral("dialog-information"));
        const QRect iconRect(int(pill.left() + 14), int(pill.center().y() - 8), 16, 16);
        icon.paint(&p, iconRect);
        const QRectF textRect = pill.adjusted(40, 0, -16, 0);
        p.setPen(text);
        p.setFont(font());
        p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
                   fontMetrics().elidedText(m_message, Qt::ElideRight, int(textRect.width())));
        return;
    }

    if (m_mode == Mode::Recording) {
        // Pulsing record dot.
        QColor red(218, 68, 83);
        red.setAlphaF(0.65 + 0.35 * std::sin(t * 4.0));
        p.setPen(Qt::NoPen);
        p.setBrush(red);
        p.drawEllipse(badge, 5, 5);

        // Elapsed time, in tabular figures so it does not wobble.
        QFont labelFont = font();
        labelFont.setFeature(QFont::Tag("tnum"), 1);
        QColor label = text;
        label.setAlphaF(0.7);
        const qint64 secs = m_recording.elapsed() / 1000;
        p.setFont(labelFont);
        p.setPen(label);
        p.drawText(QRectF(pill.right() - 54, pill.top(), 40, pill.height()), Qt::AlignVCenter | Qt::AlignRight,
                   QStringLiteral("%1:%2").arg(secs / 60).arg(secs % 60, 2, 10, QLatin1Char('0')));
    } else {
        // Spinner.
        p.setPen(QPen(accent, 2, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        const QRectF arc(badge.x() - 6, badge.y() - 6, 12, 12);
        p.drawArc(arc, int(-t * 360 * 16) % (360 * 16), 270 * 16);
    }

    const bool withLabel = m_mode == Mode::Recording;
    paintWave(p, QRectF(pill.left() + 40, pill.top() + 8, pill.width() - (withLabel ? 100 : 56), pill.height() - 16));
}

void Overlay::paintWave(QPainter &p, const QRectF &area)
{
    constexpr qreal barWidth = 3.0;
    constexpr qreal spacing = 5.0;
    const qreal t = m_clock.elapsed() / 1000.0;
    const qreal scroll = m_mode == Mode::Recording
                       ? std::clamp((m_clock.elapsed() - m_lastBar) / qreal(BarPeriodMs), 0.0, 1.0) * spacing
                       : 0.0;
    const int visible = std::min(BarCount, int(area.width() / spacing) + 1);
    QColor color = palette().color(QPalette::Highlight);
    p.setPen(Qt::NoPen);

    for (int i = 0; i < visible; ++i) {
        const qreal x = area.right() - barWidth - i * spacing - scroll;
        if (x < area.left())
            break;

        qreal level = m_bars[i];
        if (i == 0 && m_mode == Mode::Recording)
            level = std::max<qreal>(level, m_pending);
        if (m_mode == Mode::Busy) {
            const qreal wave = 0.5 + 0.5 * std::sin(t * 5.0 - i * 0.42);
            level = std::max(level, 0.08 + 0.32 * wave * wave);
        }
        const qreal h = std::max(barWidth, std::pow(level, 0.85) * area.height());

        // Fade the oldest bars out towards the left edge.
        const qreal edge = std::clamp((x - area.left()) / (area.width() * 0.3), 0.0, 1.0);
        color.setAlphaF(0.25 + 0.75 * edge);
        p.setBrush(color);
        p.drawRoundedRect(QRectF(x, area.center().y() - h / 2, barWidth, h), barWidth / 2, barWidth / 2);
    }
}

void Overlay::mousePressEvent(QMouseEvent *event)
{
    m_pressed = true;
    m_dragged = false;
    m_pressPos = event->position().toPoint();
}

void Overlay::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_pressed || !(event->buttons() & Qt::LeftButton))
        return;
    const QPoint delta = event->position().toPoint() - m_pressPos;
    if (!m_dragged && delta.manhattanLength() < 5)
        return;
    if (!m_canPosition) {
        // Plain Wayland toplevel: let the compositor move it (not persisted).
        if (!m_dragged && windowHandle())
            windowHandle()->startSystemMove();
        m_dragged = true;
        return;
    }
    const QSize screen = targetScreen()->geometry().size();
    const QSize own = pillWindowSize();
    if (m_layer) {
        if (!m_grabbing) {
            m_dragged = true;
            beginGrab();
            return;
        }
        // Wait until the compositor has resized the surface; until then the pointer is still
        // reported in the old, small surface and would throw the pill into the corner.
        if (m_grabClock.elapsed() < 120)
            return;
        const QPoint cursor = event->position().toPoint();
        place({std::clamp(cursor.x() - m_pressPos.x(), 0, screen.width() - own.width()),
               std::clamp(cursor.y() - m_pressPos.y(), 0, screen.height() - own.height())});
        return;
    }
    m_dragged = true;
    // Plain X11 window: the move is applied at once, so local coordinates snap back towards m_pressPos.
    place({std::clamp(m_topLeft.x() + delta.x(), 0, screen.width() - own.width()),
           std::clamp(m_topLeft.y() + delta.y(), 0, screen.height() - own.height())});
}

void Overlay::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_pressed)
        return;
    m_pressed = false;
    if (m_dragged) {
        endGrab();
        if (m_canPosition) {
            const QSize screen = targetScreen()->geometry().size();
            const QSize own = pillWindowSize();
            Config::setOverlayPosition({(m_topLeft.x() + own.width() / 2.0) / screen.width(),
                                        (m_topLeft.y() + own.height() / 2.0) / screen.height()});
        }
        return;
    }
    if (event->button() == Qt::RightButton)
        Q_EMIT cancelRequested();
    else if (event->button() == Qt::LeftButton)
        Q_EMIT clicked();
}
