# About

This is a browser source plugin for obs-studio (https://github.com/jp9000/obs-studio) based
on Chromium Embedded Framework (https://bitbucket.org/chromiumembedded/cef). This plugin is Linux only.

Unfortunately, I was not able to make obs-browser (https://github.com/kc5nra/obs-browser) work on Linux,
so I decided to create a separate plugin using the same engine, so both plugins should have feature parity in
terms of browser capabilities.

![Browser window](img/obs-linuxbrowser.png)

# Installing

* Download the latest release from [releases page](https://github.com/bazukas/obs-linuxbrowser/releases)
* Extract contents of the archive into `$HOME/.config/obs-studio/plugins`

# Building

## Building CEF

* Download CEF standard binary release from http://opensource.spotify.com/cefbuilds/index.html
* Extract and cd into folder
* Run `cmake ./ && make`

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
Some distributions provide packages with the plugin, or you can extract one from google chrome installation (you can easily find out location and version by opening chrome://flash/).
Flash version can be found in manifest.json that is usually found in same directory as .so file.

# Known issues

## No mp3 support

Mp3 support in CEF binary releases is turned off due to legal reasons. If you want to turn it on, you would
have to build CEF from scratch yourself.

## No sound in Streamlabs Alertbox

In order for sound to work in the alerts, you will have to enable either mp3 support or flash (even if you are
playing .ogg file).
