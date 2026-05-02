/*
Dynamic Time/Date Filter — implementation
Copyright (C) 2026 K_STYER, GPLv2 or later
*/

#include <obs-module.h>
#include <plugin-support.h>
#include <util/bmem.h>
#include <util/dstr.h>

#include <string.h>
#include <time.h>

#include "iso-week.h"

struct dtd_filter {
	obs_source_t *self;

	bool parent_valid;
	obs_source_t *parent_src;

	char *separator;

	bool show_time;
	bool show_uhr_text;
	bool show_seconds;
	bool show_date;
	bool show_weekday;
	bool show_calendar_week;

	double interval_s;
	double elapsed_s;

	char last_text[256];
};

static bool parent_is_text_source(obs_source_t *parent)
{
	if (!parent)
		return false;
	const char *id = obs_source_get_unversioned_id(parent);
	if (!id)
		return false;
	return strcmp(id, "text_gdiplus") == 0 ||
	       strcmp(id, "text_ft2_source") == 0;
}

static const char *const DE_WEEKDAY_SHORT[7] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};

/* ISO 8601 week now lives in iso-week.h (shared with dock). */
#define dtd_iso_week iso_week_from_tm

static void dtd_compose_text(const struct dtd_filter *f, char *out, size_t out_size)
{
	time_t raw = time(NULL);
	struct tm t;
#ifdef _WIN32
	localtime_s(&t, &raw);
#else
	localtime_r(&raw, &t);
#endif

	struct dstr s;
	dstr_init(&s);

	const char *sep = (f->separator && *f->separator) ? f->separator : " ";

	if (f->show_calendar_week) {
		dstr_catf(&s, "KW %d", dtd_iso_week(&t));
	}

	if (f->show_weekday) {
		if (s.len)
			dstr_cat(&s, sep);
		int wd = t.tm_wday;
		if (wd < 0 || wd > 6)
			wd = 0;
		dstr_cat(&s, DE_WEEKDAY_SHORT[wd]);
	}

	if (f->show_date) {
		if (s.len)
			dstr_cat(&s, sep);
		dstr_catf(&s, "%02d.%02d.%04d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
	}

	if (f->show_time) {
		if (s.len)
			dstr_cat(&s, sep);
		if (f->show_seconds)
			dstr_catf(&s, "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
		else
			dstr_catf(&s, "%02d:%02d", t.tm_hour, t.tm_min);
		if (f->show_uhr_text)
			dstr_cat(&s, " Uhr");
	}

	if (s.len == 0)
		dstr_cat(&s, "");

	snprintf(out, out_size, "%s", s.array ? s.array : "");
	dstr_free(&s);
}

static const char *dtd_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FilterName");
}

static void dtd_update(void *data, obs_data_t *settings)
{
	struct dtd_filter *f = data;

	int interval = (int)obs_data_get_int(settings, "update_interval");
	if (interval < 1)
		interval = 1;
	if (interval > 3600)
		interval = 3600;
	f->interval_s = (double)interval;

	const char *sep = obs_data_get_string(settings, "separator");
	bfree(f->separator);
	f->separator = bstrdup(sep && *sep ? sep : " ");

	f->show_time = obs_data_get_bool(settings, "show_time");
	f->show_uhr_text = obs_data_get_bool(settings, "show_uhr_text");
	f->show_seconds = obs_data_get_bool(settings, "show_seconds");
	f->show_date = obs_data_get_bool(settings, "show_date");
	f->show_weekday = obs_data_get_bool(settings, "show_weekday");
	f->show_calendar_week = obs_data_get_bool(settings, "show_calendar_week");

	f->last_text[0] = '\0';
	f->elapsed_s = 1e9;
}

static void *dtd_create(obs_data_t *settings, obs_source_t *source)
{
	struct dtd_filter *f = bzalloc(sizeof(struct dtd_filter));
	f->self = source;
	f->separator = bstrdup(", ");
	f->interval_s = 20.0;
	f->elapsed_s = 1e9;
	dtd_update(f, settings);
	return f;
}

static void dtd_destroy(void *data)
{
	struct dtd_filter *f = data;
	if (!f)
		return;
	bfree(f->separator);
	bfree(f);
}

static void dtd_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "update_interval", 20);
	obs_data_set_default_string(settings, "separator", ", ");
	obs_data_set_default_bool(settings, "show_time", true);
	obs_data_set_default_bool(settings, "show_uhr_text", true);
	obs_data_set_default_bool(settings, "show_seconds", false);
	obs_data_set_default_bool(settings, "show_date", false);
	obs_data_set_default_bool(settings, "show_weekday", false);
	obs_data_set_default_bool(settings, "show_calendar_week", false);
}

static void dtd_filter_add(void *data, obs_source_t *parent)
{
	struct dtd_filter *f = data;
	f->parent_src = parent;
	f->parent_valid = parent_is_text_source(parent);
}

static void dtd_filter_remove(void *data, obs_source_t *parent)
{
	UNUSED_PARAMETER(parent);
	struct dtd_filter *f = data;
	f->parent_valid = false;
	f->parent_src = NULL;
}

static obs_properties_t *dtd_properties(void *data)
{
	struct dtd_filter *f = data;
	obs_properties_t *props = obs_properties_create();

	if (!f || !f->parent_valid) {
		obs_property_t *warn = obs_properties_add_text(props,
			"warning", obs_module_text("WarningTitle"),
			OBS_TEXT_INFO);
		obs_property_text_set_info_type(warn, OBS_TEXT_INFO_WARNING);
		obs_properties_add_text(props,
			"warning_help", obs_module_text("WarningHelp"),
			OBS_TEXT_INFO);
		return props;
	}

	obs_properties_add_int(props, "update_interval", obs_module_text("UpdateInterval"), 1, 3600, 1);
	obs_properties_add_text(props, "separator", obs_module_text("Separator"), OBS_TEXT_DEFAULT);
	obs_properties_add_bool(props, "show_calendar_week", obs_module_text("ShowCalendarWeek"));
	obs_properties_add_bool(props, "show_weekday", obs_module_text("ShowWeekday"));
	obs_properties_add_bool(props, "show_date", obs_module_text("ShowDate"));
	obs_properties_add_bool(props, "show_time", obs_module_text("ShowTime"));
	obs_properties_add_bool(props, "show_seconds", obs_module_text("ShowSeconds"));
	obs_properties_add_bool(props, "show_uhr_text", obs_module_text("ShowUhrText"));

	return props;
}

static void dtd_video_tick(void *data, float seconds)
{
	struct dtd_filter *f = data;
	f->elapsed_s += (double)seconds;

	double interval = f->show_seconds ? 1.0 : f->interval_s;
	if (f->elapsed_s < interval)
		return;
	f->elapsed_s = 0.0;

	if (!f->parent_valid)
		return;

	obs_source_t *parent = obs_filter_get_parent(f->self);
	if (!parent)
		return;

	char buf[256];
	dtd_compose_text(f, buf, sizeof(buf));

	if (strcmp(buf, f->last_text) == 0)
		return;
	strncpy(f->last_text, buf, sizeof(f->last_text) - 1);
	f->last_text[sizeof(f->last_text) - 1] = '\0';

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "text", buf);
	obs_source_update(parent, settings);
	obs_data_release(settings);
}

static void dtd_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	struct dtd_filter *f = data;
	obs_source_skip_video_filter(f->self);
}

struct obs_source_info dtd_filter_info = {
	.id = "dynamic_time_date_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = dtd_get_name,
	.create = dtd_create,
	.destroy = dtd_destroy,
	.update = dtd_update,
	.get_defaults = dtd_defaults,
	.get_properties = dtd_properties,
	.filter_add     = dtd_filter_add,
	.filter_remove  = dtd_filter_remove,
	.video_tick = dtd_video_tick,
	.video_render = dtd_video_render,
};
