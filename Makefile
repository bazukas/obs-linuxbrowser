ifndef OBS_INCLUDE
OBS_INCLUDE = /usr/include/obs
endif
ifndef OBS_LIB
OBS_LIB = /usr/lib
endif
ifndef CEF_DIR
CEF_DIR = cef
endif
CEF_INCLUDE = $(CEF_DIR)
CEF_RELEASE = $(CEF_DIR)/Release
CEF_RESOURCES = $(CEF_DIR)/Resources
CEF_STATIC  = $(CEF_DIR)/libcef_dll_wrapper/libcef_dll_wrapper.a

CFLAGS     = -Wall -g -fPIC -I$(OBS_INCLUDE) -I./src
CXXFLAGS   = -std=c++11 -Wall -g -fPIC -I$(CEF_INCLUDE) -I./src
CC         = gcc
CXX        = g++
RM         = /bin/rm -rf
LDFLAGS    = -L$(OBS_LIB)
LDLIBS     = -lobs -lrt
LDFLAGS_BROWSER = -L$(CEF_RELEASE)
LDLIBS_BROWSER  = -lcef -lrt -lpthread

PLUGIN = build/plugin/obs-linuxbrowser.so
PLUGIN_OBJ = build/plugin/main.o build/plugin/manager.o

BROWSER = build/browser/browser
BROWSER_SUB = build/browser/browser-subprocess
BROWSER_SHARED_OBJ = build/browser/browser-app.o build/browser/browser-client.o $(CEF_STATIC)
BROWSER_OBJ = build/browser/browser.o $(BROWSER_SHARED_OBJ)
BROWSER_SUB_OBJ = build/browser/browser-subprocess.o $(BROWSER_SHARED_OBJ)

PLUGIN_BUILD_DIR = build/obs-linuxbrowser
PLUGIN_INSTALL_DIR = ~/.config/obs-studio/plugins
PLUGIN_DATA_DIR = data

ARCH = $(shell getconf LONG_BIT)
PLUGIN_BIN_DIR = $(PLUGIN_BUILD_DIR)/bin/$(ARCH)bit

PLUGIN_CEF_DATA_DIR = $(PLUGIN_BUILD_DIR)/$(PLUGIN_DATA_DIR)/cef

all: plugin

.PHONY: plugin
plugin: $(PLUGIN) $(BROWSER) $(BROWSER_SUB)
	mkdir -p $(PLUGIN_BIN_DIR)
	cp $(PLUGIN) $(PLUGIN_BIN_DIR)
	cp $(BROWSER) $(PLUGIN_BIN_DIR)
	cp $(BROWSER_SUB) $(PLUGIN_BIN_DIR)
	cp -r $(PLUGIN_DATA_DIR) $(PLUGIN_BUILD_DIR)
	cp $(CEF_RELEASE)/libcef.so $(PLUGIN_BIN_DIR)
	cp $(CEF_RELEASE)/natives_blob.bin $(PLUGIN_BIN_DIR)
	cp $(CEF_RELEASE)/snapshot_blob.bin $(PLUGIN_BIN_DIR)
	cp $(CEF_RESOURCES)/icudtl.dat $(PLUGIN_BIN_DIR)
	mkdir -p $(PLUGIN_CEF_DATA_DIR)
	cp $(CEF_RESOURCES)/cef.pak $(PLUGIN_CEF_DATA_DIR)
	cp $(CEF_RESOURCES)/cef_100_percent.pak $(PLUGIN_CEF_DATA_DIR)
	cp $(CEF_RESOURCES)/cef_200_percent.pak $(PLUGIN_CEF_DATA_DIR)
	cp $(CEF_RESOURCES)/cef_extensions.pak $(PLUGIN_CEF_DATA_DIR)
	cp -r $(CEF_RESOURCES)/locales $(PLUGIN_CEF_DATA_DIR)

.PHONY: install
install:
	mkdir -p $(PLUGIN_INSTALL_DIR)
	cp -r $(PLUGIN_BUILD_DIR) $(PLUGIN_INSTALL_DIR)

$(PLUGIN): $(PLUGIN_OBJ)
	$(CC) -shared $(LDFLAGS) $^ $(LDLIBS) -o $@

build/plugin/%.o: src/plugin/%.c
	mkdir -p build/plugin
	$(CC) -c $(CFLAGS) $< -o $@

$(BROWSER): $(BROWSER_OBJ)
	$(CXX) $(LDFLAGS_BROWSER) $^ $(LDLIBS_BROWSER) -o $@

$(BROWSER_SUB): $(BROWSER_SUB_OBJ)
	$(CXX) $(LDFLAGS_BROWSER) $^ $(LDLIBS_BROWSER) -o $@

build/browser/%.o: src/browser/%.cpp
	mkdir -p build/browser
	$(CXX) -c $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	$(RM) build/*
