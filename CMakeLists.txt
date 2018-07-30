cmake_minimum_required (VERSION 3.5)
project (obs-linuxbrowser LANGUAGES C CXX VERSION 0.5)

set(CMAKE_BUILD_TYPE Release CACHE STRING "CMake build type")

set(INSTALL_SYSTEMWIDE false CACHE BOOL "Install to system wide OBS directories instead of local ones")

if (NOT ${INSTALL_SYSTEMWIDE})
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}" CACHE PATH "Installation prefix")
    if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "$ENV{HOME}" CACHE PATH "Installation prefix" FORCE)
    endif()
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${obs-linuxbrowser_SOURCE_DIR}/cmake)
set(CMAKE_INCLUDE_CURRENT_DIR true)

find_package(OBS REQUIRED)
find_package(CEF REQUIRED)

math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
set(PLUGIN_DIRECTORY "${CMAKE_BINARY_DIR}/build/obs-linuxbrowser")
set(PLUGIN_BIN_DIRECTORY "${PLUGIN_DIRECTORY}/bin/${BITS}bit")
set(PLUGIN_DATA_DIRECTORY "${PLUGIN_DIRECTORY}/data")
set(PLUGIN_CEF_DATA_DIRECTORY "${PLUGIN_DATA_DIRECTORY}/cef")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_BIN_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PLUGIN_BIN_DIRECTORY})

# setting up plugin build
file(COPY ${CMAKE_SOURCE_DIR}/data/locale DESTINATION ${PLUGIN_DATA_DIRECTORY})
file(COPY ${CEF_LIBRARY_PATH}/libcef.so DESTINATION ${PLUGIN_BIN_DIRECTORY})
file(COPY ${CEF_LIBRARY_PATH}/natives_blob.bin DESTINATION ${PLUGIN_BIN_DIRECTORY})
file(COPY ${CEF_LIBRARY_PATH}/snapshot_blob.bin DESTINATION ${PLUGIN_BIN_DIRECTORY})
file(COPY ${CEF_RESOURCE_DIR}/icudtl.dat DESTINATION ${PLUGIN_BIN_DIRECTORY})
file(COPY ${CEF_RESOURCE_DIR}/cef.pak DESTINATION ${PLUGIN_CEF_DATA_DIRECTORY})
file(COPY ${CEF_RESOURCE_DIR}/cef_100_percent.pak DESTINATION ${PLUGIN_CEF_DATA_DIRECTORY})
file(COPY ${CEF_RESOURCE_DIR}/cef_200_percent.pak DESTINATION ${PLUGIN_CEF_DATA_DIRECTORY})
file(COPY ${CEF_RESOURCE_DIR}/cef_extensions.pak DESTINATION ${PLUGIN_CEF_DATA_DIRECTORY})
file(COPY ${CEF_RESOURCE_DIR}/devtools_resources.pak DESTINATION ${PLUGIN_CEF_DATA_DIRECTORY})
file(COPY ${CEF_RESOURCE_DIR}/locales DESTINATION ${PLUGIN_CEF_DATA_DIRECTORY})

# compilation
set(CMAKE_C_FLAGS "-Wall ${CMAKE_C_FLAGS} -std=gnu99")
set(CMAKE_CXX_FLAGS "-Wall ${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(src ${OBS_INCLUDE_DIR} ${CEF_INCLUDE_DIR})

set(PLUGIN_SOURCES
    src/plugin/main.c
    src/plugin/manager.c
)
set(BROWSER_SHARED_SOURCES
    src/browser/base64.cpp
    src/browser/browser-app.cpp
    src/browser/browser-client.cpp
    src/browser/split-message.cpp
)
set(BROWSER_SOURCES
    src/browser/browser.cpp
)
set(BROWSER_SUBPROCESS_SOURCES
    src/browser/browser.cpp
)

configure_file(
    ${PROJECT_SOURCE_DIR}/src/config.h.in
    ${PROJECT_SOURCE_DIR}/src/config.h
)

add_library(obs-linuxbrowser MODULE ${PLUGIN_SOURCES})
target_link_libraries(obs-linuxbrowser ${OBS_LIBRARIES} rt)
if (${INSTALL_SYSTEMWIDE}) # Failsafe for people who update linuxbrowser by just overwriting current installation without deleting it first
    # Remove 'lib' file prefix for compliance of default .so names in /usr/lib/obs-plugins
    set_target_properties(obs-linuxbrowser PROPERTIES PREFIX "")
endif()

add_library(browser_shared OBJECT ${BROWSER_SHARED_SOURCES})

add_executable(browser ${BROWSER_SOURCES} $<TARGET_OBJECTS:browser_shared>)
target_link_libraries(browser ${CEF_LIBRARIES} pthread rt -static-libstdc++)

add_executable(browser-subprocess ${BROWSER_SUBPROCESS_SOURCES} $<TARGET_OBJECTS:browser_shared>)
target_link_libraries(browser-subprocess ${CEF_LIBRARIES} pthread rt -static-libstdc++)

if (${INSTALL_SYSTEMWIDE})
    install(DIRECTORY ${PLUGIN_BIN_DIRECTORY} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/obs-plugins)
    install(DIRECTORY ${PLUGIN_DATA_DIRECTORY} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/obs/obs-plugins/obs-linuxbrowser)
else()
    install(DIRECTORY ${PLUGIN_DIRECTORY} DESTINATION ${CMAKE_INSTALL_PREFIX}/.config/obs-studio/plugins USE_SOURCE_PERMISSIONS)
endif()
