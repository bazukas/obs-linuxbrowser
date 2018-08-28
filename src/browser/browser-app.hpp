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
#pragma once

#include <string>
#include <thread>

#include <cef_app.h>

#include "browser-client.hpp"
#include "shared.h"
#include "split-message.hpp"

class BrowserApp
        : public CefApp
        , public CefBrowserProcessHandler
        , public CefRenderProcessHandler
        , public CefV8Handler {
public:
	BrowserApp(char* shm_name);
	~BrowserApp();

	virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() OVERRIDE
	{
		return this;
	}

	virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override
	{
		return this;
	}

	virtual void OnContextInitialized() OVERRIDE;

	int GetQueueId()
	{
		return qid;
	}
	CefRefPtr<CefBrowser> GetBrowser()
	{
		return browser;
	}
	void SizeChanged();
	void UrlChanged(const char* url);
	void UrlChanged(std::string url);
	void CssChanged(const char* css_file);
	void JsChanged(std::string jsFile);
	void ReloadPage();

	CefRefPtr<BrowserClient> GetClient()
	{
		return client;
	}

	void OnBeforeCommandLineProcessing(const CefString& process_type,
	                                   CefRefPtr<CefCommandLine> command_line) override;

	virtual void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
	                              CefRefPtr<CefV8Context> context) override;

	virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
	                                      CefProcessId source_process,
	                                      CefRefPtr<CefProcessMessage> message) override;

	void UpdateActiveStateJS(bool active);
	void UpdateVisibilityStateJS(bool visible);

	virtual bool Execute(const CefString& name, CefRefPtr<CefV8Value> object,
	                     const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval,
	                     CefString& exception) override;

private:
	void InitSharedData();
	void UninitSharedData();

	void ExecuteJSFunction(CefRefPtr<CefBrowser> browser, const char* functionName,
	                       CefV8ValueList arguments);

	void MessageThreadWorker();

private:
	CefRefPtr<CefBrowser> browser;
	CefRefPtr<BrowserClient> client;
	std::thread messageThread;
	std::string shm_name;
	uint32_t width;
	uint32_t height;
	int fps;
	int fd;
	int qid;
	shared_data_t* data;
	std::string css;
	std::string js;
	int in_fd;
	int in_wd{-1};

	IMPLEMENT_REFCOUNTING(BrowserApp);
};
