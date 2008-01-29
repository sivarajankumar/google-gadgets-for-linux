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

#ifndef GGADGET_MATH_UTILS_H__
#define GGADGET_MATH_UTILS_H__

#include <cmath>

namespace ggadget {

/**
 * Converts coordinates in a parent element's space to coordinates in a
 * child element.
 * @param parent_x X-coordinate in the parent space to convert.
 * @param parent_y Y-coordinate in the parent space to convert.
 * @param child_x_pos X-coordinate of the child pin point in parent space.
 * @param child_y_pos Y-coordinate of the child pin point in parent space.
 * @param child_pin_x X-coordinate of the child rotation pin in child space.
 * @param child_pin_y Y-coordinate of the child rotation pin in child space.
 * @param rotation_radians The rotation of the child element in radians.
 * @param[out] child_x Parameter to store the converted child X-coordinate.
 * @param[out] child_y Parameter to store the converted child Y-coordinate.
 */
void ParentCoordToChildCoord(double parent_x, double parent_y,
                             double child_x_pos, double child_y_pos,
                             double child_pin_x, double child_pin_y,
                             double rotation_radians,
                             double *child_x, double *child_y);

/**
 * Reversed function of the above function.
 * @param child_x X-coordinate in the child space to convert.
 * @param child_y Y-coordinate in the child space to convert.
 * @param child_x_pos X-coordinate of the child pin point in parent space.
 * @param child_y_pos Y-coordinate of the child pin point in parent space.
 * @param child_pin_x X-coordinate of the child rotation pin in child space.
 * @param child_pin_y Y-coordinate of the child rotation pin in child space.
 * @param rotation_radians The rotation of the child element in radians.
 * @param[out] parent_x Parameter to store the converted parent X-coordinate.
 * @param[out] parent_y Parameter to store the converted parent Y-coordinate.
 */
void ChildCoordToParentCoord(double child_x, double child_y,
                             double child_x_pos, double child_y_pos,
                             double child_pin_x, double child_pin_y,
                             double rotation_radians,
                             double *parent_x, double *parent_y);

/**
 * Calculate the maximum parent extent of a child.
 * @param child_x_pos X-coordinate of the child pin point in parent space.
 * @param child_y_pos Y-coordinate of the child pin point in parent space.
 * @param child_pin_x X-coordinate of the child rotation pin in child space.
 * @param child_pin_y Y-coordinate of the child rotation pin in child space.
 * @param child_width
 * @param child_height
 * @param rotation_radians The rotation of the child element in radians.
 * @param[out] extent_width The maximum X-coordinate of the child rect
 *     in parent space.
 * @param[out] extent_height The maximum Y-coordinate of the child rect
 *     in parent space.
 */
void GetChildExtentInParent(double child_x_pos, double child_y_pos,
                            double child_pin_x, double child_pin_y,
                            double child_width, double child_height,
                            double rotation_radians,
                            double *extent_width, double *extent_height);

/**
 * Calculator object used to convert a parent element's coordinate space to
 * that of a child element. This struct is a better choice if the multiple
 * coordinate conversions need to be done for the same child element.
 */
class ChildCoordCalculator {
 public:
  /**
   * Constructs the coordinate calculator object.
   * @param child_x_pos X-coordinate of the child pin point in parent space.
   * @param child_y_pos Y-coordinate of the child pin point in parent space.
   * @param child_pin_x X-coordinate of the child rotation pin in child space.
   * @param child_pin_y Y-coordinate of the child rotation pin in child space.
   * @param rotation_radians The rotation of the child element in radians.
   */
  ChildCoordCalculator(double child_x_pos, double child_y_pos,
                       double child_pin_x, double child_pin_y,
                       double rotation_radians);

  /**
   * Converts coordinates the given coordinates.
   * @param parent_x X-coordinate in the parent space to convert.
   * @param parent_y Y-coordinate in the parent space to convert.
   * @param[out] child_x Parameter to store the converted child X-coordinate.
   * @param[out] child_y Parameter to store the converted child Y-coordinate.
   */
  void Convert(double parent_x, double parent_y,
               double *child_x, double *child_y);

  /**
   * @param parent_x X-coordinate in the parent space to convert.
   * @param parent_y Y-coordinate in the parent space to convert.
   * @return The converted child X-coordinate.
   */
  double GetChildX(double parent_x, double parent_y);

  /**
   * @param parent_x X-coordinate in the parent space to convert.
   * @param parent_y Y-coordinate in the parent space to convert.
   * @return The converted child Y-coordinate.
   */
  double GetChildY(double parent_x, double parent_y);

 private:
  double sin_theta_, cos_theta_;
  double a_13_, a_23_;
};

/**
 * Calculator object used to convert a child element's coordinate space to
 * that of the parent element. This struct is a better choice if the multiple
 * coordinate conversions need to be done for the same child element.
 */
class ParentCoordCalculator {
 public:
  /**
   * Constructs the coordinate calculator object.
   * @param child_x_pos X-coordinate of the child pin point in parent space.
   * @param child_y_pos Y-coordinate of the child pin point in parent space.
   * @param child_pin_x X-coordinate of the child rotation pin in child space.
   * @param child_pin_y Y-coordinate of the child rotation pin in child space.
   * @param rotation_radians The rotation of the child element in radians.
   */
  ParentCoordCalculator(double child_x_pos, double child_y_pos,
                       double child_pin_x, double child_pin_y,
                       double rotation_radians);

  /**
   * Converts child coordinates into parent coordinations.
   * @param child_x X-coordinate in the child space to convert.
   * @param child_y Y-coordinate in the child space to convert.
   * @param[out] parent_x Parameter to store the converted parent X-coordinate.
   * @param[out] parent_y Parameter to store the converted parent Y-coordinate.
   */
  void Convert(double child_x, double child_y,
               double *parent_x, double *parent_y);
  /**
   * @param child_x X-coordinate in the child space to convert.
   * @param child_y Y-coordinate in the child space to convert.
   * @return The converted parent X-coordinate.
   */
  double GetParentX(double child_x, double child_y);

  /**
   * @param child_x X-coordinate in the child space to convert.
   * @param child_y Y-coordinate in the child space to convert.
   * @return The converted parent Y-coordinate.
   */
  double GetParentY(double child_x, double child_y);

 private:
  double sin_theta_, cos_theta_;
  double x0_, y0_;
};

/**
 * @return The radian measure of the input parameter.
 */
double DegreesToRadians(double degrees);

/**
 * Checks to see if the given (x, y) is contained in an element.
 * @param x X-coordinate of the element's (0, 0) point in parent space.
 * @param y Y-coordinate of the element's (0, 0) point in parent space.
 * @param width Width of element.
 * @param height Height of element.
 */
bool IsPointInElement(double x, double y, double width, double height);

/**
 * Returns val if low < val < high, otherwise returns low if val < low or high
 * if val > high.
 */
template<typename T>
T Clamp(T val, T low, T high) {
  return val > high ? high : (val < low ? low : val);
}

}

#endif // GGADGET_MATH_UTILS_H__