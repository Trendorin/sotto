#pragma once

#include <QElapsedTimer>
#include <QTimer>
#include <QVariantAnimation>
#include <QWidget>

#include <array>

namespace LayerShellQt {
class Window;
}

// The small floating pill: live waveform while recording, a soft wave while
// transcribing, a short message otherwise. Never takes keyboard focus.
// Drag it to move; click to stop, right-click to cancel.
class Overlay : public QWidget
{
    Q_OBJECT
public:
    explicit Overlay(QWidget *parent = nullptr);

    void showRecording();
    void showBusy();
    void showMessage(const QString &text, bool error = false);
    void dismiss();

    void pushLevel(float level);
    void setProgress(int percent);
    void resetPosition();

Q_SIGNALS:
    void clicked();
    void cancelRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    enum class Mode { Recording, Busy, Message };
    static constexpr int BarCount = 32;
    static constexpr int BarPeriodMs = 45;

    void appear();
    void tick();
    QScreen *targetScreen() const;
    QPoint topLeftFor(QPointF fraction) const;
    void place(QPoint topLeft);
    QSize pillWindowSize() const;
    void beginGrab();
    void endGrab();
    void paintWave(QPainter &p, const QRectF &area);

    Mode m_mode = Mode::Recording;
    LayerShellQt::Window *m_layer = nullptr;
    bool m_canPosition = true;

    QTimer m_frame;
    QTimer m_messageTimer;
    QElapsedTimer m_clock;     // drives animations
    QElapsedTimer m_recording; // drives the timer label
    QVariantAnimation m_fade;
    qreal m_opacity = 0.0;

    std::array<float, BarCount> m_bars{}; // newest first
    float m_pending = 0.f;                // loudest level since the last bar
    qint64 m_lastBar = 0;

    int m_progress = -1;
    QString m_message;
    bool m_error = false;

    QPoint m_topLeft;
    QPoint m_pressPos;
    bool m_pressed = false;
    bool m_dragged = false;
    // While dragging with layer-shell the surface covers the screen and the pill is painted at
    // m_topLeft: pointer coordinates are then screen coordinates and nothing depends on when the
    // compositor applies a move. Moving a small surface by margins made every event relative to a
    // surface that had not moved yet, which showed as jitter.
    bool m_grabbing = false;
    QElapsedTimer m_grabClock;
};
