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

  virtual void draw( Texture const& tx, Coord coord ) const = 0;
  // This is the physical size of the object in pixels.
  ND virtual Delta delta() const = 0;
  // Given a position, returns a bounding rect.  We need to be
  // given a position here because Objects don't know their
  // position, only their size.
  ND virtual Rect rect( Coord position ) const {
    return Rect::from( position, delta() );
  }
  // Returns true is the input was handled.
  ND virtual bool input( input::event_t const& /*unused*/ ) {
    return false;
  }
};

} // namespace rn::ui
