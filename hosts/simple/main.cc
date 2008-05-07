/*
  Copyright 2007 Google Inc.

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

#include <map>
#include <cstdlib>
#include <gtk/gtk.h>
#include <locale.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ggadget/script_runtime_interface.h>
#include <ggadget/gtk/single_view_host.h>
#include <ggadget/gtk/main_loop.h>
#include <ggadget/gtk/utilities.h>
#include <ggadget/extension_manager.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/ggadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/file_manager_wrapper.h>
#include <ggadget/dir_file_manager.h>
#include <ggadget/localized_file_manager.h>
#include <ggadget/host_interface.h>
#include <ggadget/string_utils.h>
#include <ggadget/logger.h>
#include <ggadget/system_utils.h>
#include <ggadget/options_interface.h>
#include <ggadget/gadget_manager_interface.h>
#include "sidebar_gtk_host.h"
#include "simple_gtk_host.h"

static ggadget::gtk::MainLoop g_main_loop;

static const char *kGlobalExtensions[] = {
// default framework must be loaded first, so that the default properties can
// be overrided.
  "default-framework",
  "libxml2-xml-parser",
  "default-options",
  // Disable DBUS script class for now to ensure security.
  // "dbus-script-class",
  "gtk-edit-element",
  "gtkmoz-browser-element",
  "gst-mediaplayer-element",
  "gtk-system-framework",
  "gst-audio-framework",
#ifdef GGL_HOST_LINUX
  "linux-system-framework",
#endif
  "smjs-script-runtime",
  "curl-xml-http-request",
  "google-gadget-manager",
  NULL
};

static const char *kGlobalResourcePaths[] = {
#ifdef _DEBUG
  "resources.gg",
  "resources",
#endif
#ifdef GGL_RESOURCE_DIR
  GGL_RESOURCE_DIR "/resources.gg",
  GGL_RESOURCE_DIR "/resources",
#endif
  NULL
};

static const char *g_help_string =
  "Usage: %s [Options] [Gadgets]\n"
  "Options:\n"
  "  -d mode    Specify debug modes for drawing View:\n"
  "             0 - No debug.\n"
  "             1 - Draw bounding boxes around container elements.\n"
  "             2 - Draw bounding boxes around all elements.\n"
  "             4 - Draw bounding boxes around clip region.\n"
  "  -z zoom    Specify initial zoom factor for View, not work for sidebar.\n"
  "  -n         Don't install the gadgets specified in command line.\n"
  "  -b         Draw window border for Main View.\n"
  "  -s         User sidebar mode.\n"
  "\n"
  "Gadgets:\n"
  "  Can specify one or more Desktop Gadget paths. If any gadgets are specified,\n"
  "  they will be installed by using GadgetManager.\n";

int main(int argc, char* argv[]) {
  gtk_init(&argc, &argv);


  double zoom = 1.0;
  int debug_mode = 0;
  bool install_gadgets = true;
  bool decorated = false;
  bool sidebar = false;
  // Parse command line.
  std::vector<std::string> gadget_paths;
  int i = 0;
  while (i<argc) {
    if (++i >= argc) break;

    if (std::string("-h") == argv[i] || std::string("--help") == argv[i]) {
      printf(g_help_string, argv[0]);
      return 0;
    }

    if (std::string("-n") == argv[i] || std::string("--no-inst") == argv[i]) {
      install_gadgets = false;
      continue;
    }

    if (std::string("-b") == argv[i] || std::string("--border") == argv[i]) {
      decorated = true;
      continue;
    }

    if (std::string("-s") == argv[i] || std::string("--sidebar") == argv[i]) {
      sidebar = true;
      continue;
    }

    if (std::string("-d") == argv[i] || std::string("--debug") == argv[i]) {
      if (++i < argc) {
        debug_mode = atoi(argv[i]);
      } else {
        debug_mode = 1;
      }
      continue;
    }

    if (std::string("-z") == argv[i] || std::string("--zoom") == argv[i]) {
      if (++i < argc) {
        zoom = strtod(argv[i], NULL);
        if (zoom <= 0)
          zoom = 1.0;
        DLOG("Use zoom factor %lf", zoom);
      }
      continue;
    }

    gadget_paths.push_back(argv[i]);
  };

  // set locale according to env vars
  setlocale(LC_ALL, "");

  // Set global main loop
  ggadget::SetGlobalMainLoop(&g_main_loop);

  // Set global file manager.
  ggadget::FileManagerWrapper *fm_wrapper = new ggadget::FileManagerWrapper();
  ggadget::FileManagerInterface *fm;

  for (size_t i = 0; kGlobalResourcePaths[i]; ++i) {
    fm = ggadget::CreateFileManager(kGlobalResourcePaths[i]);
    if (fm) {
      fm_wrapper->RegisterFileManager(ggadget::kGlobalResourcePrefix,
                                      new ggadget::LocalizedFileManager(fm));
      break;
    }
  }

  if ((fm = ggadget::CreateFileManager(ggadget::kDirSeparatorStr)) != NULL) {
    fm_wrapper->RegisterFileManager(ggadget::kDirSeparatorStr, fm);
  }

#ifdef _DEBUG
  std::string dot_slash(".");
  dot_slash += ggadget::kDirSeparatorStr;
  if ((fm = ggadget::CreateFileManager(dot_slash.c_str())) != NULL) {
    fm_wrapper->RegisterFileManager(dot_slash.c_str(), fm);
  }
#endif

  std::string profile_dir =
      ggadget::BuildFilePath(ggadget::GetHomeDirectory().c_str(),
                             ggadget::kDefaultProfileDirectory, NULL);
  fm = ggadget::DirFileManager::Create(profile_dir.c_str(), true);
  if (fm != NULL) {
    fm_wrapper->RegisterFileManager(ggadget::kProfilePrefix, fm);
  } else {
    LOG("Failed to initialize profile directory.");
  }

  ggadget::SetGlobalFileManager(fm_wrapper);

  // Load global extensions.
  ggadget::ExtensionManager *ext_manager =
      ggadget::ExtensionManager::CreateExtensionManager();
  ggadget::ExtensionManager::SetGlobalExtensionManager(ext_manager);

  // Ignore errors when loading extensions.
  for (size_t i = 0; kGlobalExtensions[i]; ++i)
    ext_manager->LoadExtension(kGlobalExtensions[i], false);

  // Register JavaScript runtime.
  ggadget::ScriptRuntimeManager *manager = ggadget::ScriptRuntimeManager::get();
  ggadget::ScriptRuntimeExtensionRegister script_runtime_register(manager);
  ext_manager->RegisterLoadedExtensions(&script_runtime_register);

  // Make the global extension manager readonly to avoid the potential
  // danger that a bad gadget register local extensions into the global
  // extension manager.
  ext_manager->SetReadonly();

  ggadget::HostInterface *host;

  if (sidebar)
    host = new hosts::gtk::SidebarGtkHost(decorated, debug_mode);
  else
    host = new hosts::gtk::SimpleGtkHost(zoom, decorated, debug_mode);

  std::vector<ggadget::Gadget *> temp_gadgets;

  // Load gadget files.
  if (gadget_paths.size()) {
    if (install_gadgets) {
      ggadget::GadgetManagerInterface *manager = ggadget::GetGadgetManager();
      for (size_t i = 0; i < gadget_paths.size(); ++i) {
        manager->NewGadgetInstanceFromFile(gadget_paths[i].c_str());
      }
    } else {
#ifdef _DEBUG
      // Only runs the gadgets temporarily. For debug purpose.
      for (size_t i = 0; i < gadget_paths.size(); ++i) {
        std::string opt_name = ggadget::StringPrintf("temp-gadget-%zu", i);
        ggadget::Gadget *gadget =
            new ggadget::Gadget(host, gadget_paths[i].c_str(),
                                // Note: gadget will run in trusted mode.
                                opt_name.c_str(), i + 1000, true);
        if (gadget->IsValid()) {
          temp_gadgets.push_back(gadget);
          gadget->ShowMainView();
        } else {
          DLOG("Failed to load gadget %s", gadget_paths[i].c_str());
          delete gadget;
        }
      }
#endif
    }
  }

  host->Run();

  for (size_t i = 0; i < temp_gadgets.size(); ++i)
    delete temp_gadgets[i];

  delete host;

  return 0;
}
