#include "rec-hud.h"
#include "rec-hud-overlay.h"
#include "rec-hud-settings.h"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/base.h>
#include <util/config-file.h>

#include <QTimer>
#include <QWidget>

namespace {

// Section used inside the OBS profile configuration file.
constexpr const char *kSection = "RecHud";

QString T(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

QColor ColorFromConfig(config_t *cfg, const char *key, const QColor &fallback)
{
	const char *value = config_get_string(cfg, kSection, key);
	if (!value || !*value)
		return fallback;

	const QColor color = QColor(QString::fromUtf8(value));
	return color.isValid() ? color : fallback;
}

void FrontendEventCallback(obs_frontend_event event, void *privateData)
{
	auto *self = static_cast<RecHud *>(privateData);

	// The callback may fire from a non-Qt thread (hotkeys), so bounce the
	// event onto the Qt event loop before touching any widget.
	QMetaObject::invokeMethod(self, "OnFrontendEvent", Qt::QueuedConnection,
				  Q_ARG(int, static_cast<int>(event)));
}

void OpenSettingsCallback(void *privateData)
{
	static_cast<RecHud *>(privateData)->OpenSettings();
}

void ResetPositionCallback(void *privateData)
{
	static_cast<RecHud *>(privateData)->ResetPosition();
}

// The config API has no "pass a default" overload, so defaults must be
// registered once before the first read of each key.
void RegisterConfigDefaults(config_t *cfg)
{
	const RecHudConfig d;

	config_set_default_bool(cfg, kSection, "Enabled", d.enabled);
	config_set_default_bool(cfg, kSection, "ShowDuration", d.showDuration);
	config_set_default_bool(cfg, kSection, "ShowPaused", d.showPaused);
	config_set_default_bool(cfg, kSection, "ShowAlways", d.showAlways);
	config_set_default_bool(cfg, kSection, "AlwaysOnTop", d.alwaysOnTop);
	config_set_default_bool(cfg, kSection, "ClickThrough", d.clickThrough);

	config_set_default_int(cfg, kSection, "FontSize", d.fontSize);
	config_set_default_int(cfg, kSection, "CornerRadius", d.cornerRadius);
	config_set_default_int(cfg, kSection, "Margin", d.margin);
	config_set_default_int(cfg, kSection, "Corner", d.corner);
	config_set_default_int(cfg, kSection, "PosX", -1);
	config_set_default_int(cfg, kSection, "PosY", -1);
}

} // namespace

RecHud::RecHud() = default;

RecHud::~RecHud()
{
	DestroyOverlay();
}

void RecHud::Load()
{
	LoadSettings();
	CreateOverlay();

	obs_frontend_add_event_callback(FrontendEventCallback, this);

	// OBS does not expose the recording start time, so we also poll as a
	// safety net (covers plugin reloads, profile switches, hotkeys, ...).
	pollTimer = new QTimer(this);
	pollTimer->setInterval(1000);
	connect(pollTimer, &QTimer::timeout, this, &RecHud::OnPoll);
	pollTimer->start();

	OnPoll();
}

void RecHud::AddMenuItems()
{
	if (menuAdded || unloading)
		return;
	menuAdded = true;

	// Registered here (and not in Load()) so that the locale is already loaded
	// and the menu entries are properly translated.
	obs_frontend_add_tools_menu_item(
		T("RecHud.MenuSettings").toUtf8().constData(), OpenSettingsCallback,
		this);
	obs_frontend_add_tools_menu_item(
		T("RecHud.MenuResetPosition").toUtf8().constData(),
		ResetPositionCallback, this);

	blog(LOG_INFO, "[rec-hud] tools menu entries added");
}

void RecHud::Unload()
{
	unloading = true;

	if (pollTimer) {
		pollTimer->stop();
		pollTimer->deleteLater();
		pollTimer = nullptr;
	}

	obs_frontend_remove_event_callback(FrontendEventCallback, this);

	if (overlay) {
		SaveSettings();
		DestroyOverlay();
	}
}

void RecHud::CreateOverlay()
{
	overlay = new RecHudOverlay();
	overlay->ApplyConfig(config);

	connect(overlay, &RecHudOverlay::ManualPositionChanged, this,
		&RecHud::OnManualPositionChanged);

	UpdateVisibility();
}

void RecHud::DestroyOverlay()
{
	if (!overlay)
		return;

	// disconnect() first: the overlay emits ManualPositionChanged, which would
	// otherwise reach a half-destroyed RecHud.
	disconnect(overlay, nullptr, this, nullptr);

	overlay->hide();
	delete overlay;
	overlay = nullptr;
}

void RecHud::ApplyConfig()
{
	if (overlay)
		overlay->ApplyConfig(config);
	UpdateVisibility();
}

void RecHud::UpdateVisibility()
{
	if (!overlay || unloading)
		return;

	const bool shouldShow = config.enabled && (recording || config.showAlways);

	if (shouldShow) {
		overlay->show();
	} else {
		overlay->hide();
	}
}

void RecHud::OnFrontendEvent(int event)
{
	const auto e = static_cast<obs_frontend_event>(event);

	switch (e) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		AddMenuItems();
		LoadSettings();
		ApplyConfig();
		break;

	case OBS_FRONTEND_EVENT_PROFILE_CHANGED:
		LoadSettings();
		ApplyConfig();
		break;

	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		recording = true;
		if (overlay)
			overlay->SetRecording(true);
		UpdateVisibility();
		blog(LOG_INFO, "[rec-hud] recording started, overlay %s",
		     config.enabled ? "shown" : "disabled by settings");
		break;

	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
	case OBS_FRONTEND_EVENT_EXIT:
		recording = false;
		paused = false;
		if (overlay)
			overlay->SetRecording(false);
		UpdateVisibility();
		blog(LOG_INFO, "[rec-hud] recording stopped");
		break;

	case OBS_FRONTEND_EVENT_RECORDING_PAUSED:
		paused = true;
		if (overlay)
			overlay->SetPaused(true);
		break;

	case OBS_FRONTEND_EVENT_RECORDING_UNPAUSED:
		paused = false;
		if (overlay)
			overlay->SetPaused(false);
		break;

	default:
		break;
	}
}

void RecHud::OnPoll()
{
	const bool active = obs_frontend_recording_active();

	if (active != recording) {
		recording = active;
		if (overlay)
			overlay->SetRecording(recording);
		UpdateVisibility();
	}

	const bool isPaused = obs_frontend_recording_paused();
	if (isPaused != paused) {
		paused = isPaused;
		if (overlay)
			overlay->SetPaused(paused);
	}
}

void RecHud::OpenSettings()
{
	auto *parent = static_cast<QWidget *>(obs_frontend_get_main_window());

	RecHudSettings dialog(config, parent);
	dialog.exec();

	config = dialog.Result();

	if (overlay) {
		if (config.HasManualPosition()) {
			overlay->SetManualPosition(config.pos);
		} else {
			overlay->ClearManualPosition();
		}
	}

	SaveSettings();
	ApplyConfig();
}

void RecHud::ResetPosition()
{
	config.pos = QPoint(-1, -1);

	if (overlay)
		overlay->ClearManualPosition();

	SaveSettings();
}

void RecHud::OnManualPositionChanged(const QPoint &pos)
{
	config.pos = pos;
	SaveSettings();
}

void RecHud::LoadSettings()
{
	RecHudConfig loaded;

	config_t *cfg = obs_frontend_get_profile_config();
	if (cfg) {
		RegisterConfigDefaults(cfg);

		loaded.enabled = config_get_bool(cfg, kSection, "Enabled");
		loaded.showDuration = config_get_bool(cfg, kSection, "ShowDuration");
		loaded.showPaused = config_get_bool(cfg, kSection, "ShowPaused");
		loaded.showAlways = config_get_bool(cfg, kSection, "ShowAlways");
		loaded.alwaysOnTop = config_get_bool(cfg, kSection, "AlwaysOnTop");
		loaded.clickThrough = config_get_bool(cfg, kSection, "ClickThrough");

		loaded.fontSize = (int)config_get_int(cfg, kSection, "FontSize");
		loaded.cornerRadius = (int)config_get_int(cfg, kSection, "CornerRadius");
		loaded.margin = (int)config_get_int(cfg, kSection, "Margin");
		loaded.corner = (int)config_get_int(cfg, kSection, "Corner");

		const int x = (int)config_get_int(cfg, kSection, "PosX");
		const int y = (int)config_get_int(cfg, kSection, "PosY");
		if (x >= 0 && y >= 0)
			loaded.pos = QPoint(x, y);

		loaded.dotColor =
			ColorFromConfig(cfg, "DotColor", loaded.dotColor);
		loaded.textColor =
			ColorFromConfig(cfg, "TextColor", loaded.textColor);
		loaded.bgColor = ColorFromConfig(cfg, "BgColor", loaded.bgColor);
	}

	config = loaded;
}

void RecHud::SaveSettings()
{
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return;

	RegisterConfigDefaults(cfg);

	config_set_bool(cfg, kSection, "Enabled", config.enabled);
	config_set_bool(cfg, kSection, "ShowDuration", config.showDuration);
	config_set_bool(cfg, kSection, "ShowPaused", config.showPaused);
	config_set_bool(cfg, kSection, "ShowAlways", config.showAlways);
	config_set_bool(cfg, kSection, "AlwaysOnTop", config.alwaysOnTop);
	config_set_bool(cfg, kSection, "ClickThrough", config.clickThrough);

	config_set_int(cfg, kSection, "FontSize", config.fontSize);
	config_set_int(cfg, kSection, "CornerRadius", config.cornerRadius);
	config_set_int(cfg, kSection, "Margin", config.margin);
	config_set_int(cfg, kSection, "Corner", config.corner);
	config_set_int(cfg, kSection, "PosX", config.pos.x());
	config_set_int(cfg, kSection, "PosY", config.pos.y());

	config_set_string(cfg, kSection, "DotColor",
			  config.dotColor.name(QColor::HexArgb).toUtf8().constData());
	config_set_string(cfg, kSection, "TextColor",
			  config.textColor.name(QColor::HexArgb).toUtf8().constData());
	config_set_string(cfg, kSection, "BgColor",
			  config.bgColor.name(QColor::HexArgb).toUtf8().constData());

	config_save(cfg);
}