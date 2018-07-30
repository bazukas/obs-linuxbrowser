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
#include <cef_app.h>

#include "browser-app.hpp"

int main(int argc, char* argv[])
{
	CefMainArgs main_args(argc, argv);
	CefRefPtr<BrowserApp> app{new BrowserApp(nullptr)};
	return CefExecuteProcess(main_args, app.get(), nullptr);
}
