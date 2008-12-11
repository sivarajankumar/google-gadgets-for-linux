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

#include <gtk/gtk.h>
#include <string>

#include "gtk_host_base.h"

#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/locales.h>
#include <ggadget/messages.h>
#include <ggadget/string_utils.h>
#include <ggadget/permissions.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gtk/single_view_host.h>

using namespace ggadget;
using namespace ggadget::gtk;

namespace hosts {
namespace gtk {

static bool GetPermissionsDescriptionCallback(int permission,
                                              std::string *msg) {
  if (msg->length())
    msg->append("\n");
  msg->append("  ");
  msg->append(Permissions::GetDescription(permission));
  return true;
}

static bool ShowPermissionsConfirmDialog(const std::string &download_url,
                                         const std::string &title,
                                         const std::string &description,
                                         Permissions *permissions) {
  // Get required permissions description.
  std::string permissions_msg;
  permissions->EnumerateAllRequired(
      NewSlot(GetPermissionsDescriptionCallback, &permissions_msg));

  GtkWidget *dialog = gtk_message_dialog_new(
      NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
      "%s\n\n%s\n%s\n\n%s%s\n\n%s\n%s",
      GM_("GADGET_CONFIRM_MESSAGE"), title.c_str(), download_url.c_str(),
      GM_("GADGET_DESCRIPTION"), description.c_str(),
      GM_("GADGET_REQUIRED_PERMISSIONS"), permissions_msg.c_str());

  GdkScreen *screen;
  gdk_display_get_pointer(gdk_display_get_default(), &screen,
                          NULL, NULL, NULL);
  gtk_window_set_screen(GTK_WINDOW(dialog), screen);
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(dialog), GM_("GADGET_CONFIRM_TITLE"));
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), false);
  gtk_window_present(GTK_WINDOW(dialog));
  gtk_window_set_urgency_hint(GTK_WINDOW(dialog), true);
  gint result = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

  return result == GTK_RESPONSE_YES;
}

bool GtkHostBase::ConfirmGadget(const std::string &path,
                                const std::string &options_name,
                                const std::string &download_url,
                                const std::string &title,
                                const std::string &description,
                                Permissions *permissions) {
  OptionsInterface *options = CreateOptions(options_name.c_str());
  if (!options) {
    ShowAlertDialog(GM_("GOOGLE_GADGETS"), StringPrintf(
        GM_("GADGET_LOAD_FAILURE"), path.c_str()).c_str());
    return false;
  }

  bool should_save_permissions = true;
  Variant value = options->GetInternalValue(kPermissionsOption);
  if (value.type() == Variant::TYPE_STRING) {
    Permissions granted_permissions;
    granted_permissions.FromString(VariantValue<const char*>()(value));
    permissions->SetGrantedByPermissions(granted_permissions, true);
    should_save_permissions = false;
  }

  if (permissions->HasUngranted()) {
    should_save_permissions = true;
    if (!ShowPermissionsConfirmDialog(download_url, title,
                                      description, permissions)) {
      options->DeleteStorage();
      delete options;
      return false;
    }
    // TODO: Is it necessary to let user grant individual permissions
    // separately?
    permissions->GrantAllRequired();
  }

  if (should_save_permissions) {
    Permissions save_permissions = *permissions;
    // Don't save required permissions.
    save_permissions.RemoveAllRequired();
    options->PutInternalValue(kPermissionsOption,
                              Variant(save_permissions.ToString()));
    options->Flush();
  }
  delete options;
  return true;
}

bool GtkHostBase::ConfirmManagedGadget(int id, bool grant) {
  GadgetManagerInterface *gadget_manager = GetGadgetManager();

  std::string path = gadget_manager->GetGadgetInstancePath(id);
  std::string options_name = gadget_manager->GetGadgetInstanceOptionsName(id);
  std::string download_url, title, description;
  Permissions permissions;
  if (!gadget_manager->GetGadgetInstanceInfo(id, GetSystemLocaleName().c_str(),
                                             NULL, &download_url,
                                             &title, &description) ||
      !gadget_manager->GetGadgetDefaultPermissions(id, &permissions)) {
    ShowAlertDialog(GM_("GOOGLE_GADGETS"), StringPrintf(
        GM_("GADGET_LOAD_FAILURE"), path.c_str()).c_str());
    return false;
  }

  if (grant)
    permissions.GrantAllRequired();

  return ConfirmGadget(path, options_name, download_url, title,
                       description, &permissions);
}

int GtkHostBase::FlagsToViewHostFlags(int flags) {
  int vh_flags = SingleViewHost::DEFAULT;

  if (flags & WINDOW_MANAGER_BORDER)
    vh_flags |= SingleViewHost::DECORATED;
  if (flags & MATCHBOX_WORKAROUND)
    vh_flags |= SingleViewHost::DIALOG_TYPE_HINT;
  if (flags & NO_TRANSPARENT)
    vh_flags |= SingleViewHost::OPAQUE_BACKGROUND;
  return vh_flags;
}

} // namespace gtk
} // namespace hosts