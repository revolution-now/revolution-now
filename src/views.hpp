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
#include "co-combinator.hpp"
#include "line-editor.hpp"
#include "text.hpp"
#include "ui-enums.hpp"
#include "ui.hpp"
#include "view.hpp"
#include "wait.hpp"

// config
#include "config/tile-enum-fwd.hpp"

// ss
#include "ss/nation.rds.hpp"
#include "ss/unit-type.hpp"
#include "ss/unit.hpp"

// gfx
#include "gfx/pixel.hpp"

// C++ standard library
#include <memory>

namespace rr {
struct ITextometer;
}

namespace rn {

struct Colony;
struct Dwelling;
struct IVisibility;
struct SSConst;

enum class e_native_unit_type;
enum class e_tribe;
enum class e_tile;

} // namespace rn

namespace rn::ui {

// NOTE: Don't put anymore views in here that are specific to
// game logic.

TextMarkupInfo const& default_text_markup_info();
TextReflowInfo const& default_text_reflow_info();

/****************************************************************
** Fundamental Views
*****************************************************************/
class CompositeView : public View {
 public:
  // Implement Object
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;
  // Implement Object
  Delta delta() const override;

  // Override ui::Object.
  virtual void advance_state() override;

  bool on_key( input::key_event_t const& event ) override;

  bool on_wheel(
      input::mouse_wheel_event_t const& event ) override;

  bool on_mouse_move(
      input::mouse_move_event_t const& event ) override;

  bool on_mouse_button(
      input::mouse_button_event_t const& event ) override;

  bool on_mouse_drag(
      input::mouse_drag_event_t const& event ) override;

  bool on_win_event( input::win_event_t const& event ) override;

  // Implement ui::Object
  void on_mouse_leave( Coord from ) override;
  void on_mouse_enter( Coord to ) override;

  virtual int count() const = 0;

  virtual std::unique_ptr<View>& mutable_at( int idx ) = 0;
  virtual Coord pos_of( int idx ) const                = 0;

  // By default this view will be eligible for auto-padding be-
  // tween the views inside of it. However this is not always de-
  // sirable and so can be turned off. Turning this off only
  // means that padding will not be added on the inside of this
  // view and one level deep; i.e., this will not apply on its
  // border or further down recursively.
  virtual bool can_pad_immediate_children() const {
    return true;
  }

  // Composite views generally don't draw anything themselves,
  // thus they don't need padding.
  bool needs_padding() const override { return false; }

  virtual PositionedViewConst at( int idx ) const;
  virtual PositionedView at( int idx );

  // This should be called to notify all of the child views that
  // one of their children may have been updated and that they
  // should recompute their state.
  void children_updated();

  struct iter {
    CompositeView* cview;
    int idx;
    auto operator*() { return cview->at( idx ); }
    void operator++() { ++idx; }
    bool operator!=( iter const& rhs ) { return rhs.idx != idx; }
  };
  struct citer {
    CompositeView const* cview;
    int idx;
    auto operator*() { return cview->at( idx ); }
    void operator++() { ++idx; }
    bool operator!=( citer const& rhs ) {
      return rhs.idx != idx;
    }
  };

  iter begin() { return iter{ this, 0 }; }
  iter end() { return iter{ this, count() }; }
  citer begin() const { return citer{ this, 0 }; }
  citer end() const { return citer{ this, count() }; }

 protected:
  // This has to be implemented if the view holds any state that
  // must be recomputed if the child views are changed in either
  // number of geometry. For example, this may be called if the
  // child views change size.
  //
  // Note that this is protected because it should not be called
  // by client code; instead, if a child in a view hierarchy has
  // been changed, just all the children_updated method once at
  // the end and it should recursively update everything.
  virtual void notify_children_updated() = 0;

 private:
  // As usual, pos is relative to this view.
  maybe<PositionedViewConst> first_view_under_cursor(
      Coord pos ) const;
  maybe<PositionedView> first_view_under_cursor( Coord pos );

  // This is reused by any mouse event that involves a motion
  // (which should inherit from mouse_move_event); in those cases
  // we need to send on_mouse_leave and on_mouse_enter events.
  void send_mouse_enter_leave_events(
      input::mouse_move_event_t const& event );

  bool dispatch_mouse_event( input::event_t const& event );
};

// This is a view that holds another view by reference and just
// forwards everything to it. This is used while transitioning
// the framework from a heap/unique_ptr based approach to a ref-
// erence based approach.
class RefView : public View {
 public:
  RefView( View& view ) : view_( view ) {}

  View const& referenced() const { return view_; }
  View& referenced() { return view_; }

  // Implement Object
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;
  // Implement Object
  Delta delta() const override;

  // Override ui::Object.
  void advance_state() override;

  bool on_key( input::key_event_t const& event ) override;

  bool on_wheel(
      input::mouse_wheel_event_t const& event ) override;

  bool on_mouse_move(
      input::mouse_move_event_t const& event ) override;

  bool on_mouse_button(
      input::mouse_button_event_t const& event ) override;

  bool on_mouse_drag(
      input::mouse_drag_event_t const& event ) override;

  bool on_win_event( input::win_event_t const& event ) override;

  // Implement ui::Object
  void on_mouse_leave( Coord from ) override;
  void on_mouse_enter( Coord to ) override;
  Rect bounds( Coord position ) const override;
  bool input( input::event_t const& e ) override;
  bool needs_padding() const override;
  bool on_resolution_event(
      input::resolution_event_t const& event ) override;
  bool on_cheat_event(
      input::cheat_event_t const& event ) override;

 private:
  View& view_;
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

  View* single() { return view_.get(); }
  View const* single() const { return view_.get(); }

  void set_view( std::unique_ptr<View> view, Coord coord ) {
    view_  = std::move( view );
    coord_ = coord;
  }

 private:
  std::unique_ptr<View> view_;
  Coord coord_;
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
  InvisibleView( Delta size,
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
class EmptyView : public View {
 public:
  EmptyView() = default;

  EmptyView( Delta delta ) : delta_( delta ) {}

  // Implement Object
  void draw( rr::Renderer&, Coord ) const override {}

  // Implement Object
  Delta delta() const override { return delta_; }

  void set_delta( Delta const& delta ) { delta_ = delta; }

  // No padding because this view does not draw anything.
  bool needs_padding() const override { return false; }

 protected:
  Delta delta_;
};

class SolidRectView : public View {
 public:
  SolidRectView( gfx::pixel color )
    : color_( color ), delta_{} {}

  SolidRectView( gfx::pixel color, Delta delta )
    : color_( color ), delta_( delta ) {}

  // Implement Object
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;
  // Implement Object
  Delta delta() const override { return delta_; }

  void set_delta( Delta const& delta ) { delta_ = delta; }

 protected:
  gfx::pixel color_;
  Delta delta_;
};

class OneLineStringView : public View {
 public:
  OneLineStringView( rr::ITextometer const& textometer,
                     std::string msg, gfx::pixel color );

  OneLineStringView( rr::ITextometer const& textometer,
                     std::string msg, gfx::pixel color,
                     Delta size_override );

  // Implement Object
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;
  // Implement Object
  Delta delta() const override;

  std::string const& msg() const { return msg_; }

 protected:
  std::string msg_;
  Delta view_size_;
  Delta text_size_; // rendered pixel size.
  gfx::pixel color_;
};

class TextView : public View {
 public:
  TextView( rr::ITextometer const& textometer, std::string msg );

  TextView( rr::ITextometer const& textometer, std::string msg,
            TextMarkupInfo const& m_info,
            TextReflowInfo const& r_info );

  // Implement Object
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

  // Implement Object
  Delta delta() const override;

 private:
  std::string msg_;
  Delta text_size_; // rendered pixel size.
  TextMarkupInfo markup_info_;
  TextReflowInfo reflow_info_;
};

class ButtonBaseView : public View {
 public:
  enum class e_type {
    standard,
    blink
  };

  ButtonBaseView( rr::ITextometer const& textometer,
                  std::string label );
  ButtonBaseView( rr::ITextometer const& textometer,
                  std::string label, e_type type );
  ButtonBaseView( rr::ITextometer const& textometer,
                  std::string label, Delta size_in_blocks );
  ButtonBaseView( rr::ITextometer const& textometer,
                  std::string label, Delta size_in_blocks,
                  e_type type );

  // Implement Object
  void draw( rr::Renderer& renderer,
             Coord coord ) const override final;
  // Implement Object
  Delta delta() const override final;

 protected:
  enum class button_state {
    down,
    up,
    hover,
    disabled
  };

  void set_state( button_state state ) { state_ = state; }
  button_state state() const { return state_; }

  // NOTE: It just so happens that it is safe to set the type
  // after creation, but that may change in the future if more
  // complicated types are added.
  void set_type( e_type type ) { type_ = type; }
  e_type type() const { return type_; }

 private:
  void render_disabled( rr::Renderer& renderer,
                        gfx::point where ) const;
  void render_pressed( rr::Renderer& renderer,
                       gfx::point where ) const;
  void render_unpressed( rr::Renderer& renderer,
                         gfx::point where ) const;
  void render_hover( rr::Renderer& renderer,
                     gfx::point where ) const;

  button_state state_ = button_state::up;

  std::string label_;
  e_type type_;
  Delta size_in_pixels_;
  Delta text_size_in_pixels_;
};

class SpriteView : public View {
 public:
  SpriteView( e_tile const tile ) : tile_( tile ) {}

  // Implement Object
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;
  // Implement Object
  Delta delta() const override;

 private:
  e_tile tile_;
};

class LineEditorView : public View {
 public:
  using OnChangeFunc = std::function<void( std::string const& )>;

  LineEditorView( rr::ITextometer const& textometer,
                  int chars_wide,
                  std::string_view initial_text );
  LineEditorView( rr::ITextometer const& textometer,
                  int chars_wide, std::string_view initial_text,
                  OnChangeFunc on_change );
  LineEditorView( rr::ITextometer const& textometer, e_font font,
                  W pixels_wide, OnChangeFunc on_change,
                  gfx::pixel fg, gfx::pixel bg,
                  std::string_view prompt,
                  std::string_view initial_text );

  // Implement Object
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;
  // Implement Object
  Delta delta() const override;

  // Implement UI.
  bool on_key( input::key_event_t const& event ) override;

  std::string const& text() const { return current_rendering_; }
  // Absolute cursor position.
  int cursor_pos() const { return line_editor_.pos(); }

  void set_on_change_fn( OnChangeFunc on_change ) {
    on_change_ = std::move( on_change );
  }

  void clear();
  // Leaving off cursor position, it will attempt to keep it
  // where it is, unless it is out of bounds in which case it
  // will be put at the end. One can specify -1 for the cursor
  // position which means one-past-the-ene; -2 places the cursor
  // over the last character, etc. Regardless of the `cursor_pos`
  // specified, it will always be clamped to the bounds of the
  // new string.
  void set( std::string_view new_string,
            maybe<int> cursor_pos = nothing );

 private:
  void render_background( rr::Renderer& renderer,
                          Rect const& r ) const;
  void update_visible_string();

  rr::ITextometer const& textometer_;
  std::string prompt_;
  gfx::pixel fg_;
  gfx::pixel bg_;
  e_font font_;
  OnChangeFunc on_change_;
  LineEditor line_editor_;
  LineEditorInputView input_view_;
  std::string current_rendering_;
  W cursor_width_;
};

/****************************************************************
** Derived Views
*****************************************************************/
class PlainMessageBoxView : public CompositeSingleView {
 public:
  static std::unique_ptr<PlainMessageBoxView> create(
      rr::ITextometer const& textometer, std::string_view msg,
      wait_promise<>& on_close );

  // Implement CompositeView
  void notify_children_updated() override {}

  // Should call the static create method instead.
  PlainMessageBoxView( std::unique_ptr<TextView> tview,
                       wait_promise<>& on_close );

  bool on_key( input::key_event_t const& event ) override;

 private:
  wait_promise<>& on_close_;
};

// Should not be used directly; will generally be inserted
// automatically by the auto-pad mechanism.
class PaddingView : public CompositeSingleView {
 public:
  PaddingView( std::unique_ptr<View> view, int pixels );

  PaddingView( std::unique_ptr<View> view, int pixels, bool l,
               bool r, bool u, bool d );

  // Implement Object
  Delta delta() const override { return delta_; }

  // Implement CompositeView
  void notify_children_updated() override;

  bool can_pad_immediate_children() const override;

 private:
  int pixels_;
  bool l_, r_, u_, d_;
  Delta delta_;
};

class ButtonView : public ButtonBaseView {
 public:
  using OnClickFunc = std::function<void( void )>;
  ButtonView( rr::ITextometer const& textometer,
              std::string label, OnClickFunc on_click );
  ButtonView( rr::ITextometer const& textometer,
              std::string label, Delta size_in_blocks,
              OnClickFunc on_click );

  bool on_mouse_move(
      input::mouse_move_event_t const& event ) override;
  bool on_mouse_button(
      input::mouse_button_event_t const& event ) override;
  void on_mouse_leave( Coord from ) override;

  void enable( bool enabled = true );
  bool enabled() const;

  void blink( bool enabled = true );

  void click() const;

 private:
  OnClickFunc on_click_;
};

// TODO: Rename to OkCancelView when the original is removed.
class OkCancelView2 : public CompositeView {
 public:
  OkCancelView2( rr::ITextometer const& textometer );

  // Must be immobile since it is self-referential.
  OkCancelView2( OkCancelView2&& ) = delete;

  wait<e_ok_cancel> next();

  ButtonView* ok_button() { return ok_ref_; }
  ButtonView* cancel_button() { return cancel_ref_; }

  // Implement CompositeView
  Coord pos_of( int idx ) const override;

  // Implement CompositeView
  std::unique_ptr<View>& mutable_at( int idx ) override;

  // Implement CompositeView
  int count() const override { return 2; }

  // Implement CompositeView
  void notify_children_updated() override {}

  // Override ui::Object.
  bool on_key( input::key_event_t const& event ) override;

 private:
  std::unique_ptr<View> ok_;
  std::unique_ptr<View> cancel_;
  // Cache these to avoid dynamic_casts.
  ButtonView* ok_ref_;
  ButtonView* cancel_ref_;
  // Feeds clicks to the consumer coroutine.
  co::stream<e_ok_cancel> clicks_;
};

// Deprecated.
class OkCancelView : public CompositeView {
 public:
  OkCancelView( rr::ITextometer const& textometer,
                ButtonView::OnClickFunc on_ok,
                ButtonView::OnClickFunc on_cancel );

  // Implement CompositeView
  Coord pos_of( int idx ) const override;
  // Implement CompositeView
  std::unique_ptr<View>& mutable_at( int idx ) override;
  // Implement CompositeView
  int count() const override { return 2; }
  // Implement CompositeView
  void notify_children_updated() override {}

  // Override ui::Object.
  bool on_key( input::key_event_t const& event ) override;

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
  OkButtonView( rr::ITextometer const& textometer,
                ButtonView::OnClickFunc on_ok );

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
  enum class align {
    left,
    right,
    center
  };
  VerticalArrayView( align how );

  // This will compute child positions.
  VerticalArrayView( std::vector<std::unique_ptr<View>> views,
                     align how );

  // Will add a view. After finished adding views, need to call
  // recompute_child_positions.
  void add_view( std::unique_ptr<View> view );

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
  enum class align {
    up,
    down,
    middle
  };

  HorizontalArrayView( align how );

  // This will compute child positions.
  HorizontalArrayView( std::vector<std::unique_ptr<View>> views,
                       align how );

  // Will add a view. After finished adding views, need to call
  // recompute_child_positions.
  void add_view( std::unique_ptr<View> view );

  // Implement CompositeView
  void notify_children_updated() override;

  void recompute_child_positions();

 private:
  align alignment_;
};

struct IRadioButton {
  virtual ~IRadioButton() = default;

  virtual bool on() const = 0;
  virtual void turn_off() = 0;
  virtual void turn_on()  = 0;
};

struct RadioButtonGroup {
  RadioButtonGroup() = default;

  void add( IRadioButton& button );

  void on_child_clicked( IRadioButton& self );

  void set( int idx );

  maybe<int> get_selected() const;

 private:
  // Must be immobile.
  RadioButtonGroup( RadioButtonGroup&& ) = delete;

  std::vector<IRadioButton*> buttons_;
};

struct RadioButtonView : public View, IRadioButton {
  RadioButtonView( RadioButtonGroup& group );

  // Implement Object
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

  // Implement Object
  Delta delta() const override;

  // Override ui::Object.
  bool on_mouse_button(
      input::mouse_button_event_t const& event ) override;

 public: // IRadioButton
  bool on() const override { return on_; }
  void turn_off() override { on_ = false; }
  void turn_on() override { on_ = true; }

 private:
  RadioButtonGroup& group_;
  bool on_ = false;
};

struct LabeledRadioButtonView : public HorizontalArrayView,
                                IRadioButton {
  LabeledRadioButtonView( RadioButtonGroup& group,
                          std::unique_ptr<View> label );

  // Override ui::Object.
  bool on_mouse_button(
      input::mouse_button_event_t const& event ) override;

 public: // IRadioButton
  bool on() const override { return radio_button_->on(); }
  void turn_off() override { radio_button_->turn_off(); }
  void turn_on() override { radio_button_->turn_on(); }

 private:
  RadioButtonView* radio_button_ = nullptr;
};

struct TextLabeledRadioButtonView
  : public LabeledRadioButtonView {
  TextLabeledRadioButtonView( RadioButtonGroup& group,
                              rr::ITextometer const& textometer,
                              std::string label );
};

struct CheckBoxView : public View {
  CheckBoxView( bool on = false );

  // Implement Object
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

  // Implement Object
  Delta delta() const override;

  bool on() const { return on_; }

  // Override ui::Object.
  bool on_mouse_button(
      input::mouse_button_event_t const& event ) override;

 private:
  bool on_ = false;
};

struct LabeledCheckBoxView : public HorizontalArrayView {
  LabeledCheckBoxView( std::unique_ptr<View> label,
                       bool on = false );

  bool on() const { return check_box_->on(); }

  // Override ui::Object.
  bool on_mouse_button(
      input::mouse_button_event_t const& event ) override;

 private:
  CheckBoxView* check_box_ = nullptr;
};

struct TextLabeledCheckBoxView : public LabeledCheckBoxView {
  TextLabeledCheckBoxView( rr::ITextometer const& textometer,
                           std::string label, bool on = false );
};

class OkCancelAdapterView : public VerticalArrayView {
 public:
  using OnClickFunc = std::function<void( e_ok_cancel )>;
  OkCancelAdapterView( rr::ITextometer const& textometer,
                       std::unique_ptr<View> view,
                       OnClickFunc on_click );
};

enum class e_option_active {
  inactive,
  active
};

class OptionSelectItemView : public CompositeView {
 public:
  struct Option {
    std::string name = {};
    bool enabled     = {};
  };

  OptionSelectItemView( rr::ITextometer const& textometer,
                        Option option );

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

  bool enabled() const { return enabled_; }

 private:
  e_option_active active_;
  std::unique_ptr<View> background_active_;
  std::unique_ptr<View> background_inactive_;
  std::unique_ptr<View> foreground_active_;
  std::unique_ptr<View> foreground_inactive_;
  bool enabled_ = true;
};

// TODO: reimplement this by inheriting from the VerticalAr-
// rayView.
class OptionSelectView : public VectorView {
 public:
  // If the initial selection is either not set or refers to an
  // item that is not enabled then the initial selection will be
  // set to the first enabled item. If there are no enabled items
  // then nothing will be selected.
  OptionSelectView(
      rr::ITextometer const& textometer,
      std::vector<OptionSelectItemView::Option> const& options,
      maybe<int> initial_selection );

  // Implement CompositeView
  void notify_children_updated() override {}

  // Implement ui::Object.
  bool on_key( input::key_event_t const& event ) override;

  // Implement ui::Object.
  bool on_mouse_button(
      input::mouse_button_event_t const& event ) override;

  maybe<int> get_selected() const;

  void grow_to( W w );

  bool can_pad_immediate_children() const override {
    return false;
  }

  bool enabled_option_at_point( Coord coord ) const;

 private:
  maybe<int> item_under_point( Coord coord ) const;

  OptionSelectItemView* get_view( int item );
  OptionSelectItemView const* get_view( int item ) const;
  void update_selected();

  maybe<int> selected_;
};

class FakeUnitView : public View {
 public:
  FakeUnitView( e_unit_type type, e_player player,
                unit_orders const& orders );

  // Implement Object
  Delta delta() const override;

  // Implement Object
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

  unit_orders const& orders() const { return orders_; }

  void set_orders( unit_orders const& orders ) {
    orders_ = orders;
  }

 private:
  e_unit_type const type_;
  e_player const player_;
  unit_orders orders_;
};

class FakeNativeUnitView : public View {
 public:
  FakeNativeUnitView( e_native_unit_type type, e_tribe tribe );

  // Implement Object
  Delta delta() const override;

  // Implement Object
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

 private:
  e_native_unit_type const type_;
  e_tribe const tribe_;
};

// Should work for either a real or frozen colony.
class RenderedColonyView : public View {
 public:
  RenderedColonyView( SSConst const& ss,
                      Colony const& colony ATTR_LIFETIMEBOUND );
  ~RenderedColonyView() override;

  // Implement Object
  Delta delta() const override;

  // Implement Object
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

 private:
  SSConst const& ss_;
  Colony const& colony_;
  gfx::size const size_;
  std::unique_ptr<IVisibility> viz_;
};

// Should work for either a real or frozen dwelling.
class RenderedDwellingView : public View {
 public:
  RenderedDwellingView( SSConst const& ss,
                        Dwelling const& dwelling
                            ATTR_LIFETIMEBOUND );
  ~RenderedDwellingView() override;

  // Implement Object
  Delta delta() const override;

  // Implement Object
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

 private:
  SSConst const& ss_;
  Dwelling const& dwelling_;
  gfx::size const size_;
  std::unique_ptr<IVisibility> viz_;
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

class OnInputView : public CompositeSingleView {
 public:
  using OnInput = std::function<bool( input::event_t const& )>;

  OnInputView( std::unique_ptr<View> view, OnInput on_input );

  // Implement CompositeView
  void notify_children_updated() override {}

  // Implement UI.
  bool input( input::event_t const& e ) override;

 private:
  OnInput on_input_;
};

class BorderView : public CompositeSingleView {
 public:
  // padding is how many pixels between inner view and border.
  BorderView( std::unique_ptr<View> view, gfx::pixel color,
              int padding, bool on_initially );

  // Implement Object
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;
  // Implement Object
  Delta delta() const override;

  // Implement CompositeView
  void notify_children_updated() override {}

  // This one does its own padding around the border.
  bool can_pad_immediate_children() const override {
    return false;
  }

  // Needs padding despite being a composite view because this
  // view actually draws something on its own.
  bool needs_padding() const override { return true; }

  void toggle() { on_ = !on_; }
  void on( bool v ) { on_ = v; }
  bool is_on() const { return on_; }

 private:
  gfx::pixel color_;
  bool on_;
  int padding_;
};

} // namespace rn::ui
