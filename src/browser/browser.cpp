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
#include <signal.h>
#include <sys/prctl.h>

#include <include/cef_app.h>

#include "browser-app.hpp"

/* first arugment is full path to the binary
 * second is shared memory id */
int main(int argc, char* argv[])
{
	/* shutdown if parent process dies */
	prctl(PR_SET_PDEATHSIG, SIGTERM);

	/* different path settings for cef */
	std::string dir(argv[0]);
	dir.resize(dir.length() - strlen("/bin/64bit/browser"));
	std::string resources_dir = dir + "/data/cef";
	std::string locales_dir = resources_dir + "/locales";
	std::string subprocess_path = std::string(argv[0]) + "-subprocess";

	CefMainArgs main_args(argc, argv);
	CefRefPtr<BrowserApp> app(new BrowserApp(argv[1]));

	CefSettings settings;
	CefString(&settings.browser_subprocess_path).FromString(subprocess_path);
	CefString(&settings.resources_dir_path).FromString(resources_dir);
	CefString(&settings.locales_dir_path).FromString(locales_dir);
	CefString(&settings.cache_path).FromString(dir);
	settings.no_sandbox = true;
	settings.windowless_rendering_enabled = true;

	CefInitialize(main_args, settings, app.get(), NULL);
	CefRunMessageLoop();
	CefShutdown();

	return 0;
}
