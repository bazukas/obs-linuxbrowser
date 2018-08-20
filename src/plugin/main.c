/*
Copyright (C) 2017 by Azat Khasanshin <azat.khasanshin@gmail.com>
Copyright (C) 2018 by Adrian Schollmeyer <nexadn@yandex.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <obs-module.h>
#include <pthread.h>
#include <stdio.h>

#include "manager.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("linuxbrowser-source", "en-US")

struct browser_data {
	/* settings */
	char* url;
	uint32_t width;
	uint32_t height;
	uint32_t fps;
	char* css_file;
	bool hide_scrollbars;
	uint32_t zoom;
	uint32_t scroll_vertical;
	uint32_t scroll_horizontal;
	bool reload_on_scene;
	bool stop_on_hide;

	/* internal data */
	obs_source_t* source;
	gs_texture_t* activeTexture;
	pthread_mutex_t textureLock;
	browser_manager_t* manager;

	obs_hotkey_id reload_page_key;
};

static const char* browser_get_name(void* unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("LinuxBrowser");
}

/* update stored parameters, see if they have changed and call
 * browser_manager methods based on that */
static void browser_update(void* vptr, obs_data_t* settings)
{
	struct browser_data* data = vptr;

	uint32_t width = obs_data_get_int(settings, "width");
	uint32_t height = obs_data_get_int(settings, "height");
	bool resize = width != data->width || height != data->height;
	data->width = width;
	data->height = height;
	data->fps = obs_data_get_int(settings, "fps");
	bool hide_scrollbars = obs_data_get_bool(settings, "hide_scrollbars");
	uint32_t zoom = obs_data_get_int(settings, "zoom");
	uint32_t scroll_vertical = obs_data_get_int(settings, "scroll_vertical");
	uint32_t scroll_horizontal = obs_data_get_int(settings, "scroll_horizontal");
	data->reload_on_scene = obs_data_get_bool(settings, "reload_on_scene");
	data->stop_on_hide = obs_data_get_bool(settings, "stop_on_hide");

	bool is_local = obs_data_get_bool(settings, "is_local_file");
	const char* url;
	if (is_local)
		url = obs_data_get_string(settings, "local_file");
	else
		url = obs_data_get_string(settings, "url");

	const char* css_file = obs_data_get_string(settings, "css_file");

	if (!data->manager)
		data->manager = create_browser_manager(data->width, data->height, data->fps,
		                                       settings, obs_source_get_name(data->source));

	if (data->hide_scrollbars != hide_scrollbars) {
		data->hide_scrollbars = hide_scrollbars;
		browser_manager_set_scrollbars(data->manager, !hide_scrollbars);
	}
	if (data->zoom != zoom) {
		data->zoom = zoom;
		browser_manager_set_zoom(data->manager, zoom);
	}
	if (data->scroll_vertical != scroll_vertical
	    || data->scroll_horizontal != scroll_horizontal) {
		data->scroll_vertical = scroll_vertical;
		data->scroll_horizontal = scroll_horizontal;
		browser_manager_set_scroll(data->manager, scroll_vertical, scroll_horizontal);
	}

	/* comparing and saving c-strings is tedious */
	if (!data->url || strcmp(url, data->url) != 0) {
		if (data->url)
			bfree(data->url);
		if (is_local) {
			size_t len = strlen("file://") + strlen(url) + 1;
			data->url = bzalloc(len);
			snprintf(data->url, len, "file://%s", url);
		} else {
			data->url = bstrdup(url);
		}
		browser_manager_change_url(data->manager, data->url);
	}
	if (!data->css_file || strcmp(css_file, data->css_file) != 0) {
		if (data->css_file)
			bfree(data->css_file);
		data->css_file = bstrdup(css_file);
		browser_manager_change_css_file(data->manager, data->css_file);
	}

	/* need to recreate texture if size changed */
	pthread_mutex_lock(&data->textureLock);
	obs_enter_graphics();
	if (resize || !data->activeTexture) {
		if (resize)
			browser_manager_change_size(data->manager, data->width, data->height);
		if (data->activeTexture)
			gs_texture_destroy(data->activeTexture);
		data->activeTexture =
		    gs_texture_create(width, height, GS_BGRA, 1, NULL, GS_DYNAMIC);
	}
	obs_leave_graphics();
	pthread_mutex_unlock(&data->textureLock);
}

static void reload_hotkey_pressed(void* vptr, obs_hotkey_id id, obs_hotkey_t* key, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(key);
	UNUSED_PARAMETER(pressed);

	struct browser_data* data = vptr;
	browser_manager_change_css_file(data->manager, data->css_file);
	browser_manager_reload_page(data->manager);
}

static void* browser_create(obs_data_t* settings, obs_source_t* source)
{
	struct browser_data* data = bzalloc(sizeof(struct browser_data));
	data->source = source;
	pthread_mutex_init(&data->textureLock, NULL);

	browser_update(data, settings);

	data->reload_page_key =
	    obs_hotkey_register_source(source, "linuxbrowser.reloadpage",
	                               obs_module_text("ReloadPage"), reload_hotkey_pressed, data);
	return data;
}

static void browser_destroy(void* vptr)
{
	struct browser_data* data = vptr;
	if (!data)
		return;

	pthread_mutex_destroy(&data->textureLock);
	if (data->activeTexture) {
		obs_enter_graphics();
		gs_texture_destroy(data->activeTexture);
		data->activeTexture = NULL;
		obs_leave_graphics();
	}

	if (data->manager)
		destroy_browser_manager(data->manager);

	obs_hotkey_unregister(data->reload_page_key);

	if (data->url)
		bfree(data->url);
	if (data->css_file)
		bfree(data->css_file);
	bfree(data);
}

static uint32_t browser_get_width(void* vptr)
{
	struct browser_data* data = vptr;
	return data->width;
}

static uint32_t browser_get_height(void* vptr)
{
	struct browser_data* data = vptr;
	return data->height;
}

static bool reload_button_clicked(obs_properties_t* props, obs_property_t* property, void* vptr)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);
	struct browser_data* data = vptr;
	browser_manager_change_css_file(data->manager, data->css_file);
	browser_manager_reload_page(data->manager);
	return true;
}

static void reload_on_scene(void* vptr)
{
	struct browser_data* data = vptr;
	if (data->reload_on_scene)
		browser_manager_reload_page(data->manager);
}

static bool restart_button_clicked(obs_properties_t* props, obs_property_t* property, void* vptr)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);
	struct browser_data* data = vptr;
	browser_manager_restart_browser(data->manager);
	browser_manager_change_css_file(data->manager, data->css_file);
	browser_manager_change_url(data->manager, data->url);
	browser_manager_set_scrollbars(data->manager, !data->hide_scrollbars);
	browser_manager_set_zoom(data->manager, data->zoom);
	browser_manager_set_scroll(data->manager, data->scroll_vertical, data->scroll_horizontal);
	return true;
}

static bool is_local_file_modified(obs_properties_t* props, obs_property_t* prop,
                                   obs_data_t* settings)
{
	UNUSED_PARAMETER(prop);

	bool enabled = obs_data_get_bool(settings, "is_local_file");
	obs_property_t* url = obs_properties_get(props, "url");
	obs_property_t* local_file = obs_properties_get(props, "local_file");
	obs_property_set_visible(url, !enabled);
	obs_property_set_visible(local_file, enabled);

	return true;
}

static obs_properties_t* browser_get_properties(void* vptr)
{
	UNUSED_PARAMETER(vptr);
	obs_properties_t* props = obs_properties_create();

	obs_property_t* prop =
	    obs_properties_add_bool(props, "is_local_file", obs_module_text("LocalFile"));
	obs_property_set_modified_callback(prop, is_local_file_modified);
	obs_properties_add_path(props, "local_file", obs_module_text("LocalFile"), OBS_PATH_FILE,
	                        "*.*", NULL);

	obs_properties_add_text(props, "url", obs_module_text("URL"), OBS_TEXT_DEFAULT);
	obs_properties_add_int(props, "width", obs_module_text("Width"), 1, MAX_BROWSER_WIDTH, 1);
	obs_properties_add_int(props, "height", obs_module_text("Height"), 1, MAX_BROWSER_HEIGHT,
	                       1);
	obs_properties_add_int(props, "fps", obs_module_text("FPS"), 1, 60, 1);
	obs_properties_add_bool(props, "hide_scrollbars", obs_module_text("HideScrollbars"));
	obs_properties_add_int(props, "zoom", obs_module_text("Zoom"), 1, 500, 1);
	obs_properties_add_int(props, "scroll_vertical", obs_module_text("ScrollVertical"), 0,
	                       10000000, 1);
	obs_properties_add_int(props, "scroll_horizontal", obs_module_text("ScrollHorizontal"), 0,
	                       10000000, 1);

	obs_properties_add_button(props, "reload", obs_module_text("ReloadPage"),
	                          reload_button_clicked);
	obs_properties_add_bool(props, "reload_on_scene", obs_module_text("ReloadOnScene"));

	obs_properties_add_path(props, "css_file", obs_module_text("CustomCSS"), OBS_PATH_FILE,
	                        "*.css", NULL);

	obs_properties_add_path(props, "flash_path", obs_module_text("FlashPath"), OBS_PATH_FILE,
	                        "*.so", NULL);
	obs_properties_add_text(props, "flash_version", obs_module_text("FlashVersion"),
	                        OBS_TEXT_DEFAULT);
	obs_properties_add_editable_list(props, "cef_command_line",
	                                 obs_module_text("CommandLineArguments"),
	                                 OBS_EDITABLE_LIST_TYPE_STRINGS, NULL, NULL);

	obs_properties_add_button(props, "restart", obs_module_text("RestartBrowser"),
	                          restart_button_clicked);
	obs_properties_add_bool(props, "stop_on_hide", obs_module_text("StopOnHide"));

	return props;
}

static void browser_get_defaults(obs_data_t* settings)
{
	obs_data_set_default_string(settings, "url", "http://www.obsproject.com");
	obs_data_set_default_int(settings, "width", 800);
	obs_data_set_default_int(settings, "height", 600);
	obs_data_set_default_int(settings, "fps", 30);
	obs_data_set_default_string(settings, "flash_path", "");
	obs_data_set_default_string(settings, "flash_version", "");
	obs_data_set_default_int(settings, "zoom", 100);
}

static void browser_tick(void* vptr, float seconds)
{
	UNUSED_PARAMETER(seconds);
	struct browser_data* data = vptr;
	pthread_mutex_lock(&data->textureLock);

	if (!data->activeTexture || !obs_source_showing(data->source)) {
		pthread_mutex_unlock(&data->textureLock);
		return;
	}

	lock_browser_manager(data->manager);
	obs_enter_graphics();
	gs_texture_set_image(data->activeTexture, get_browser_manager_data(data->manager),
	                     data->width * 4, false);
	obs_leave_graphics();
	unlock_browser_manager(data->manager);

	pthread_mutex_unlock(&data->textureLock);
}

static void browser_render(void* vptr, gs_effect_t* effect)
{
	struct browser_data* data = vptr;
	pthread_mutex_lock(&data->textureLock);

	if (!data->activeTexture) {
		pthread_mutex_unlock(&data->textureLock);
		return;
	}

	gs_reset_blend_state();
	gs_effect_set_texture(gs_effect_get_param_by_name(effect, "image"), data->activeTexture);
	gs_draw_sprite(data->activeTexture, 0, data->width, data->height);

	pthread_mutex_unlock(&data->textureLock);
}

static void browser_mouse_click(void* vptr, const struct obs_mouse_event* event, int32_t type,
                                bool mouse_up, uint32_t click_count)
{
	struct browser_data* data = vptr;
	browser_manager_send_mouse_click(data->manager, event->x, event->y, event->modifiers, type,
	                                 mouse_up, click_count);
}

static void browser_mouse_move(void* vptr, const struct obs_mouse_event* event, bool mouse_leave)
{
	struct browser_data* data = vptr;
	browser_manager_send_mouse_move(data->manager, event->x, event->y, event->modifiers,
	                                mouse_leave);
}

static void browser_mouse_wheel(void* vptr, const struct obs_mouse_event* event, int x_delta,
                                int y_delta)
{
	struct browser_data* data = vptr;
	browser_manager_send_mouse_wheel(data->manager, event->x, event->y, event->modifiers,
	                                 x_delta, y_delta);
}

static void browser_focus(void* vptr, bool focus)
{
	struct browser_data* data = vptr;
	browser_manager_send_focus(data->manager, focus);
}

static void browser_key_click(void* vptr, const struct obs_key_event* event, bool key_up)
{
	struct browser_data* data = vptr;
	char chr = 0;
	if (event->text)
		chr = event->text[0];
	blog(LOG_INFO, "Key: %s %d %d", event->text, event->native_vkey, event->native_scancode);

	browser_manager_send_key(data->manager, key_up, event->native_vkey, event->modifiers, chr);
}

static void browser_source_activate(void* vptr)
{
	reload_on_scene(vptr);

	struct browser_data* data = vptr;
	browser_manager_send_active_state_change(data->manager, true);
}

static void browser_source_deactivate(void* vptr)
{
	struct browser_data* data = vptr;
	browser_manager_send_active_state_change(data->manager, false);
}

static void browser_source_show(void* vptr)
{
	struct browser_data* data = vptr;
	browser_manager_send_visibility_change(data->manager, true);

	if (data->stop_on_hide) {
		browser_manager_start_browser(data->manager);
		browser_manager_change_css_file(data->manager, data->css_file);
		browser_manager_change_url(data->manager, data->url);
		browser_manager_set_scrollbars(data->manager, !data->hide_scrollbars);
		browser_manager_set_zoom(data->manager, data->zoom);
		browser_manager_set_scroll(data->manager, data->scroll_vertical,
		                           data->scroll_horizontal);
	}
}

static void browser_source_hide(void* vptr)
{
	struct browser_data* data = vptr;
	browser_manager_send_visibility_change(data->manager, false);

	if (data->stop_on_hide)
		browser_manager_stop_browser(data->manager);
}

bool obs_module_load(void)
{
	struct obs_source_info info = {};
	info.id = "linuxbrowser-source";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_INTERACTION;

	info.get_name = browser_get_name;
	info.create = browser_create;
	info.destroy = browser_destroy;
	info.update = browser_update;
	info.get_width = browser_get_width;
	info.get_height = browser_get_height;
	info.get_properties = browser_get_properties;
	info.get_defaults = browser_get_defaults;
	info.video_tick = browser_tick;
	info.video_render = browser_render;

	info.mouse_click = browser_mouse_click;
	info.mouse_move = browser_mouse_move;
	info.mouse_wheel = browser_mouse_wheel;
	info.focus = browser_focus;
	info.key_click = browser_key_click;

	info.activate = browser_source_activate;
	info.deactivate = browser_source_deactivate;
	info.show = browser_source_show;
	info.hide = browser_source_hide;

	obs_register_source(&info);
	return true;
}
