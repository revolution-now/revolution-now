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

#include "core-config.hpp"

// Revolution Now
#include "input.hpp"

// render
#include "render/renderer.hpp"

namespace rn::ui {

class Object {
 public:
  Object()                = default;
  virtual ~Object()       = default;
  Object( Object const& ) = delete;
  Object( Object&& )      = delete;

  Object& operator=( Object const& ) = delete;
  Object& operator=( Object&& )      = delete;

  template<typename T>
  T* cast() {
    auto* casted = dynamic_cast<T*>( this );
    CHECK( casted != nullptr );
    return casted;
  }

  template<typename T>
  T const* cast() const {
    auto* casted = dynamic_cast<T const*>( this );
    CHECK( casted != nullptr );
    return casted;
  }

  template<typename T>
  maybe<T*> cast_safe() {
    auto* casted = dynamic_cast<T*>( this );
    if( !casted ) return nothing;
    return casted;
  }

  template<typename T>
  maybe<T const*> cast_safe() const {
    auto* casted = dynamic_cast<T const*>( this );
    if( !casted ) return nothing;
    return casted;
  }

  // Called once per frame.
  virtual void advance_state();

  // TODO: remove this coord parameter and just add a renderer
  // shift before calling a child view, that way the view can al-
  // ways draw relative to the origin of zero.
  virtual void draw( rr::Renderer& renderer,
                     Coord coord ) const = 0;
  // This is the physical size of the object in pixels.
  ND virtual Delta delta() const = 0;
  // Given a position, returns a bounding rect.  We need to be
  // given a position here because Objects don't know their
  // position, only their size.
  ND virtual Rect rect( Coord position ) const {
    return Rect::from( position, delta() );
  }
  // Returns true is the input was handled. Mouse coordinates
  // will be adjusted to be relative to upper-left corner of the
  // view. Mouse events where the cursor is outside the bounds of
  // the view will not be sent to this function.
  ND virtual bool input( input::event_t const& e );

  // This one is only used for the "simple" method of autopadding
  // (as opposed to the "fancy" method). The jury is still out as
  // to which one is better. If the "simple" method proves best
  // then the "fancy" method (and all the complicated logic that
  // goes with it) can be deleted; otherwise this interface
  // method can be deleted.
  virtual bool needs_padding() const { return false; }

  bool disabled() const { return disabled_; }
  void set_disabled( bool disabled ) { disabled_ = disabled; }

  /**************************************************************
  ** Input handlers
  ***************************************************************/
  ND virtual bool on_key( input::key_event_t const& event );

  ND virtual bool on_wheel(
      input::mouse_wheel_event_t const& event );

  ND virtual bool on_mouse_move(
      input::mouse_move_event_t const& event );

  // This is for when you want to receive raw drag events, which
  // is sometimes but not always. E.g., the drag-and-drop frame-
  // work will receive these so that you don't have to directly.
  ND virtual bool on_mouse_drag(
      input::mouse_drag_event_t const& event );

  ND virtual bool on_mouse_button(
      input::mouse_button_event_t const& event );

  virtual void on_mouse_leave( Coord from );

  virtual void on_mouse_enter( Coord to );

  ND virtual bool on_win_event(
      input::win_event_t const& event );

  ND virtual bool on_resolution_event(
      input::resolution_event_t const& event );

  ND virtual bool on_cheat_event(
      input::cheat_event_t const& event );

 private:
  bool disabled_ = false;
};

} // namespace rn::ui
