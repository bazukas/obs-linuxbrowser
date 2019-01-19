/*
Copyright (C) 2017 by Azat Khasanshin <azat.khasanshin@gmail.com>,
                      John R. Bradley <jrb@turrettech.com>
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
#include <algorithm>
#include <cmath>

#include "base64.hpp"
#include "browser-client.hpp"

BrowserClient::BrowserClient(shared_data_t* data, std::string css)
{
	this->data = data;
	this->css = css;
}

BC_GET_VIEW_RECT_RETURN_TYPE BrowserClient::GetViewRect(CefRefPtr<CefBrowser> browser,
                                                        CefRect& rect)
{
	pthread_mutex_lock(&data->mutex);
	rect.Set(0, 0, data->width, data->height);
	pthread_mutex_unlock(&data->mutex);
	BC_GET_VIEW_RECT_RETURN
}

void BrowserClient::OnPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type,
                            const CefRenderHandler::RectList& dirtyRects, const void* buffer,
                            int vwidth, int vheight)
{
	// Don't draw popups for now
	if (type == PET_VIEW) {
		pthread_mutex_lock(&data->mutex);
		size_t len = std::min(vwidth * vheight * 4, int(data->width * data->height * 4));
		std::memcpy(&data->data, buffer, len);
		pthread_mutex_unlock(&data->mutex);
	}
}

void BrowserClient::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                              int httpStatusCode)
{
	if (frame->IsMain() && css != "") {
		std::string base64EncodedCSS = base64_encode(
		    reinterpret_cast<const unsigned char*>(css.c_str()), css.length());
		std::string href = "data:text/css;charset=utf-8;base64," + base64EncodedCSS;

		std::string script = "";
		script += "let link = document.createElement('link');";
		script += "link.setAttribute('rel', 'stylesheet');";
		script += "link.setAttribute('type', 'text/css');";
		script += "link.setAttribute('href', '" + href + "');";
		script += "document.getElementsByTagName('head')[0].appendChild(link);";

		frame->ExecuteJavaScript(script, href, 0);
	}
	if (frame->IsMain() && js != "") {
		frame->ExecuteJavaScript(this->js, "", 0);
	}
	SetScrollbars(browser, show_scrollbars);
	SetZoom(browser, zoom);
}

void BrowserClient::SetScrollbars(CefRefPtr<CefBrowser> browser, bool show)
{
	this->show_scrollbars = show;
	CefRefPtr<CefFrame> frame = browser->GetMainFrame();
	if (show) {
		frame->ExecuteJavaScript(
		    std::string("document.documentElement.style.overflow = 'auto';"),
		    frame->GetURL(), 0);
	} else {
		frame->ExecuteJavaScript(
		    std::string("document.documentElement.style.overflow = 'hidden';"),
		    frame->GetURL(), 0);
	}
	SetScroll(browser, scroll_vertical, scroll_horizontal);
}

void BrowserClient::SetZoom(CefRefPtr<CefBrowser> browser, uint32_t zoom)
{
	this->zoom = zoom;
	double zoom_scale{log(zoom / 100.0) / log(1.2)};
	browser->GetHost()->SetZoomLevel(zoom_scale);
}

void BrowserClient::SetScroll(CefRefPtr<CefBrowser> browser, uint32_t vertical, uint32_t horizontal)
{
	this->scroll_vertical = vertical;
	this->scroll_horizontal = horizontal;
	CefRefPtr<CefFrame> frame{browser->GetMainFrame()};
	std::string script{"window.scrollTo("};
	script += std::to_string(horizontal) + "," + std::to_string(vertical) + ");";
	frame->ExecuteJavaScript(script, frame->GetURL(), 0);
}

void BrowserClient::ChangeJs(std::string js)
{
	this->js = js;
}
