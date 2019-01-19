/*
Copyright (C) 2017 by Azat Khasanshin <azat.khasanshin@gmail.com>
Copyright (C) 2018, 2019 by Adrian Schollmeyer <nexadn@yandex.com>

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

#include <cef_client.h>

#include "shared.h"

#include "config.h"

class BrowserClient
        : public CefClient
        , public CefRenderHandler
        , public CefLoadHandler {
public:
	BrowserClient(shared_data_t* data, std::string css);

	virtual CefRefPtr<CefRenderHandler> GetRenderHandler() OVERRIDE
	{
		return this;
	}
	virtual CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE
	{
		return this;
	}

	virtual BC_GET_VIEW_RECT_RETURN_TYPE GetViewRect(CefRefPtr<CefBrowser> browser,
	                                                 CefRect& rect) override;
	virtual void OnPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type,
	                     const CefRenderHandler::RectList& dirtyRects, const void* buffer,
	                     int width, int height) OVERRIDE;

	virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
	                       int httpStatusCode) OVERRIDE;

	void ChangeCss(std::string css)
	{
		this->css = css;
	};
	void ChangeJs(std::string js);
	void SetScrollbars(CefRefPtr<CefBrowser> browser, bool show);
	void SetZoom(CefRefPtr<CefBrowser> browser, uint32_t zoom);
	void SetScroll(CefRefPtr<CefBrowser> browser, uint32_t vertical, uint32_t horizontal);

private:
	shared_data_t* data;
	std::string css;
	std::string js;
	bool show_scrollbars{true};
	uint32_t zoom;
	uint32_t scroll_vertical;
	uint32_t scroll_horizontal;

	IMPLEMENT_REFCOUNTING(BrowserClient);
};
