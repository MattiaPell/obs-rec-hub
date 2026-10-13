#pragma once

#include "rec-hud-config.h"

#include <QObject>

class RecHudOverlay;
class QTimer;

/**
 * Plugin controller.
 *
 * Owns the overlay window, listens to OBS frontend events (and polls as a
 * safety net), and persists the settings in the current OBS profile.
 */
class RecHud : public QObject {
	Q_OBJECT

public:
	RecHud();
	~RecHud() override;

	void Load();
	void Unload();

public slots:
	/** Queued from the libobs callback so we always run on the Qt thread. */
	void OnFrontendEvent(int event);

	/** Invoked from the OBS Tools menu. */
	void OpenSettings();
	void ResetPosition();

private slots:
	void OnPoll();
	void OnManualPositionChanged(const QPoint &pos);

private:
	void CreateOverlay();
	void DestroyOverlay();
	void AddMenuItems();
	void LoadSettings();
	void SaveSettings();
	void ApplyConfig();
	void UpdateVisibility();

	RecHudOverlay *overlay = nullptr;
	QTimer *pollTimer = nullptr;
	RecHudConfig config;
	bool recording = false;
	bool paused = false;
	bool unloading = false;
	bool menuAdded = false;
};