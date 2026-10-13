#include "rec-hud-settings.h"

#include <obs-module.h>

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {

QString T(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

} // namespace

RecHudSettings::RecHudSettings(const RecHudConfig &current, QWidget *parent)
	: QDialog(parent), result(current)
{
	setWindowTitle(T("RecHud.Settings.Title"));
	setModal(true);

	auto *tabs = new QTabWidget(this);
	tabs->addTab(BuildGeneralPage(this), T("RecHud.Settings.TabGeneral"));
	tabs->addTab(BuildAppearancePage(this), T("RecHud.Settings.TabAppearance"));
	tabs->addTab(BuildPositionPage(this), T("RecHud.Settings.TabPosition"));

	auto *buttons = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults,
		this);
	buttons->button(QDialogButtonBox::Ok)->setText(T("RecHud.Settings.Ok"));
	buttons->button(QDialogButtonBox::Cancel)->setText(T("RecHud.Settings.Cancel"));
	buttons->button(QDialogButtonBox::RestoreDefaults)
		->setText(T("RecHud.Settings.RestoreDefaults"));

	connect(buttons, &QDialogButtonBox::accepted, this, &RecHudSettings::OnAccept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(buttons->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked,
		this, &RecHudSettings::OnRestoreDefaults);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(tabs);
	layout->addWidget(buttons);
	setMinimumWidth(460);
}

QWidget *RecHudSettings::BuildGeneralPage(QWidget *parent)
{
	auto *page = new QWidget(parent);
	auto *box = new QVBoxLayout(page);

	auto *group = new QGroupBox(T("RecHud.Settings.GroupBehaviour"), page);
	auto *form = new QFormLayout(group);

	enabledBox = new QCheckBox(T("RecHud.Settings.Enabled"), group);
	durationBox = new QCheckBox(T("RecHud.Settings.ShowDuration"), group);
	pausedBox = new QCheckBox(T("RecHud.Settings.ShowPaused"), group);
	showAlwaysBox = new QCheckBox(T("RecHud.Settings.ShowAlways"), group);
	topMostBox = new QCheckBox(T("RecHud.Settings.AlwaysOnTop"), group);
	clickThroughBox = new QCheckBox(T("RecHud.Settings.ClickThrough"), group);

	enabledBox->setChecked(result.enabled);
	durationBox->setChecked(result.showDuration);
	pausedBox->setChecked(result.showPaused);
	showAlwaysBox->setChecked(result.showAlways);
	topMostBox->setChecked(result.alwaysOnTop);
	clickThroughBox->setChecked(result.clickThrough);

	form->addRow(enabledBox);
	form->addRow(durationBox);
	form->addRow(pausedBox);
	form->addRow(showAlwaysBox);
	form->addRow(topMostBox);
	form->addRow(clickThroughBox);

	auto *hint = new QLabel(T("RecHud.Settings.ClickThroughHint"), page);
	hint->setWordWrap(true);
	hint->setStyleSheet("color: gray;");

	box->addWidget(group);
	box->addWidget(hint);
	box->addStretch();
	return page;
}

QWidget *RecHudSettings::BuildAppearancePage(QWidget *parent)
{
	auto *page = new QWidget(parent);
	auto *box = new QVBoxLayout(page);

	auto *group = new QGroupBox(T("RecHud.Settings.GroupAppearance"), page);
	auto *form = new QFormLayout(group);

	fontSizeBox = new QSpinBox(group);
	fontSizeBox->setRange(10, 96);
	fontSizeBox->setSuffix(" px");

	radiusBox = new QSpinBox(group);
	radiusBox->setRange(0, 48);
	radiusBox->setSuffix(" px");

	fontSizeBox->setValue(result.fontSize);
	radiusBox->setValue(result.cornerRadius);

	dotColorButton = MakeColorButton(result.dotColor);
	textColorButton = MakeColorButton(result.textColor);
	bgColorButton = MakeColorButton(result.bgColor);

	form->addRow(T("RecHud.Settings.FontSize"), fontSizeBox);
	form->addRow(T("RecHud.Settings.CornerRadius"), radiusBox);
	form->addRow(T("RecHud.Settings.DotColor"), dotColorButton);
	form->addRow(T("RecHud.Settings.TextColor"), textColorButton);
	form->addRow(T("RecHud.Settings.BgColor"), bgColorButton);

	connect(dotColorButton, &QPushButton::clicked, this, &RecHudSettings::OnPickDotColor);
	connect(textColorButton, &QPushButton::clicked, this,
		&RecHudSettings::OnPickTextColor);
	connect(bgColorButton, &QPushButton::clicked, this, &RecHudSettings::OnPickBgColor);

	box->addWidget(group);
	box->addStretch();
	return page;
}

QWidget *RecHudSettings::BuildPositionPage(QWidget *parent)
{
	auto *page = new QWidget(parent);
	auto *box = new QVBoxLayout(page);

	auto *group = new QGroupBox(T("RecHud.Settings.GroupPosition"), page);
	auto *form = new QFormLayout(group);

	cornerBox = new QComboBox(group);
	cornerBox->addItem(T("RecHud.Settings.Corner.TopLeft"), RecHudCornerTopLeft);
	cornerBox->addItem(T("RecHud.Settings.Corner.TopRight"), RecHudCornerTopRight);
	cornerBox->addItem(T("RecHud.Settings.Corner.BottomLeft"), RecHudCornerBottomLeft);
	cornerBox->addItem(T("RecHud.Settings.Corner.BottomRight"),
			   RecHudCornerBottomRight);
	cornerBox->addItem(T("RecHud.Settings.Corner.Custom"), RecHudCornerCustom);
	const int cornerIndex = cornerBox->findData(result.corner);
	cornerBox->setCurrentIndex(cornerIndex >= 0 ? cornerIndex : RecHudCornerTopRight);

	marginBox = new QSpinBox(group);
	marginBox->setRange(0, 400);
	marginBox->setSuffix(" px");
	marginBox->setValue(result.margin);

	xBox = new QSpinBox(group);
	xBox->setRange(-1, 32000);
	xBox->setValue(result.pos.x());
	yBox = new QSpinBox(group);
	yBox->setRange(-1, 32000);
	yBox->setValue(result.pos.y());

	auto *xy = new QWidget(group);
	auto *xyLayout = new QHBoxLayout(xy);
	xyLayout->setContentsMargins(0, 0, 0, 0);
	xyLayout->addWidget(new QLabel(QStringLiteral("X"), xy));
	xyLayout->addWidget(xBox);
	xyLayout->addWidget(new QLabel(QStringLiteral("Y"), xy));
	xyLayout->addWidget(yBox);

	form->addRow(T("RecHud.Settings.Corner"), cornerBox);
	form->addRow(T("RecHud.Settings.Margin"), marginBox);
	form->addRow(T("RecHud.Settings.ManualPosition"), xy);

	connect(cornerBox, &QComboBox::currentIndexChanged, this,
		&RecHudSettings::OnCornerChanged);

	box->addWidget(group);
	box->addStretch();
	OnCornerChanged();
	return page;
}

void RecHudSettings::OnCornerChanged()
{
	const bool custom = cornerBox->currentData().toInt() == RecHudCornerCustom;
	xBox->setEnabled(custom);
	yBox->setEnabled(custom);
	marginBox->setEnabled(!custom);
}

void RecHudSettings::OnPickDotColor()
{
	OnPickColorFor(dotColorButton);
}

void RecHudSettings::OnPickTextColor()
{
	OnPickColorFor(textColorButton);
}

void RecHudSettings::OnPickBgColor()
{
	OnPickColorFor(bgColorButton);
}

void RecHudSettings::OnPickColorFor(QPushButton *button)
{
	const QColor current = button->property("color").value<QColor>();
	const QColor chosen = QColorDialog::getColor(
		current, this, T("RecHud.Settings.PickColor"),
		QColorDialog::ShowAlphaChannel);
	if (!chosen.isValid())
		return;

	UpdateColorButton(button, chosen);
}

void RecHudSettings::OnRestoreDefaults()
{
	result = defaults;

	enabledBox->setChecked(result.enabled);
	durationBox->setChecked(result.showDuration);
	pausedBox->setChecked(result.showPaused);
	showAlwaysBox->setChecked(result.showAlways);
	topMostBox->setChecked(result.alwaysOnTop);
	clickThroughBox->setChecked(result.clickThrough);

	fontSizeBox->setValue(result.fontSize);
	radiusBox->setValue(result.cornerRadius);
	marginBox->setValue(result.margin);

	UpdateColorButton(dotColorButton, result.dotColor);
	UpdateColorButton(textColorButton, result.textColor);
	UpdateColorButton(bgColorButton, result.bgColor);

	const int index = cornerBox->findData(result.corner);
	cornerBox->setCurrentIndex(index >= 0 ? index : RecHudCornerTopRight);
	xBox->setValue(result.pos.x());
	yBox->setValue(result.pos.y());
	OnCornerChanged();
}

void RecHudSettings::OnAccept()
{
	result.enabled = enabledBox->isChecked();
	result.showDuration = durationBox->isChecked();
	result.showPaused = pausedBox->isChecked();
	result.showAlways = showAlwaysBox->isChecked();
	result.alwaysOnTop = topMostBox->isChecked();
	result.clickThrough = clickThroughBox->isChecked();

	result.fontSize = fontSizeBox->value();
	result.cornerRadius = radiusBox->value();
	result.margin = marginBox->value();

	result.dotColor = dotColorButton->property("color").value<QColor>();
	result.textColor = textColorButton->property("color").value<QColor>();
	result.bgColor = bgColorButton->property("color").value<QColor>();

	result.corner = cornerBox->currentData().toInt();
	if (result.corner == RecHudCornerCustom && xBox->value() >= 0 &&
	    yBox->value() >= 0) {
		result.pos = QPoint(xBox->value(), yBox->value());
	} else {
		result.corner = result.corner == RecHudCornerCustom
					? RecHudCornerTopRight
					: result.corner;
		result.pos = QPoint(-1, -1);
	}

	accept();
}

QPushButton *RecHudSettings::MakeColorButton(const QColor &color)
{
	auto *button = new QPushButton(this);
	button->setMinimumWidth(120);
	UpdateColorButton(button, color);
	return button;
}

void RecHudSettings::UpdateColorButton(QPushButton *button, const QColor &color)
{
	button->setProperty("color", color);

	const QString text = color.alpha() < 255
				     ? QStringLiteral("%1  %2%")
					       .arg(color.name(), color.alpha())
				     : color.name();
	button->setText(text);

	// CSS does not understand #AARRGGBB, build an rgba() literal instead.
	const QString rgba =
		QStringLiteral("rgba(%1, %2, %3, %4)")
			.arg(color.red())
			.arg(color.green())
			.arg(color.blue())
			.arg(color.alphaF(), 0, 'f', 2);

	const QString foreground =
		color.lightnessF() > 0.5 ? QStringLiteral("#000000")
					: QStringLiteral("#ffffff");

	button->setStyleSheet(
		QStringLiteral("background-color: %1; color: %2;").arg(rgba, foreground));
}