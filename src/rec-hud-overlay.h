#pragma once

#include "rec-hud-config.h"

#include <QElapsedTimer>
#include <QPoint>
#include <QTimer>
#include <QWidget>

/**
 * Frameless, translucent, always-on-top overlay window.
 *
 * The widget paints a small "pill": a pulsing dot plus an optional duration
 * label. Everything is drawn manually with QPainter so the plugin carries no
 * image assets.
 */
class RecHudOverlay : public QWidget {
	Q_OBJECT

public:
	explicit RecHudOverlay(QWidget *parent = nullptr);
	~RecHudOverlay() override;

	/** Apply a (new) configuration. Takes effect immediately. */
	void ApplyConfig(const RecHudConfig &config);
	const RecHudConfig &Config() const
	{
		return config;
	}

	/** Recording state. The duration counter restarts on a false -> true. */
	void SetRecording(bool recording);
	bool Recording() const
	{
		return recording;
	}

	void SetPaused(bool paused);

	/** Current top-left position of the window on the virtual desktop. */
	QPoint Position() const;

	/** Place the window manually; disables automatic corner docking. */
	void SetManualPosition(const QPoint &pos);

	/** Go back to automatic corner docking. */
	void ClearManualPosition();

	/** Snap to the configured corner (or the screen cursor is on). */
	void SnapToCorner();

signals:
	/** Emitted when the user finishes dragging the window. */
	void ManualPositionChanged(const QPoint &pos);

protected:
	void paintEvent(QPaintEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

private:
	void BuildWindow();
	void RebuildLayout();
	void AnchorToCorner();
	void ApplyPositionInternal();
	void MoveToCorner(int winW, int winH);
	void UpdateTimers();
	QString LabelText() const;
	QString DurationText() const;
	QColor EffectiveTextColor() const;

	RecHudConfig config;

	QTimer *tickTimer = nullptr;
	QTimer *pulseTimer = nullptr;
	QElapsedTimer elapsed;

	bool recording = false;
	bool paused = false;
	bool dragging = false;
	bool manualPosition = false;
	bool anchorPending = false;
	qint64 accumulatedMs = 0;
	int pulsePhase = 0;
	QPoint dragOffset;

	/** Last label text laid out; used to detect width changes while recording. */
	QString lastLabelText;
};
