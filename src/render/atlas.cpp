/****************************************************************
**atlas.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-04.
*
* Description: Creates a texture atlas.
*
*****************************************************************/
#include "atlas.hpp"

// render
#include "rect-pack.hpp"

// gfx
#include "gfx/image-analysis.hpp"
#include "gfx/image.hpp"

// base
#include "base/error.hpp"

using namespace std;

namespace rr {

using ::base::maybe;
using ::gfx::image;
using ::gfx::rect;
using ::gfx::size;

/****************************************************************
** AtlasMap
*****************************************************************/
AtlasMap::AtlasMap( std::vector<gfx::rect> rects,
                    std::vector<gfx::rect> trimmed_rects )
  : rects_( std::move( rects ) ),
    trimmed_rects_( std::move( trimmed_rects ) ) {
  CHECK_EQ( rects_.size(), trimmed_rects_.size() );
}

rect const& AtlasMap::lookup( int const id ) const {
  CHECK( id >= 0 && id < int( rects_.size() ) );
  return rects_[id];
}

rect const& AtlasMap::trimmed_bounds( int const id ) const {
  CHECK( id >= 0 && id < int( trimmed_rects_.size() ) );
  return trimmed_rects_[id];
}

/****************************************************************
** AtlasBuilder
*****************************************************************/
int AtlasBuilder::ImageBuilder::add_sprite(
    rect const r ) const {
  CHECK( !atlas_builder_.images_.empty() );
  AtlasImage& atlas_img = atlas_builder_.images_.back();
  ++atlas_img.count;
  int const id = int( atlas_builder_.rects_.size() );
  atlas_builder_.rects_.push_back( r );
  rect const trimmed =
      find_trimmed_bounds_in( atlas_img.img, r );
  CHECK( trimmed.is_inside( r ) );
  // We want to store the trimmed rect relative to the untrimmed
  // one (not relative to the atlas image origin) because it is
  // more useful that way.
  atlas_builder_.trimmed_rects_.push_back(
      trimmed.point_becomes_origin( r.origin ) );
  return id;
}

AtlasBuilder::ImageBuilder AtlasBuilder::add_image(
    image img ) & {
  images_.push_back(
      AtlasImage{ .img = std::move( img ), .count = 0 } );
  return ImageBuilder( *this );
}

maybe<Atlas> AtlasBuilder::build( size const max_size ) const {
  CHECK( rects_.size() == trimmed_rects_.size() );
  // First pack the rects.
  vector<rect> packed_rects = rects_;
  UNWRAP_RETURN( packed_size,
                 pack_rects( packed_rects, max_size ) );
  CHECK( rects_.size() == packed_rects.size() );

  // Now copy them to a large image.
  image atlas_img = gfx::new_empty_image( packed_size );
  int id          = 0;
  for( AtlasImage const& img_and_count : images_ ) {
    image const& src_img = img_and_count.img;
    for( int j = 0; j < img_and_count.count; ++j ) {
      rect const& src             = rects_[id];
      gfx::point const& dst_point = packed_rects[id].origin;
      CHECK( src.is_inside( rect{
        .origin = {}, .size = src_img.size_pixels() } ) );
      CHECK( ( rect{ .origin = {},
                     .size   = atlas_img.size_pixels() } )
                 .contains( dst_point ) );
      atlas_img.blit_from( src_img, src, dst_point );
      ++id;
    }
  }

  return Atlas{
    .img  = std::move( atlas_img ),
    .dict = AtlasMap( std::move( packed_rects ),
                      std::move( trimmed_rects_ ) ) };
}

} // namespace rr
