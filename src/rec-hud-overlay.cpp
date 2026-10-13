#include "rec-hud-overlay.h"

#include <QCursor>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>

namespace {

constexpr int kPadH = 12;
constexpr int kPadV = 6;
constexpr int kGap = 9;
constexpr int kMinDot = 12;
constexpr int kPulseSteps = 40;

// U+2022 BULLET, built from its code point so the source file stays pure ASCII.
const QChar kBullet = QChar(0x2022);

} // namespace

RecHudOverlay::RecHudOverlay(QWidget *parent) : QWidget(parent)
{
	setAttribute(Qt::WA_TranslucentBackground, true);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_ShowWithoutActivating, true);
	setFocusPolicy(Qt::NoFocus);

	tickTimer = new QTimer(this);
	tickTimer->setInterval(200);
	connect(tickTimer, &QTimer::timeout, this, [this]() {
		if (recording)
			update();
	});

	pulseTimer = new QTimer(this);
	pulseTimer->setInterval(33);
	connect(pulseTimer, &QTimer::timeout, this, [this]() {
		pulsePhase = (pulsePhase + 1) % kPulseSteps;
		if (recording && !paused)
			update();
	});

	BuildWindow();
	RebuildLayout();
}

RecHudOverlay::~RecHudOverlay() = default;

void RecHudOverlay::BuildWindow()
{
	Qt::WindowFlags flags = Qt::Tool | Qt::FramelessWindowHint |
				Qt::WindowDoesNotAcceptFocus | Qt::NoDropShadowWindowHint;
	if (config.alwaysOnTop)
		flags |= Qt::WindowStaysOnTopHint;

	setWindowFlags(flags);
	setAttribute(Qt::WA_TransparentForMouseEvents, config.clickThrough);
}

void RecHudOverlay::ApplyConfig(const RecHudConfig &newConfig)
{
	const bool windowFlagsChanged =
		newConfig.alwaysOnTop != config.alwaysOnTop ||
		newConfig.clickThrough != config.clickThrough;

	config = newConfig;
	manualPosition = config.HasManualPosition();

	QFont f = font();
	f.setPixelSize(config.fontSize);
	f.setBold(true);
	f.setLetterSpacing(QFont::AbsoluteSpacing, 0.4);
	setFont(f);

	RebuildLayout();

	if (manualPosition) {
		// Honour a position saved from a previous session.
		move(config.pos);
	} else {
		ApplyPositionInternal();
	}

	if (windowFlagsChanged) {
		// Changing window flags recreates the native window, so put it back.
		const bool wasVisible = isVisible();
		if (wasVisible)
			hide();
		BuildWindow();
		RebuildLayout();
		if (manualPosition)
			move(config.pos);
		else
			ApplyPositionInternal();
		if (wasVisible)
			show();
	}

	if (recording)
		update();
}

void RecHudOverlay::RebuildLayout()
{
	const QFontMetrics fm(font());
	const int dot = qMax(kMinDot, config.fontSize * 2 / 3);
	const QSize textSize = fm.size(Qt::TextSingleLine, LabelText());

	const int w = kPadH * 2 + dot + kGap + textSize.width();
	const int h = kPadV * 2 + qMax(dot, textSize.height());
	setFixedSize(qMax(1, w), qMax(1, h));

	// The label grows and shrinks while recording ("PAUSED", MM:SS -> HH:MM:SS).
	AnchorToCorner();
}

void RecHudOverlay::AnchorToCorner()
{
	if (manualPosition || anchorPending)
		return;

	// Anchoring has to happen *after* the resize reached the native window:
	// during this call width()/height() can still report the old geometry,
	// which would compute a position that leaves the window off screen once it
	// grows. A queued call runs once the resize has been flushed.
	anchorPending = true;
	QTimer::singleShot(0, this, [this]() {
		anchorPending = false;
		if (!manualPosition)
			SnapToCorner();
	});
}

void RecHudOverlay::ApplyPositionInternal()
{
	if (manualPosition || config.HasManualPosition())
		return;
	SnapToCorner();
}

void RecHudOverlay::SnapToCorner()
{
	MoveToCorner(width(), height());
}

void RecHudOverlay::MoveToCorner(int winW, int winH)
{
	QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
	if (!screen)
		screen = QGuiApplication::primaryScreen();
	if (!screen)
		return;

	const QRect area = screen->availableGeometry();
	const int m = config.margin;

	int x = area.x() + m;
	int y = area.y() + m;

	switch (config.corner) {
	case RecHudCornerTopLeft:
		break;
	case RecHudCornerTopRight:
		x = area.right() - m - winW + 1;
		break;
	case RecHudCornerBottomLeft:
		y = area.bottom() - m - winH + 1;
		break;
	case RecHudCornerBottomRight:
		x = area.right() - m - winW + 1;
		y = area.bottom() - m - winH + 1;
		break;
	default:
		// "Custom" without an explicit position: behave like top-right.
		x = area.right() - m - winW + 1;
		break;
	}

	// A single setGeometry() keeps the size and the position in sync.
	setGeometry(QRect(x, y, winW, winH));
}

void RecHudOverlay::SetManualPosition(const QPoint &pos)
{
	manualPosition = true;
	move(pos);
}

void RecHudOverlay::ClearManualPosition()
{
	manualPosition = false;
	SnapToCorner();
}

QPoint RecHudOverlay::Position() const
{
	return pos();
}

void RecHudOverlay::SetRecording(bool active)
{
	if (active == recording)
		return;

	recording = active;

	if (recording) {
		paused = false;
		accumulatedMs = 0;
		elapsed.start();
	} else {
		elapsed.invalidate();
		paused = false;
		accumulatedMs = 0;
	}

	pulsePhase = 0;
	RebuildLayout();
	UpdateTimers();
	update();
}

void RecHudOverlay::SetPaused(bool value)
{
	if (value == paused)
		return;

	if (recording) {
		// QElapsedTimer::restart() resets the counter, so the time already
		// elapsed has to be accumulated explicitly to survive a pause.
		if (value)
			accumulatedMs += elapsed.elapsed();
		else
			elapsed.start();
	}

	paused = value;
	RebuildLayout();
	update();
}

void RecHudOverlay::UpdateTimers()
{
	if (recording) {
		if (!tickTimer->isActive())
			tickTimer->start();
		if (!paused && !pulseTimer->isActive())
			pulseTimer->start();
	} else {
		tickTimer->stop();
		pulseTimer->stop();
	}
}

QString RecHudOverlay::DurationText() const
{
	qint64 ms = 0;
	if (recording)
		ms = accumulatedMs + (paused ? 0 : elapsed.elapsed());
	if (ms < 0)
		ms = 0;

	const qint64 totalSeconds = ms / 1000;
	const qint64 h = totalSeconds / 3600;
	const qint64 m = (totalSeconds % 3600) / 60;
	const qint64 s = totalSeconds % 60;

	if (h > 0) {
		return QStringLiteral("%1:%2:%3")
			.arg(h, 2, 10, QLatin1Char('0'))
			.arg(m, 2, 10, QLatin1Char('0'))
			.arg(s, 2, 10, QLatin1Char('0'));
	}

	return QStringLiteral("%1:%2")
		.arg(m, 2, 10, QLatin1Char('0'))
		.arg(s, 2, 10, QLatin1Char('0'));
}

QString RecHudOverlay::LabelText() const
{
	QString text = QStringLiteral("REC");

	const QString separator = QStringLiteral("  ") + kBullet + QStringLiteral("  ");

	if (config.showDuration)
		text += separator + DurationText();

	if (recording && paused && config.showPaused)
		text += separator + QStringLiteral("PAUSED");

	return text;
}

QColor RecHudOverlay::EffectiveTextColor() const
{
	if (recording && paused)
		return QColor(255, 183, 77);
	return config.textColor;
}

void RecHudOverlay::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setRenderHint(QPainter::TextAntialiasing, true);

	// Pill background
	p.setPen(Qt::NoPen);
	p.setBrush(config.bgColor);
	p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), config.cornerRadius,
			  config.cornerRadius);

	// Dot
	const int dot = qMax(kMinDot, config.fontSize * 2 / 3);
	const QRectF dotRect(kPadH, (height() - dot) / 2.0, dot, dot);

	if (recording && !paused) {
		const double t = static_cast<double>(pulsePhase) /
				 static_cast<double>(kPulseSteps);
		const double grow = dot * 0.55 * t;
		QColor halo = config.dotColor;
		halo.setAlphaF(0.45 * (1.0 - t));
		p.setBrush(halo);
		p.drawEllipse(dotRect.adjusted(-grow, -grow, grow, grow));
	}

	p.setBrush(recording && paused ? QColor(245, 245, 245) : config.dotColor);
	p.setPen(Qt::NoPen);
	p.drawEllipse(dotRect);

	// Text
	const QString text = LabelText();
	const QFontMetrics fm(font());
	const int textX = int(dotRect.right()) + 1 + kGap;
	const int textY = (height() + fm.ascent() - fm.descent()) / 2;

	p.setPen(EffectiveTextColor());
	p.drawText(QPoint(textX, textY), text);
}

void RecHudOverlay::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);

	// The size must be up to date before computing the corner position.
	RebuildLayout();

	if (!manualPosition)
		SnapToCorner();

	UpdateTimers();
}

void RecHudOverlay::mousePressEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton) {
		QWidget::mousePressEvent(event);
		return;
	}

	dragging = true;
	dragOffset = event->globalPosition().toPoint() - pos();
	event->accept();
}

void RecHudOverlay::mouseMoveEvent(QMouseEvent *event)
{
	if (!dragging) {
		QWidget::mouseMoveEvent(event);
		return;
	}

	move(event->globalPosition().toPoint() - dragOffset);
	event->accept();
}

void RecHudOverlay::mouseReleaseEvent(QMouseEvent *event)
{
	if (!dragging || event->button() != Qt::LeftButton) {
		QWidget::mouseReleaseEvent(event);
		return;
	}

	dragging = false;
	manualPosition = true;
	emit ManualPositionChanged(pos());
	event->accept();
}