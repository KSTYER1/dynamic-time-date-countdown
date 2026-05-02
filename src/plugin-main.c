/*
Dynamic Time/Date/Countdown - bundled OBS filter module
Copyright (C) 2026 K_STYER

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <plugin-support.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Dynamic Time/Date/Countdown - collection of filters that periodically rewrite text sources (clock, countdown, ...)";
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return "Dynamic Time/Date/Countdown";
}

MODULE_EXPORT const char *obs_module_author(void)
{
	return "K_STYER";
}

extern struct obs_source_info dtd_filter_info;
extern struct obs_source_info countdown_filter_info;

bool obs_module_load(void)
{
	obs_register_source(&dtd_filter_info);
	obs_register_source(&countdown_filter_info);
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}
