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
#include "windows_keycode.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("linuxbrowser-source", "en-US")

struct browser_data {
	/* settings */
	char* url;
	uint32_t width;
	uint32_t height;
	uint32_t fps;
	char* css_file;
	char* js_file;
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
	const char* js_file = obs_data_get_string(settings, "js_file");

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

	if (!data->url || strcmp(url, data->url) != 0) {
		if (data->url) {
			bfree(data->url);
			data->url = NULL;
		}
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
		if (data->css_file) {
			bfree(data->css_file);
			data->css_file = NULL;
		}
		data->css_file = bstrdup(css_file);
		browser_manager_change_css_file(data->manager, data->css_file);
	}
	if (!data->js_file || strcmp(js_file, data->js_file) != 0) {
		if (data->js_file) {
			bfree(data->js_file);
			data->js_file = NULL;
		}
		data->js_file = bstrdup(js_file);
		browser_manager_change_js_file(data->manager, data->js_file);
	}

	/* need to recreate texture if size changed */
	pthread_mutex_lock(&data->textureLock);
	obs_enter_graphics();
	if (resize || !data->activeTexture) {
		if (resize)
			browser_manager_change_size(data->manager, data->width, data->height);
		if (data->activeTexture) {
			gs_texture_destroy(data->activeTexture);
			data->activeTexture = NULL;
		}
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
	browser_manager_change_js_file(data->manager, data->js_file);
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

	if (data->manager) {
		destroy_browser_manager(data->manager);
		data->manager = NULL;
	}

	obs_hotkey_unregister(data->reload_page_key);

	if (data->url) {
		bfree(data->url);
		data->url = NULL;
	}
	if (data->css_file) {
		bfree(data->css_file);
		data->css_file = NULL;
	}
	if (data->js_file) {
		bfree(data->js_file);
		data->js_file = NULL;
	}
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
	browser_manager_change_js_file(data->manager, data->js_file);
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
	browser_manager_change_js_file(data->manager, data->js_file);
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
	obs_properties_add_path(props, "js_file", obs_module_text("CustomJS"), OBS_PATH_FILE,
	                        "*.js", NULL);

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


static int custom_get_virtual_key(obs_key_t key)
{
	switch (key) {
	case OBS_KEY_RETURN: return VK_RETURN;
	case OBS_KEY_ESCAPE: return VK_ESCAPE;
	case OBS_KEY_TAB: return VK_TAB;
	case OBS_KEY_BACKTAB: return VK_OEM_BACKTAB;
	case OBS_KEY_BACKSPACE: return VK_BACK;
	case OBS_KEY_INSERT: return VK_INSERT;
	case OBS_KEY_DELETE: return VK_DELETE;
	case OBS_KEY_PAUSE: return VK_PAUSE;
	case OBS_KEY_PRINT: return VK_SNAPSHOT;
	case OBS_KEY_CLEAR: return VK_CLEAR;
	case OBS_KEY_HOME: return VK_HOME;
	case OBS_KEY_END: return VK_END;
	case OBS_KEY_LEFT: return VK_LEFT;
	case OBS_KEY_UP: return VK_UP;
	case OBS_KEY_RIGHT: return VK_RIGHT;
	case OBS_KEY_DOWN: return VK_DOWN;
	case OBS_KEY_PAGEUP: return VK_PRIOR;
	case OBS_KEY_PAGEDOWN: return VK_NEXT;

	case OBS_KEY_SHIFT: return VK_SHIFT;
	case OBS_KEY_CONTROL: return VK_CONTROL;
	case OBS_KEY_ALT: return VK_MENU;
	case OBS_KEY_CAPSLOCK: return VK_CAPITAL;
	case OBS_KEY_NUMLOCK: return VK_NUMLOCK;
	case OBS_KEY_SCROLLLOCK: return VK_SCROLL;

	case OBS_KEY_F1: return VK_F1;
	case OBS_KEY_F2: return VK_F2;
	case OBS_KEY_F3: return VK_F3;
	case OBS_KEY_F4: return VK_F4;
	case OBS_KEY_F5: return VK_F5;
	case OBS_KEY_F6: return VK_F6;
	case OBS_KEY_F7: return VK_F7;
	case OBS_KEY_F8: return VK_F8;
	case OBS_KEY_F9: return VK_F9;
	case OBS_KEY_F10: return VK_F10;
	case OBS_KEY_F11: return VK_F11;
	case OBS_KEY_F12: return VK_F12;
	case OBS_KEY_F13: return VK_F13;
	case OBS_KEY_F14: return VK_F14;
	case OBS_KEY_F15: return VK_F15;
	case OBS_KEY_F16: return VK_F16;
	case OBS_KEY_F17: return VK_F17;
	case OBS_KEY_F18: return VK_F18;
	case OBS_KEY_F19: return VK_F19;
	case OBS_KEY_F20: return VK_F20;
	case OBS_KEY_F21: return VK_F21;
	case OBS_KEY_F22: return VK_F22;
	case OBS_KEY_F23: return VK_F23;
	case OBS_KEY_F24: return VK_F24;

	case OBS_KEY_SPACE: return VK_SPACE;

	case OBS_KEY_APOSTROPHE: return VK_OEM_7;
	case OBS_KEY_PLUS: return VK_OEM_PLUS;
	case OBS_KEY_COMMA: return VK_OEM_COMMA;
	case OBS_KEY_MINUS: return VK_OEM_MINUS;
	case OBS_KEY_PERIOD: return VK_OEM_PERIOD;
	case OBS_KEY_SLASH: return VK_OEM_2;
	case OBS_KEY_0: return '0';
	case OBS_KEY_1: return '1';
	case OBS_KEY_2: return '2';
	case OBS_KEY_3: return '3';
	case OBS_KEY_4: return '4';
	case OBS_KEY_5: return '5';
	case OBS_KEY_6: return '6';
	case OBS_KEY_7: return '7';
	case OBS_KEY_8: return '8';
	case OBS_KEY_9: return '9';
	case OBS_KEY_NUMASTERISK: return VK_MULTIPLY;
	case OBS_KEY_NUMPLUS: return VK_ADD;
	case OBS_KEY_NUMMINUS: return VK_SUBTRACT;
	case OBS_KEY_NUMPERIOD: return VK_DECIMAL;
	case OBS_KEY_NUMSLASH: return VK_DIVIDE;
	case OBS_KEY_NUM0: return VK_NUMPAD0;
	case OBS_KEY_NUM1: return VK_NUMPAD1;
	case OBS_KEY_NUM2: return VK_NUMPAD2;
	case OBS_KEY_NUM3: return VK_NUMPAD3;
	case OBS_KEY_NUM4: return VK_NUMPAD4;
	case OBS_KEY_NUM5: return VK_NUMPAD5;
	case OBS_KEY_NUM6: return VK_NUMPAD6;
	case OBS_KEY_NUM7: return VK_NUMPAD7;
	case OBS_KEY_NUM8: return VK_NUMPAD8;
	case OBS_KEY_NUM9: return VK_NUMPAD9;
	case OBS_KEY_SEMICOLON: return VK_OEM_1;
	case OBS_KEY_A: return 'A';
	case OBS_KEY_B: return 'B';
	case OBS_KEY_C: return 'C';
	case OBS_KEY_D: return 'D';
	case OBS_KEY_E: return 'E';
	case OBS_KEY_F: return 'F';
	case OBS_KEY_G: return 'G';
	case OBS_KEY_H: return 'H';
	case OBS_KEY_I: return 'I';
	case OBS_KEY_J: return 'J';
	case OBS_KEY_K: return 'K';
	case OBS_KEY_L: return 'L';
	case OBS_KEY_M: return 'M';
	case OBS_KEY_N: return 'N';
	case OBS_KEY_O: return 'O';
	case OBS_KEY_P: return 'P';
	case OBS_KEY_Q: return 'Q';
	case OBS_KEY_R: return 'R';
	case OBS_KEY_S: return 'S';
	case OBS_KEY_T: return 'T';
	case OBS_KEY_U: return 'U';
	case OBS_KEY_V: return 'V';
	case OBS_KEY_W: return 'W';
	case OBS_KEY_X: return 'X';
	case OBS_KEY_Y: return 'Y';
	case OBS_KEY_Z: return 'Z';
	case OBS_KEY_BRACKETLEFT: return VK_OEM_4;
	case OBS_KEY_BACKSLASH: return VK_OEM_5;
	case OBS_KEY_BRACKETRIGHT: return VK_OEM_6;
	case OBS_KEY_ASCIITILDE: return VK_OEM_3;

	case OBS_KEY_HENKAN: return VK_CONVERT;
	case OBS_KEY_MUHENKAN: return VK_NONCONVERT;
	case OBS_KEY_KANJI: return VK_KANJI;
	case OBS_KEY_TOUROKU: return VK_OEM_FJ_TOUROKU;
	case OBS_KEY_MASSYO: return VK_OEM_FJ_MASSHOU;

	case OBS_KEY_HANGUL: return VK_HANGUL;

	case OBS_KEY_BACKSLASH_RT102: return VK_OEM_102;

	case OBS_KEY_MOUSE1: return VK_LBUTTON;
	case OBS_KEY_MOUSE2: return VK_RBUTTON;
	case OBS_KEY_MOUSE3: return VK_MBUTTON;
	case OBS_KEY_MOUSE4: return VK_XBUTTON1;
	case OBS_KEY_MOUSE5: return VK_XBUTTON2;

	/* TODO: Implement keys for non-US keyboards */
	default:;
	}
	return 0;
}

static void browser_key_click(void* vptr, const struct obs_key_event* event, bool key_up)
{
	struct browser_data* data = vptr;
	char chr = 0;
	if (event->text)
		chr = event->text[0];
	blog(LOG_INFO, "Key: %s %d %d %d", event->text, event->native_vkey, event->native_scancode, custom_get_virtual_key(obs_key_from_virtual_key(event->native_vkey)));

	browser_manager_send_key(data->manager, key_up, custom_get_virtual_key(obs_key_from_virtual_key(event->native_vkey)), event->modifiers, chr);
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
		browser_manager_change_js_file(data->manager, data->js_file);
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
