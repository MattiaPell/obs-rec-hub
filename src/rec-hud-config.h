#pragma once

#include <QColor>
#include <QPoint>
#include <QString>

/**
 * Corners the overlay can be docked to when no manual position is set.
 */
enum RecHudCorner {
	RecHudCornerTopLeft = 0,
	RecHudCornerTopRight = 1,
	RecHudCornerBottomLeft = 2,
	RecHudCornerBottomRight = 3,
	RecHudCornerCustom = 4,
};

/**
 * All user-tunable options of the REC HUD.
 *
 * A single struct keeps the whole plugin simple: the settings dialog edits a
 * copy of it, the module loads/saves it from the OBS profile configuration and
 * the overlay widget only ever reads it.
 */
struct RecHudConfig {
	/** Master switch: when false the overlay is never shown. */
	bool enabled = true;

	/** Show the elapsed recording time next to the "REC" label. */
	bool showDuration = true;

	/** Append a "PAUSED" marker while the recording is paused. */
	bool showPaused = true;

	/** Keep the overlay visible even when OBS is not recording. */
	bool showAlways = false;

	/** Keep the overlay above every other window. */
	bool alwaysOnTop = true;

	/** Let mouse clicks pass through the overlay (useful while gaming). */
	bool clickThrough = false;

	/** Label font size, in pixels. */
	int fontSize = 18;

	/** Border radius of the pill background, in pixels. */
	int cornerRadius = 8;

	/** Distance from the screen edge when docked to a corner, in pixels. */
	int margin = 20;

	/** One of RecHudCorner. */
	int corner = RecHudCornerTopRight;

	/** Manual position; invalid (-1, -1) means "use corner". */
	QPoint pos = QPoint(-1, -1);

	QColor dotColor = QColor(255, 59, 48);
	QColor textColor = QColor(255, 255, 255);
	QColor bgColor = QColor(0, 0, 0, 115);

	bool HasManualPosition() const
	{
		return pos.x() >= 0 && pos.y() >= 0;
	}
};