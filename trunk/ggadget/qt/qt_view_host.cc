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

#include <sys/time.h>

#include <QtGui/QCursor>
#include <QtGui/QBitmap>
#include <QtGui/QToolTip>
#include <QtGui/QMessageBox>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QInputDialog>
#include <ggadget/file_manager_interface.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/logger.h>
#include <ggadget/options_interface.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/script_runtime_interface.h>
#include <ggadget/script_runtime_manager.h>
#include "utilities.h"
#include "qt_view_host.h"
#include "qt_menu.h"
#include "qt_graphics.h"
#include "qt_gadget_widget.h"

namespace ggadget {
namespace qt {

class QtViewHost::Impl {
 public:
  Impl(ViewHostInterface::Type type,
       double zoom, bool decorated,
       int debug_mode)
    : view_(NULL),
      type_(type),
      widget_(NULL),
      window_(NULL),
      dialog_(NULL),
      debug_mode_(debug_mode),
      zoom_(zoom),
      onoptionchanged_connection_(NULL),
      feedback_handler_(NULL),
      composite_(false),
      input_shape_mask_(true),
      qt_obj_(new QtViewHostObject(this)) {
    if (type == ViewHostInterface::VIEW_HOST_MAIN)
      composite_ = true;
    // debug_mode_ = ViewInterface::DEBUG_ALL;
  }

  ~Impl() {
    if (onoptionchanged_connection_)
      onoptionchanged_connection_->Disconnect();

    if (view_) {
      delete view_;
      view_ = NULL;
    }

    if (qt_obj_) delete qt_obj_;
  }

  void Detach() {
    view_ = NULL;
    if (window_)
      delete window_;
    if (dialog_)
      delete dialog_;
    window_ = widget_ = NULL;
    dialog_ = NULL;
  }

  bool ShowView(bool modal, int flags,
                Slot1<void, int> *feedback_handler) {
    ASSERT(view_);
    if (feedback_handler_)
      delete feedback_handler_;
    feedback_handler_ = feedback_handler;

    // Initialize window and widget.
    if (type_ == ViewHostInterface::VIEW_HOST_OPTIONS) {
      QVBoxLayout *layout = new QVBoxLayout();
      widget_->setFixedSize(D2I(view_->GetWidth()), D2I(view_->GetHeight()));
      layout->addWidget(widget_);
      dialog_ = new QDialog();

      QDialogButtonBox::StandardButtons what_buttons = 0;
      if (flags & ViewInterface::OPTIONS_VIEW_FLAG_OK)
        what_buttons |= QDialogButtonBox::Ok;

      if (flags & ViewInterface::OPTIONS_VIEW_FLAG_CANCEL)
        what_buttons |= QDialogButtonBox::Cancel;

      if (what_buttons != 0) {
        QDialogButtonBox *buttons = new QDialogButtonBox(what_buttons);
        if (flags & ViewInterface::OPTIONS_VIEW_FLAG_OK)
          dialog_->connect(buttons, SIGNAL(accepted()),
                           qt_obj_, SLOT(OnOptionViewOK()));
        if (flags & ViewInterface::OPTIONS_VIEW_FLAG_CANCEL)
          dialog_->connect(buttons, SIGNAL(rejected()),
                           qt_obj_, SLOT(OnOptionViewCancel()));
        layout->addWidget(buttons);
      }

      dialog_->setLayout(layout);

      if (modal) {
        dialog_->exec();
      } else {
        dialog_->show();
      }
    } else if (type_ == ViewHostInterface::VIEW_HOST_DETAILS) {
      window_ = widget_;
      window_->setAttribute(Qt::WA_DeleteOnClose, false);
      widget_->connect(widget_, SIGNAL(closed()),
                       qt_obj_, SLOT(OnDetailsViewClose()));
      window_->show();
    } else {
      window_ = widget_;
      widget_->EnableInputShapeMask(input_shape_mask_);
      window_->show();
    }

    return true;
  }

  void HandleOptionViewResponse(ViewInterface::OptionsViewFlags flag) {
    if (feedback_handler_) {
      (*feedback_handler_)(flag);
      delete feedback_handler_;
      feedback_handler_ = NULL;
    }
    dialog_->hide();
  }

  void HandleDetailsViewClose() {
    if (feedback_handler_) {
      (*feedback_handler_)(ViewInterface::DETAILS_VIEW_FLAG_NONE);
      delete feedback_handler_;
      feedback_handler_ = NULL;
    }
  }

  ViewInterface *view_;
  ViewHostInterface::Type type_;
  QGadgetWidget *widget_;
  QWidget *window_;     // Top level window of the view
  QDialog *dialog_;     // Top level window of the view
  int debug_mode_;
  double zoom_;
  Connection *onoptionchanged_connection_;

  static const unsigned int kShowTooltipDelay = 500;
  static const unsigned int kHideTooltipDelay = 4000;

  Slot1<void, int> *feedback_handler_;

  bool composite_;
  bool input_shape_mask_;
  QtViewHostObject *qt_obj_;    // used for handling qt signal
};

void QtViewHostObject::OnOptionViewOK() {
  owner_->HandleOptionViewResponse(ViewInterface::OPTIONS_VIEW_FLAG_OK);
}

void QtViewHostObject::OnOptionViewCancel() {
  owner_->HandleOptionViewResponse(ViewInterface::OPTIONS_VIEW_FLAG_CANCEL);
}

void QtViewHostObject::OnDetailsViewClose() {
  owner_->HandleDetailsViewClose();
}

QtViewHost::QtViewHost(ViewHostInterface::Type type,
                       double zoom, bool decorated,
                       int debug_mode)
  : impl_(new Impl(type, zoom, debug_mode, debug_mode)) {
}

QtViewHost::~QtViewHost() {
  if (impl_) delete impl_;
}

ViewHostInterface::Type QtViewHost::GetType() const {
  return impl_->type_;
}

ViewInterface *QtViewHost::GetView() const {
  return impl_->view_;
}

GraphicsInterface *QtViewHost::NewGraphics() const {
  return new QtGraphics(impl_->zoom_);
}

void *QtViewHost::GetNativeWidget() const {
  return impl_->widget_;
}

void QtViewHost::Destroy() {
  delete this;
}

void QtViewHost::SetView(ViewInterface *view) {
  impl_->Detach();
  if (view == NULL) return;
  impl_->view_ = view;
  impl_->widget_ = new QGadgetWidget(view, this, impl_->composite_);
}

void QtViewHost::ViewCoordToNativeWidgetCoord(
    double x, double y, double *widget_x, double *widget_y) const {
  double zoom = impl_->view_->GetGraphics()->GetZoom();
  if (widget_x)
    *widget_x = x * zoom;
  if (widget_y)
    *widget_y = y * zoom;
}

void QtViewHost::QueueDraw() {
  ASSERT(impl_->widget_);
  impl_->widget_->update();
}

void QtViewHost::QueueResize() {
  // TODO
}

void QtViewHost::EnableInputShapeMask(bool enable) {
  if (impl_->input_shape_mask_ != enable) {
    impl_->input_shape_mask_ = enable;
    if (impl_->widget_) impl_->widget_->EnableInputShapeMask(enable);
  }
}

void QtViewHost::SetResizable(ViewInterface::ResizableMode mode) {
  // TODO:
}

void QtViewHost::SetCaption(const char *caption) {
  // TODO:
}

void QtViewHost::SetShowCaptionAlways(bool always) {
}

void QtViewHost::SetCursor(int type) {
  QCursor cursor(GetQtCursorShape(type));
  impl_->widget_->setCursor(cursor);
}

void QtViewHost::SetTooltip(const char *tooltip) {
  QToolTip::showText(QCursor::pos(), QString::fromUtf8(tooltip));
}

bool QtViewHost::ShowView(bool modal, int flags,
                          Slot1<void, int> *feedback_handler) {
  return impl_->ShowView(modal, flags, feedback_handler);
 }

void QtViewHost::CloseView() {
  // DetailsView will be only hiden here since it may be reused later.
  // window_ will be freed when SetView is called
  if (impl_->window_) {
    impl_->window_->close();
  }
}

bool QtViewHost::ShowContextMenu(int button) {
  ASSERT(impl_->view_);
  QMenu menu;
  QtMenu qt_menu(&menu);
  impl_->view_->OnAddContextMenuItems(&qt_menu);
  if (!menu.isEmpty()) {
    menu.exec(QCursor::pos());
    return true;
  } else {
    return false;
  }
}

void QtViewHost::BeginMoveDrag(int button) {
}

void QtViewHost::Alert(const char *message) {
  QMessageBox::information(NULL,
                           impl_->view_->GetCaption().c_str(),
                           message);
}

bool QtViewHost::Confirm(const char *message) {
  int ret = QMessageBox::question(NULL,
                                  impl_->view_->GetCaption().c_str(),
                                  message,
                                  QMessageBox::Yes| QMessageBox::No,
                                  QMessageBox::Yes);
  return ret == QMessageBox::Yes;
}

std::string QtViewHost::Prompt(const char *message,
                                const char *default_value) {
  QString s= QInputDialog::getText(NULL,
                                   impl_->view_->GetCaption().c_str(),
                                   message,
                                   QLineEdit::Normal);
  return s.toStdString();
}

int QtViewHost::GetDebugMode() const {
  return impl_->debug_mode_;
}

#include "qt_view_host.moc"

} // namespace qt
} // namespace ggadget