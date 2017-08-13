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

#include <include/cef_client.h>

#include "shared.h"

class BrowserClient : public CefClient,
                      public CefRenderHandler,
		      public CefLoadHandler {
public:
	BrowserClient(struct shared_data *data, std::string css);

	virtual CefRefPtr<CefRenderHandler> GetRenderHandler()
		OVERRIDE { return this; }
	virtual CefRefPtr<CefLoadHandler> GetLoadHandler()
		OVERRIDE { return this; }

	virtual bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) OVERRIDE;
	virtual void OnPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type,
			const CefRenderHandler::RectList &dirtyRects, const void *buffer,
			int width, int height) OVERRIDE;

	virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
			int httpStatusCode) OVERRIDE;

	void ChangeCss(std::string css) { this->css = css; };
	void SetScrollbars(bool show) { this->show_scrollbars = show; };
private:
	struct shared_data *data;
	std::string css;
	bool show_scrollbars = true;

	IMPLEMENT_REFCOUNTING(BrowserClient);
};
