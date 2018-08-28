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

#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>

#include "manager.h"

char* get_shm_name(const char* uid)
{
	char* shm_name = bzalloc(SHM_MAX);
	snprintf(shm_name, SHM_MAX, "%s-%s", SHM_NAME, uid);
	// escape out '/' character
	char* current_pos = strchr(shm_name + 1, '/');
	while (current_pos) {
		*current_pos = '|';
		current_pos = strchr(current_pos, '/');
	}
	return shm_name;
}

/* mostly building strings for arguments and env variables for
 * browser process */
static void spawn_renderer(browser_manager_t* manager)
{
	if (manager->spawned)
		return;

	char* bin_dir = bstrdup(obs_get_module_binary_path(obs_current_module()));
	char* s = strrchr(bin_dir, '/');
	if (s)
		*(s + 1) = '\0';

	const char* sflash_path = obs_data_get_string(manager->settings, "flash_path");
	const char* sflash_version = obs_data_get_string(manager->settings, "flash_version");

	size_t flash_path_size = strlen("--ppapi-flash-path=") + strlen(sflash_path) + 1;
	char* flash_path = bzalloc(flash_path_size);
	snprintf(flash_path, flash_path_size, "--ppapi-flash-path=%s", sflash_path);

	size_t flash_version_size = strlen("--ppapi-flash-version=") + strlen(sflash_version) + 1;
	char* flash_version = bzalloc(flash_version_size);
	snprintf(flash_version, flash_version_size, "--ppapi-flash-version=%s", sflash_version);

	size_t renderer_size = strlen(bin_dir) + strlen("browser") + 1;
	char* renderer = bzalloc(renderer_size);
	snprintf(renderer, renderer_size, "%sbrowser", bin_dir);

	char* data_path = (char*) obs_get_module_data_path(obs_current_module());

	obs_data_array_t* command_lines = obs_data_get_array(manager->settings, "cef_command_line");
	size_t arg_num = 6 + obs_data_array_count(command_lines);

	char** argv = bzalloc(sizeof(char*) * arg_num);
	argv[0] = renderer;
	argv[1] = data_path;
	argv[2] = manager->shmname;
	argv[3] = flash_path;
	argv[4] = flash_version;

	for (int i = 0; i < arg_num - 6; ++i) {
		obs_data_t* item = obs_data_array_item(command_lines, i);
		const char* value = obs_data_get_string(item, "value");
		argv[5 + i] = bstrdup(value);
		obs_data_release(item);
	}

	argv[arg_num - 1] = NULL;

	manager->pid = fork();
	if (manager->pid == 0) {
		setenv("LD_LIBRARY_PATH", bin_dir, 1);
		execv(renderer, argv);
	}

	for (int i = 0; i < arg_num - 6; ++i) {
		bfree(argv[5 + i]);
	}
	obs_data_array_release(command_lines);
	bfree(argv);

	bfree(bin_dir);
	bfree(flash_path);
	bfree(flash_version);
	bfree(renderer);

	manager->spawned = true;
}

static void kill_renderer(browser_manager_t* manager)
{
	if (manager->pid > 0) {
		kill(manager->pid, SIGTERM);
		waitpid(manager->pid, NULL, 0);
		manager->spawned = false;
	}
}

/* setting up shared data for the browser process */
browser_manager_t* create_browser_manager(uint32_t width, uint32_t height, int fps,
                                          obs_data_t* settings, const char* uid)
{
	if (width > MAX_BROWSER_WIDTH || height > MAX_BROWSER_HEIGHT)
		return NULL;

	struct browser_manager* manager = bzalloc(sizeof(struct browser_manager));
	manager->settings = settings;

	manager->qid = msgget(IPC_PRIVATE, S_IRUSR | S_IWUSR);
	if (manager->qid == -1) {
		blog(LOG_ERROR, "msgget error");
		return NULL;
	}

	manager->shmname = get_shm_name(uid);
	manager->fd = shm_open(manager->shmname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (manager->fd == -1) {
		blog(LOG_ERROR, "shm_open error");
		return NULL;
	}
	if (ftruncate(manager->fd, sizeof(struct shared_data) + MAX_DATA_SIZE) == -1) {
		blog(LOG_ERROR, "ftruncate error");
		return NULL;
	}
	manager->data =
	    (struct shared_data*) mmap(NULL, sizeof(struct shared_data) + MAX_DATA_SIZE,
	                               PROT_READ | PROT_WRITE, MAP_SHARED, manager->fd, 0);
	if (manager->data == MAP_FAILED) {
		blog(LOG_ERROR, "mmap error");
		return NULL;
	}
	manager->data->qid = manager->qid;
	manager->data->width = width;
	manager->data->height = height;
	manager->data->fps = fps;

	pthread_mutexattr_t attrmutex;
	pthread_mutexattr_init(&attrmutex);
	pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&manager->data->mutex, &attrmutex);

	spawn_renderer(manager);

	return manager;
};

void destroy_browser_manager(browser_manager_t* manager)
{
	kill_renderer(manager);
	pthread_mutex_destroy(&manager->data->mutex);
	if (manager->data != NULL && manager->data != MAP_FAILED)
		munmap(manager->data, sizeof(struct shared_data) + MAX_DATA_SIZE);
	if (manager->fd != -1 && manager->shmname != NULL) {
		shm_unlink(manager->shmname);
	}
	if (manager->shmname)
		bfree(manager->shmname);
	bfree(manager);
}

void lock_browser_manager(browser_manager_t* manager)
{
	pthread_mutex_lock(&manager->data->mutex);
}

void unlock_browser_manager(browser_manager_t* manager)
{
	pthread_mutex_unlock(&manager->data->mutex);
}

uint8_t* get_browser_manager_data(browser_manager_t* manager)
{
	return &manager->data->data;
}

void browser_manager_change_url(browser_manager_t* manager, const char* url)
{
	if (manager->qid == -1)
		return;

	if (strlen(url) < (MAX_MESSAGE_SIZE - 1)) {
		browser_message_t buf;
		buf.text.type = MESSAGE_TYPE_URL;
		strncpy(buf.text.text, url, MAX_MESSAGE_SIZE - 1);

		msgsnd(manager->qid, &buf, strlen(url) + 1, 0);
	} else {
		uint8_t packages = ceil((double) strlen(url) / (double) (MAX_MESSAGE_SIZE - 3));

		static uint8_t split_id = 0;
		split_id++;
		for (uint8_t i = 0; i < packages; i++) {
			browser_message_t buf;
			buf.split_text.id = split_id;
			buf.split_text.count = i;
			buf.split_text.max = packages;
			buf.split_text.type = MESSAGE_TYPE_URL_LONG;

			strncpy(buf.split_text.text, &url[i * (MAX_MESSAGE_SIZE - 3)],
			        MAX_MESSAGE_SIZE - 3);
			msgsnd(manager->qid, &buf, strlen(buf.split_text.text) + 3, 0);
		}
	}
}

void browser_manager_change_css_file(browser_manager_t* manager, const char* css_file)
{
	if (manager->qid == -1)
		return;

	browser_message_t buf;
	buf.text.type = MESSAGE_TYPE_CSS;
	strncpy(buf.text.text, css_file, MAX_MESSAGE_SIZE);

	msgsnd(manager->qid, &buf, strlen(css_file) + 1, 0);
}

void browser_manager_change_js_file(browser_manager_t* manager, const char* js_file)
{
	if (manager->qid < 0)
		return;

	browser_message_t buf;
	buf.text.type = MESSAGE_TYPE_JS;
	strncpy(buf.text.text, js_file, MAX_MESSAGE_SIZE);

	msgsnd(manager->qid, &buf, strlen(js_file) + 1, 0);
}

void browser_manager_change_size(browser_manager_t* manager, uint32_t width, uint32_t height)
{
	pthread_mutex_lock(&manager->data->mutex);

	manager->data->width = width;
	manager->data->height = height;

	if (manager->qid == -1)
		return;

	browser_message_t buf;
	buf.generic.type = MESSAGE_TYPE_SIZE;
	msgsnd(manager->qid, &buf, 0, 0);

	pthread_mutex_unlock(&manager->data->mutex);
}

void browser_manager_set_scrollbars(browser_manager_t* manager, bool show)
{
	if (manager->qid == -1)
		return;

	browser_message_t buf;
	buf.generic_state.type = MESSAGE_TYPE_SCROLLBARS;
	buf.generic_state.state = show;
	msgsnd(manager->qid, &buf, 1, 0);
}

void browser_manager_set_zoom(browser_manager_t* manager, uint32_t zoom)
{
	if (manager->qid == -1)
		return;

	browser_message_t buf;
	buf.zoom.type = MESSAGE_TYPE_ZOOM;
	buf.zoom.zoom = zoom;
	msgsnd(manager->qid, &buf, sizeof(zoom), 0);
}

void browser_manager_set_scroll(browser_manager_t* manager, uint32_t vertical, uint32_t horizontal)
{
	if (manager->qid == -1)
		return;

	browser_message_t buf;
	buf.scroll.type = MESSAGE_TYPE_SCROLL;
	buf.scroll.vertical = vertical;
	buf.scroll.horizontal = horizontal;
	msgsnd(manager->qid, &buf, sizeof(vertical) + sizeof(horizontal), 0);
}

void browser_manager_reload_page(browser_manager_t* manager)
{
	if (manager->qid == -1)
		return;

	browser_message_t buf;
	buf.generic.type = MESSAGE_TYPE_RELOAD;
	msgsnd(manager->qid, &buf, 0, 0);
}

void browser_manager_restart_browser(browser_manager_t* manager)
{
	pthread_mutex_lock(&manager->data->mutex);
	kill_renderer(manager);
	spawn_renderer(manager);
	pthread_mutex_unlock(&manager->data->mutex);
}

void browser_manager_start_browser(browser_manager_t* manager)
{
	pthread_mutex_lock(&manager->data->mutex);
	// TODO Double spawn prevention?
	spawn_renderer(manager);
	pthread_mutex_unlock(&manager->data->mutex);
}

void browser_manager_stop_browser(browser_manager_t* manager)
{
	pthread_mutex_lock(&manager->data->mutex);
	// TODO Double kill prevention?
	kill_renderer(manager);
	pthread_mutex_unlock(&manager->data->mutex);
}

void browser_manager_send_mouse_click(browser_manager_t* manager, int32_t x, int32_t y,
                                      uint32_t modifiers, int32_t button_type, bool mouse_up,
                                      uint32_t click_count)
{
	if (manager->qid == -1)
		return;

	browser_message_t buf;
	buf.mouse_click.type = MESSAGE_TYPE_MOUSE_CLICK;
	buf.mouse_click.x = x;
	buf.mouse_click.y = y;
	buf.mouse_click.modifiers = modifiers;
	buf.mouse_click.button_type = button_type;
	buf.mouse_click.mouse_up = mouse_up;
	buf.mouse_click.click_count = click_count;

	msgsnd(manager->qid, &buf, sizeof(buf), 0);
}

void browser_manager_send_mouse_move(browser_manager_t* manager, int32_t x, int32_t y,
                                     uint32_t modifiers, bool mouse_leave)
{
	if (manager->qid == -1)
		return;

	// struct mouse_move_message buf;
	browser_message_t buf;
	buf.mouse_move.type = MESSAGE_TYPE_MOUSE_MOVE;
	buf.mouse_move.x = x;
	buf.mouse_move.y = y;
	buf.mouse_move.modifiers = modifiers;
	buf.mouse_move.mouse_leave = mouse_leave;

	msgsnd(manager->qid, &buf, sizeof(buf), 0);
}

void browser_manager_send_mouse_wheel(browser_manager_t* manager, int32_t x, int32_t y,
                                      uint32_t modifiers, int x_delta, int y_delta)
{
	if (manager->qid == -1)
		return;

	browser_message_t buf;
	buf.mouse_wheel.type = MESSAGE_TYPE_MOUSE_WHEEL;
	buf.mouse_wheel.x = x;
	buf.mouse_wheel.y = y;
	buf.mouse_wheel.modifiers = modifiers;
	buf.mouse_wheel.x_delta = x_delta;
	buf.mouse_wheel.y_delta = y_delta;

	msgsnd(manager->qid, &buf, sizeof(buf), 0);
}

void browser_manager_send_focus(browser_manager_t* manager, bool focus)
{
	if (manager->qid == -1)
		return;

	browser_message_t buf;
	buf.focus.type = MESSAGE_TYPE_FOCUS;
	buf.focus.focus = focus;

	msgsnd(manager->qid, &buf, sizeof(buf), 0);
}

void browser_manager_send_key(browser_manager_t* manager, bool key_up, uint32_t native_vkey,
                              uint32_t modifiers, char chr)
{
	if (manager->qid == -1)
		return;

	browser_message_t buf;
	buf.key.type = MESSAGE_TYPE_KEY;
	buf.key.key_up = key_up;
	buf.key.native_vkey = native_vkey;
	buf.key.modifiers = modifiers;
	buf.key.chr = chr;

	msgsnd(manager->qid, &buf, sizeof(buf), 0);
}

void browser_manager_send_active_state_change(browser_manager_t* manager, bool active)
{
	if (manager->qid == -1)
		return;

	browser_message_t buf;
	buf.active_state.type = MESSAGE_TYPE_ACTIVE_STATE_CHANGE;
	buf.active_state.active = active;
	msgsnd(manager->qid, &buf, sizeof(buf), 0);
}

void browser_manager_send_visibility_change(browser_manager_t* manager, bool visible)
{
	if (manager->qid == -1)
		return;

	browser_message_t buf;
	buf.visibility.type = MESSAGE_TYPE_VISIBILITY_CHANGE;
	buf.visibility.visible = visible;
	msgsnd(manager->qid, &buf, sizeof(buf), 0);
}
