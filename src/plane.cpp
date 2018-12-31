/****************************************************************
**plane.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-30.
*
* Description: Rendering planes.
*
*****************************************************************/
#include "plane.hpp"

// Revolution Now
#include "aliases.hpp"
#include "render.hpp"
#include "window.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/misc.hpp"

// C++ standard library
#include <algorithm>
#include <array>

using namespace std;

namespace rn {

namespace {

constexpr auto num_planes =
    static_cast<size_t>( Plane::id::count );

// Planes are rendered from 0 --> count.
array<ObserverPtr<Plane>, num_planes> planes;
array<Texture, num_planes>            textures;

ObserverPtr<Plane>& plane( Plane::id id_ ) {
  auto idx = static_cast<size_t>( id_ );
  CHECK( idx < planes.size() );
  return planes[idx];
}

struct InactivePlane : public Plane {
  InactivePlane() {}
  bool enabled() const override { return false; }
  bool covers_screen() const override { return false; }
  void draw( Texture const& /*unused*/ ) const override {}
};

InactivePlane dummy;

} // namespace

Plane& Plane::get( id id_ ) { return *plane( id_ ); }

bool Plane::input( input::event_t const& /*unused*/ ) {
  return false;
}

void initialize_planes() {
  // By default, all planes are dummies, unless we provide an
  // object below.
  planes.fill( ObserverPtr<Plane>( &dummy ) );

  plane( Plane::id::viewport ).reset( viewport_plane() );
  plane( Plane::id::panel ).reset( panel_plane() );
  // plane( Plane::id::colony ).reset( colony_plane() );
  // plane( Plane::id::europe ).reset( europe_plane() );
  // plane( Plane::id::menu ).reset( menu_plane() );
  // plane( Plane::id::image ).reset( image_plane() );
  plane( Plane::id::effects ).reset( effects_plane() );
  plane( Plane::id::window ).reset( window_plane() );
  // plane( Plane::id::console ).reset( console_plane() );

  // No plane must be null, they must all point to a valid Plane
  // object even if it is the dummy above.
  for( auto p : planes ) { CHECK( p ); }

  // Initialize the textures. These are intended to cover the
  // entire screen and are measured in logical coordinates (which
  // means, typically, that they will be smaller than the full
  // screen resolution).
  for( auto& tx : textures ) {
    tx = create_screen_sized_texture();
    clear_texture_transparent( tx );
  }
}

void destroy_planes() {
  // This actually just destroys the textures, since the planes
  // will be held by value as global variables elsewhere.
  for( auto& tx : textures ) tx = {};
}

void draw_all_planes( Texture const& tx ) {
  // First find the last plane that will render (opaquely) over
  // every pixel. If one is found then we will not render any
  // planes before it. This is technically not necessary, but
  // saves rendering work by avoiding to render things that would
  // go unseen anyway.
  auto   blocking = L( _->enabled() && _->covers_screen() );
  size_t start    = 0;
  if( auto coverer = util::find_last_if( planes, blocking ) )
    start = coverer.value();

  clear_texture_black( tx );

  CHECK( start < num_planes );
  for( size_t idx = start; idx < num_planes; ++idx ) {
    if( !planes[idx]->enabled() ) continue;
    set_render_target( textures[idx] );
    planes[idx]->draw( textures[idx] );
    copy_texture( textures[idx], tx );
  }
}

bool send_input_to_planes( input::event_t const& event ) {
  for( size_t idx = planes.size(); idx > 0; --idx )
    if( planes[idx - 1]->input( event ) ) return true;
  return false;
}

} // namespace rn
