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

// Abseil
#include "absl/container/flat_hash_set.h"

namespace rn {
class Texture;
}

namespace rn::ui {

class Object {
public:
  Object()                = default;
  virtual ~Object()       = default;
  Object( Object const& ) = delete;
  Object( Object&& )      = delete;

  Object& operator=( Object const& ) = delete;
  Object& operator=( Object&& ) = delete;

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
  Opt<T*> cast_safe() {
    auto* casted = dynamic_cast<T*>( this );
    if( !casted ) return std::nullopt;
    return casted;
  }

  template<typename T>
  Opt<T const*> cast_safe() const {
    auto* casted = dynamic_cast<T const*>( this );
    if( !casted ) return std::nullopt;
    return casted;
  }

  virtual void draw( Texture const& tx, Coord coord ) const = 0;
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

  using ObjectSet = absl::flat_hash_set<ui::Object*>;
  // This method is not const because it needs to insert
  // non-const pointers to child objects into the set, which in
  // turn is needed because we may need to call non-const methods
  // on those objects.
  virtual void children_under_coord( Coord      where,
                                     ObjectSet& objects ) = 0;

  // This one is only used for the "simple" method of autopadding
  // (as opposed to the "fancy" method). The jury is still out as
  // to which one is better. If the "simple" method proves best
  // then the "fancy" method (and all the complicated logic that
  // goes with it) can be deleted; otherwise this interface
  // method can be deleted.
  virtual bool needs_padding() const { return false; }

  /**************************************************************
  ** Input handlers
  ***************************************************************/
  ND virtual bool on_key( input::key_event_t const& event );
  ND virtual bool on_wheel(
      input::mouse_wheel_event_t const& event );
  ND virtual bool on_mouse_move(
      input::mouse_move_event_t const& event );
  ND virtual bool on_mouse_button(
      input::mouse_button_event_t const& event );
  virtual void on_mouse_leave( Coord from );
  virtual void on_mouse_enter( Coord to );
};

} // namespace rn::ui
