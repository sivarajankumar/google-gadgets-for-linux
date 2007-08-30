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

#ifndef GGADGET_CANVAS_INTERFACE_H__
#define GGADGET_CANVAS_INTERFACE_H__

#include "color.h"
#include "font_interface.h"

namespace ggadget {

/** 
 * This class is the interface abstracting all of ggadget's drawing 
 * functionality. It is designed to be independent from the rendering 
 * library used to realize this functionality. The user assumes that the object 
 * is ready to use on creation. This means that all the prerequisites for the 
 * graphics environment has been set up. In most cases, this includes an 
 * appropiately-sized blank drawing surface, an unset clipping and mask, a 
 * starting opacity of 1.0, and the identity matrix as the default 
 * transformation matrix, although these may change depending on the object 
 * creator's needs.
 */
class CanvasInterface {
 protected:
  /**
   * Disallow direct deletion.
   */
  virtual ~CanvasInterface() { }

 public:

  typedef int TextFlag;

  /**
   * Enum used to specify horizontal alignment.
   */
  enum Alignment {
    LEFT, 
    CENTER, 
    RIGHT 
  };


  /**
   * Enum used to specify vertical alignment.
   */
  enum VAlignment {
    TOP, 
    MIDDLE, 
    BOTTOM 
  };

  /**
   * Enum used to specify trimming type.
   */
  enum Trimming {
    TRIMMING_NONE,
    TRIMMING_CHARACTER,
    TRIMMING_WORD,
    TRIMMING_CHARACTER_ELLIPSIS,
    TRIMMING_WORD_ELLIPSIS,
    TRIMMING_PATH_ELLIPSIS
  };

  /**
   * Enum used to specify text flags.
   */
  enum {
    TEXT_FLAGS_UNDERLINE = 1 << 0,
    TEXT_FLAGS_STRIKEOUT = 1 << 1,
    TEXT_FLAGS_WORDWRAP = 1 << 2
  };

  /**
   * Frees this CanvasInterface object.
   */
  virtual void Destroy() = 0;

  /**
   * @return The width of the canvas in pixels.
   */
  virtual size_t GetWidth() const = 0;
  /**
   * @return The height of the canvas in pixels.
   */
  virtual size_t GetHeight() const = 0;  
  
  /**
   * @return true if the canvas is a mask, false otherwise.
   */
  virtual bool IsMask() const = 0;  
  
  /**
   * @return A name unique to the concrete class type.
   */
  virtual const char *ClassType() const = 0;
  
  /** 
   * Saves the current graphics state in a stack, while not changing the current 
   * state. Specifically, four aspects of the state are stored: clipping, 
   * transformation matrix, and opacity. Since the states are 
   * saved in a stack, this method may be called multiple times without losing 
   * any of the previously saved states.
   * @return true on success, false otherwise.
   */
  virtual bool PushState() = 0;
  
  /** 
   * Restores the last pushed graphics state from the state stack, overwriting 
   * the current state. Specifically, clipping, transformation matrix, 
   * and opacity are restored with each pop. Since the states are saved in 
   * a stack, this method may be called multiple times to pop multiple 
   * previously saved states. 
   * @return true on success, false otherwise.
   */
  virtual bool PopState() = 0;

  /**
   * Multiply the current opacity by the input parameter. This method changes 
   * the current state.
   * @param opacity factor in [0, 1] to multiply the existing opacity by.
   * @return true if successful, false if no changes are made (i.e. error).
   */
  virtual bool MultiplyOpacity(double opacity) = 0;
  
  /**
   * Rotates the current user coordinates.
   */
  virtual void RotateCoordinates(double radians) = 0;
  /**
   * Translates the current user coordinates.
   */
  virtual void TranslateCoordinates(double dx, double dy) = 0;
  /**
   * Scales the current user coordinates.
   */
  virtual void ScaleCoordinates(double cx, double cy) = 0;
  
  /** 
   * Draw a line from point (x0, y0) to (x1, y1).
   * @param x0 X-coordinate of the line starting point.
   * @param y0 Y-coordinate of the line starting point.
   * @param x1 X-coordinate of the line ending point.
   * @param y1 Y-coordinate of the line ending point.
   * @param width Width of the line to draw.
   * @param c Line color.
   * @return true on success, false otherwise.
   */
  virtual bool DrawLine(double x0, double y0, double x1, double y1, 
                        double width, const Color &c) = 0;

  /**
   * Draw a filled rectangle with top left corner at (x, y).
   * @param x The X-coordinate of the rectangle's top left corner.
   * @param y The Y-coordinate of the rectangle's top left corner.
   * @param w Width of rectangle.
   * @param h Height of rectangle.
   * @param c Fill color.
   * @return true on success, false otherwise.
   */
  virtual bool DrawFilledRect(double x, double y, 
                              double w, double h, const Color &c) = 0;  

  /** 
   * Draws the given canvas starting at (x, y). The current 
   * transformation matrix of the target canvas is respected, ignoring
   * the current transformation matrix defined in the image parameter. However, 
   * the width and height of the parameter is preserved.
   * @param x The X-coordinate of the image's top left corner.
   * @param y The Y-coordinate of the image's top left corner.
   * @param img Canvas to draw.
   */
  virtual bool DrawCanvas(double x, double y, const CanvasInterface *img) = 0;

  /** 
   * Draws the given canvas at (x, y) using a mask on this canvas, while 
   * respecting the current transformation matrix of the target canvas. 
   * The current transformation matrix defined in the image and mask 
   * parameters is ignored, with the exception of width and height.
   * @param x The X-coordinate of the image's top left corner.
   * @param y The Y-coordinate of the image's top left corner.
   * @param img The image to draw.
   * @param mask The new mask to use. It must be a mask canvas.
   * @param mx The X-coordinate of the mask's top left corner.
   * @param my The Y-coordinate of the mask's top left corner.
   */
  virtual bool DrawCanvasWithMask(double x, double y, 
                                  const CanvasInterface *img,
                                  double mx, double my,
                                  const CanvasInterface *mask) = 0;

  /**
   * Draws the specified text on the screen at the given (x, y).
   * @return true on success, false otherwise.
   */
  virtual bool DrawText(double x, double y, double width, double height,
                        const char *text, const FontInterface *f,
                        const Color &c, Alignment align, VAlignment valign,
                        Trimming trimming, TextFlag text_flag) = 0;
  
  /**
   * Intersect the clipping region with a rectangular region.
   * @param x X-coordinate of top left corner of rectangle.
   * @param y Y-coordinate of top left corner of rectangle.
   * @param w Width of rectangle.
   * @param h Height of rectangle.
   * @return true on success, false otherwise.
   */
  virtual bool IntersectRectClipRegion(double x, double y, 
                                       double w, double h) = 0;  
};

} // namespace ggadget

#endif // GGADGET_CANVAS_INTERFACE_H__
