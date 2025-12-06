/****************************************************************
**ui.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-03-16.
*
* Description: Fundamentals for UI.
*
*****************************************************************/
#pragma once

// base
#include "base/maybe.hpp"

// gfx
#include "gfx/coord.hpp" // FIXME

namespace rr {
struct Renderer;
}

namespace rn::input {
struct event_t;
struct key_event_t;
struct mouse_wheel_event_t;
struct mouse_move_event_t;
struct mouse_drag_event_t;
struct mouse_button_event_t;
struct win_event_t;
struct resolution_event_t;
struct cheat_event_t;
struct unknown_event_t;
struct quit_event_t;
} // namespace rn::input

namespace rn::ui {

/****************************************************************
** ui::object
*****************************************************************/
class object {
 public:
  object()          = default;
  virtual ~object() = default;

  object( object const& )            = delete;
  object( object&& )                 = delete;
  object& operator=( object const& ) = delete;
  object& operator=( object&& )      = delete;

  /**************************************************************
  ** Casting.
  ***************************************************************/
  template<typename T>
  T* cast() {
    T* const casted = dynamic_cast<T*>( this );
    CHECK( casted != nullptr );
    return casted;
  }

  template<typename T>
  T const* cast() const {
    T const* const casted = dynamic_cast<T const*>( this );
    CHECK( casted != nullptr );
    return casted;
  }

  template<typename T>
  base::maybe<T&> cast_safe() {
    T* const casted = dynamic_cast<T*>( this );
    if( !casted ) return base::nothing;
    return *casted;
  }

  template<typename T>
  base::maybe<T const&> cast_safe() const {
    T const* const casted = dynamic_cast<T const*>( this );
    if( !casted ) return base::nothing;
    return *casted;
  }

  /**************************************************************
  ** State Advancing.
  ***************************************************************/
  // Called once per frame.
  virtual void advance_state();

  /**************************************************************
  ** Sizing.
  ***************************************************************/
  // This is the physical size of the object in pixels.
  [[nodiscard]] virtual Delta delta() const = 0;

  // Given a position, returns a bounding rect.  We need to be
  // given a position here because Objects don't know their
  // position, only their size.
  [[nodiscard]] virtual Rect bounds( Coord position ) const {
    return Rect::from( position, delta() );
  }

  /**************************************************************
  ** Drawing.
  ***************************************************************/
  // TODO: remove this coord parameter and just add a renderer
  // shift before calling a child view, that way the view can al-
  // ways draw relative to the origin of zero.
  virtual void draw( rr::Renderer& renderer,
                     Coord coord ) const = 0;

  /**************************************************************
  ** Input.
  ***************************************************************/
  // Returns true if the input was handled. Mouse coordinates
  // will be adjusted to be relative to upper-left corner of the
  // view. Mouse events where the cursor is outside the bounds of
  // the view will not be sent to this function.
  [[nodiscard]] virtual bool input( input::event_t const& e );

  /**************************************************************
  ** Padding.
  ***************************************************************/
  // As a rule of thumb, this should only be overridden to be
  // false for nodes that satisfy the following:
  //
  //   1. They are "composite" nodes in the view graph, meaning
  //      that they have children.
  //   2. They don't themselves draw anything.
  //
  virtual bool needs_padding() const { return true; }

  /**************************************************************
  ** Disablement.
  ***************************************************************/
  bool disabled() const { return disabled_; }
  void set_disabled( bool disabled ) { disabled_ = disabled; }

  /**************************************************************
  ** Input handlers
  ***************************************************************/
  [[nodiscard]] virtual bool on_key(
      input::key_event_t const& event );

  [[nodiscard]] virtual bool on_wheel(
      input::mouse_wheel_event_t const& event );

  [[nodiscard]] virtual bool on_mouse_move(
      input::mouse_move_event_t const& event );

  // This is for when you want to receive raw drag events, which
  // is sometimes but not always. E.g., the drag-and-drop frame-
  // work will receive these so that you don't have to directly.
  [[nodiscard]] virtual bool on_mouse_drag(
      input::mouse_drag_event_t const& event );

  [[nodiscard]] virtual bool on_mouse_button(
      input::mouse_button_event_t const& event );

  [[nodiscard]] virtual bool on_win_event(
      input::win_event_t const& event );

  [[nodiscard]] virtual bool on_resolution_event(
      input::resolution_event_t const& event );

  [[nodiscard]] virtual bool on_cheat_event(
      input::cheat_event_t const& event );

  [[nodiscard]] virtual bool on_unknown_event(
      input::unknown_event_t const& event );

  [[nodiscard]] virtual bool on_quit(
      input::quit_event_t const& event );

  /**************************************************************
  ** Special Input handlers
  ***************************************************************/
  virtual void on_mouse_leave( Coord from );

  virtual void on_mouse_enter( Coord to );

 private:
  bool disabled_ = false;
};

} // namespace rn::ui
