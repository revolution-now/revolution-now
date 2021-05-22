/****************************************************************
**views.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-03-16.
*
* Description: Views for populating windows in the UI.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "color.hpp"
#include "line-editor.hpp"
#include "nation.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "tx.hpp"
#include "ui-enums.hpp"
#include "ui.hpp"
#include "unit.hpp"
#include "utype.hpp"
#include "view.hpp"
#include "waitable.hpp"

// C++ standard library
#include <memory>

namespace rn::ui {

// NOTE: Don't put anymore views in here that are specific to
// game logic.

/****************************************************************
** Fundamental Views
*****************************************************************/
class CompositeView : public View {
public:
  // Implement Object
  void draw( Texture& tx, Coord coord ) const override;
  // Implement Object
  Delta delta() const override;

  bool on_key( input::key_event_t const& event ) override;
  bool on_wheel(
      input::mouse_wheel_event_t const& event ) override;
  bool on_mouse_move(
      input::mouse_move_event_t const& event ) override;
  bool on_mouse_button(
      input::mouse_button_event_t const& event ) override;

  // Implement ui::Object
  void on_mouse_leave( Coord from ) override;
  void on_mouse_enter( Coord to ) override;

  virtual int count() const = 0;

  virtual std::unique_ptr<View>& mutable_at( int idx )   = 0;
  virtual Coord                  pos_of( int idx ) const = 0;

  // By default this view will be eligible for auto-padding be-
  // tween the views inside of it. However this is not always de-
  // sirable and so can be turned off. Turning this off only
  // means that padding will not be added on the inside of this
  // view and one level deep; i.e., this will not apply on its
  // border or further down recursively.
  virtual bool can_pad_immediate_children() const {
    return true;
  }

  virtual PositionedViewConst at( int idx ) const;
  virtual PositionedView      at( int idx );

  // This has to be implemented if the view holds any state that
  // must be recomputed if the child views are changed in either
  // number of geometry. For example, this may be called if the
  // child views change size.
  virtual void notify_children_updated() = 0;

  struct iter {
    CompositeView* cview;
    int            idx;
    auto           operator*() { return cview->at( idx ); }
    void           operator++() { ++idx; }
    bool operator!=( iter const& rhs ) { return rhs.idx != idx; }
  };
  struct citer {
    CompositeView const* cview;
    int                  idx;
    auto                 operator*() { return cview->at( idx ); }
    void                 operator++() { ++idx; }
    bool                 operator!=( citer const& rhs ) {
      return rhs.idx != idx;
    }
  };

  iter  begin() { return iter{ this, 0 }; }
  iter  end() { return iter{ this, count() }; }
  citer begin() const { return citer{ this, 0 }; }
  citer end() const { return citer{ this, count() }; }

private:
  bool dispatch_mouse_event( input::event_t const& event );
};

class CompositeSingleView : public CompositeView {
public:
  CompositeSingleView() = default;
  CompositeSingleView( std::unique_ptr<View> view, Coord coord );

  // Implement CompositeView
  Coord pos_of( int idx ) const override;
  // Implement CompositeView
  std::unique_ptr<View>& mutable_at( int idx ) override;
  // Implement CompositeView
  int count() const override { return 1; }

  View*       single() { return view_.get(); }
  View const* single() const { return view_.get(); }

  void set_view( std::unique_ptr<View> view, Coord coord ) {
    view_  = std::move( view );
    coord_ = coord;
    notify_children_updated();
  }

private:
  std::unique_ptr<View> view_;
  Coord                 coord_;
};

class VectorView : public CompositeView {
public:
  VectorView() {}

  VectorView( std::vector<OwningPositionedView> views )
    : views_( std::move( views ) ) {}

  // Implement CompositeView
  Coord pos_of( int idx ) const override;
  // Implement CompositeView
  std::unique_ptr<View>& mutable_at( int idx ) override;
  // Implement CompositeView
  int count() const override { return int( views_.size() ); }

  void push_back( OwningPositionedView view ) {
    views_.push_back( std::move( view ) );
  }

  OwningPositionedView const& operator[]( int idx ) const {
    CHECK( idx >= 0 && idx < int( views_.size() ) );
    return views_[idx];
  }
  OwningPositionedView& operator[]( int idx ) {
    CHECK( idx >= 0 && idx < int( views_.size() ) );
    return views_[idx];
  }

private:
  std::vector<OwningPositionedView> views_;
};

// Just a view for holding a collection of other views but which
// has a fixed size and is invisible.
class InvisibleView : public VectorView {
public:
  InvisibleView( Delta                             size,
                 std::vector<OwningPositionedView> views )
    : VectorView( std::move( views ) ), size_( size ) {}

  // Implement CompositeView
  void notify_children_updated() override {}

  // Implement Object
  Delta delta() const override { return size_; }

  void set_delta( Delta const& size ) { size_ = size; }

private:
  // We need to store the size because it cannot be derived from
  // the child views.
  Delta size_;
};

/****************************************************************
** Simple Views
*****************************************************************/
class SolidRectView : public View {
public:
  SolidRectView( Color color ) : color_( color ), delta_{} {}

  SolidRectView( Color color, Delta delta )
    : color_( color ), delta_( delta ) {}

  // Implement Object
  void draw( Texture& tx, Coord coord ) const override;
  // Implement Object
  Delta delta() const override { return delta_; }

  void set_delta( Delta const& delta ) { delta_ = delta; }

protected:
  Color color_;
  Delta delta_;
};

class OneLineStringView : public View {
public:
  OneLineStringView( std::string msg, Color color, bool shadow );

  // Implement Object
  void draw( Texture& tx, Coord coord ) const override;
  // Implement Object
  Delta delta() const override { return tx_.size(); }

  std::string const& msg() const { return msg_; }

  bool needs_padding() const override { return true; }

protected:
  std::string msg_;
  Texture     tx_;
};

class TextView : public View {
public:
  TextView( std::string_view msg, TextMarkupInfo const& m_info,
            TextReflowInfo const& r_info );

  // Implement Object
  void draw( Texture& tx, Coord coord ) const override;
  // Implement Object
  Delta delta() const override { return tx_.size(); }

  bool needs_padding() const override { return true; }

protected:
  Texture tx_;
};

class ButtonBaseView : public View {
public:
  enum class e_type { standard, blink };

  ButtonBaseView( std::string label );
  ButtonBaseView( std::string label, e_type type );
  ButtonBaseView( std::string label, Delta size_in_blocks );
  ButtonBaseView( std::string label, Delta size_in_blocks,
                  e_type type );

  // Implement Object
  void draw( Texture& tx, Coord coord ) const override final;
  // Implement Object
  Delta delta() const override final { return pressed_.size(); }

  bool needs_padding() const override { return true; }

protected:
  enum class button_state { down, up, hover, disabled };

  void set_state( button_state state ) { state_ = state; }
  button_state state() const { return state_; }

  // NOTE: It just so happens that it is safe to set the type
  // after creation, but that may change in the future if more
  // complicated types are added.
  void   set_type( e_type type ) { type_ = type; }
  e_type type() const { return type_; }

private:
  void render( std::string const& label, Delta size_in_blocks );

  button_state state_{ button_state::up };

  e_type type_;

  Texture pressed_{};
  Texture hover_{}; // used for blinking when type is `blinking`
  Texture unpressed_{};
  Texture disabled_{};
};

class SpriteView : public View {
public:
  SpriteView( e_tile tile ) : tile_( tile ) {}

  // Implement Object
  void draw( Texture& tx, Coord coord ) const override;
  // Implement Object
  Delta delta() const override {
    return Delta{ 1_w, 1_h } * lookup_sprite( tile_ ).scale;
  }

private:
  e_tile tile_;
};

class LineEditorView : public View {
public:
  using OnChangeFunc = std::function<void( std::string const& )>;

  LineEditorView( int              chars_wide,
                  std::string_view initial_text );
  LineEditorView( int chars_wide, std::string_view initial_text,
                  OnChangeFunc on_change );
  LineEditorView( e_font font, W pixels_wide,
                  OnChangeFunc on_change, Color fg, Color bg,
                  std::string_view prompt,
                  std::string_view initial_text );

  // Implement Object
  void draw( Texture& tx, Coord coord ) const override;
  // Implement Object
  Delta delta() const override { return background_.size(); }

  bool needs_padding() const override { return true; }

  // Implement UI.
  bool on_key( input::key_event_t const& event ) override;

  std::string const& text() const { return current_rendering_; }
  // Absolute cursor position.
  int cursor_pos() const { return line_editor_.pos(); }

  void set_on_change_fn( OnChangeFunc on_change ) {
    on_change_ = std::move( on_change );
  }

  void set_pixel_size( Delta const& size );

  void clear();
  // Leaving off cursor position, it will attempt to keep it
  // where it is, unless it is out of bounds in which case it
  // will be put at the end. One can specify -1 for the cursor
  // position which means one-past-the-ene; -2 places the cursor
  // over the last character, etc. Regardless of the `cursor_pos`
  // specified, it will always be clamped to the bounds of the
  // new string.
  void set( std::string_view new_string,
            maybe<int>       cursor_pos = nothing );

private:
  void render_background( Delta const& size );
  void update_visible_string();

  std::string         prompt_;
  Color               fg_;
  Color               bg_;
  e_font              font_;
  OnChangeFunc        on_change_;
  LineEditor          line_editor_;
  LineEditorInputView input_view_;
  Texture             background_;
  std::string         current_rendering_;
  W                   cursor_width_;
};

/****************************************************************
** Derived Views
*****************************************************************/
class PlainMessageBoxView : public CompositeSingleView {
public:
  static std::unique_ptr<PlainMessageBoxView> create(
      std::string_view msg, waitable_promise<> on_close );

  // Implement CompositeView
  void notify_children_updated() override {}

  // Should call the static create method instead.
  PlainMessageBoxView( std::unique_ptr<TextView> tview,
                       waitable_promise<>        on_close );

  bool on_key( input::key_event_t const& event ) override;

private:
  waitable_promise<> on_close_;
};

// Should not be used directly; will generally be inserted
// automatically by the auto-pad mechanism.
class PaddingView : public CompositeSingleView {
public:
  PaddingView( std::unique_ptr<View> view, int pixels, bool l,
               bool r, bool u, bool d );

  // Implement Object
  Delta delta() const override { return delta_; }

  // Implement CompositeView
  void notify_children_updated() override;

  bool can_pad_immediate_children() const override;

private:
  int   pixels_;
  bool  l_, r_, u_, d_;
  Delta delta_;
};

class ButtonView : public ButtonBaseView {
public:
  using OnClickFunc = std::function<void( void )>;
  ButtonView( std::string label, OnClickFunc on_click );
  ButtonView( std::string label, Delta size_in_blocks,
              OnClickFunc on_click );

  bool on_mouse_move(
      input::mouse_move_event_t const& event ) override;
  bool on_mouse_button(
      input::mouse_button_event_t const& event ) override;
  void on_mouse_leave( Coord from ) override;

  void enable( bool enabled = true );
  bool enabled() const;

  void blink( bool enabled = true );

private:
  OnClickFunc on_click_;
};

class OkCancelView : public CompositeView {
public:
  OkCancelView( ButtonView::OnClickFunc on_ok,
                ButtonView::OnClickFunc on_cancel );

  // Implement CompositeView
  Coord pos_of( int idx ) const override;
  // Implement CompositeView
  std::unique_ptr<View>& mutable_at( int idx ) override;
  // Implement CompositeView
  int count() const override { return 2; }
  // Implement CompositeView
  void notify_children_updated() override {}

  ButtonView* ok_button() { return ok_ref_; }
  ButtonView* cancel_button() { return cancel_ref_; }

private:
  std::unique_ptr<View> ok_;
  std::unique_ptr<View> cancel_;
  // Cache these to avoid dynamic_casts.
  ButtonView* ok_ref_;
  ButtonView* cancel_ref_;
};

class OkButtonView : public CompositeSingleView {
public:
  OkButtonView( ButtonView::OnClickFunc on_ok );

  // Implement CompositeView
  void notify_children_updated() override {}

  ButtonView* ok_button() { return ok_ref_; }

private:
  // Cache this to avoid dynamic_casts.
  ButtonView* ok_ref_;
};

// VerticalArrayView: a view that wraps a list of views and dis-
// plays them vertically. On creation, one can specify how to
// justify the views, either left, right, or center. This ques-
// tion of justification arises because the views in the array
// will generally have different widths.
class VerticalArrayView : public VectorView {
public:
  enum class align { left, right, center };
  VerticalArrayView( std::vector<std::unique_ptr<View>> views,
                     align                              how );

  // Implement CompositeView
  void notify_children_updated() override;

  void recompute_child_positions();

private:
  align alignment_;
};

// HorizontalArrayView: a view that wraps a list of views and
// displays them horizontally. On creation, one can specify how
// to justify the views, either up, down, or middle. This ques-
// tion of justification arises because the views in the array
// will generally have different heights.
class HorizontalArrayView : public VectorView {
public:
  enum class align { up, down, middle };
  HorizontalArrayView( std::vector<std::unique_ptr<View>> views,
                       align                              how );

  // Implement CompositeView
  void notify_children_updated() override;

  void recompute_child_positions();

private:
  align alignment_;
};

class OkCancelAdapterView : public VerticalArrayView {
public:
  using OnClickFunc = std::function<void( e_ok_cancel )>;
  OkCancelAdapterView( std::unique_ptr<View> view,
                       OnClickFunc           on_click );
};

enum class e_option_active { inactive, active };

class OptionSelectItemView : public CompositeView {
public:
  OptionSelectItemView( std::string msg );

  // Implement CompositeView
  Coord pos_of( int idx ) const override;
  // Implement CompositeView
  std::unique_ptr<View>& mutable_at( int idx ) override;
  // Implement CompositeView
  int count() const override { return 2; }
  // Implement CompositeView
  void notify_children_updated() override {}

  void set_active( e_option_active active ) { active_ = active; }

  std::string const& line() const {
    return foreground_active_->cast<OneLineStringView>()->msg();
  }

  void grow_to( W w );

  bool can_pad_immediate_children() const override {
    return false;
  }

private:
  e_option_active       active_;
  std::unique_ptr<View> background_active_;
  std::unique_ptr<View> background_inactive_;
  std::unique_ptr<View> foreground_active_;
  std::unique_ptr<View> foreground_inactive_;
};

class OptionSelectView : public VectorView {
public:
  OptionSelectView( std::vector<std::string> const& options,
                    int initial_selection );

  // Implement CompositeView
  void notify_children_updated() override {}

  bool on_key( input::key_event_t const& event ) override;

  std::string const& get_selected() const;

  void grow_to( W w );

  bool can_pad_immediate_children() const override {
    return false;
  }
  bool needs_padding() const override { return true; }

private:
  OptionSelectItemView*       get_view( int item );
  OptionSelectItemView const* get_view( int item ) const;
  void                        set_selected( int item );

  int selected_;
};

class FakeUnitView : public CompositeSingleView {
public:
  FakeUnitView( e_unit_type type, e_nation nation,
                e_unit_orders orders );

  // Implement Object
  void draw( Texture& tx, Coord coord ) const override;
  // Implement CompositeView
  void notify_children_updated() override {}

  e_unit_orders orders() const { return orders_; }
  void set_orders( e_unit_orders orders ) { orders_ = orders; }

  bool needs_padding() const override { return true; }

private:
  e_unit_type   type_;
  e_nation      nation_;
  e_unit_orders orders_;
};

class ClickableView : public CompositeSingleView {
public:
  using OnClick = std::function<void( void )>;

  ClickableView( std::unique_ptr<View> view, OnClick on_click );

  bool on_mouse_button(
      input::mouse_button_event_t const& event ) override;

  // Implement CompositeView
  void notify_children_updated() override {}

private:
  OnClick on_click_;
};

class BorderView : public CompositeSingleView {
public:
  // padding is how many pixels between inner view and border.
  BorderView( std::unique_ptr<View> view, Color color,
              int padding, bool on_initially );

  // Implement Object
  void draw( Texture& tx, Coord coord ) const override;
  // Implement Object
  Delta delta() const override;

  // Implement CompositeView
  void notify_children_updated() override {}

  // This one does its own padding around the border.
  bool can_pad_immediate_children() const override {
    return false;
  }

  // Padding outside of border.
  bool needs_padding() const override { return true; }

  void toggle() { on_ = !on_; }
  void on( bool v ) { on_ = v; }
  bool is_on() const { return on_; }

private:
  Color color_;
  bool  on_;
  int   padding_;
};

} // namespace rn::ui
