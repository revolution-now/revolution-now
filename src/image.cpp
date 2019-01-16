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
#include "errors.hpp"
#include "geo-types.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "sdl-util.hpp"
#include "util.hpp"

// abseil
#include "absl/container/flat_hash_map.h"

namespace rn {

namespace {

// These textures are not owned by us.
absl::flat_hash_map<e_image, Ref<Texture>> g_images;

fs::path const& image_file_path( e_image image ) {
  switch( image ) {
    case +e_image::old_world: return config_art.images.old_world;
  }
  SHOULD_NOT_BE_HERE;
}

struct ImagePlane : public Plane {
  // Implement Plane
  bool enabled() const override {
    check_invariants();
    return enabled_;
  }
  // Implement Plane
  bool covers_screen() const override { return true; }
  // Implement Plane
  void draw( Texture const& tx ) const override {
    check_invariants();
    CHECK( enabled_ );
    auto& image_tx    = val_or_die( g_images, *image );
    auto  image_delta = texture_delta( image_tx );
    auto  tx_rect = Rect::from( Coord{}, texture_delta( tx ) );
    auto  dest_coord = centered( image_delta, tx_rect );
    copy_texture( image_tx, tx, dest_coord );
  }

  void check_invariants() const {
    if( enabled_ ) CHECK( image.has_value() );
  }

  bool         enabled_{false};
  Opt<e_image> image{};
};

ImagePlane g_image_plane;

} // namespace

void load_all_images() {
  for( auto image : values<e_image> ) {
    g_images.insert(
        {image, load_texture( image_file_path( image ) )} );
  }
  // Sanity check. This checks that the underlying SDL texture
  // pointers are non-null.
  for( auto const& p : g_images ) CHECK( p.second.get().get() );
}

Plane* image_plane() { return &g_image_plane; }

void image_plane_enable( bool enable ) {
  g_image_plane.enabled_ = enable;
  g_image_plane.check_invariants();
}

void image_plane_set( e_image image ) {
  logger->info( "setting image background to {}", image );
  g_image_plane.image = image;
}

} // namespace rn
