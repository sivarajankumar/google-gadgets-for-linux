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

#ifndef GGADGET_COLOR_H__
#define GGADGET_COLOR_H__

#include <cmath>
#include <ggadget/common.h>
#include <ggadget/string_utils.h>

namespace ggadget {

/**
 * Struct for holding color information.
 * Currently, there is no support for the alpha channel.
 * TODO: color.cc ?
 */
struct Color {
  Color() : red(0), green(0), blue(0) {
  }

  Color(double r, double g, double b) : red(r), green(g), blue(b) {
    ASSERT(r >= 0. && r <= 1.);
    ASSERT(g >= 0. && g <= 1.);
    ASSERT(b >= 0. && b <= 1.);
  };

  explicit Color(const char *name) {
    ASSERT(name && ParseColorName(name, this, NULL));
  }

  Color(const Color &c) : red(c.red), green(c.green), blue(c.blue) {}

  bool operator==(const Color &c) const {
    return red == c.red && green == c.green && blue == c.blue;
  }

  std::string ToString() const {
    return StringPrintf("#%02x%02x%02x",
                        static_cast<int>(round(red * 255)),
                        static_cast<int>(round(green * 255)),
                        static_cast<int>(round(blue * 255)));
  }

  /**
   * Utility function to create a Color object from 8-bit color channel values.
   */
  static Color ColorFromChars(unsigned char r, unsigned char g,
                              unsigned char b) {
    return Color(r / 255.0, g / 255.0, b / 255.0);
  };

  double red, green, blue;
};

} // namespace ggadget

#endif // GGADGET_COLOR_H__
