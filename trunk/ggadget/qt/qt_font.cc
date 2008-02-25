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

#include <QFontMetrics>
#include "qt_font.h"

namespace ggadget {
namespace qt {

QtFont::QtFont(const char *family, size_t size, Style style,
                     Weight weight) 
    : style_(style), weight_(weight) {
  int qt_weight = QFont::Normal;
  bool italic = false;
  if (weight == WEIGHT_BOLD) qt_weight = QFont::Bold;
  if (style == STYLE_ITALIC) italic = true;

  font_ = new QFont(family, size, qt_weight, italic);
  size_ = font_->pointSize();
}

QtFont::~QtFont() {
  if (font_) delete font_;
}

bool QtFont::GetTextExtents(const char *text, double *width, double *height) const {
  QFontMetrics qm(*font_);
  QRect r = qm.boundingRect(text);
  *width = r.width();
  *height = r.height();
  return true;
}

} // namespace gtk
} // namespace ggadget