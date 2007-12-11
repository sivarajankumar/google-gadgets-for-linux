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

#ifndef GGADGET_ITEM_ELEMENT_H__
#define GGADGET_ITEM_ELEMENT_H__

#include <ggadget/basic_element.h>

namespace ggadget {

class Texture;

class ItemElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x93a09b61fb8a4fda, BasicElement);

  ItemElement(BasicElement *parent, View *view,
              const char *tag_name, const char *name);
  virtual ~ItemElement();

 public:
  /** Gets and sets whether this item is currently selected. */
  bool IsSelected() const;
  void SetSelected(bool selected);

  /**
   * Gets and sets the background color or image of the element. The image is
   * repeated if necessary, not stretched.
   */
  Variant GetBackground() const;
  void SetBackground(const Variant &background);

  virtual void SetWidth(const Variant &width);
  virtual void SetHeight(const Variant &height);

  virtual void SetX(const Variant &x);
  virtual void SetY(const Variant &y);

  bool IsMouseOver() const;

  /** 
   * Hook these methods to also mark the parent as changed.
   * This is OK in all scenarios since item is only used inside ListBoxes.
   */
  virtual void QueueDraw(); 

  /** 
   * Sets whether mouseover/selected overlays should be drawn. 
   * This method is used in Draw() calls to temporarily disable overlay drawing. 
   */
  void SetDrawOverlay(bool draw);

  /**
   * Sets the current index of the item in the parent.
   */
  void SetIndex(int index);

  /** 
   * Gets and sets the text of the label contained inside this element. 
   * AddLabelWithText will create the Label, the other methods will 
   * assume it exists.
   */
  const char *GetLabelText() const;
  void SetLabelText(const char *text);
  bool AddLabelWithText(const char *text);

 public:
  static BasicElement *CreateInstance(BasicElement *parent, View *view,
                                      const char *name);

  /** For backward compatibility of listitem. */
  static BasicElement *CreateListItemInstance(BasicElement *parent, View *view,
                                              const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas,
                      const CanvasInterface *children_canvas);
  virtual void GetDefaultSize(double *width, double *height) const;
  virtual void GetDefaultPosition(double *x, double *y) const;
  virtual EventResult HandleMouseEvent(const MouseEvent &event);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ItemElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_ITEM_ELEMENT_H__
