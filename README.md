# About

This is a browser source plugin for [obs-studio](https://github.com/jp9000/obs-studio) based
on [Chromium Embedded Framework](https://bitbucket.org/chromiumembedded/cef). This plugin is Linux only.

Unfortunately, I was not able to make [kc5rna/obs-browser](https://github.com/kc5nra/obs-browser) work on Linux,
so I decided to create a separate plugin using the same engine, so both plugins should have feature parity in
terms of browser capabilities.

![Browser window](img/obs-linuxbrowser.png)

# Dependencies

* OBS-Studio
* libgconf

# Installing

* Download the latest release from [releases page](https://github.com/bazukas/obs-linuxbrowser/releases). Make sure the release version matches obs-studio version on your system [1].
* `mkdir -p $HOME/.config/obs-studio/plugins`
* `tar xfvz linuxbrowser0.4.0-obs21.1.1-64bit.tgz -C $HOME/.config/obs-studio/plugins/`
* Install the dependencies (see Dependencies section)

You don't need to build the plugin if you downloaded a binary release, instructions below are for people
who want to compile the plugin themselves.

Arch Linux users can install obs-linuxbrowser from the official AUR packages [obs-linuxbrowser](https://aur.archlinux.org/packages/obs-linuxbrowser) or [obs-linuxbrowser-bin](https://aur.archlinux.org/packages/obs-linuxbrowser-bin).

[1] Every binary release has the version number of OBS contained as part of the file name, e.g. “linuxbrowser0.4.0-obs21.1.1-64bit.tgz” refers to obs-linuxbrowser version 0.4.0 with OBS version 21.1.1.

# Building

## Building CEF

* Download CEF minimal or standard binary release from http://opensource.spotify.com/cefbuilds/index.html
* Extract and cd into folder
* Run `cmake ./ && make libcef_dll_wrapper`

## Building Plugin

Make sure you have obs-studio installed.

* `cd obs-linuxbrowser`
* `mkdir build`
* `cd build`
* `cmake -D CEF_DIR=<path to your cef dir> ..` (You might also need to set `OBS_INCLUDE` and/or `OBS_LIB` build variables to point to your OBS installation's include and library locations repectively. In most cases, this is not necessary.)
* `make`

## Installing

* Run `make install` to copy plugin binaries into $HOME/.config/obs-studio/plugins.
* Make sure to have all dependencies installed on your system

# Flash

You can enable flash by providing the path to your installed pepper flash library file and its version.
Some distributions provide packages with the plugin, but you can also extract one from google chrome installation.
The Flash version can be found in manifest.json that is usually found in same directory as .so file.

# JavaScript bindings
All bindings are children of `window.obsstudio`.

## Callbacks
* `onActiveChange(bool isActive)` – called whenever the source becomes activated or deactivated
* `onVisibilityChange(bool isVisible)` – called whenever the source is shown or hidden

## Constants
* `linuxbrowser = true` – Indicates obs-linuxbrowser is being used
* `pluginVersion` – A string containing the plugin's version

# Known issues
## Old version of OBS not starting with new version of obs-linuxbrowser installed
Using obs-linuxbrowser with an older OBS version than the one which has been used for compilation makes OBS break. Compile obs-linuxbrowser with the same OBS version you are going to use for streaming/recording or select a binary release whose OBS version matches the same version as yours.

## OBS-Linuxbrowser not displaying any content with certain versions of CEF
Some builds of CEF seem to be not working with obs-linuxbrowser.
We weren't able to figure out the exact cause of this, but we assume that it's a CEF-related issue we can't fix.
Check issue [#63](https://github.com/bazukas/obs-linuxbrowser/issues/63) for information about CEF versions that are known to be working.

## Long URLs not working
Currently, obs-linuxbrowser only supports URLs shorter than 1000 characters.
You can use a standard URL shortener (e.g. tinyurl) to shorten your URL.

Check issue [#70](https://github.com/bazukas/obs-linuxbrowser/issues/70) to track progress on this bug.

## Transparency not working correctly: Transparent white browser content appears gray on white scene background.
As stated in issue [#58](https://github.com/bazukas/obs-linuxbrowser/issues/58), there is a limitation in CEF, making it unable to detect the background of the OBS scenes.
Instead, premultiplied alpha values are used, which somehow make transparent white colors appear gray (every color on the grayscale is a bit darker than normal when using transparency).
With a black background, transparency seems to be working out quite fine.

This issue cannot be fixed.

## Browser instances always seem active
The browser instances which obs-linuxbrowser creates to show the websites, are constantly active in the background, even if the source is hidden.
This is due to the fact, that starting up the browser instance takes way longer than the average time a transition needs to open a scene. Restarting the browser every time the source gets activated wouldn't look very appealing to the viewers. That's why this feature hasn't been implemented yet, but it's work in progress. See [#48](https://github.com/bazukas/obs-linuxbrowser/issues/48).

A workaround is using the JS bindings provided by obs-linuxbrowser. They are partly compatible with those of obs-browser, so using a setup that works on obs-browser should in most cases also work with obs-linuxbrowser.
