/****************************************************************
**image.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-16.
*
* Description: Handles loading and displaying fullscreen images.
*
*****************************************************************/
#include "image.hpp"

// Revolution Now
#include "compositor.hpp"
#include "config-files.hpp"
#include "coord.hpp"
#include "error.hpp"
#include "gfx.hpp"
#include "init.hpp"
#include "logger.hpp"
#include "plane.hpp"
#include "screen.hpp"

// Revolution Now (config)
#include "../config/rcl/art.inl"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"

// C++ standard library
#include <unordered_map>

namespace rn {

using namespace std;

namespace {

unordered_map<e_image, Texture> g_images;

fs::path const& image_file_path( e_image image ) {
  switch( image ) {
    case e_image::europe: return config_art.images.europe;
  }
  SHOULD_NOT_BE_HERE;
}

struct ImagePlane : public Plane {
  // Implement Plane
  bool covers_screen() const override { return true; }
  // Implement Plane
  void draw( Texture& tx ) const override {
    clear_texture_transparent( tx );
    UNWRAP_CHECK( image_tx, base::lookup( g_images, *image ) );
    auto image_delta = image_tx.size();
    UNWRAP_CHECK(
        normal_area,
        compositor::section( compositor::e_section::normal ) );
    auto dest_coord = centered( image_delta, normal_area );
    copy_texture( image_tx, tx, dest_coord );
  }

  maybe<e_image> image{};
};

ImagePlane g_image_plane;

// This will cause all images to be loaded into memory but the
// resulting textures will not be owned by this module, so there
// is no need for a corresponding `release` function.
void init_images() {
  for( auto image : refl::enum_values<e_image> ) {
    g_images.insert( { image, Texture::load_image(
                                  image_file_path( image ) ) } );
  }
}

void cleanup_images() {
  for( auto& p : g_images ) p.second.free();
}

REGISTER_INIT_ROUTINE( images );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
Plane* image_plane() { return &g_image_plane; }

void image_plane_set( e_image image ) {
  lg.debug( "setting image background to {}", image );
  g_image_plane.image = image;
}

Texture const& image( e_image which ) {
  UNWRAP_CHECK( tx, base::lookup( g_images, which ) );
  return tx;
}

} // namespace rn
