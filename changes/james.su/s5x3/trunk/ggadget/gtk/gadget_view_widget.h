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

#ifndef GGADGET_GTK_GADGET_VIEW_WIDGET_H__
#define GGADGET_GTK_GADGET_VIEW_WIDGET_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <string>

namespace ggadget {
namespace gtk {
class GtkViewHost;
}
}

G_BEGIN_DECLS

#define GADGETVIEWWIDGET_TYPE        (GadgetViewWidget_get_type())
#define GADGETVIEWWIDGET(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), \
                                      GADGETVIEWWIDGET_TYPE, GadgetViewWidget))
#define GADGETVIEWWIDGET_CLASS(c)    (G_TYPE_CHECK_CLASS_CAST((c), \
                                      GADGETVIEWWIDGET_TYPE, \
                                      GadgetViewWidgetClass))
#define IS_GADGETVIEWWIDGET(obj)     (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
                                      GADGETVIEWWIDGET_TYPE))
#define IS_GADGETVIEWWIDGET_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE((c), \
                                      GADGETVIEWWIDGET_TYPE))

typedef struct _GadgetViewWidget GadgetViewWidget;
typedef struct _GadgetViewWidgetClass GadgetViewWidgetClass;

GType GadgetViewWidget_get_type();
GtkWidget* GadgetViewWidget_new(ggadget::gtk::GtkViewHost *host, double zoom,
                                bool composited, bool useshapemask);

G_END_DECLS

#endif // GGADGET_GTK_GADGET_VIEW_WIDGET_H__