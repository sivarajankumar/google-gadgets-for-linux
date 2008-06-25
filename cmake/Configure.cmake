#
# Copyright 2008 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

SET(PRODUCT_NAME libggadget)

PROJECT(google_gadgets_for_linux)

SET(LIBGGADGET_BINARY_VERSION \"1.0.0\")
SET(GGL_MAJOR_VERSION 0)
SET(GGL_MINOR_VERSION 9)
SET(GGL_MICRO_VERSION 3)
SET(GGL_VERSION \"${GGL_MAJOR_VERSION}.${GGL_MINOR_VERSION}.${GGL_MICRO_VERSION}\")

# This string is used in auto update request. It should be updated to the
# time of a release build is made. Its format is yymmdd-HHMMSS.
SET(GGL_VERSION_TIMESTAMP \"080613-000000\")

# Define the version of Google Desktop Gadget API that this release supports.
SET(GGL_API_MAJOR_VERSION 5)
SET(GGL_API_MINOR_VERSION 7)
SET(GGL_API_VERSION \"${GGL_API_MAJOR_VERSION}.${GGL_API_MINOR_VERSION}.0.0\")

SET(GGL_PLATFORM_SHORT \"linux\")
SET(GGL_PLATFORM \"linux\")

INCLUDE(CheckFunctionExists)
INCLUDE(FindPkgConfig)

INCLUDE(CheckTypeSize)
SET(HAVE_STDDEF_H 1)
CHECK_TYPE_SIZE(int GGL_SIZEOF_INT)
CHECK_TYPE_SIZE(long GGL_SIZEOF_LONG_INT)
CHECK_TYPE_SIZE(size_t GGL_SIZEOF_SIZE_T)
CHECK_TYPE_SIZE(double GGL_SIZEOF_DOUBLE)

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/ggadget/sysdeps.h.in
               ${CMAKE_BINARY_DIR}/ggadget/sysdeps.h)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"
    AND EXISTS ${CMAKE_SOURCE_DIR}/CTestConfig.cmake)
  INCLUDE(Dart)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"
    AND EXISTS ${CMAKE_SOURCE_DIR}/CTestConfig.cmake)

INCLUDE(CopyUtils)
INCLUDE(GenerateOutput)
INCLUDE(PkgConfigEx)
INCLUDE(TestSuite)
INCLUDE(ZipUtils)

ADD_DEFINITIONS(
  -DUNIX
  -DPREFIX=${CMAKE_INSTALL_PREFIX}
  -DPRODUCT_NAME=${PRODUCT_NAME}
  # For stdint.h macros like INT64_C etc.
  -D__STDC_CONSTANT_MACROS
  # TODO: only for Linux by now
  -DGGL_HOST_LINUX
  -DGGL_MODULE_DIR=\\\"../modules\\\"
  # TODO:
  -DGGL_RESOURCE_DIR=\\\".\\\"
  # TODO:
  -DGGL_LIBEXEC_DIR=\\\".\\\")

IF(UNIX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Werror -Wconversion")
  # No "-Wall -Werror" for C flags, to avoid third_party code break.
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  SET(PROJECT_RESOURCE_DIR share/${PRODUCT_NAME})
ELSE(UNIX)
  SET(PROJECT_RESOURCE_DIR resource)
ENDIF(UNIX)
# SET(CMAKE_SKIP_RPATH ON)
ENABLE_TESTING()

IF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  ADD_DEFINITIONS(-D_DEBUG)
ELSE("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  ADD_DEFINITIONS(-DNDEBUG)
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})

SET(GGL_BUILD_GTK_HOST 1)
SET(GGL_BUILD_QT_HOST 1)
SET(GGL_BUILD_LIBGGADGET_DBUS 1)
SET(GGL_BUILD_LIBGGADGET_GTK 1)
SET(GGL_BUILD_LIBGGADGET_QT 1)
SET(GGL_BUILD_QTWEBKIT_BROWSER_ELEMENT 1)
SET(GGL_BUILD_GTKMOZ_BROWSER_ELEMENT 1)
SET(GGL_BUILD_GST_AUDIO_FRAMEWORK 1)
SET(GGL_BUILD_GST_MEDIAPLAYER_ELEMENT 1)
SET(GGL_BUILD_SMJS_SCRIPT_RUNTIME 1)
SET(GGL_BUILD_CURL_XML_HTTP_REQUEST 1)
SET(GGL_BUILD_LIBXML2_XML_PARSER 1)
SET(GGL_BUILD_LINUX_SYSTEM_FRAMEWORK 1)

# Check necessary libraries.

FIND_PACKAGE(ZLIB REQUIRED)

FIND_PACKAGE(Threads)
IF(CMAKE_USE_PTHREADS_INIT)
  SET(PTHREAD_FOUND 1)
  SET(PTHREAD_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
  ADD_DEFINITIONS(-DHAVE_PTHREAD=1)
ENDIF(CMAKE_USE_PTHREADS_INIT)

FIND_PACKAGE(X11)
IF(X11_FOUND)
  ADD_DEFINITIONS(-DHAVE_X11=1)
ENDIF(X11_FOUND)

GET_CONFIG(libxml-2.0 2.6.0 LIBXML2 LIBXML2_FOUND)
IF(NOT LIBXML2_FOUND AND GGL_BUILD_LIBXML2_XML_PARSER)
  MESSAGE("Library libxml2 is not available, libxml2-xml-parser extension won't be built.")
  SET(GGL_BUILD_LIBXML2_XML_PARSER 0)
ENDIF(NOT LIBXML2_FOUND AND GGL_BUILD_LIBXML2_XML_PARSER)

GET_CONFIG(libcurl 7.15 LIBCURL LIBCURL_FOUND)
IF(NOT LIBCURL_FOUND)
  IF(GGL_BUILD_CURL_XML_HTTP_REQUEST)
    MESSAGE("Library curl is not available, curl-xml-http-request extension won't be built.")
    SET(GGL_BUILD_CURL_XML_HTTP_REQUEST 0)
  ENDIF(GGL_BUILD_CURL_XML_HTTP_REQUEST)
ENDIF(NOT LIBCURL_FOUND)

GET_CONFIG(gstreamer-0.10 0.10.0 GSTREAMER GSTREAMER_FOUND)
GET_CONFIG(gstreamer-plugins-base-0.10 0.10.0 GSTREAMER_PLUGINS_BASE GSTREAMER_PLUGINS_BASE_FOUND)
IF(GSTREAMER_FOUND AND GSTREAMER_PLUGINS_BASE_FOUND)
  FIND_LIBRARY(VIDEOSINK_LIBRARY NAMES gstvideo-0.10 PATH /usr/lib /usr/lib/gstreamer-0.10 /usr/local/lib /usr/local/lib/gstreamer-0.10)
  IF(VIDEOSINK_LIBRARY)
    SET(GSTREAMER_MEDIAPLAYER_LIBRARIES "${GSTREAMER_LIBRARIES}  -lgstvideo-0.10")
  ELSE(VIDEOSINK_LIBRARY)
    MESSAGE("Libgstreamer-plugins-base-dev is not available, gst-mediaplayer-element extension won't be built.")
    SET(GGL_BUILD_GST_MEDIAPLAYER_ELEMENT 0)
  ENDIF(VIDEOSINK_LIBRARY)
ELSE(GSTREAMER_FOUND AND GSTREAMER_PLUGINS_BASE_FOUND)
  IF(GGL_BUILD_GST_AUDIO_FRAMEWORK)
    MESSAGE("Library gstreamer-plugins-base(>=0.10.0) is not available, gst-audio-framework extension won't be built.")
    SET(GGL_BUILD_GST_AUDIO_FRAMEWORK 0)
  ENDIF(GGL_BUILD_GST_AUDIO_FRAMEWORK)
  IF(GGL_BUILD_GST_MEDIAPLAYER_ELEMENT)
    MESSAGE("Library gstreamer-plugins-base(>=0.10.0) is not available, gst-mediaplayer-element extension won't be built.")
    SET(GGL_BUILD_GST_MEDIAPLAYER_ELEMENT 0)
  ENDIF(GGL_BUILD_GST_MEDIAPLAYER_ELEMENT)
ENDIF(GSTREAMER_FOUND AND GSTREAMER_PLUGINS_BASE_FOUND)

GET_CONFIG(pango 1.10.0 PANGO PANGO_FOUND)
GET_CONFIG(cairo 1.0.0 CAIRO CAIRO_FOUND)
GET_CONFIG(gdk-2.0 2.8.0 GDK2 GDK2_FOUND)
GET_CONFIG(gtk+-2.0 2.8.0 GTK2 GTK2_FOUND)
GET_CONFIG(gthread-2.0 2.8.0 GTHREAD GTHREAD_FOUND)
IF(NOT PANGO_FOUND OR NOT CAIRO_FOUND OR NOT GDK2_FOUND OR NOT GTK2_FOUND OR NOT GTHREAD_FOUND)
  SET(GGL_BUILD_LIBGGADGET_GTK 0)
  SET(GGL_BUILD_GTK_HOST 0)
  MESSAGE("Library cairo, pando or gtk-2.0 are not available, gtk-system-framework extension, libggadget-gtk, gtk host won't be built.")
ENDIF(NOT PANGO_FOUND OR NOT CAIRO_FOUND OR NOT GDK2_FOUND OR NOT GTK2_FOUND OR NOT GTHREAD_FOUND)

GET_CONFIG(librsvg-2.0 2.14 RSVG RSVG_FOUND)

SET(QT_MIN_VERSION 4.3.0)
SET(QT_USE_QTWEBKIT 1)
SET(QT_USE_QTNETWORK 1)
FIND_PACKAGE(Qt4)
IF(QT4_FOUND AND QT_QTCORE_FOUND AND QT_QTGUI_FOUND)
  INCLUDE(${QT_USE_FILE})
  SET(QT_LINK_DIR ${QT_LIBRARY_DIR})
  SET(QT_LIBRARIES ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY})

  PKG_CHECK_MODULES(QT_QTSCRIPT QtScript>=4.4.0)
  IF(QT_QTSCRIPT_FOUND)
    LIST(APPEND QT_LIBRARIES ${QT_QTSCRIPT_LIBRARIES})
    SET(GGL_BUILD_QT_SCRIPT_RUNTIME 1)
  ENDIF(QT_QTSCRIPT_FOUND)

  IF(NOT QT_QTWEBKIT_FOUND)
    PKG_CHECK_MODULES(QT_QTWEBKIT QtWebKit)
    IF(QT_QTWEBKIT_FOUND)
      SET(QT_QTWEBKIT_LIBRARY ${QT_QTWEBKIT_LIBRARIES})
    ENDIF(QT_QTWEBKIT_FOUND)
  ENDIF(NOT QT_QTWEBKIT_FOUND)

  IF(QT_QTWEBKIT_FOUND)
    LIST(APPEND QT_LIBRARIES ${QT_QTWEBKIT_LIBRARIES})
    INCLUDE_DIRECTORIES(${QT_QTWEBKIT_INCLUDE_DIRS})
    SET(GGL_BUILD_QTWEBKIT_BROWSER_ELEMENT 1)
  ELSE(QT_QTWEBKIT_FOUND)
    SET(GGL_BUILD_QTWEBKIT_BROWSER_ELEMENT 0)
    MESSAGE("QtWebKit is needed to build qtwebkit_browser_element")
  ENDIF(QT_QTWEBKIT_FOUND)
ELSE(QT4_FOUND AND QT_QTCORE_FOUND AND QT_QTGUI_FOUND)
  SET(GGL_BUILD_LIBGGADGET_QT 0)
  SET(GGL_BUILD_QT_HOST 0)
  SET(GGL_BUILD_QTWEBKIT_BROWSER_ELEMENT 0)
  MESSAGE("Library qt-4.3 or above is not available, libggadget-qt, qt host and qt related extensions won't be built.")
ENDIF(QT4_FOUND AND QT_QTCORE_FOUND AND QT_QTGUI_FOUND)

GET_CONFIG(dbus-1 1.0 DBUS DBUS_FOUND)
IF(NOT DBUS_FOUND AND GGL_BUILD_LIBGGADGET_DBUS)
  SET(GGL_BUILD_LIBGGADGET_DBUS 0)
  MESSAGE("Library D-Bus is not available, libggadget-dbus won't be built.")
ELSE(NOT DBUS_FOUND AND GGL_BUILD_LIBGGADGET_DBUS)
  SET(CMAKE_REQUIRED_INCLUDES ${DBUS_INCLUDE_DIR})
  SET(CMAKE_REQUIRED_LIBRARIES ${DBUS_LIBRARIES})
  CHECK_FUNCTION_EXISTS(dbus_watch_get_unix_fd DBUS_UNIX_FD_FUNC_FOUND)
  IF(DBUS_UNIX_FD_FUNC_FOUND)
    LIST(APPEND DBUS_DEFINITIONS "-DHAVE_DBUS_WATCH_GET_UNIX_FD")
  ENDIF(DBUS_UNIX_FD_FUNC_FOUND)
ENDIF(NOT DBUS_FOUND AND GGL_BUILD_LIBGGADGET_DBUS)

INCLUDE(SpiderMonkey)
IF(NOT SMJS_FOUND AND GGL_BUILD_SMJS_SCRIPT_RUNTIME)
  SET(GGL_BUILD_SMJS_SCRIPT_RUNTIME 0)
  MESSAGE("Library SpiderMonkey is not available, smjs-script-runtime extension won't be built.")
ENDIF(NOT SMJS_FOUND AND GGL_BUILD_SMJS_SCRIPT_RUNTIME)

IF(GGL_BUILD_LIBGGADGET_GTK AND SMJS_FOUND)
  GET_CONFIG(xulrunner-gtkmozembed 1.8 GTKMOZEMBED GTKMOZEMBED_FOUND)
  IF(NOT GTKMOZEMBED_FOUND)
    GET_CONFIG(firefox2-gtkmozembed 2.0 GTKMOZEMBED GTKMOZEMBED_FOUND)
    IF(NOT GTKMOZEMBED_FOUND)
      GET_CONFIG(firefox-gtkmozembed 1.5 GTKMOZEMBED GTKMOZEMBED_FOUND)
      IF(NOT GTKMOZEMBED_FOUND)
        GET_CONFIG(mozilla-gtkmozembed 1.8 GTKMOZEMBED GTKMOZEMBED_FOUND)
      ENDIF(NOT GTKMOZEMBED_FOUND)
    ENDIF(NOT GTKMOZEMBED_FOUND)
  ENDIF(NOT GTKMOZEMBED_FOUND)
  IF(NOT GTKMOZEMBED_FOUND AND GGL_BUILD_GTKMOZ_BROWSER_ELEMENT)
    SET(GGL_BUILD_GTKMOZ_BROWSER_ELEMENT 0)
    MESSAGE("Library GtkMozEmbed is not available, gtkmoz-browser-element extension won't be built.")
  ENDIF(NOT GTKMOZEMBED_FOUND AND GGL_BUILD_GTKMOZ_BROWSER_ELEMENT)
ELSE(GGL_BUILD_LIBGGADGET_GTK AND SMJS_FOUND)
  SET(GGL_BUILD_GTKMOZ_BROWSER_ELEMENT 0)
  MESSAGE("Library gtk-2.0 or SpiderMonkey is not available, gtkmoz-browser-element extension won't be built.")
ENDIF(GGL_BUILD_LIBGGADGET_GTK AND SMJS_FOUND)

MESSAGE("
Build options:
  Version                          ${GGL_VERSION}
  Build type                       ${CMAKE_BUILD_TYPE}

 Libraries:
  GTK SVG Support                  ${RSVG_FOUND}
  Build libggadget-gtk             ${GGL_BUILD_LIBGGADGET_GTK}
  Build libggadget-qt              ${GGL_BUILD_LIBGGADGET_QT}
  Build libggadget-dbus            ${GGL_BUILD_LIBGGADGET_DBUS}

 Extensions:
  Build dbus-script-class          ${GGL_BUILD_LIBGGADGET_DBUS}
  Build gtkmoz-browser-element     ${GGL_BUILD_GTKMOZ_BROWSER_ELEMENT}
  Build gst-audio-framework        ${GGL_BUILD_GST_AUDIO_FRAMEWORK}
  Build gst-mediaplayer-element    ${GGL_BUILD_GST_MEDIAPLAYER_ELEMENT}
  Build linux-system-framework     ${GGL_BUILD_LINUX_SYSTEM_FRAMEWORK}
  Build smjs-script-runtime        ${GGL_BUILD_SMJS_SCRIPT_RUNTIME}
  Build curl-xml-http-request      ${GGL_BUILD_CURL_XML_HTTP_REQUEST}
  Build libxml2-xml-parser         ${GGL_BUILD_LIBXML2_XML_PARSER}
  Build gtk-edit-element           ${GGL_BUILD_LIBGGADGET_GTK}
  Build gtk-system-framework       ${GGL_BUILD_LIBGGADGET_GTK}
  Build qt-edit-element            ${GGL_BUILD_LIBGGADGET_QT}
  Build qtwebkit-browser-element   ${GGL_BUILD_QTWEBKIT_BROWSER_ELEMENT}
  Build qt-xml-http-request        ${GGL_BUILD_LIBGGADGET_QT}
  Build qt-script-runtime          ${GGL_BUILD_QT_SCRIPT_RUNTIME}
  Build qt-system-framework        ${GGL_BUILD_LIBGGADGET_QT}

 Hosts:
  Build gtk host                   ${GGL_BUILD_GTK_HOST}
  Build qt host                    ${GGL_BUILD_QT_HOST}
")
