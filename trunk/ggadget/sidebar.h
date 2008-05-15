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

#ifndef GGADGET_SIDEBAR_H__
#define GGADGET_SIDEBAR_H__

#include <ggadget/common.h>
#include <ggadget/variant.h>
#include <ggadget/view_host_interface.h>

namespace ggadget {

class HostInterface;
class MenuInterface;
class ViewElement;
class ViewInterface;

#ifndef DECLARED_VARIANT_PTR_TYPE_MENU_INTERFACE
#define DECLARED_VARIANT_PTR_TYPE_MENU_INTERFACE
DECLARE_VARIANT_PTR_TYPE(MenuInterface);
#endif

/*
 * Object that represent the side bar.
 * SideBar is a container of view element, each view element is combined with a
 * ViewHost.
 */
class SideBar {
 public:
  /*
   * Constructor.
   * @param host Host Interface for the side bar, not owned by the @SideBar.
   *        Should be destroyed after the object.
   * @param view_host @c SideBar acts as @c View, @c view_host is the ViewHost
   *        instance that is associated with the @c SideBar instance.
   *        It's not owned by the object, and shall be destroyed after
   *        destroying the object.
   */
  SideBar(HostInterface *host, ViewHostInterface *view_host);

  /* Destructor. */
  virtual ~SideBar();

 public:
  /**
   * Creates a new ViewHost instance and of curse a new view element
   * hold in the side bar.
   *
   * @param height The initial height of the new ViewHost instance. Note this is
   *        only a hint, @SideBar will choose the proper height for the instance
   *        based on the @c height
   * @return a new Viewhost instance.
   */
  ViewHostInterface *NewViewHost(double height);

  /**
   * @return the ViewHost instance associated with this instance.
   */
  ViewHostInterface *GetViewHost() const;

  /**
   * Set the size of the sidebar.
   */
  void SetSize(double width, double height);

  /** Retrieves the width of the side bar in pixels. */
  double GetWidth() const;

  /** Retrieves the height of side bar in pixels. */
  double GetHeight() const;

  /**
   * Insert a null element in the side bar.
   *
   * @param height The initial height of the null element. Note this is
   *        only a hint, @SideBar will choose the proper height.
   * @param view The side bar will talk with the instance to decide the size
   *        of the null element.
   */
  void InsertNullElement(double height, ViewInterface *view);

  /**
   * Clear null element(s).
   */
  void ClearNullElement();

  /**
   * Explicitly let side bar reorganize the layout.
   */
  void Layout();

  /**
   * @return Return the element that is moused over. Return @c NULL if no
   * element is moused over.
   */
  ViewElement *GetMouseOverElement() const;

  /**
   * Find if any view element in the side bar is the container of the @c view.
   * @param view the queried view.
   * @return @c NULL if no such view element is found.
   */
  ViewElement *FindViewElementByView(ViewInterface *view) const;

  /**
   * Set pop out view.
   * When a element is poped out, the @c host associated with
   * the sidebar should let sidebar know it.
   * @param view the view that was poped out
   * @return the view element that contains the @c view.
   */
  ViewElement *SetPopoutedView(ViewInterface *view);

  /**
   * Event connection methods.
   */
  Connection *ConnectOnUndock(Slot0<void> *slot);
  Connection *ConnectOnPopIn(Slot0<void> *slot);
  Connection *ConnectOnAddGadget(Slot0<void> *slot);
  Connection *ConnectOnMenuOpen(Slot1<bool, MenuInterface *> *slot);
  Connection *ConnectOnClose(Slot0<void> *slot);
  Connection *ConnectOnSizeEvent(Slot0<void> *slot);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(SideBar);
};

}  // namespace ggadget

#endif  // GGADGET_SIDEBAR_H__