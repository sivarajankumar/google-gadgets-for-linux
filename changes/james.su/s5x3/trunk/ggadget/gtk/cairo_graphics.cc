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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdk/gdkcairo.h>
#include <ggadget/color.h>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include "cairo_graphics.h"
#include "cairo_canvas.h"
#include "cairo_font.h"
#include "pixbuf_image.h"

#ifdef HAVE_RSVG_LIBRARY
#include "rsvg_image.h"
#endif

namespace ggadget {
namespace gtk {

class CairoGraphics::Impl {
 public:
  Impl(double zoom) : zoom_(zoom) {
    if (zoom_ <= 0) zoom_ = 1;
  }

  double zoom_;
  Signal1<void, double> on_zoom_signal_;
};

CairoGraphics::CairoGraphics(double zoom) : impl_(new Impl(zoom)) {
}

double CairoGraphics::GetZoom() const {
  return impl_->zoom_;
}

void CairoGraphics::SetZoom(double zoom) {
  if (impl_->zoom_ != zoom) {
    impl_->zoom_ = (zoom > 0 ? zoom : 1);
    impl_->on_zoom_signal_(impl_->zoom_);
  }
}

Connection *CairoGraphics::ConnectOnZoom(Slot1<void, double> *slot) const {
  return impl_->on_zoom_signal_.Connect(slot);
}

CanvasInterface *CairoGraphics::NewCanvas(size_t w, size_t h) const {
  if (!w || !h) return NULL;

  CairoCanvas *canvas = new CairoCanvas(this, w, h, CAIRO_FORMAT_ARGB32);
  if (!canvas->IsValid()) {
    delete canvas;
    canvas = NULL;
  }
  return canvas;
}

#ifdef HAVE_RSVG_LIBRARY
static bool IsSvg(const std::string &data) {
  //TODO: better detection method?
  return data.find("<?xml") != std::string::npos &&
         data.find("<svg") != std::string::npos;
}
#endif

ImageInterface *CairoGraphics::NewImage(const std::string &data,
                                        bool is_mask) const {
  if (data.empty()) return NULL;

  ImageInterface *img = NULL;

#ifdef HAVE_RSVG_LIBRARY
  // Only use RsvgImage for ordinary svg image.
  if (IsSvg(data) && !is_mask) {
    img = new RsvgImage(this, data, is_mask);
    if (!down_cast<RsvgImage*>(img)->IsValid()) {
      img->Destroy();
      img = NULL;
    }
  } else {
#else
  if (1) {
#endif
    img = new PixbufImage(this, data, is_mask);
    if (!down_cast<PixbufImage*>(img)->IsValid()) {
      img->Destroy();
      img = NULL;
    }
  }
  return img;
}

FontInterface *CairoGraphics::NewFont(const char *family, size_t pt_size,
                                      FontInterface::Style style,
                                      FontInterface::Weight weight) const {
  PangoFontDescription *font = pango_font_description_new();

  pango_font_description_set_family(font, family);
  // Calculate pixel size based on the Windows DPI of 96 for compatibility
  // reasons.
  double px_size = pt_size * PANGO_SCALE * 96. / 72.;
  pango_font_description_set_absolute_size(font, px_size);

  if (weight == FontInterface::WEIGHT_BOLD) {
    pango_font_description_set_weight(font, PANGO_WEIGHT_BOLD);
  }

  if (style == FontInterface::STYLE_ITALIC) {
    pango_font_description_set_style(font, PANGO_STYLE_ITALIC);
  }

  return new CairoFont(font, pt_size, style, weight);
}

} // namespace gtk
} // namespace ggadget