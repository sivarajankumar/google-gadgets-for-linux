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

#ifndef GGADGET_QT_QT_MENU_H__
#define GGADGET_QT_QT_MENU_H__

#include <QtGui/QMenu>
#include <map>
#include <string>
#include <vector>
#include <ggadget/common.h>
#include <ggadget/menu_interface.h>

namespace ggadget {
namespace qt {

/**
 * An implementation of @c MenuInterface for the simple gadget host.
 */
class QtMenu : public ggadget::MenuInterface {
 public:
  QtMenu(QMenu *qmenu);
  virtual ~QtMenu();

  virtual void AddItem(const char *item_text, int style,
                       ggadget::Slot1<void, const char *> *handler);
  virtual void SetItemStyle(const char *item_text, int style);
  virtual MenuInterface *AddPopup(const char *popup_text);

  QMenu *GetNativeMenu();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(QtMenu);

 /* static void OnItemActivate(GtkMenuItem *menu_item, gpointer user_data);
  static void SetMenuItemStyle(GtkMenuItem *menu_item, int style);

  struct MenuItemInfo {
    std::string item_text;
    GtkMenuItem *gtk_menu_item;
    int style;
    ggadget::Slot1<void, const char *> *handler;
    QtMenu *submenu;
  };

  typedef std::multimap<std::string, MenuItemInfo> ItemMap;
  ItemMap item_map_;
  static bool setting_style_; */
  class Impl;
  Impl *impl_;
};

class MenuItemInfo : public QObject {
  Q_OBJECT
 public:
  MenuItemInfo(const char *text,
               ggadget::Slot1<void, const char *> *handler,
               QAction *action)
    : item_text_(text), handler_(handler), action_(action) {
      connect(action, SIGNAL(triggered()), this, SLOT(OnTriggered()));
  }

  std::string item_text_;
  ggadget::Slot1<void, const char *> *handler_;
  QAction *action_;

 public slots:
  void OnTriggered() {
    if (handler_) (*handler_)(item_text_.c_str());
  }
};

} // namesapce qt
} // namespace ggadget

#endif // GGADGET_QT_QT_MENU_H__