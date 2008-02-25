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

#ifndef GGADGET_QT_QT_FONT_H__
#define GGADGET_QT_QT_FONT_H__

#include <QFont>
#include <QString>
#include <ggadget/font_interface.h>

namespace ggadget {
namespace qt {

/**
 * A Qt implementation of the FontInterface.
 */
class QtFont : public FontInterface {
 public:
  QtFont(const char *family, size_t size, Style style,
          Weight weight);
  virtual ~QtFont();

  virtual Style GetStyle() const { return style_; };
  virtual Weight GetWeight() const { return weight_; };
  virtual size_t GetPointSize() const { return size_; };

  virtual void Destroy() { delete this; };

  bool GetTextExtents(const char *text, double *width, double *height) const;
  QFont *GetQFont() const { return font_; }
 private:
  QFont *font_;
  size_t size_;
  Style style_;
  Weight weight_;
};

} // namespace qt 
} // namespace ggadget

#endif // GGADGET_QT_QT_FONT_H__