/*
  Copyright 2008 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <cstdlib>
#include <locale.h>
#include <signal.h>
#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QMenu>
#include <QtGui/QWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

#include <ggadget/extension_manager.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/host_interface.h>
#include <ggadget/host_utils.h>
#include <ggadget/messages.h>
#include <ggadget/qt/qt_view_widget.h>
#include <ggadget/qt/qt_view_host.h>
#include <ggadget/qt/qt_menu.h>
#include <ggadget/qt/qt_main_loop.h>
#include <ggadget/run_once.h>
#include <ggadget/script_runtime_interface.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/system_utils.h>
#include <ggadget/xml_http_request_interface.h>

#include "qt_host.h"
#if defined(Q_WS_X11) && defined(HAVE_X11)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#endif

static ggadget::qt::QtMainLoop *g_main_loop;
static const char kRunOnceSocketName[] = "ggl-host-socket";

static const char *kGlobalExtensions[] = {
  "default-framework",
  "libxml2-xml-parser",
  "default-options",
  "dbus-script-class",
  "qtwebkit-browser-element",
  "qt-system-framework",
  "qt-edit-element",
// gst and Qt may not work together.
//  "gst-audio-framework",
  "gst-video-element",
#ifdef GGL_HOST_LINUX
  "linux-system-framework",
#endif
  "qt-xml-http-request",
  "google-gadget-manager",
  NULL
};

static const char *g_help_string =
  "Google Gadgets for Linux " GGL_VERSION "\n"
  "Usage: " GGL_APP_NAME " [Options] [Gadgets]\n"
  "Options:\n"
#ifdef _DEBUG
  "  -d mode, --debug mode\n"
  "      Specify debug modes for drawing View:\n"
  "      0 - No debug.\n"
  "      1 - Draw bounding boxes around container elements.\n"
  "      2 - Draw bounding boxes around all elements.\n"
  "      4 - Draw bounding boxes around clip region.\n"
#endif
#if QT_VERSION >= 0x040400
  "  -s script_runtime, --script-runtime script_runtime\n"
  "      Specify which script runtime to use\n"
  "      smjs - spidermonkey js runtime\n"
  "      qt   - QtScript js runtime(experimental)\n"
#endif
  "  -l loglevel, --log-level loglevel\n"
  "      Specify the minimum gadget.debug log level.\n"
  "      0 - Trace(All)  1 - Info  2 - Warning  3 - Error  >=4 - No log\n"
  "  -ll, --long-log\n"
  "      Output logs using long format.\n"
  "  -dc, --debug-console debug_console_config\n"
  "      Change debug console configuration:\n"
  "      0 - No debug console allowed\n"
  "      1 - Gadgets has debug console menu item\n"
  "      2 - Open debug console when gadget is added to debug startup code\n"
  "  -p, --plasma\n"
  "      Install gadget into KDE4's plasma\n"
  "  -h, --help\n"
  "      Print this message and exit.\n"
  "\n"
  "Gadgets:\n"
  "  Can specify one or more Desktop Gadget paths.\n"
  "  If any gadgets are specified, they will be installed by using\n"
  "  GadgetManager.\n";

#if defined(Q_WS_X11) && defined(HAVE_X11)
static Display *dpy;
static Colormap colormap = 0;
static Visual *visual = 0;
static bool InitArgb() {
  dpy = XOpenDisplay(0); // open default display
  if (!dpy) {
    qWarning("Cannot connect to the X server");
    exit(1);
  }

  int screen = DefaultScreen(dpy);
  int eventBase, errorBase;

  if (XRenderQueryExtension(dpy, &eventBase, &errorBase)) {
    int nvi;
    XVisualInfo templ;
    templ.screen  = screen;
    templ.depth   = 32;
    templ.c_class = TrueColor;
    XVisualInfo *xvi = XGetVisualInfo(dpy, VisualScreenMask |
                                      VisualDepthMask |
                                      VisualClassMask, &templ, &nvi);

    for (int i = 0; i < nvi; ++i) {
      XRenderPictFormat *format = XRenderFindVisualFormat(dpy,
                                                          xvi[i].visual);
      if (format->type == PictTypeDirect && format->direct.alphaMask) {
        visual = xvi[i].visual;
        colormap = XCreateColormap(dpy, RootWindow(dpy, screen),
                                   visual, AllocNone);
        return true;
      }
    }
  }
  return false;
}
static bool CheckCompositingManager(Display *dpy) {
  Atom net_wm_state = XInternAtom(dpy, "_NET_WM_CM_S0", False);
  return XGetSelectionOwner(dpy, net_wm_state);
}
#endif

static void OnClientMessage(const std::string &data) {
  ggadget::GetGadgetManager()->NewGadgetInstanceFromFile(data.c_str());
}

static void DefaultSignalHandler(int sig) {
  DLOG("Signal caught: %d, exit.", sig);
  g_main_loop->Quit();
}

int main(int argc, char* argv[]) {
#ifdef _DEBUG
  int log_level = ggadget::LOG_TRACE;
  bool long_log = true;
#else
  int log_level = ggadget::LOG_WARNING;
  bool long_log = false;
#endif
  bool composite = false;
  bool with_plasma = false;
  int debug_mode = 0;
  ggadget::Gadget::DebugConsoleConfig debug_console =
      ggadget::Gadget::DEBUG_CONSOLE_DISABLED;
  const char* js_runtime = "smjs-script-runtime";
  // set locale according to env vars
  setlocale(LC_ALL, "");

#if defined(Q_WS_X11) && defined(HAVE_X11)
  if (InitArgb() && CheckCompositingManager(dpy)) {
    composite = true;
  } else {
    visual = NULL;
    if (colormap) {
      XFreeColormap(dpy, colormap);
      colormap = 0;
    }
  }
  QApplication app(dpy, argc, argv,
                   Qt::HANDLE(visual), Qt::HANDLE(colormap));
#else
  QApplication app(argc, argv);
#endif

  // Set global main loop
  g_main_loop = new ggadget::qt::QtMainLoop();
  ggadget::SetGlobalMainLoop(g_main_loop);

  std::string profile_dir =
      ggadget::BuildFilePath(ggadget::GetHomeDirectory().c_str(),
                             ggadget::kDefaultProfileDirectory, NULL);

  ggadget::EnsureDirectories(profile_dir.c_str());

  ggadget::RunOnce run_once(
      ggadget::BuildFilePath(profile_dir.c_str(),
                             kRunOnceSocketName,
                             NULL).c_str());
  run_once.ConnectOnMessage(ggadget::NewSlot(OnClientMessage));

  // Parse command line.
  std::vector<std::string> gadget_paths;
  for (int i = 1; i < argc; i++) {
    if (strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
      printf("%s", g_help_string);
      return 0;
#ifdef _DEBUG
    } else if (strcmp("-d", argv[i]) == 0 || strcmp("--debug", argv[i]) == 0) {
      if (++i < argc) {
        debug_mode = atoi(argv[i]);
      } else {
        debug_mode = 1;
      }
#endif
#if QT_VERSION >= 0x040400
    } else if (strcmp("-s", argv[i]) == 0 ||
               strcmp("--script-runtime", argv[i]) == 0) {
      if (++i < argc) {
        if (strcmp(argv[i], "qt") == 0) {
          js_runtime = "qt-script-runtime";
          printf("QtScript runtime is chosen. It's still incomplete\n");
        }
      }
#endif
    } else if (strcmp("-p", argv[i]) == 0 ||
               strcmp("--plasma", argv[i]) == 0) {
      with_plasma = true;
    } else if (strcmp("-l", argv[i]) == 0 ||
               strcmp("--log-level", argv[i]) == 0) {
      if (++i < argc)
        log_level = atoi(argv[i]);
    } else if (strcmp("-ll", argv[i]) == 0 ||
               strcmp("--long-log", argv[i]) == 0) {
      long_log = true;
    } else if (strcmp("-dc", argv[i]) == 0 ||
               strcmp("--debug-console", argv[i]) == 0) {
      debug_console = ggadget::Gadget::DEBUG_CONSOLE_ON_DEMMAND;
      if (++i < argc) {
        debug_console =
            static_cast<ggadget::Gadget::DebugConsoleConfig>(atoi(argv[i]));
      }
   } else {
      std::string path = ggadget::GetAbsolutePath(argv[i]);
      if (run_once.IsRunning()) {
        run_once.SendMessage(path);
      } else {
        gadget_paths.push_back(path);
      }
    }
  }

  if (run_once.IsRunning()) {
    DLOG("Another instance already exists.");
    exit(0);
  }

  ggadget::SetupLogger(log_level, long_log);

  // Set global file manager.
  ggadget::SetupGlobalFileManager(profile_dir.c_str());

  // Load global extensions.
  ggadget::ExtensionManager *ext_manager =
      ggadget::ExtensionManager::CreateExtensionManager();
  ggadget::ExtensionManager::SetGlobalExtensionManager(ext_manager);

  // Ignore errors when loading extensions.
  for (size_t i = 0; kGlobalExtensions[i]; ++i)
    ext_manager->LoadExtension(kGlobalExtensions[i], false);
  ext_manager->LoadExtension(js_runtime, false);

  // Register JavaScript runtime.
  ggadget::ScriptRuntimeManager *manager = ggadget::ScriptRuntimeManager::get();
  ggadget::ScriptRuntimeExtensionRegister script_runtime_register(manager);
  ext_manager->RegisterLoadedExtensions(&script_runtime_register);

  std::string error;
  if (!ggadget::CheckRequiredExtensions(&error)) {
    // Don't use _GM here because localized messages may be unavailable.
    QMessageBox::information(NULL, "Google Gadgets",
                             QString::fromUtf8(error.c_str()));

    return 1;
  }

  ext_manager->SetReadonly();
  ggadget::InitXHRUserAgent(GGL_APP_NAME);
  // Initialize the gadget manager before creating the host.
  ggadget::GadgetManagerInterface *manager = ggadget::GetGadgetManager();
  manager->Init();

  hosts::qt::QtHost host = hosts::qt::QtHost(composite, debug_mode,
                                             debug_console, with_plasma);

  // Load gadget files.
  if (gadget_paths.size()) {
    for (size_t i = 0; i < gadget_paths.size(); ++i)
      manager->NewGadgetInstanceFromFile(gadget_paths[i].c_str());
  }

  // Hook popular signals to exit gracefully.
  signal(SIGHUP, DefaultSignalHandler);
  signal(SIGINT, DefaultSignalHandler);
  signal(SIGTERM, DefaultSignalHandler);
  signal(SIGUSR1, DefaultSignalHandler);
  signal(SIGUSR2, DefaultSignalHandler);

  g_main_loop->Run();

  return 0;
}
