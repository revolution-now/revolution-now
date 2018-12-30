/****************************************************************
**plane.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-30.
*
* Description: Rendering planes.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "input.hpp"
#include "sdl-util.hpp"

// base-util
#include "base-util/non-copyable.hpp"

namespace rn {

struct Plane : public util::non_copy_non_move {
  enum class id {
    world = 0, // land, units, colonies, etc.
    panel,     // the info panel on the right
    colony,    // colony view
    europe,    // the old world
    menu,      // the menus at the top of screen
    image,     // any of the fullscreen pics displayed
    window,    // the windows
    console,   // the developer console
    /**/
    count // must always be last
  };

  static Plane& get( id id_ );

  // Is this plane enabled.  If not, it won't be rendered.
  bool virtual enabled() const = 0;

  // Will rendering this plane cover all pixels?  If so, then
  // planes under it will not be rendered.
  bool virtual covers_screen() const = 0;

  // Draw the plane to the given texture; note that this texture
  // may not need to be referenced explicitly because it will
  // already be set as the default rendering target before this
  // function is called. The texture is initialized with zero
  // alpha everywhere once during initialization, but thereafter
  // will not be touched; i.e., it will only be modified through
  // these draw() function calls. That way, this function can
  // rely on the texture having the same state that it had at the
  // end of the last such call (if that happens to be useful).
  void virtual draw( Texture const& tx ) const = 0;

  // Accept input; returns true/false depending on whether the
  // input was handled or not.  If it was handled (true) then
  // this input will not be given to any further planes.
  ND bool virtual input( input::event_t const& event );
};

void initialize_planes();
void destroy_planes();

void draw_all_planes( Texture const& tx = Texture() );

bool send_input_to_planes( input::event_t const& event );

} // namespace rn
