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

#define MAX_BROWSER_WIDTH 4096
#define MAX_BROWSER_HEIGHT 4096
#define MAX_DATA_SIZE MAX_BROWSER_WIDTH * MAX_BROWSER_HEIGHT * 4

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

struct generic_message {
	long type;
	void *data;
};

struct url_message {
	long type;
	char url[MAX_MESSAGE_SIZE];
};
