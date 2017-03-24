# About

This is a browser source plugin for obs-studio (https://github.com/jp9000/obs-studio) based
on Chromium Embedded Framework (https://bitbucket.org/chromiumembedded/cef). This plugin is Linux only.

Unfortunately, I was not able to make obs-browser (https://github.com/kc5nra/obs-browser) work on Linux,
so I decided to create a separate plugin using the same engine, so both plugins should have feature parity in
terms of browser capabilities.

![Browser window](img/obs-linuxbrowser.png)

# Building

## Building CEF

* Download CEF binary release from http://opensource.spotify.com/cefbuilds/index.html
* Extract and cd into folder
* Run `cmake ./ && make`

## Building Plugin

Make sure you have obs-studio installed. You might need to set `OBS_INCLUDE` and `OBS_LIB`
env variables (see Makefile).

* export `CEF_DIR` environment variable setting it to downloaded and built CEF release, for example
`export CEF_DIR=/opt/cef`

* cd into obs-linuxbrowser directory and run `make`

# Installing

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
