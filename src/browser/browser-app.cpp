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
#include <signal.h>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <unordered_map>

#include "browser-app.hpp"
#include "config.h"

/* for signal handling */
namespace
{
int static_in_fd{0};
BrowserApp* ba{nullptr};
void file_changed(int signum)
{
	inotify_event event;

	if (static_in_fd != 0 && ba != nullptr) {
		read(static_in_fd, (void*) &event, sizeof(inotify_event));
		ba->ReloadPage();
	}
}

auto beginsWith{[](const std::string base, const std::string beginning) {
	return base.substr(0, beginning.size()) == beginning;
}};
} // namespace

BrowserApp::BrowserApp(char* shmname)
{
	if (shmname != nullptr && beginsWith({shmname}, {SHM_NAME})) {
		shm_name = shmname;

		// init inotify
		in_fd = inotify_init1(IN_NONBLOCK);
		fcntl(in_fd, F_SETFL, O_ASYNC);
		fcntl(in_fd, F_SETOWN, getpid());
		struct sigaction action;
		std::memset(&action, 0, sizeof(struct sigaction));
		action.sa_handler = file_changed;
		sigaction(SIGIO, &action, nullptr);
		static_in_fd = in_fd;
		ba = this;

		InitSharedData();
	}
}

BrowserApp::~BrowserApp()
{
	UninitSharedData();
	if (in_fd)
		close(in_fd);
}

void BrowserApp::OnBeforeCommandLineProcessing(const CefString& processType,
                                               CefRefPtr<CefCommandLine> commandLine)
{
	commandLine->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");
}

// Open shared memory and read initial data
void BrowserApp::InitSharedData()
{
	fd = shm_open(shm_name.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		std::cerr << "Browser: shared memory open failed\n";
		return;
	}
	data = reinterpret_cast<shared_data*>(
	    mmap(nullptr, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

	if (data == MAP_FAILED) {
		std::cerr << "Browser: data mapping failed\n";
		return;
	}

	pthread_mutex_lock(&data->mutex);
	qid = data->qid;
	width = data->width;
	height = data->height;
	fps = data->fps;
	pthread_mutex_unlock(&data->mutex);

	data = reinterpret_cast<shared_data*>(mmap(nullptr, sizeof(shared_data) + MAX_DATA_SIZE,
	                                           PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
	if (data == MAP_FAILED) {
		std::cerr << "Browser: data remapping failed\n";
		return;
	}
}

void BrowserApp::UninitSharedData()
{
	if (data && data != MAP_FAILED) {
		munmap(data, sizeof(shared_data) + MAX_DATA_SIZE);
	}
}

// Message receiver loop
void BrowserApp::MessageThreadWorker()
{
	std::unordered_map<uint8_t, SplitMessage*> splitMessages;
	size_t max_buf_size = MAX_MESSAGE_SIZE;
	ssize_t received;
	CefMouseEvent e;
	CefKeyEvent ke;

	browser_message_t msg;

	while (true) {
		received = msgrcv(this->GetQueueId(), &msg, max_buf_size, 0, MSG_NOERROR);
		if (received != -1) {
			switch (msg.generic.type) {
			case MESSAGE_TYPE_URL:
				this->UrlChanged(msg.text.text);
				break;
			case MESSAGE_TYPE_URL_LONG:

				if (splitMessages.count(msg.split_text.id) <= 0) {
					splitMessages.insert(
					    {msg.split_text.id,
					     new SplitMessage(msg.split_text.id,
					                      msg.split_text.max)});
				}

				splitMessages.at(msg.split_text.id)->addMessage(msg);

				if (splitMessages.at(msg.split_text.id)->dataIsReady()) {
					this->UrlChanged(
					    splitMessages.at(msg.split_text.id)->getData());
					delete splitMessages.at(msg.split_text.id);
					splitMessages.erase(msg.split_text.id);
				}
				break;
			case MESSAGE_TYPE_SIZE:
				this->SizeChanged();
				break;
			case MESSAGE_TYPE_RELOAD:
				this->ReloadPage();
				break;
			case MESSAGE_TYPE_CSS:
				this->CssChanged(msg.text.text);
				break;
			case MESSAGE_TYPE_JS:
				this->JsChanged(msg.text.text);
				break;
			case MESSAGE_TYPE_MOUSE_CLICK:
				e.modifiers = msg.mouse_click.modifiers;
				e.x = msg.mouse_click.x;
				e.y = msg.mouse_click.y;
				this->GetBrowser()->GetHost()->SendMouseClickEvent(
				    e,
				    static_cast<CefBrowserHost::MouseButtonType>(
				        msg.mouse_click.button_type),
				    msg.mouse_click.mouse_up, msg.mouse_click.click_count);
				break;
			case MESSAGE_TYPE_MOUSE_MOVE:
				e.modifiers = msg.mouse_move.modifiers;
				e.x = msg.mouse_move.x;
				e.y = msg.mouse_move.y;
				this->GetBrowser()->GetHost()->SendMouseMoveEvent(
				    e, msg.mouse_move.mouse_leave);
				break;
			case MESSAGE_TYPE_MOUSE_WHEEL:
				e.modifiers = msg.mouse_wheel.modifiers;
				e.x = msg.mouse_wheel.x;
				e.y = msg.mouse_wheel.y;
				this->GetBrowser()->GetHost()->SendMouseWheelEvent(
				    e, msg.mouse_wheel.x_delta, msg.mouse_wheel.y_delta);
				break;
			case MESSAGE_TYPE_FOCUS:
				this->GetBrowser()->GetHost()->SendFocusEvent(msg.focus.focus);
				break;
			case MESSAGE_TYPE_KEY:
				/* I have no idea what is happening */
				ke.windows_key_code = msg.key.native_vkey;
				ke.native_key_code = msg.key.native_vkey;
				ke.modifiers = msg.key.modifiers;
				ke.type = msg.key.key_up ? KEYEVENT_KEYUP : KEYEVENT_RAWKEYDOWN;
				if (msg.key.chr != 0) {
					ke.character = msg.key.chr;
					if (!msg.key.key_up)
						ke.type = KEYEVENT_CHAR;
				}
				this->GetBrowser()->GetHost()->SendKeyEvent(ke);
				break;
			case MESSAGE_TYPE_SCROLLBARS:
				this->GetClient()->SetScrollbars(this->GetBrowser(),
				                                 msg.generic_state.state);
				break;
			case MESSAGE_TYPE_ZOOM:
				this->GetClient()->SetZoom(this->GetBrowser(), msg.zoom.zoom);
				break;
			case MESSAGE_TYPE_SCROLL:
				this->GetClient()->SetScroll(
				    this->GetBrowser(), msg.scroll.vertical, msg.scroll.horizontal);
				break;
			case MESSAGE_TYPE_ACTIVE_STATE_CHANGE:
				this->UpdateActiveStateJS(msg.active_state.active);
				break;
			case MESSAGE_TYPE_VISIBILITY_CHANGE:
				this->UpdateVisibilityStateJS(msg.visibility.visible);
				break;
			}
		}
	}
}

void BrowserApp::SizeChanged()
{
	pthread_mutex_lock(&data->mutex);
	width = data->width;
	height = data->height;

	browser->GetHost()->WasResized();
	pthread_mutex_unlock(&data->mutex);
}

void BrowserApp::UrlChanged(const char* url)
{
	this->UrlChanged(std::string(url));
}

void BrowserApp::UrlChanged(std::string url)
{
	CefString cef_url;
	cef_url.FromString(url);
	browser->GetMainFrame()->LoadURL(cef_url);

	if (in_wd >= 0) {
		inotify_rm_watch(in_fd, in_wd);
		in_wd = -1;
	}

	if (beginsWith(url, "file:///")) {
		in_wd = inotify_add_watch(in_fd, url.substr(8).c_str(), IN_MODIFY);
	}
}

void BrowserApp::CssChanged(const char* css_file)
{
	std::ifstream t{css_file, std::ifstream::in};
	if (t.good()) {
		std::stringstream buffer;
		buffer << t.rdbuf();
		css = buffer.str();
	} else {
		css = "";
	}

	client->ChangeCss(css);
}

void BrowserApp::JsChanged(std::string jsFile)
{
	std::ifstream t{jsFile, std::ifstream::in};
	if (t.good()) {
		std::stringstream buf;
		buf << t.rdbuf();
		this->js = buf.str();
	} else {
		this->js = "";
	}

	this->client->ChangeJs(this->js);
}

void BrowserApp::ReloadPage()
{
	browser->ReloadIgnoreCache();
}

// Browser instance is being initialized here
void BrowserApp::OnContextInitialized()
{
	if (shm_name.empty())
		return;

	CefWindowInfo info;
	info.width = width;
	info.height = height;
	info.windowless_rendering_enabled = true;

	CefBrowserSettings settings;
	settings.windowless_frame_rate = fps;

	CefRefPtr<BrowserClient> client{new BrowserClient(data, css)};
	this->client = client;

	browser = CefBrowserHost::CreateBrowserSync(
	    info, client.get(), "https://github.com/bazukas/obs-linuxbrowser/", settings, nullptr);
	client->SetScroll(browser, 0, 0); // workaround for scroll to bottom bug

	messageThread = std::thread{[this] { this->MessageThreadWorker(); }};
}

void BrowserApp::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefV8Context> context)
{
	CefRefPtr<CefV8Value> globalObj = context->GetGlobal();

	CefRefPtr<CefV8Value> obsStudioObj = CefV8Value::CreateObject(0, 0);
	globalObj->SetValue("obsstudio", obsStudioObj, V8_PROPERTY_ATTRIBUTE_NONE);

	obsStudioObj->SetValue("linuxbrowser", CefV8Value::CreateBool(true),
	                       V8_PROPERTY_ATTRIBUTE_NONE);

	obsStudioObj->SetValue("pluginVersion", CefV8Value::CreateString(LINUXBROWSER_VERSION),
	                       V8_PROPERTY_ATTRIBUTE_NONE);
}

bool BrowserApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                          CefProcessId source_process,
                                          CefRefPtr<CefProcessMessage> message)
{
	DCHECK(source_process == PID_BROWSER);

	CefRefPtr<CefListValue> args = message->GetArgumentList();

	if (message->GetName() == "Visibility") {
		CefV8ValueList arguments{CefV8Value::CreateBool(args->GetBool(0))};
		ExecuteJSFunction(browser, "onVisiblityChange", arguments);
	} else if (message->GetName() == "Active") {
		CefV8ValueList arguments{CefV8Value::CreateBool(args->GetBool(0))};
		ExecuteJSFunction(browser, "onActiveChange", arguments);
	} else {
		return false;
	}

	return true;
}

void BrowserApp::ExecuteJSFunction(CefRefPtr<CefBrowser> browser, const char* functionName,
                                   CefV8ValueList arguments)
{
	CefRefPtr<CefV8Context> context = browser->GetMainFrame()->GetV8Context();

	context->Enter();

	CefRefPtr<CefV8Value> globalObj = context->GetGlobal();
	CefRefPtr<CefV8Value> obsStudioObj = globalObj->GetValue("obsstudio");
	CefRefPtr<CefV8Value> jsFunction = obsStudioObj->GetValue(functionName);

	if (jsFunction && jsFunction->IsFunction()) {
		jsFunction->ExecuteFunction(nullptr, arguments);
	}

	context->Exit();
}

bool BrowserApp::Execute(const CefString& name, CefRefPtr<CefV8Value> object,
                         const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval,
                         CefString& exception)
{
	return true;
}

void BrowserApp::UpdateActiveStateJS(bool active)
{
	CefRefPtr<CefProcessMessage> msg{CefProcessMessage::Create("Active")};
	msg->GetArgumentList()->SetBool(0, active);
	this->browser->SendProcessMessage(PID_BROWSER, msg);
}

void BrowserApp::UpdateVisibilityStateJS(bool visible)
{
	CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("Visibility");
	CefRefPtr<CefListValue> args = msg->GetArgumentList();
	args->SetBool(0, visible);
	this->browser->SendProcessMessage(PID_BROWSER, msg);
}
