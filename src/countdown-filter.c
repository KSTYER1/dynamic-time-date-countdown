/*
Dynamic Countdown Filter — implementation
Copyright (C) 2026 K_STYER, GPLv2 or later

Counts down (or up after expiry) from a target date to/from now.
Calendar-correct year/month breakdown using struct tm arithmetic.
*/

#include <obs-module.h>
#include <plugin-support.h>
#include <util/bmem.h>
#include <util/dstr.h>

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

struct cd_filter {
	obs_source_t *self;

	bool parent_valid;
	obs_source_t *parent_src;

	char *target_date_str;
	bool date_valid;
	time_t target_time;

	char *prefix_pre;
	char *suffix_pre;
	char *prefix_post;
	char *suffix_post;
	char *separator;
	bool obs_multiline;

	bool show_years;
	bool show_months;
	bool show_weeks;
	bool show_days;
	bool show_hours;
	bool show_minutes;
	bool show_seconds;

	double interval_s;
	double elapsed_s;

	bool txt_save;
	char *txt_filepath;
	bool txt_save_prefix;
	bool txt_save_suffix;

	char last_obs_text[512];
	char last_file_text[512];
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

struct duration {
	int y, mo, w, d, h, mi, s;
};

static int days_in_month(int year, int month_0)
{
	static const int dm[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (month_0 == 1) {
		bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
		return leap ? 29 : 28;
	}
	if (month_0 < 0 || month_0 > 11)
		return 30;
	return dm[month_0];
}

static bool parse_target_date(const char *s, time_t *out)
{
	if (!s || !*s)
		return false;
	int day = 0, month = 0, year = 0, hour = 0, minute = 0;
	char sep1 = 0, sep2 = 0;
	int n = sscanf(s, "%d%c%d%c%d %d:%d", &day, &sep1, &month, &sep2, &year, &hour, &minute);
	if (n < 5)
		return false;
	if ((sep1 != '-' && sep1 != '.') || (sep2 != '-' && sep2 != '.'))
		return false;
	if (year < 1970 || year > 9999)
		return false;
	if (month < 1 || month > 12)
		return false;
	if (day < 1 || day > 31)
		return false;
	if (n < 6)
		hour = 0;
	if (n < 7)
		minute = 0;
	if (hour < 0 || hour > 23 || minute < 0 || minute > 59)
		return false;

	struct tm t = {0};
	t.tm_year = year - 1900;
	t.tm_mon = month - 1;
	t.tm_mday = day;
	t.tm_hour = hour;
	t.tm_min = minute;
	t.tm_sec = 0;
	t.tm_isdst = -1;
	time_t r = mktime(&t);
	if (r == (time_t)-1)
		return false;
	*out = r;
	return true;
}

static void calendar_diff(time_t earlier, time_t later, struct duration *out)
{
	struct tm a, b;
#ifdef _WIN32
	localtime_s(&a, &earlier);
	localtime_s(&b, &later);
#else
	localtime_r(&earlier, &a);
	localtime_r(&later, &b);
#endif

	int Y = (b.tm_year + 1900) - (a.tm_year + 1900);
	int Mo = b.tm_mon - a.tm_mon;
	int D = b.tm_mday - a.tm_mday;
	int H = b.tm_hour - a.tm_hour;
	int Mi = b.tm_min - a.tm_min;
	int S = b.tm_sec - a.tm_sec;

	if (S < 0) {
		S += 60;
		Mi--;
	}
	if (Mi < 0) {
		Mi += 60;
		H--;
	}
	if (H < 0) {
		H += 24;
		D--;
	}
	if (D < 0) {
		int pm = b.tm_mon - 1;
		int py = b.tm_year + 1900;
		if (pm < 0) {
			pm = 11;
			py--;
		}
		D += days_in_month(py, pm);
		Mo--;
	}
	if (Mo < 0) {
		Mo += 12;
		Y--;
	}

	out->y = Y;
	out->mo = Mo;
	out->w = 0;
	out->d = D;
	out->h = H;
	out->mi = Mi;
	out->s = S;
}

static void apply_unit_toggles(struct duration *d, const struct cd_filter *f, time_t earlier, time_t later)
{
	if (!f->show_years) {
		d->mo += d->y * 12;
		d->y = 0;
	}

	if (!f->show_years && !f->show_months) {
		time_t diff = (later >= earlier) ? (later - earlier) : (earlier - later);
		long long total_days = (long long)(diff / 86400);
		long long rem = (long long)(diff % 86400);
		d->y = 0;
		d->mo = 0;
		d->d = (int)total_days;
		d->h = (int)(rem / 3600);
		rem %= 3600;
		d->mi = (int)(rem / 60);
		d->s = (int)(rem % 60);
	} else if (f->show_years && !f->show_months) {
		d->mo = 0;
	}

	if (f->show_weeks) {
		d->w = d->d / 7;
		d->d = d->d % 7;
	} else {
		d->w = 0;
	}

	if (!f->show_days)
		d->d = 0;
	if (!f->show_hours)
		d->h = 0;
	if (!f->show_minutes)
		d->mi = 0;
	if (!f->show_seconds)
		d->s = 0;
}

static void format_duration(const struct cd_filter *f, const struct duration *d, char *out, size_t out_size)
{
	const char *sep = (f->separator && *f->separator) ? f->separator : " ";
	struct dstr s;
	dstr_init(&s);

#define APPEND_UNIT(flag, val, sing, plur)                  \
	do {                                                \
		if (flag) {                                 \
			if (s.len)                          \
				dstr_cat(&s, sep);          \
			int v = (val);                      \
			dstr_catf(&s, "%d %s", v,           \
				  v == 1 ? sing : plur);    \
		}                                           \
	} while (0)

	APPEND_UNIT(f->show_years, d->y, "Jahr", "Jahre");
	APPEND_UNIT(f->show_months, d->mo, "Monat", "Monate");
	APPEND_UNIT(f->show_weeks, d->w, "Woche", "Wochen");
	APPEND_UNIT(f->show_days, d->d, "Tag", "Tage");
	APPEND_UNIT(f->show_hours, d->h, "Stunde", "Stunden");
	APPEND_UNIT(f->show_minutes, d->mi, "Minute", "Minuten");
	APPEND_UNIT(f->show_seconds, d->s, "Sekunde", "Sekunden");
#undef APPEND_UNIT

	if (s.len == 0)
		dstr_cat(&s, "–");
	snprintf(out, out_size, "%s", s.array ? s.array : "–");
	dstr_free(&s);
}

static void compose_outputs(const struct cd_filter *f, const char *time_str, const char *prefix, const char *suffix,
			    char *obs_out, size_t obs_size, char *file_out, size_t file_size)
{
	const char *sep = (f->separator && *f->separator) ? f->separator : " ";
	bool has_prefix = prefix && *prefix;
	bool has_suffix = suffix && *suffix;

	struct dstr ob;
	dstr_init(&ob);

	if (f->obs_multiline && has_prefix) {
		dstr_cat(&ob, prefix);
		dstr_cat(&ob, "\n");
		dstr_cat(&ob, time_str);
		if (has_suffix) {
			dstr_cat(&ob, sep);
			dstr_cat(&ob, suffix);
		}
	} else {
		if (has_prefix) {
			dstr_cat(&ob, prefix);
			dstr_cat(&ob, sep);
		}
		dstr_cat(&ob, time_str);
		if (has_suffix) {
			dstr_cat(&ob, sep);
			dstr_cat(&ob, suffix);
		}
	}
	snprintf(obs_out, obs_size, "%s", ob.array ? ob.array : "");
	dstr_free(&ob);

	struct dstr fb;
	dstr_init(&fb);
	if (f->txt_save_prefix && has_prefix) {
		dstr_cat(&fb, prefix);
		dstr_cat(&fb, sep);
	}
	dstr_cat(&fb, time_str);
	if (f->txt_save_suffix && has_suffix) {
		dstr_cat(&fb, sep);
		dstr_cat(&fb, suffix);
	}
	snprintf(file_out, file_size, "%s", fb.array ? fb.array : "");
	dstr_free(&fb);
}

static const char *cd_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("CountdownFilterName");
}

static void cd_update(void *data, obs_data_t *settings)
{
	struct cd_filter *f = data;

	int interval = (int)obs_data_get_int(settings, "update_interval");
	if (interval < 1)
		interval = 1;
	if (interval > 3600)
		interval = 3600;
	f->interval_s = (double)interval;

	bfree(f->target_date_str);
	f->target_date_str = bstrdup(obs_data_get_string(settings, "target_date"));
	f->date_valid = parse_target_date(f->target_date_str, &f->target_time);

	bfree(f->prefix_pre);
	bfree(f->suffix_pre);
	bfree(f->prefix_post);
	bfree(f->suffix_post);
	bfree(f->separator);
	f->prefix_pre = bstrdup(obs_data_get_string(settings, "prefix_pre"));
	f->suffix_pre = bstrdup(obs_data_get_string(settings, "suffix_pre"));
	f->prefix_post = bstrdup(obs_data_get_string(settings, "prefix_post"));
	f->suffix_post = bstrdup(obs_data_get_string(settings, "suffix_post"));
	f->separator = bstrdup(obs_data_get_string(settings, "separator"));

	f->obs_multiline = obs_data_get_bool(settings, "obs_multiline");

	f->show_years = obs_data_get_bool(settings, "show_years");
	f->show_months = obs_data_get_bool(settings, "show_months");
	f->show_weeks = obs_data_get_bool(settings, "show_weeks");
	f->show_days = obs_data_get_bool(settings, "show_days");
	f->show_hours = obs_data_get_bool(settings, "show_hours");
	f->show_minutes = obs_data_get_bool(settings, "show_minutes");
	f->show_seconds = obs_data_get_bool(settings, "show_seconds");

	f->txt_save = obs_data_get_bool(settings, "txt_save");
	bfree(f->txt_filepath);
	f->txt_filepath = bstrdup(obs_data_get_string(settings, "txt_filepath"));
	f->txt_save_prefix = obs_data_get_bool(settings, "txt_save_prefix");
	f->txt_save_suffix = obs_data_get_bool(settings, "txt_save_suffix");

	f->last_obs_text[0] = '\0';
	f->last_file_text[0] = '\0';
	f->elapsed_s = 1e9;
}

static void *cd_create(obs_data_t *settings, obs_source_t *source)
{
	struct cd_filter *f = bzalloc(sizeof(struct cd_filter));
	f->self = source;
	f->interval_s = 60.0;
	f->elapsed_s = 1e9;
	cd_update(f, settings);
	return f;
}

static void cd_destroy(void *data)
{
	struct cd_filter *f = data;
	if (!f)
		return;
	bfree(f->target_date_str);
	bfree(f->prefix_pre);
	bfree(f->suffix_pre);
	bfree(f->prefix_post);
	bfree(f->suffix_post);
	bfree(f->separator);
	bfree(f->txt_filepath);
	bfree(f);
}

static void cd_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "update_interval", 60);
	obs_data_set_default_string(settings, "target_date", "");
	obs_data_set_default_string(settings, "prefix_pre", "");
	obs_data_set_default_string(settings, "suffix_pre", "");
	obs_data_set_default_string(settings, "prefix_post", "");
	obs_data_set_default_string(settings, "suffix_post", "");
	obs_data_set_default_string(settings, "separator", " ");
	obs_data_set_default_bool(settings, "obs_multiline", false);
	obs_data_set_default_bool(settings, "show_years", false);
	obs_data_set_default_bool(settings, "show_months", false);
	obs_data_set_default_bool(settings, "show_weeks", false);
	obs_data_set_default_bool(settings, "show_days", true);
	obs_data_set_default_bool(settings, "show_hours", true);
	obs_data_set_default_bool(settings, "show_minutes", true);
	obs_data_set_default_bool(settings, "show_seconds", false);
	obs_data_set_default_bool(settings, "txt_save", false);
	obs_data_set_default_string(settings, "txt_filepath", "");
	obs_data_set_default_bool(settings, "txt_save_prefix", true);
	obs_data_set_default_bool(settings, "txt_save_suffix", true);
}

static bool txt_save_modified(obs_properties_t *props, obs_property_t *prop, obs_data_t *settings)
{
	UNUSED_PARAMETER(prop);
	bool enabled = obs_data_get_bool(settings, "txt_save");
	obs_property_set_visible(obs_properties_get(props, "txt_filepath"), enabled);
	obs_property_set_visible(obs_properties_get(props, "txt_save_prefix"), enabled);
	obs_property_set_visible(obs_properties_get(props, "txt_save_suffix"), enabled);
	return true;
}

static void cd_filter_add(void *data, obs_source_t *parent)
{
	struct cd_filter *f = data;
	f->parent_src = parent;
	f->parent_valid = parent_is_text_source(parent);
}

static void cd_filter_remove(void *data, obs_source_t *parent)
{
	UNUSED_PARAMETER(parent);
	struct cd_filter *f = data;
	f->parent_valid = false;
	f->parent_src = NULL;
}

static obs_properties_t *cd_properties(void *data)
{
	struct cd_filter *f = data;
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

	obs_properties_add_int(props, "update_interval", obs_module_text("CountdownUpdateInterval"), 1, 3600, 1);
	obs_properties_add_text(props, "target_date", obs_module_text("CountdownTargetDate"), OBS_TEXT_DEFAULT);

	obs_properties_t *grp_fmt = obs_properties_create();
	obs_properties_add_text(grp_fmt, "prefix_pre", obs_module_text("CountdownPrefixPre"), OBS_TEXT_DEFAULT);
	obs_properties_add_text(grp_fmt, "suffix_pre", obs_module_text("CountdownSuffixPre"), OBS_TEXT_DEFAULT);
	obs_properties_add_text(grp_fmt, "prefix_post", obs_module_text("CountdownPrefixPost"), OBS_TEXT_DEFAULT);
	obs_properties_add_text(grp_fmt, "suffix_post", obs_module_text("CountdownSuffixPost"), OBS_TEXT_DEFAULT);
	obs_properties_add_text(grp_fmt, "separator", obs_module_text("CountdownSeparator"), OBS_TEXT_DEFAULT);
	obs_properties_add_bool(grp_fmt, "obs_multiline", obs_module_text("CountdownMultiline"));
	obs_properties_add_group(props, "grp_fmt", obs_module_text("CountdownGroupFormat"), OBS_GROUP_NORMAL, grp_fmt);

	obs_properties_t *grp_units = obs_properties_create();
	obs_properties_add_bool(grp_units, "show_years", obs_module_text("CountdownShowYears"));
	obs_properties_add_bool(grp_units, "show_months", obs_module_text("CountdownShowMonths"));
	obs_properties_add_bool(grp_units, "show_weeks", obs_module_text("CountdownShowWeeks"));
	obs_properties_add_bool(grp_units, "show_days", obs_module_text("CountdownShowDays"));
	obs_properties_add_bool(grp_units, "show_hours", obs_module_text("CountdownShowHours"));
	obs_properties_add_bool(grp_units, "show_minutes", obs_module_text("CountdownShowMinutes"));
	obs_properties_add_bool(grp_units, "show_seconds", obs_module_text("CountdownShowSeconds"));
	obs_properties_add_group(props, "grp_units", obs_module_text("CountdownGroupUnits"), OBS_GROUP_NORMAL, grp_units);

	obs_properties_t *grp_txt = obs_properties_create();
	obs_property_t *txt_toggle = obs_properties_add_bool(grp_txt, "txt_save", obs_module_text("CountdownTxtSave"));
	obs_properties_add_path(grp_txt, "txt_filepath", obs_module_text("CountdownTxtFilepath"), OBS_PATH_FILE_SAVE,
				"Text Files (*.txt)", NULL);
	obs_properties_add_bool(grp_txt, "txt_save_prefix", obs_module_text("CountdownTxtSavePrefix"));
	obs_properties_add_bool(grp_txt, "txt_save_suffix", obs_module_text("CountdownTxtSaveSuffix"));
	obs_property_set_modified_callback(txt_toggle, txt_save_modified);
	obs_properties_add_group(props, "grp_txt", obs_module_text("CountdownGroupTxt"), OBS_GROUP_NORMAL, grp_txt);

	return props;
}

static void cd_video_tick(void *data, float seconds)
{
	struct cd_filter *f = data;
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

	char obs_text[512];
	char file_text[512];

	if (!f->date_valid) {
		snprintf(obs_text, sizeof(obs_text), "%s", obs_module_text("CountdownInvalidDate"));
		snprintf(file_text, sizeof(file_text), "%s", obs_text);
	} else {
		time_t now = time(NULL);
		bool expired = now >= f->target_time;
		time_t earlier = expired ? f->target_time : now;
		time_t later = expired ? now : f->target_time;

		struct duration dur;
		calendar_diff(earlier, later, &dur);
		apply_unit_toggles(&dur, f, earlier, later);

		char time_str[256];
		format_duration(f, &dur, time_str, sizeof(time_str));

		const char *prefix = expired ? f->prefix_post : f->prefix_pre;
		const char *suffix = expired ? f->suffix_post : f->suffix_pre;

		compose_outputs(f, time_str, prefix, suffix, obs_text, sizeof(obs_text), file_text, sizeof(file_text));
	}

	if (strcmp(obs_text, f->last_obs_text) != 0) {
		strncpy(f->last_obs_text, obs_text, sizeof(f->last_obs_text) - 1);
		f->last_obs_text[sizeof(f->last_obs_text) - 1] = '\0';

		obs_data_t *sset = obs_data_create();
		obs_data_set_string(sset, "text", obs_text);
		obs_source_update(parent, sset);
		obs_data_release(sset);
	}

	if (f->txt_save && f->txt_filepath && *f->txt_filepath) {
		if (strcmp(file_text, f->last_file_text) != 0) {
			strncpy(f->last_file_text, file_text, sizeof(f->last_file_text) - 1);
			f->last_file_text[sizeof(f->last_file_text) - 1] = '\0';
			FILE *fp = fopen(f->txt_filepath, "w");
			if (fp) {
				fputs(file_text, fp);
				fclose(fp);
			} else {
				obs_log(LOG_WARNING, "countdown: cannot write %s", f->txt_filepath);
			}
		}
	}
}

static void cd_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	struct cd_filter *f = data;
	obs_source_skip_video_filter(f->self);
}

struct obs_source_info countdown_filter_info = {
	.id = "dynamic_countdown_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = cd_get_name,
	.create = cd_create,
	.destroy = cd_destroy,
	.update = cd_update,
	.get_defaults = cd_defaults,
	.get_properties = cd_properties,
	.filter_add     = cd_filter_add,
	.filter_remove  = cd_filter_remove,
	.video_tick = cd_video_tick,
	.video_render = cd_video_render,
};
