cmake_minimum_required (VERSION 2.8)
project (obs-linuxbrowser)
set(CMAKE_BUILD_TYPE Release)

set(LINUXBROWSER_VERSION 0.4.0)

set(OBS_INCLUDE "/usr/include/obs" CACHE PATH "OBS Include Dir")
set(OBS_LIB "/usr/lib" CACHE PATH "OBS Lib Dir")
set(CEF_DIR "/opt/cef" CACHE PATH "CEF Binary Release Dir")

set(CEF_WRAPPER "${CEF_DIR}/libcef_dll_wrapper/libcef_dll_wrapper.a")
set(CEF_RELEASE "${CEF_DIR}/Release")
set(CEF_RESOURCES "${CEF_DIR}/Resources")

math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
set(PLUGIN_DIRECTORY "${CMAKE_BINARY_DIR}/build/obs-linuxbrowser")
set(PLUGIN_BIN_DIRECTORY "${PLUGIN_DIRECTORY}/bin/${BITS}bit")
set(PLUGIN_DATA_DIRECTORY "${PLUGIN_DIRECTORY}/data")
set(PLUGIN_CEF_DATA_DIRECTORY "${PLUGIN_DATA_DIRECTORY}/cef")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_BIN_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PLUGIN_BIN_DIRECTORY})

# setting up plugin build
file(COPY ${CMAKE_SOURCE_DIR}/data/locale DESTINATION ${PLUGIN_DATA_DIRECTORY})
file(COPY ${CEF_RELEASE}/libcef.so DESTINATION ${PLUGIN_BIN_DIRECTORY})
file(COPY ${CEF_RELEASE}/natives_blob.bin DESTINATION ${PLUGIN_BIN_DIRECTORY})
file(COPY ${CEF_RELEASE}/snapshot_blob.bin DESTINATION ${PLUGIN_BIN_DIRECTORY})
file(COPY ${CEF_RESOURCES}/icudtl.dat DESTINATION ${PLUGIN_BIN_DIRECTORY})
file(COPY ${CEF_RESOURCES}/cef.pak DESTINATION ${PLUGIN_CEF_DATA_DIRECTORY})
file(COPY ${CEF_RESOURCES}/cef_100_percent.pak DESTINATION ${PLUGIN_CEF_DATA_DIRECTORY})
file(COPY ${CEF_RESOURCES}/cef_200_percent.pak DESTINATION ${PLUGIN_CEF_DATA_DIRECTORY})
file(COPY ${CEF_RESOURCES}/cef_extensions.pak DESTINATION ${PLUGIN_CEF_DATA_DIRECTORY})
file(COPY ${CEF_RESOURCES}/devtools_resources.pak DESTINATION ${PLUGIN_CEF_DATA_DIRECTORY})
file(COPY ${CEF_RESOURCES}/locales DESTINATION ${PLUGIN_CEF_DATA_DIRECTORY})

# compilation
set(CMAKE_C_FLAGS "-Wall ${CMAKE_C_FLAGS} -std=gnu99")
set(CMAKE_CXX_FLAGS "-Wall ${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(src ${OBS_INCLUDE} ${CEF_DIR})
link_directories(${OBS_LIB} ${CEF_RELEASE})

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
    ${BROWSER_SHARED_SOURCES}
    src/browser/browser.cpp
)
set(BROWSER_SUBPROCESS_SOURCES
    ${BROWSER_SHARED_SOURCES}
    src/browser/browser.cpp
)

configure_file(
    ${PROJECT_SOURCE_DIR}/src/config.h.in
    ${PROJECT_SOURCE_DIR}/src/config.h
)

add_library(obs-linuxbrowser MODULE ${PLUGIN_SOURCES})
target_link_libraries(obs-linuxbrowser obs rt)
add_executable(browser ${BROWSER_SOURCES})
target_link_libraries(browser cef pthread rt ${CEF_WRAPPER} -static-libstdc++)
add_executable(browser-subprocess ${BROWSER_SUBPROCESS_SOURCES})
target_link_libraries(browser-subprocess cef pthread rt ${CEF_WRAPPER} -static-libstdc++)

# install
install(DIRECTORY ${PLUGIN_DIRECTORY} DESTINATION $ENV{HOME}/.config/obs-studio/plugins USE_SOURCE_PERMISSIONS)
