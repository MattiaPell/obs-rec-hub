#pragma once

#include "rec-hud-config.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QPushButton;
class QSpinBox;

/**
 * Modal settings dialog for the REC HUD.
 *
 * The dialog works on a copy of the configuration; it is only handed back to
 * the module once the user accepts the dialog.
 */
class RecHudSettings : public QDialog {
	Q_OBJECT

public:
	explicit RecHudSettings(const RecHudConfig &current, QWidget *parent = nullptr);

	RecHudConfig Result() const
	{
		return result;
	}

private slots:
	void OnAccept();
	void OnRestoreDefaults();
	void OnCornerChanged();
	void OnPickDotColor();
	void OnPickTextColor();
	void OnPickBgColor();
	void OnPickColorFor(QPushButton *button);

private:
	QWidget *BuildGeneralPage(QWidget *parent);
	QWidget *BuildAppearancePage(QWidget *parent);
	QWidget *BuildPositionPage(QWidget *parent);
	QPushButton *MakeColorButton(const QColor &color);
	void UpdateColorButton(QPushButton *button, const QColor &color);

	RecHudConfig result;
	RecHudConfig defaults;

	QCheckBox *enabledBox = nullptr;
	QCheckBox *durationBox = nullptr;
	QCheckBox *pausedBox = nullptr;
	QCheckBox *showAlwaysBox = nullptr;
	QCheckBox *topMostBox = nullptr;
	QCheckBox *clickThroughBox = nullptr;

	QSpinBox *fontSizeBox = nullptr;
	QSpinBox *radiusBox = nullptr;
	QSpinBox *marginBox = nullptr;
	QPushButton *dotColorButton = nullptr;
	QPushButton *textColorButton = nullptr;
	QPushButton *bgColorButton = nullptr;

	QComboBox *cornerBox = nullptr;
	QSpinBox *xBox = nullptr;
	QSpinBox *yBox = nullptr;
};