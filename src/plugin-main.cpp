#include "rec-hud.h"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/base.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-rec-hud", "en-US")

MODULE_EXPORT const char *obs_module_name(void)
{
	return "REC HUD";
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Shows a click-through, always-on-top overlay with a REC indicator "
	       "and an optional recording timer, visible even on a fullscreen game.";
}

namespace {

RecHud *g_instance = nullptr;

} // namespace

MODULE_EXPORT bool obs_module_load(void)
{
	if (g_instance)
		return true;

	g_instance = new RecHud();
	g_instance->Load();

	blog(LOG_INFO, "[rec-hud] loaded");
	return true;
}

MODULE_EXPORT void obs_module_unload(void)
{
	if (!g_instance)
		return;

	g_instance->Unload();
	delete g_instance;
	g_instance = nullptr;

	blog(LOG_INFO, "[rec-hud] unloaded");
}