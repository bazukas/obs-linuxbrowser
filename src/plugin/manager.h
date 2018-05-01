/*
Copyright (C) 2017 by Azat Khasanshin <azat.khasanshin@gmail.com>

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

#pragma once

#include <obs-module.h>

#include "shared.h"

#define blog(level, msg, ...) blog(level, "obs-linuxbrowser: " msg, ##__VA_ARGS__)

typedef struct browser_manager {
	int fd;
	int pid;
	int qid;
	char* shmname;
	obs_data_t* settings;
	struct shared_data* data;
} browser_manager_t;

browser_manager_t* create_browser_manager(uint32_t width, uint32_t height, int fps,
                                          obs_data_t* settings, const char* uid);
void destroy_browser_manager(browser_manager_t* manager);
void lock_browser_manager(browser_manager_t* manager);
void unlock_browser_manager(browser_manager_t* manager);
uint8_t* get_browser_manager_data(browser_manager_t* manager);

void browser_manager_change_url(browser_manager_t* manager, const char* url);
void browser_manager_change_css_file(browser_manager_t* manager, const char* css_file);
void browser_manager_change_size(browser_manager_t* manager, uint32_t width, uint32_t height);
void browser_manager_set_scrollbars(browser_manager_t* manager, bool show);
void browser_manager_set_zoom(browser_manager_t* manager, uint32_t zoom);
void browser_manager_set_scroll(browser_manager_t* manager, uint32_t vertical, uint32_t horizontal);
void browser_manager_reload_page(browser_manager_t* manager);
void browser_manager_restart_browser(browser_manager_t* manager);

void browser_manager_send_mouse_click(browser_manager_t* manager, int32_t x, int32_t y,
                                      uint32_t modifiers, int32_t button_type, bool mouse_up,
                                      uint32_t click_count);
void browser_manager_send_mouse_move(browser_manager_t* manager, int32_t x, int32_t y,
                                     uint32_t modifiers, bool mouse_leave);
void browser_manager_send_mouse_wheel(browser_manager_t* manager, int32_t x, int32_t y,
                                      uint32_t modifiers, int x_delta, int y_delta);
void browser_manager_send_focus(browser_manager_t* manager, bool focus);
void browser_manager_send_key(browser_manager_t* manager, bool key_up, uint32_t native_vkey,
                              uint32_t modifiers, char chr);
