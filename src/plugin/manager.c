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

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>

#include "manager.h"

#define SHM_NAME "/obslinuxbrowser"
#define SHM_MAX 50

char *get_shm_name(int uid)
{
	char *shm_name = bzalloc(SHM_MAX);
	snprintf(shm_name, SHM_MAX, "%s%d", SHM_NAME, uid);
	return shm_name;
}

static void spawn_renderer(browser_manager_t *manager)
{
	char renderer[512];
	char path[512];
	char display[512];
	char flash_path[1024];
	char flash_version[512];
	strncpy(path, "LD_LIBRARY_PATH=", 512);
	char *s;
	const char *file = obs_get_module_binary_path(obs_current_module());
	strncpy(renderer, file, 512);
	s = strrchr(renderer, '/');
	if (s)
		*(s+1) = '\0';
	strncat(path, renderer, 512-strlen(path)-1);
	strncat(renderer, "browser", 512-strlen(renderer)-1);
	snprintf(display, 512, "DISPLAY=%s", getenv("DISPLAY"));

	snprintf(flash_path, 1024, "--ppapi-flash-path=%s", manager->flash_path);
	snprintf(flash_version, 512, "--ppapi-flash-version=%s", manager->flash_version);
	char *argv[] = { renderer, manager->shmname, flash_path, flash_version, NULL };
	char *envp[] = { path, display, NULL };

	manager->pid = fork();
	if (manager->pid == 0)
		execve(renderer, argv, envp);
}

static void kill_renderer(browser_manager_t *manager)
{
	if (manager->pid > 0) {
		kill(manager->pid, SIGTERM);
		waitpid(manager->pid, NULL, 0);
	}
}

browser_manager_t *create_browser_manager(uint32_t width, uint32_t height, int fps,
			const char *flash_path, const char *flash_version)
{
	if (width > MAX_BROWSER_WIDTH || height > MAX_BROWSER_HEIGHT)
		return NULL;

	struct browser_manager *manager = bzalloc(sizeof(struct browser_manager));

	manager->qid = msgget(IPC_PRIVATE, S_IRUSR | S_IWUSR);
	if (manager->qid == -1) {
		blog(LOG_ERROR, "msgget error");
		return NULL;
	}

	manager->shmname = get_shm_name(rand());
	manager->fd = shm_open(manager->shmname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (manager->fd == -1) {
		blog(LOG_ERROR, "shm_open error");
		return NULL;
	}
	if (ftruncate(manager->fd, sizeof(struct shared_data) + MAX_DATA_SIZE) == -1) {
		blog(LOG_ERROR, "ftruncate error");
		return NULL;
	}
	manager->data = (struct shared_data *) mmap(NULL, sizeof(struct shared_data) + MAX_DATA_SIZE,
				PROT_READ | PROT_WRITE, MAP_SHARED, manager->fd, 0);
	if (manager->data == MAP_FAILED) {
		blog(LOG_ERROR, "mmap error");
		return NULL;
	}
	manager->data->qid = manager->qid;
	manager->data->width = width;
	manager->data->height = height;
	manager->data->fps = fps;
	manager->flash_path = bstrdup(flash_path);
	manager->flash_version = bstrdup(flash_version);

	pthread_mutexattr_t attrmutex;
	pthread_mutexattr_init(&attrmutex);
	pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&manager->data->mutex, &attrmutex);

	spawn_renderer(manager);

	return manager;
};

void destroy_browser_manager(browser_manager_t *manager) {
	kill_renderer(manager);
	pthread_mutex_destroy(&manager->data->mutex);
	if (manager->data != NULL && manager->data != MAP_FAILED)
		munmap(manager->data, sizeof(struct shared_data) + MAX_DATA_SIZE);
	if (manager->fd != -1 && manager->shmname != NULL) {
		shm_unlink(manager->shmname);
	}
	if (manager->shmname)
		bfree(manager->shmname);
	if (manager->flash_path)
		bfree(manager->flash_path);
	if (manager->flash_version)
		bfree(manager->flash_version);
	bfree(manager);
}

void lock_browser_manager(browser_manager_t *manager)
{
	pthread_mutex_lock(&manager->data->mutex);
}

void unlock_browser_manager(browser_manager_t *manager)
{
	pthread_mutex_unlock(&manager->data->mutex);
}

uint8_t *get_browser_manager_data(browser_manager_t *manager)
{
	return &manager->data->data;
}

void browser_manager_change_url(browser_manager_t *manager, const char *url)
{
	if (manager->qid == -1)
		return;

	struct url_message buf;
	buf.type = MESSAGE_TYPE_URL;
	strncpy(buf.url, url, MAX_MESSAGE_SIZE);

	msgsnd(manager->qid, &buf, strlen(url)+1, 0);
}

void browser_manager_change_size(browser_manager_t *manager, uint32_t width, uint32_t height)
{
	pthread_mutex_lock(&manager->data->mutex);

	manager->data->width = width;
	manager->data->height = height;

	if (manager->qid == -1)
		return;

	struct generic_message buf;
	buf.type = MESSAGE_TYPE_SIZE;
	msgsnd(manager->qid, &buf, 0, 0);

	pthread_mutex_unlock(&manager->data->mutex);
}

void browser_manager_reload_page(browser_manager_t *manager)
{
	if (manager->qid == -1)
		return;

	struct generic_message buf;
	buf.type = MESSAGE_TYPE_RELOAD;
	msgsnd(manager->qid, &buf, 0, 0);
}

void browser_manager_restart_browser(browser_manager_t *manager)
{
	pthread_mutex_lock(&manager->data->mutex);
	kill_renderer(manager);
	spawn_renderer(manager);
	pthread_mutex_unlock(&manager->data->mutex);
}

void browser_manager_set_flash(browser_manager_t *manager, const char *flash_path, const char *flash_version)
{
	if (manager->flash_path)
		bfree(manager->flash_path);
	manager->flash_path = bstrdup(flash_path);
	if (manager->flash_version)
		bfree(manager->flash_version);
	manager->flash_version = bstrdup(flash_version);
}
