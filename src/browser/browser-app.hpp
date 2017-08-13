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

#include <include/cef_app.h>

#include "browser-client.hpp"
#include "shared.h"

class BrowserApp : public CefApp,
                   public CefBrowserProcessHandler {
public:
	BrowserApp(char *shm_name);
	~BrowserApp();

	virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler()
		OVERRIDE { return this; }

	virtual void OnContextInitialized() OVERRIDE;

	int GetQueueId() { return qid; };
	CefRefPtr<CefBrowser> GetBrowser() { return browser; };
	void SizeChanged();
	void UrlChanged(const char *url);
	void CssChanged(const char *css_file);
	void ReloadPage();
	void SetScrollbars(bool show);
private:
	void InitSharedData();
	void UninitSharedData();

private:
	CefRefPtr<CefBrowser> browser;
	CefRefPtr<BrowserClient> client;
	pthread_t message_thread;
	char *shm_name;
	uint32_t width;
	uint32_t height;
	int fps;
	int fd;
	int qid;
	struct shared_data *data;
	std::string css;
	int in_fd;
	int in_wd = -1;

	IMPLEMENT_REFCOUNTING(BrowserApp);
};
