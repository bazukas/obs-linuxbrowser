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

#define SHM_NAME "/linuxbrowser"
#define SHM_MAX 50

#define MAX_BROWSER_WIDTH 4096
#define MAX_BROWSER_HEIGHT 4096
#define MAX_DATA_SIZE MAX_BROWSER_WIDTH* MAX_BROWSER_HEIGHT * 4

struct shared_data {
	pthread_mutex_t mutex;
	int qid;
	int fps;
	uint32_t width;
	uint32_t height;
	uint8_t data;
};

#define MAX_MESSAGE_SIZE 1024
#define MESSAGE_TYPE_URL 1
#define MESSAGE_TYPE_SIZE 2
#define MESSAGE_TYPE_RELOAD 3
#define MESSAGE_TYPE_CSS 4
#define MESSAGE_TYPE_MOUSE_CLICK 5
#define MESSAGE_TYPE_MOUSE_MOVE 6
#define MESSAGE_TYPE_MOUSE_WHEEL 7
#define MESSAGE_TYPE_FOCUS 8
#define MESSAGE_TYPE_KEY 9
#define MESSAGE_TYPE_SCROLLBARS 10
#define MESSAGE_TYPE_ZOOM 11
#define MESSAGE_TYPE_SCROLL 12

struct generic_message {
	long type;
	uint8_t data[MAX_MESSAGE_SIZE];
};

struct text_message {
	long type;
	char text[MAX_MESSAGE_SIZE];
};

struct mouse_click_message {
	long type;
	int32_t x;
	int32_t y;
	uint32_t modifiers;
	int32_t button_type;
	bool mouse_up;
	uint32_t click_count;
};

struct mouse_move_message {
	long type;
	int32_t x;
	int32_t y;
	uint32_t modifiers;
	bool mouse_leave;
};

struct mouse_wheel_message {
	long type;
	int32_t x;
	int32_t y;
	uint32_t modifiers;
	int x_delta;
	int y_delta;
};

struct focus_message {
	long type;
	bool focus;
};

struct key_message {
	long type;
	bool key_up;
	uint32_t native_vkey;
	uint32_t modifiers;
	char chr;
};

struct zoom_message {
	long type;
	uint32_t zoom;
};

struct scroll_message {
	long type;
	uint32_t vertical;
	uint32_t horizontal;
};
