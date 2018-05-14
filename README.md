# About

This is a browser source plugin for obs-studio (https://github.com/jp9000/obs-studio) based
on Chromium Embedded Framework (https://bitbucket.org/chromiumembedded/cef). This plugin is Linux only.

Unfortunately, I was not able to make obs-browser (https://github.com/kc5nra/obs-browser) work on Linux,
so I decided to create a separate plugin using the same engine, so both plugins should have feature parity in
terms of browser capabilities.

![Browser window](img/obs-linuxbrowser.png)

# Installing

* Download the latest release from [releases page](https://github.com/bazukas/obs-linuxbrowser/releases). Make sure the release version matches obs-studio version on your system. (Currently 20.0.1 for Ubuntu).
* `mkdir -p $HOME/.config/obs-studio/plugins`
* `tar xfvz linuxbrowser0.3.1-obs20.0.1-64bit.tgz -C $HOME/.config/obs-studio/plugins/`
* Install the dependencies: libgconf-2-4 (`sudo apt-get install libgconf-2-4` in Ubuntu)

You don't need to build the plugin if you downloaded a binary release, instructions below are for people
who want to compile the plugin themselves.

Note from current maintainer: Because of lack of time and interest I don't plan on providing binary releases anymore.
Last available release works with newer OBS versions and shouldn't break. If you need functionality that has been
added since the last release, you can build the plugin using instructions below.

# Building

## Building CEF

* Download CEF standard binary release from http://opensource.spotify.com/cefbuilds/index.html
* Extract and cd into folder
* Run `cmake ./ && make libcef_dll_wrapper`

## Building Plugin

Make sure you have obs-studio installed.

* `cd obs-linuxbrowser`
* `mkdir build`
* `cd build`
* `cmake -D CEF_DIR=<path to your cef dir> ..` You might need to also set `OBS_INCLUDE` and/or `OBS_LIB`
build variables
* `make`

## Installing

* Run `make install` to copy plugin binaries into $HOME/.config/obs-studio/plugins.
* Make sure libgconf-2-4 dependency installed in your system

# Flash

You can enable flash by providing path to pepper flash library file and its version.
Some distributions provide packages with the plugin, or you can extract one from google chrome installation.
Flash version can be found in manifest.json that is usually found in same directory as .so file.
