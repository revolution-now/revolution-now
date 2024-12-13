/****************************************************************
**rect-pack.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-24.
*
* Description: Rect Packer.
*
*****************************************************************/
#include "rect-pack.hpp"

using namespace std;

namespace rr {

namespace {

using ::base::maybe;
using ::base::nothing;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

struct packer {
  enum class [[nodiscard]] e_status {
    failed,
    good
  };

  e_status pack_rect( rect& what, rect const allowed ) {
    if( allowed.size.h < what.size.h ||
        allowed.size.w < what.size.w )
      return e_status::failed;
    point const& where = allowed.origin;
    what.origin        = where;
    size_used_         = size_used_.max_with(
        ( where + what.size ).distance_from_origin() );
    ++cur_;
    return e_status::good;
  }

  e_status pack_cols( rect allowed ) {
    // Iterate through the columns.
    while( true ) {
      if( cur_ == end_ ) return e_status::good;

      // Try to pack the first rect.
      rect& first     = **cur_;
      e_status status = pack_rect( first, allowed );
      if( status == e_status::failed ) return status;

      // Move to the remaining space in this column. Note that
      // this is not a failure if this fails to find a spot for
      // the next rect since we can always then try the next col-
      // umn.
      (void)pack_rows( rect{
        .origin = { .x = allowed.origin.x,
                    .y = allowed.origin.y + first.size.h },
        .size   = { .w = first.size.w,
                    .h = allowed.size.h - first.size.h } } );

      // Move to the next column in this row.
      allowed =
          rect{ .origin = { .x = allowed.origin.x + first.size.w,
                            .y = allowed.origin.y },
                .size   = { .w = allowed.size.w - first.size.w,
                            .h = allowed.size.h } };
    }
  }

  e_status pack_rows( rect allowed ) {
    // Iterate through the rows.
    while( true ) {
      if( cur_ == end_ ) return e_status::good;
      if( allowed.size.negative() ) return e_status::failed;
      rect& first = **cur_;

      // Start the first row, whose height will be the height of
      // the first rect. Note that this is not a failure if this
      // fails to find a spot for the next rect since we can al-
      // ways then try the next
      (void)pack_cols( rect{
        .origin = allowed.origin,
        .size   = {
            .w = allowed.size.w,
            .h = std::min( first.size.h, allowed.size.h ) } } );

      // Move to the remainder of this allowed region (i.e. sub-
      // sequent rows).
      allowed = rect{
        .origin = { .x = allowed.origin.x,
                    .y = allowed.origin.y + first.size.h },
        .size   = { .w = allowed.size.w,
                    .h = allowed.size.h - first.size.h } };
    }
  }

  e_status pack_rects( rect const allowed ) {
    // We need to sort:
    //
    //   1. By height in order for the algorithm to work cor-
    //      rectly.
    //   2. By width (for blocks of equal height) in order to
    //      make it more optimal (though it would still be cor-
    //      rect without this).
    //   3. Stably for unit tests so that they can have determin-
    //      istic output for elements that compare equal.
    //
    std::stable_sort( cur_, end_,
                      []( rect const* l, rect const* r ) {
                        // Sort by decreasing heights first.
                        if( l->size.h != r->size.h )
                          return l->size.h > r->size.h;
                        // Then if heights are equal, sort by de-
                        // creasing width.
                        return l->size.w > r->size.w;
                      } );
    return pack_rows( allowed );
  }

  vector<rect*>::iterator cur_       = {};
  vector<rect*>::iterator const end_ = {};
  size size_used_                    = {};
};

} // namespace

maybe<size> pack_rects( span<rect> rects, size const max_size ) {
  vector<rect*> ptrs;
  ptrs.reserve( rects.size() );
  for( rect& r : rects ) ptrs.push_back( &r );
  // Sort in descending order of heights.
  packer p{ .cur_ = ptrs.begin(), .end_ = ptrs.end() };
  if( p.pack_rects( rect{ .origin = point::origin(),
                          .size   = max_size } ) ==
      packer::e_status::failed )
    return nothing;
  return p.size_used_;
}

} // namespace rr
