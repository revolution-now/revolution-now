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
#include "aliases.hpp"
#include "config-files.hpp"
#include "coord.hpp"
#include "errors.hpp"
#include "gfx.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "screen.hpp"

// Revolution Now (config)
#include "../config/ucl/art.inl"

// base
#include "base/keyval.hpp"

// magic enum
#include "magic_enum.hpp"

// abseil
#include "absl/container/flat_hash_map.h"

namespace rn {

namespace {

absl::flat_hash_map<e_image, Texture> g_images;

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
    ASSIGN_CHECK_OPT( image_tx,
                      base::lookup( g_images, *image ) );
    auto image_delta = image_tx.size();
    auto win_rect    = main_window_logical_rect();
    auto dest_coord  = centered( image_delta, win_rect );
    copy_texture( image_tx, tx, dest_coord );
  }

  maybe<e_image> image{};
};

ImagePlane g_image_plane;

// This will cause all images to be loaded into memory but the
// resulting textures will not be owned by this module, so there
// is no need for a corresponding `release` function.
void init_images() {
  for( auto image : magic_enum::enum_values<e_image>() ) {
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
  ASSIGN_CHECK_OPT( tx, base::lookup( g_images, which ) );
  return tx;
}

} // namespace rn
