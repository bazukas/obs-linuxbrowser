# About

This is a browser source plugin for obs-studio (https://github.com/jp9000/obs-studio) based
on Chromium Embedded Framework (https://bitbucket.org/chromiumembedded/cef). This plugin is Linux only.

Unfortunately, I was not able to make obs-browser (https://github.com/kc5nra/obs-browser) work on Linux,
so I decided to create a separate plugin using the same engine, so both plugins should have feature parity in
terms of browser capabilities.

![Browser window](img/obs-linuxbrowser.png)

# Installing

* Download the latest release from [releases page](https://github.com/bazukas/obs-linuxbrowser/releases) (built with mp3 support)
* Extract contents of the archive into `$HOME/.config/obs-studio/plugins`

You don't need to build the plugin if you downloaded a binary release, instructions below are for people
who want to compile the plugin themselves.

# Building

## Building CEF
*Notice: public CEF releases have mp3 support turned off*

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

Run `make install` to copy plugin binaries into $HOME/.config/obs-studio/plugins.

# Flash

You can enable flash by providing path to pepper flash library file and its version.
Some distributions provide packages with the plugin, or you can extract one from google chrome installation.
Flash version can be found in manifest.json that is usually found in same directory as .so file.

# Known issues

## No html5 mp3 support

Our binary builds provided on the [releases page](https://github.com/bazukas/obs-linuxbrowser/releases)
now have mp3 support. If you build the plugin yourself, you'll have to build CEF from scratch with proprietary
codecs support enabled (takes 2-3 hours on i7 4770k with 16GB RAM).

## Streamlabs/Muxy.io widgets don't work

Unfortunately, currently those require either mp3 support or flash to be enabled in order to work properly
(even if they are playing .ogg audio - developers are aware of this issue).
Same issue might affect other similar services.
