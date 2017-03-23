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

int main(int argc, char* argv[])
{
	// shutdown if parent process dies
	prctl(PR_SET_PDEATHSIG, SIGTERM);

	char dir[2048];
	strncpy(dir, argv[0], 2048);
	dir[strlen(dir)-strlen("/bin/64bit/browser")] = 0;
	char resources_dir[2048];
	char locales_dir[2048];
	snprintf(resources_dir, 2048, "%s/data/cef", dir);
	snprintf(locales_dir, 2048, "%s/locales", resources_dir);
	char subprocess_path[2048];
	snprintf(subprocess_path, 2048, "%s-subprocess", argv[0]);

	CefMainArgs main_args(argc, argv);
	CefRefPtr<BrowserApp> app(new BrowserApp(argv[1]));

	CefSettings settings;
	CefString(&settings.browser_subprocess_path).FromASCII(subprocess_path);
	CefString(&settings.resources_dir_path).FromASCII(resources_dir);
	CefString(&settings.locales_dir_path).FromASCII(locales_dir);
	CefString(&settings.cache_path).FromASCII(dir);
	settings.no_sandbox = true;
	settings.windowless_rendering_enabled = true;

	CefInitialize(main_args, settings, app.get(), NULL);
	CefRunMessageLoop();
	CefShutdown();

	return 0;
}
