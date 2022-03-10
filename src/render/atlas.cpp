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
rect const& AtlasMap::lookup( int id ) const {
  DCHECK( id >= 0 && id < int( rects_.size() ) );
  return rects_[id];
}

/****************************************************************
** AtlasBuilder
*****************************************************************/
int AtlasBuilder::ImageBuilder::add_sprite(
    rect const r ) const {
  DCHECK( !atlas_builder_.images_.empty() );
  ++atlas_builder_.images_.back().count;
  int id = int( atlas_builder_.rects_.size() );
  atlas_builder_.rects_.push_back( r );
  return id;
}

AtlasBuilder::ImageBuilder AtlasBuilder::add_image(
    image img ) & {
  images_.push_back(
      AtlasImage{ .img = std::move( img ), .count = 0 } );
  return ImageBuilder( *this );
}

maybe<Atlas> AtlasBuilder::build( size max_size ) const {
  // First pack the rects.
  vector<rect> packed_rects = rects_;
  UNWRAP_RETURN( packed_size,
                 pack_rects( packed_rects, max_size ) );
  DCHECK( rects_.size() == packed_rects.size() );

  // Now copy them to a large image.
  image atlas_img = gfx::new_empty_image( packed_size );
  int   id        = 0;
  for( AtlasImage const& img_and_count : images_ ) {
    image const& src_img = img_and_count.img;
    for( int j = 0; j < img_and_count.count; ++j ) {
      rect const&       src       = rects_[id];
      gfx::point const& dst_point = packed_rects[id].origin;
      DCHECK( src.is_inside( rect{
          .origin = {}, .size = src_img.size_pixels() } ) );
      DCHECK( ( rect{ .origin = {},
                      .size   = atlas_img.size_pixels() } )
                  .contains( dst_point ) );
      atlas_img.blit_from( src_img, src, dst_point );
      ++id;
    }
  }

  return Atlas{ .img  = std::move( atlas_img ),
                .dict = AtlasMap( std::move( packed_rects ) ) };
}

} // namespace rr
