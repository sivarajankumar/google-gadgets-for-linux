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

#include "main_view_decorator_base.h"

#include <string>
#include <algorithm>
#include "logger.h"
#include "common.h"
#include "elements.h"
#include "gadget_consts.h"
#include "gadget.h"
#include "main_loop_interface.h"
#include "options_interface.h"
#include "signals.h"
#include "slot.h"
#include "view.h"
#include "view_host_interface.h"
#include "view_element.h"
#include "button_element.h"
#include "div_element.h"
#include "img_element.h"
#include "label_element.h"
#include "text_frame.h"
#include "messages.h"
#include "menu_interface.h"

namespace ggadget {

static const double kVDMainMinimizedHeight = 26;
static const double kVDMainIconHeight = 30;
static const double kVDMainIconWidth = 30;
static const double kVDMainIconMarginH = 4;
static const double kVDMainCaptionMarginH = 4;
static const double kVDMainButtonMargin = 1;

static const unsigned int kVDShowTimeout = 200;
static const unsigned int kVDHideTimeout = 500;
static const double kVDMainFrozenOpacity = 0.5;

class  MainViewDecoratorBase::Impl {
 public:
  struct ButtonInfo {
    const char *tooltip;
    const char *normal;
    const char *over;
    const char *down;

    void (Impl::*handler)(void);
  };

  Impl(MainViewDecoratorBase *owner, const char *option_prefix,
       bool show_minimized_background)
    : owner_(owner),
      minimized_(false),
      popped_out_(false),
      menu_button_clicked_(false),
      button_box_position_(MainViewDecoratorBase::TOP_RIGHT),
      button_box_orientation_(MainViewDecoratorBase::HORIZONTAL),
      popout_direction_(MainViewDecoratorBase::POPOUT_TO_LEFT),
      decorator_show_hide_timer_(0),
      decorator_show_timeout_(kVDShowTimeout),
      decorator_hide_timeout_(kVDHideTimeout),
      buttons_div_(new DivElement(NULL, owner, NULL)),
      minimized_bkgnd_(show_minimized_background ?
                       (new ImgElement(NULL, owner, NULL)) : NULL),
      minimized_icon_(new ImgElement(NULL, owner, NULL)),
      minimized_caption_(new LabelElement(NULL, owner, NULL)),
      original_child_view_(NULL),
      plugin_flags_connection_(NULL) {
    ASSERT(option_prefix && *option_prefix);
  }

  void InitDecorator() {
    // Initialize elements for minimized mode.
    if (minimized_bkgnd_) {
      minimized_bkgnd_->SetSrc(Variant(kVDMainBackgroundMinimized));
      minimized_bkgnd_->SetStretchMiddle(true);
      minimized_bkgnd_->SetPixelX(0);
      minimized_bkgnd_->SetRelativePinY(0.5);
      minimized_bkgnd_->SetPixelHeight(kVDMainMinimizedHeight);
      minimized_bkgnd_->SetVisible(false);
      minimized_bkgnd_->SetEnabled(true);
      minimized_bkgnd_->ConnectOnClickEvent(
          NewSlot(this, &Impl::OnPopInOutButtonClicked));
      owner_->InsertDecoratorElement(minimized_bkgnd_, false);
    }

    minimized_icon_->SetRelativePinY(0.5);
    minimized_icon_->SetVisible(false);
    minimized_icon_->SetEnabled(true);
    minimized_icon_->ConnectOnClickEvent(
        NewSlot(this, &Impl::OnPopInOutButtonClicked));
    owner_->InsertDecoratorElement(minimized_icon_, false);

    minimized_caption_->GetTextFrame()->SetSize(10);
    minimized_caption_->GetTextFrame()->SetColor(Color::kWhite, 1);
    minimized_caption_->GetTextFrame()->SetWordWrap(false);
    minimized_caption_->GetTextFrame()->SetTrimming(
        CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
    minimized_caption_->GetTextFrame()->SetVAlign(
        CanvasInterface::VALIGN_MIDDLE);
    minimized_caption_->SetRelativePinY(0.5);
    minimized_caption_->SetVisible(false);
    minimized_caption_->SetEnabled(true);
    minimized_caption_->ConnectOnClickEvent(
        NewSlot(this, &Impl::OnPopInOutButtonClicked));
    owner_->InsertDecoratorElement(minimized_caption_, false);

    buttons_div_->SetRelativePinX(1);
    buttons_div_->SetPixelPinY(0);
    buttons_div_->SetRelativeX(1);
    buttons_div_->SetPixelY(0);
    buttons_div_->SetBackgroundMode(
        DivElement::BACKGROUND_MODE_STRETCH_MIDDLE);
    buttons_div_->SetBackground(Variant(kVDButtonBackground));
    buttons_div_->SetVisible(false);
    owner_->InsertDecoratorElement(buttons_div_, false);

    Elements *elements = buttons_div_->GetChildren();
    for (size_t i = 0; i < MainViewDecoratorBase::NUMBER_OF_BUTTONS; ++i) {
      ButtonElement *button = new ButtonElement(buttons_div_, owner_, NULL);
      button->SetTooltip(GM_(kButtonsInfo[i].tooltip));
      button->SetImage(Variant(kButtonsInfo[i].normal));
      button->SetOverImage(Variant(kButtonsInfo[i].over));
      button->SetDownImage(Variant(kButtonsInfo[i].down));
      button->SetVisible(true);
      button->ConnectOnClickEvent(NewSlot(this, kButtonsInfo[i].handler));
      elements->InsertElement(button, NULL);
    }

    owner_->ConnectOnMouseOverEvent(NewSlot(this, &Impl::OnMouseOver));
    owner_->ConnectOnMouseOutEvent(NewSlot(this, &Impl::OnMouseOut));
  }

  ~Impl() {
    if (decorator_show_hide_timer_)
      owner_->ClearTimeout(decorator_show_hide_timer_);
    if (plugin_flags_connection_)
      plugin_flags_connection_->Disconnect();
  }

  void LayoutButtons() {
    Elements *elements = buttons_div_->GetChildren();
    double width = kVDMainButtonMargin;
    double height = kVDMainButtonMargin;
    int count = elements->GetCount();
    for (int i = 0; i < count; ++i) {
      BasicElement *button = elements->GetItemByIndex(i);
      button->Layout();
      if (button->IsVisible()) {
        if (button_box_orientation_ == MainViewDecoratorBase::HORIZONTAL) {
          button->SetPixelY(0);
          button->SetPixelX(width);
          width += button->GetPixelWidth();
          height = std::max(height, button->GetPixelHeight());
        } else {
          button->SetPixelX(0);
          button->SetPixelY(height);
          height += button->GetPixelHeight();
          width = std::max(width, button->GetPixelWidth());
        }
      }
    }
    buttons_div_->SetPixelWidth(width + kVDMainButtonMargin);
    buttons_div_->SetPixelHeight(height + kVDMainButtonMargin);
  }

  void UpdatePopInOutButton() {
    bool unexpand =
        (popout_direction_ == MainViewDecoratorBase::POPOUT_TO_LEFT ?
         popped_out_ : !popped_out_);
    Elements *elements = buttons_div_->GetChildren();
    ButtonElement *btn = down_cast<ButtonElement *>(
        elements->GetItemByIndex(POP_IN_OUT_BUTTON));
    btn->SetImage(Variant(
        unexpand ? kVDButtonPopInNormal : kVDButtonPopOutNormal));
    btn->SetOverImage(Variant(
        unexpand ? kVDButtonPopInOver : kVDButtonPopOutOver));
    btn->SetDownImage(Variant(
        unexpand ? kVDButtonPopInDown : kVDButtonPopOutDown));
  }

  void OnBackButtonClicked() {
    Gadget *gadget = owner_->GetGadget();
    if (gadget)
      gadget->OnCommand(Gadget::CMD_TOOLBAR_BACK);
  }

  void OnForwardButtonClicked() {
    Gadget *gadget = owner_->GetGadget();
    if (gadget)
      gadget->OnCommand(Gadget::CMD_TOOLBAR_FORWARD);
  }

  void OnPopInOutButtonClicked() {
    if (popped_out_)
      on_popin_signal_();
    else
      on_popout_signal_();
  }

  void OnMenuButtonClicked() {
    menu_button_clicked_ = true;
    owner_->GetViewHost()->ShowContextMenu(MouseEvent::BUTTON_LEFT);
  }

  void OnCloseButtonClicked() {
    if (popped_out_)
      on_popin_signal_();
    owner_->PostCloseSignal();
  }

  void OnPluginFlagsChanged(int flags) {
    Elements *elements = buttons_div_->GetChildren();
    elements->GetItemByIndex(BACK_BUTTON)->SetVisible(
        flags & Gadget::PLUGIN_FLAG_TOOLBAR_BACK);
    elements->GetItemByIndex(FORWARD_BUTTON)->SetVisible(
        flags & Gadget::PLUGIN_FLAG_TOOLBAR_FORWARD);
    LayoutButtons();
  }

  void ClearDecoratorShowHideTimer() {
    if (decorator_show_hide_timer_) {
      owner_->ClearTimeout(decorator_show_hide_timer_);
      decorator_show_hide_timer_ = 0;
    }
  }

  void OnMouseOver() {
    ClearDecoratorShowHideTimer();
    if (decorator_show_timeout_ > 0) {
      decorator_show_hide_timer_ = owner_->SetTimeout(
          NewSlot(owner_, &MainViewDecoratorBase::OnShowDecorator),
          decorator_show_timeout_);
    } else if (decorator_show_timeout_ == 0) {
      owner_->OnShowDecorator();
    }
  }

  void OnMouseOut() {
    ClearDecoratorShowHideTimer();
    if (decorator_hide_timeout_ > 0) {
      decorator_show_hide_timer_ = owner_->SetTimeout(
          NewSlot(owner_, &MainViewDecoratorBase::OnHideDecorator),
          decorator_hide_timeout_);
    } else if (decorator_hide_timeout_ == 0) {
      owner_->OnHideDecorator();
    }
  }

  void OnMinimizedChanged() {
    SaveMinimizedState();
    if (minimized_bkgnd_)
      minimized_bkgnd_->SetVisible(minimized_);
    minimized_icon_->SetVisible(minimized_);
    minimized_caption_->SetVisible(minimized_);

    View *child = owner_->GetChildView();
    if (child) {
      SimpleEvent event(minimized_ ? Event::EVENT_MINIMIZE :
                        Event::EVENT_RESTORE);
      child->OnOtherEvent(event);
    }
  }

  void SaveMinimizedState() {
    // Don't use GetGadget(), which might return the gadget for popout view.
    // It's not what we want.
    View *child = owner_->GetChildView();
    Gadget *gadget = child ? child->GetGadget() : NULL;
    if (gadget) {
      OptionsInterface *opt = gadget->GetOptions();
      opt->PutInternalValue("main_view_minimized", Variant(minimized_));
      DLOG("Save main view minimized state for gadget %d: %s",
           gadget->GetInstanceID(), minimized_ ? "true" : "false");
    }
  }

  void LoadMinimizedState() {
    // Don't use GetGadget(), which might return the gadget for popout view.
    // It's not what we want.
    View *child = owner_->GetChildView();
    Gadget *gadget = child ? child->GetGadget() : NULL;
    if (gadget) {
      OptionsInterface *opt = gadget->GetOptions();
      Variant var = opt->GetInternalValue("main_view_minimized");
      if (var.type() == Variant::TYPE_BOOL &&
          minimized_ != VariantValue<bool>()(var)) {
        owner_->SetMinimized(!minimized_);
      }
    }
  }

 public:
  MainViewDecoratorBase *owner_;

  bool minimized_;
  bool popped_out_;
  bool menu_button_clicked_;

  MainViewDecoratorBase::ButtonBoxPosition button_box_position_;
  MainViewDecoratorBase::ButtonBoxOrientation button_box_orientation_;
  MainViewDecoratorBase::PopOutDirection popout_direction_;
  int decorator_show_hide_timer_;
  int decorator_show_timeout_;
  int decorator_hide_timeout_;

  DivElement *buttons_div_;
  ImgElement *minimized_bkgnd_;
  ImgElement *minimized_icon_;
  LabelElement *minimized_caption_;

  View *original_child_view_;
  Connection *plugin_flags_connection_;

  Signal0<void> on_popin_signal_;;
  Signal0<void> on_popout_signal_;;

  static const ButtonInfo kButtonsInfo[];
};

const MainViewDecoratorBase::Impl::ButtonInfo
MainViewDecoratorBase::Impl::kButtonsInfo[] = {
  {
    "VD_BACK_BUTTON_TOOLTIP",
    kVDButtonBackNormal,
    kVDButtonBackOver,
    kVDButtonBackDown,
    &MainViewDecoratorBase::Impl::OnBackButtonClicked
  },
  {
    "VD_FORWARD_BUTTON_TOOLTIP",
    kVDButtonForwardNormal,
    kVDButtonForwardOver,
    kVDButtonForwardDown,
    &MainViewDecoratorBase::Impl::OnForwardButtonClicked
  },
  {
    "VD_POP_IN_OUT_BUTTON_TOOLTIP",
    kVDButtonPopOutNormal,
    kVDButtonPopOutOver,
    kVDButtonPopOutDown,
    &MainViewDecoratorBase::Impl::OnPopInOutButtonClicked,
  },
  {
    "VD_MENU_BUTTON_TOOLTIP",
    kVDButtonMenuNormal,
    kVDButtonMenuOver,
    kVDButtonMenuDown,
    &MainViewDecoratorBase::Impl::OnMenuButtonClicked,
  },
  {
    "VD_CLOSE_BUTTON_TOOLTIP",
    kVDButtonCloseNormal,
    kVDButtonCloseOver,
    kVDButtonCloseDown,
    &MainViewDecoratorBase::Impl::OnCloseButtonClicked,
  }
};

MainViewDecoratorBase::MainViewDecoratorBase(ViewHostInterface *host,
                                             const char *option_prefix,
                                             bool allow_x_margin,
                                             bool allow_y_margin,
                                             bool show_minimized_background)
  : ViewDecoratorBase(host, option_prefix, allow_x_margin, allow_y_margin),
    impl_(new Impl(this, option_prefix, show_minimized_background)) {
  impl_->InitDecorator();
}

MainViewDecoratorBase::~MainViewDecoratorBase() {
  delete impl_;
  impl_ = NULL;
}

void MainViewDecoratorBase::SetButtonVisible(ButtonId button_id, bool visible) {
  if (button_id >= 0 && button_id < NUMBER_OF_BUTTONS) {
    Elements *elements = impl_->buttons_div_->GetChildren();
    elements->GetItemByIndex(button_id)->SetVisible(visible);
    impl_->LayoutButtons();
  }
}

bool MainViewDecoratorBase::IsButtonVisible(ButtonId button_id) const {
  if (button_id >= 0 && button_id < NUMBER_OF_BUTTONS) {
    const Elements *elements = impl_->buttons_div_->GetChildren();
    return elements->GetItemByIndex(button_id)->IsVisible();
  }
  return false;
}

void MainViewDecoratorBase::SetButtonBoxVisible(bool visible) {
  impl_->buttons_div_->SetVisible(visible);
}

bool MainViewDecoratorBase::IsButtonBoxVisible() const {
  return impl_->buttons_div_->IsVisible();
}

void MainViewDecoratorBase::SetButtonBoxPosition(ButtonBoxPosition position) {
  impl_->button_box_position_ = position;
  switch (position) {
    case TOP_LEFT:
      impl_->buttons_div_->SetPixelX(0);
      impl_->buttons_div_->SetRelativePinX(0);
      impl_->buttons_div_->SetPixelY(0);
      impl_->buttons_div_->SetRelativePinY(0);
      break;
    case TOP_RIGHT:
      impl_->buttons_div_->SetRelativeX(1);
      impl_->buttons_div_->SetRelativePinX(1);
      impl_->buttons_div_->SetPixelY(0);
      impl_->buttons_div_->SetRelativePinY(0);
      break;
    case BOTTOM_LEFT:
      impl_->buttons_div_->SetPixelX(0);
      impl_->buttons_div_->SetRelativePinX(0);
      impl_->buttons_div_->SetRelativeY(1);
      impl_->buttons_div_->SetRelativePinY(1);
      break;
    case BOTTOM_RIGHT:
      impl_->buttons_div_->SetRelativeX(1);
      impl_->buttons_div_->SetRelativePinX(1);
      impl_->buttons_div_->SetRelativeY(1);
      impl_->buttons_div_->SetRelativePinY(1);
      break;
  }
  UpdateViewSize();
}

MainViewDecoratorBase::ButtonBoxPosition
MainViewDecoratorBase::GetButtonBoxPosition() const {
  return impl_->button_box_position_;
}

void MainViewDecoratorBase::SetButtonBoxOrientation(
    ButtonBoxOrientation orientation) {
  impl_->button_box_orientation_ = orientation;
  impl_->LayoutButtons();
  UpdateViewSize();
}

MainViewDecoratorBase::ButtonBoxOrientation
MainViewDecoratorBase::GetButtonBoxOrientation() const {
  return impl_->button_box_orientation_;
}

void MainViewDecoratorBase::GetButtonBoxSize(double *width,
                                             double *height) const {
  if (width) *width = impl_->buttons_div_->GetPixelWidth();
  if (height) *height = impl_->buttons_div_->GetPixelHeight();
}

void MainViewDecoratorBase::SetPopOutDirection(PopOutDirection direction) {
  impl_->popout_direction_ = direction;
  impl_->UpdatePopInOutButton();
}

void MainViewDecoratorBase::SetMinimized(bool minimized) {
  if (impl_->minimized_ != minimized) {
    impl_->minimized_ = minimized;
    SetChildViewVisible(!minimized);
    impl_->OnMinimizedChanged();
  }
}

bool MainViewDecoratorBase::IsMinimized() const {
  return impl_->minimized_;
}

bool MainViewDecoratorBase::IsPoppedOut() const {
  return impl_->popped_out_;
}

MainViewDecoratorBase::PopOutDirection
MainViewDecoratorBase::GetPopOutDirection() const {
  return impl_->popout_direction_;
}

void MainViewDecoratorBase::SetDecoratorShowHideTimeout(int show_timeout,
                                                        int hide_timeout) {
  impl_->decorator_show_timeout_ = show_timeout;
  impl_->decorator_hide_timeout_ = hide_timeout;
}

Connection *MainViewDecoratorBase::ConnectOnPopIn(Slot0<void> *slot) {
  return impl_->on_popin_signal_.Connect(slot);
}

Connection *MainViewDecoratorBase::ConnectOnPopOut(Slot0<void> *slot) {
  return impl_->on_popout_signal_.Connect(slot);
}

Gadget *MainViewDecoratorBase::GetGadget() const {
  if (impl_->popped_out_ && impl_->original_child_view_)
    return impl_->original_child_view_->GetGadget();
  return ViewDecoratorBase::GetGadget();
}

bool MainViewDecoratorBase::OnAddContextMenuItems(MenuInterface *menu) {
  bool result = false;
  View *child = GetChildView();
  if (child)
    result = child->OnAddContextMenuItems(menu);
  else if (impl_->original_child_view_)
    result = impl_->original_child_view_->OnAddContextMenuItems(menu);

  // Always show decorator menu items if the menu is activated by clicking
  // menu button.
  if (impl_->menu_button_clicked_) {
    result = true;
    impl_->menu_button_clicked_ = false;
  }

  if (result)
    OnAddDecoratorMenuItems(menu);

  return result;
}

EventResult MainViewDecoratorBase::OnOtherEvent(const Event &event) {
  EventResult result = EVENT_RESULT_UNHANDLED;
  Event::Type type = event.GetType();
  if (type == Event::EVENT_POPOUT && !impl_->popped_out_) {
    impl_->original_child_view_ = GetChildView();
    impl_->popped_out_ = true;
    impl_->UpdatePopInOutButton();
    SetChildViewFrozen(true);
    SetChildViewOpacity(kVDMainFrozenOpacity);
    if (impl_->minimized_) {
      // Send restore event to the view, so that it can be restored correctly
      // in popout view.
      SimpleEvent restore_event(Event::EVENT_RESTORE);
      ViewDecoratorBase::OnOtherEvent(restore_event);
    }
    // Let child view to handle popout event after it has been popped out.
    result = ViewDecoratorBase::OnOtherEvent(event);
  } else if (type == Event::EVENT_POPIN && impl_->popped_out_) {
    // Let child view to handle popin event first.
    result = ViewDecoratorBase::OnOtherEvent(event);
    impl_->original_child_view_ = NULL;
    impl_->popped_out_ = false;
    impl_->UpdatePopInOutButton();
    SetChildViewFrozen(false);
    SetChildViewOpacity(1);
    if (impl_->minimized_) {
      // Send minimize event to the view, so that it can be minimized correctly
      // in popin view.
      SimpleEvent minimize_event(Event::EVENT_MINIMIZE);
      ViewDecoratorBase::OnOtherEvent(minimize_event);
    }
  } else {
    result = ViewDecoratorBase::OnOtherEvent(event);
  }
  return result;
}

void MainViewDecoratorBase::SetCaption(const char *caption) {
  impl_->minimized_caption_->GetTextFrame()->SetText(caption);
  ViewDecoratorBase::SetCaption(caption);
}

bool MainViewDecoratorBase::ShowDecoratedView(
    bool modal, int flags, Slot1<void, int> *feedback_handler) {
  // Send minimize or restore event to child view, to make sure it's in correct
  // state, especially for iGoogle gadgets.
  View *child = GetChildView();
  if (child) {
    SimpleEvent event(impl_->minimized_ ? Event::EVENT_MINIMIZE :
                      Event::EVENT_RESTORE);
    child->OnOtherEvent(event);
  }
  return ViewDecoratorBase::ShowDecoratedView(modal, flags, feedback_handler);
}

void MainViewDecoratorBase::OnChildViewChanged() {
  // Call through parent's method.
  ViewDecoratorBase::OnChildViewChanged();
  if (impl_->plugin_flags_connection_) {
    impl_->plugin_flags_connection_->Disconnect();
    impl_->plugin_flags_connection_ = NULL;
  }

  Gadget *gadget = GetGadget();
  if (gadget) {
    impl_->plugin_flags_connection_ = gadget->ConnectOnPluginFlagsChanged(
        NewSlot(impl_, &Impl::OnPluginFlagsChanged));
    impl_->OnPluginFlagsChanged(gadget->GetPluginFlags());

    impl_->minimized_icon_->SetSrc(
        Variant(gadget->GetManifestInfo(kManifestSmallIcon)));
    impl_->minimized_icon_->SetPixelWidth(
        std::min(kVDMainIconWidth, impl_->minimized_icon_->GetSrcWidth()));
    impl_->minimized_icon_->SetPixelHeight(
        std::min(kVDMainIconHeight, impl_->minimized_icon_->GetSrcHeight()));

    View *child = gadget->GetMainView();
    if (child)
      impl_->minimized_caption_->GetTextFrame()->SetText(child->GetCaption());
  } else {
    impl_->OnPluginFlagsChanged(0);
    // Keep the icon unchanged.
  }

  impl_->LoadMinimizedState();
}

void MainViewDecoratorBase::DoLayout() {
  // Call through parent's method.
  ViewDecoratorBase::DoLayout();

  if (impl_->minimized_ != !IsChildViewVisible()) {
    impl_->minimized_ = !IsChildViewVisible();
    impl_->OnMinimizedChanged();
  }

  double top, left, bottom, right;
  GetMargins(&top, &left, &bottom, &right);
  double width = GetWidth();
  double height = GetHeight();
  double client_center = top + (height - top - bottom) / 2.0;

  if (impl_->minimized_bkgnd_) {
    impl_->minimized_bkgnd_->SetPixelX(left);
    impl_->minimized_bkgnd_->SetPixelWidth(width - left - right);
    // relative pin y = 0.5
    impl_->minimized_bkgnd_->SetPixelY(client_center);
    // pixel height is fixed.
  }

  impl_->minimized_icon_->SetPixelX(left + kVDMainIconMarginH);
  impl_->minimized_icon_->SetPixelY(client_center);

  impl_->minimized_caption_->SetPixelX(impl_->minimized_icon_->GetPixelX() +
                                       impl_->minimized_icon_->GetPixelWidth() +
                                       kVDMainIconMarginH);
  impl_->minimized_caption_->SetPixelY(client_center);
  impl_->minimized_caption_->SetPixelWidth(
      width - right - kVDMainCaptionMarginH -
      impl_->minimized_caption_->GetPixelX());
}

void MainViewDecoratorBase::GetMinimumClientExtents(double *width,
                                                    double *height) const {
  ViewDecoratorBase::GetMinimumClientExtents(width, height);
  if (impl_->minimized_) {
    *width = std::max(*width, kVDMainIconWidth + kVDMainIconMarginH * 2);
    *height = std::max(*height, kVDMainMinimizedHeight);
  }
}

void MainViewDecoratorBase::GetClientExtents(double *width,
                                             double *height) const {
  if (impl_->minimized_) {
    *height = kVDMainMinimizedHeight;
    if (*width <= 0)
      GetChildViewSize(width, NULL);
  }
}

bool MainViewDecoratorBase::OnClientSizing(double *width, double *height) {
  if (impl_->minimized_)
    *height = kVDMainMinimizedHeight;
  return true;
}

void MainViewDecoratorBase::OnAddDecoratorMenuItems(MenuInterface *menu) {
  // Do nothing.
}

void MainViewDecoratorBase::OnShowDecorator() {
  SetButtonBoxVisible(true);
}

void MainViewDecoratorBase::OnHideDecorator() {
  SetButtonBoxVisible(false);
}

} // namespace ggadget
