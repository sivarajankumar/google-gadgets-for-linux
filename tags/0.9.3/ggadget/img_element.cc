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

#include <algorithm>

#include "img_element.h"
#include "canvas_interface.h"
#include "canvas_utils.h"
#include "color.h"
#include "image_interface.h"
#include "string_utils.h"
#include "texture.h"
#include "view.h"

namespace ggadget {

static const char *kCropMaintainAspectNames[] = {
  "false", "true", "photo"
};

class ImgElement::Impl {
 public:
  Impl()
    : image_(NULL), color_multiplied_image_(NULL),
      src_width_(0), src_height_(0),
      crop_(CROP_FALSE),
      stretch_middle_(false) {
  }

  ~Impl() {
    DestroyImage(image_);
    image_ = NULL;
    DestroyImage(color_multiplied_image_);
    color_multiplied_image_ = NULL;
  }

  void ApplyColorMultiply() {
    DestroyImage(color_multiplied_image_);
    color_multiplied_image_ = NULL;

    if (image_) {
      Color c(Color::kWhite);
      double op = 0;
      Color::FromString(color_multiply_.c_str(), &c, &op);
      // For now, the opacity value of colorMultiply only acts like a switch:
      // if zero, colorMultiply will be disabled; otherwise enabled.
      if (op != 0 && c != Color::kWhite)
        color_multiplied_image_ = image_->MultiplyColor(c);
    }
  }

  ImageInterface *GetImage() {
    return color_multiplied_image_ ? color_multiplied_image_ : image_;
  }

  ImageInterface *image_, *color_multiplied_image_;
  double src_width_;
  double src_height_;
  CropMaintainAspect crop_;
  std::string color_multiply_;
  bool stretch_middle_;
};

ImgElement::ImgElement(BasicElement *parent, View *view, const char *name)
    : BasicElement(parent, view, "img", name, false),
      impl_(new Impl) {
}

void ImgElement::DoRegister() {
  BasicElement::DoRegister();
  RegisterProperty("src",
                   NewSlot(this, &ImgElement::GetSrc),
                   NewSlot(this, &ImgElement::SetSrc));
  RegisterProperty("srcWidth", NewSlot(this, &ImgElement::GetSrcWidth), NULL);
  RegisterProperty("srcHeight", NewSlot(this, &ImgElement::GetSrcHeight), NULL);
  RegisterProperty("colorMultiply",
                   NewSlot(this, &ImgElement::GetColorMultiply),
                   NewSlot(this, &ImgElement::SetColorMultiply));
  RegisterStringEnumProperty("cropMaintainAspect",
                             NewSlot(this, &ImgElement::GetCropMaintainAspect),
                             NewSlot(this, &ImgElement::SetCropMaintainAspect),
                             kCropMaintainAspectNames,
                             arraysize(kCropMaintainAspectNames));
  RegisterProperty("stretchMiddle",
                   NewSlot(this, &ImgElement::IsStretchMiddle),
                   NewSlot(this, &ImgElement::SetStretchMiddle));
  RegisterMethod("setSrcSize", NewSlot(this, &ImgElement::SetSrcSize));
}

ImgElement::~ImgElement() {
  delete impl_;
  impl_ = NULL;
}

bool ImgElement::IsPointIn(double x, double y) const {
  // Return false directly if the point is outside the element boundary.
  if (!BasicElement::IsPointIn(x, y))
    return false;

  double pxwidth = GetPixelWidth();
  double pxheight = GetPixelHeight();
  ImageInterface *image = impl_->GetImage();
  if (image && pxwidth > 0 && pxheight > 0) {
    double imgw = static_cast<double>(image->GetWidth());
    double imgh = static_cast<double>(image->GetHeight());
    if (impl_->crop_ == CROP_FALSE) {
      if (impl_->stretch_middle_) {
        MapStretchMiddleCoordDestToSrc(x, y, imgw, imgh, pxwidth, pxheight,
                                       -1, -1, -1, -1, &x, &y);
      } else {
        // Stretch x and y.
        x = x * imgw / pxwidth;
        y = y * imgh / pxheight;
      }
    } else {
      double scale = std::max(pxwidth / imgw, pxheight / imgh);
      // Windows also caps the scale to a fixed maximum. This is probably a bug.
      double w = scale * imgw;
      double h = scale * imgh;

      double x0 = (pxwidth - w) / 2.;
      double y0 = (pxheight - h) / 2.;
      if (impl_->crop_ == CROP_PHOTO && y0 < 0.) {
        y0 = 0.; // Never crop the top in photo setting.
      }
      // Stretch x and y.
      x = (x - x0) * imgw / w;
      y = (y - y0) * imgh / h;
    }

    double opacity;
    // If failed to get the value of the point, then just return true, assuming
    // it's an opaque point.
    if (!image->GetPointValue(x, y, NULL, &opacity))
      return true;

    return opacity > 0;
  }
  return false;
}

void ImgElement::DoDraw(CanvasInterface *canvas) {
  ImageInterface *image = impl_->GetImage();
  if (image) {
    double pxwidth = GetPixelWidth();
    double pxheight = GetPixelHeight();
    if (impl_->crop_ == CROP_FALSE) {
      if (impl_->stretch_middle_) {
        StretchMiddleDrawImage(image, canvas, 0, 0, pxwidth, pxheight,
                               -1, -1, -1, -1);
      } else {
        image->StretchDraw(canvas, 0, 0, pxwidth, pxheight);
      }
    } else {
      double imgw = image->GetWidth();
      double imgh = image->GetHeight();
      if (imgw <= 0 || imgh <= 0) {
        return;
      }

      double scale = std::max(pxwidth / imgw, pxheight / imgh);
      // Windows also caps the scale to a fixed maximum. This is probably a bug.
      double w = scale * imgw;
      double h = scale * imgh;

      double x = (pxwidth - w) / 2.;
      double y = (pxheight - h) / 2.;
      if (impl_->crop_ == CROP_PHOTO && y < 0.) {
        y = 0.; // Never crop the top in photo setting.
      }
      image->StretchDraw(canvas, x, y, w, h);
    }
  }
}

Variant ImgElement::GetSrc() const {
  return Variant(GetImageTag(impl_->image_));
}

void ImgElement::SetSrc(const Variant &src) {
  if (src != GetSrc()) {
    DestroyImage(impl_->image_);
    impl_->image_ = GetView()->LoadImage(src, false);
    if (impl_->image_) {
      impl_->src_width_ = impl_->image_->GetWidth();
      impl_->src_height_ = impl_->image_->GetHeight();
    } else {
      impl_->src_width_ = 0;
      impl_->src_height_ = 0;
    }

    impl_->ApplyColorMultiply();
    QueueDraw();
  }
}

std::string ImgElement::GetColorMultiply() const {
  return impl_->color_multiply_;
}

void ImgElement::SetColorMultiply(const char *color) {
  if (AssignIfDiffer(color, &impl_->color_multiply_, strcmp)) {
    impl_->ApplyColorMultiply();
    QueueDraw();
  }
}

ImgElement::CropMaintainAspect ImgElement::GetCropMaintainAspect() const {
  return impl_->crop_;
}

void ImgElement::SetCropMaintainAspect(ImgElement::CropMaintainAspect crop) {
  if (crop != impl_->crop_) {
    impl_->crop_ = crop;
    QueueDraw();
  }
}

bool ImgElement::IsStretchMiddle() const {
  return impl_->stretch_middle_;
}

void ImgElement::SetStretchMiddle(bool stretch_middle) {
  if (stretch_middle != impl_->stretch_middle_) {
    impl_->stretch_middle_ = stretch_middle;
    //EnableCanvasCache(stretch_middle);
    QueueDraw();
  }
}

double ImgElement::GetSrcWidth() const {
  return impl_->src_width_;
}

double ImgElement::GetSrcHeight() const {
  return impl_->src_height_;
}

void ImgElement::GetDefaultSize(double *width, double *height) const {
  *width = impl_->src_width_;
  *height = impl_->src_height_;
}

void ImgElement::SetSrcSize(double width, double height) {
  // Because image data may shared among elements, we don't think this method
  // is useful, because we may require more memory to store the new resized
  // canvas.
  impl_->src_width_ = width;
  impl_->src_height_ = height;
}

bool ImgElement::HasOpaqueBackground() const {
  ImageInterface *image = impl_->GetImage();
  return image ? image->IsFullyOpaque() : false;
}

BasicElement *ImgElement::CreateInstance(BasicElement *parent, View *view,
                                         const char *name) {
  return new ImgElement(parent, view, name);
}


} // namespace ggadget