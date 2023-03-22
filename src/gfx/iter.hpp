/****************************************************************
**iter.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-16.
*
* Description: Useful generators.
*
*****************************************************************/
#pragma once

// gfx
#include "cartesian.hpp"
#include "coord.hpp"

// base
#include "base/generator.hpp"

namespace gfx {

// These will divide the inside of the rect into subrects of the
// given delta and will return an object which, when iterated
// over, will yield those rects. If the rect size is not an even
// multiple of the chunks then some subrects may be smaller than
// the chunk size. In other words, all subrects generated will be
// inside the main rect.
base::generator<rn::Rect> subrects( rn::Rect  rect,
                                    rn::Delta chunk = rn::Delta{
                                        .w = 1, .h = 1 } );

base::generator<rect> subrects( rect rect,
                                size chunk = size{ .w = 1,
                                                   .h = 1 } );

/****************************************************************
** rect_iterator
*****************************************************************/
// Allows iterating over the coordinates of the points within a
// rectangle.
struct rect_iterator {
 private:
  rect r_;

 public:
  rect_iterator( rect r ) : r_( r ) {}

  // Iteration begins at the upper-left corner of the rect, and
  // it will yields coordinates for each point such that x <
  // right edge and y < bottom edge. So i.e. no points along the
  // right edge or bottom edge will be yielded.
  struct const_iterator {
   private:
    rect      r_      = {};
    rn::Coord cursor_ = {};

   public:
    using iterator_category = std::input_iterator_tag;
    using difference_type   = int;
    using value_type        = rn::Coord;
    using pointer           = rn::Coord const*;
    using reference         = rn::Coord const&;
    const_iterator()        = default;
    const_iterator( rect r, rn::Coord cursor );
    bool operator==( const_iterator const& ) const = default;
    value_type const& operator*() const { return cursor_; }
    const_iterator&   operator++();
    const_iterator    operator++( int );
  };

  using iterator = const_iterator;

  auto begin() const {
    return const_iterator( r_, rn::Coord::from_gfx( r_.nw() ) );
  }

  auto end() const {
    return const_iterator( r_, rn::Coord::from_gfx( r_.sw() ) );
  }
};

// If these are not found with the current headers then we should
// just comment them out -- probably not worth including them,
// even though we do need them to be true.
static_assert( std::ranges::range<rect_iterator> );
static_assert(
    std::input_iterator<rect_iterator::const_iterator> );

} // namespace gfx
