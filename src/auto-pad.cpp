/****************************************************************
**auto-pad.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-04-05.
*
* Description: Auto merged padding around UI elements.
*
*****************************************************************/
#include "auto-pad.hpp"

// Revolution Now
#include "fmt-helper.hpp"
#include "logging.hpp"
#include "util.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

using namespace std;

namespace rn::autopad {

namespace {

struct block {
  // Each Rect is in coordinates relative to the origin of its
  // parent.
  using PositionedBlock = std::pair<Coord, block>;

  block( Delta                        size_,
         std::vector<PositionedBlock> subdivisions_ )
    : size( size_ ), subdivisions( std::move( subdivisions_ ) ) {
    static int next_id = 0;
    id                 = next_id++;
  }

  Matrix<int> to_matrix() const {
    Matrix<int> res( size );
    if( size.area() == 0 ) return res;
    for( auto coord : res.rect() ) res[coord] = 0;
    for( auto const& [origin, b] : subdivisions ) {
      auto matrix = b.to_matrix();
      for( auto coord : Rect::from( origin, b.size ) ) {
        auto i = matrix[coord.with_new_origin( origin )];
        if( i == -1 || res[coord] == -1 )
          res[coord] = -1;
        else
          res[coord] += i;
      }
    }
    if( l )
      for( Y y = 1_y; y < 0_y + size.h - 1_h; ++y )
        if( res[y][0_x] != -1 ) res[y][0_x]++;
    if( r )
      for( Y y = 1_y; y < 0_y + size.h - 1_h; ++y )
        if( res[y][0_x + size.w - 1] != -1 )
          res[y][0_x + size.w - 1]++;
    if( u )
      for( X x = 1_x; x < 0_x + size.w - 1_w; ++x )
        if( res[0_y][x] != -1 ) res[0_y][x]++;
    if( d )
      for( X x = 1_x; x < 0_x + size.w - 1_w; ++x )
        if( res[0_y + size.h - 1_h][x] != -1 )
          res[0_y + size.h - 1_h][x]++;
    if( l && u ) res[0_y][0_x] = -1;
    if( l && d ) res[0_y + size.h - 1_h][0_x] = -1;
    if( r && u ) res[0_y][0_x + size.w - 1] = -1;
    if( r && d ) res[0_y + size.h - 1_h][0_x + size.w - 1] = -1;
    return res;
  }

  int   id{};
  Delta size{};

  // Flags indicating whether padding is needed on which side
  // (left, right, up, down).
  bool l{false};
  bool r{false};
  bool u{false};
  bool d{false};

  std::vector<PositionedBlock> subdivisions{};
};

void print_matrix( Matrix<int> const& m ) {
  for( Y y{0}; y < m.rect().bottom_edge(); ++y ) {
    for( X x{0}; x < m.rect().right_edge(); ++x ) {
      auto i = m[y][x];
      if( i < 0 )
        fmt::print( "+" );
      else if( i == 0 )
        fmt::print( "." );
      else if( i < 10 )
        fmt::print( "{}", i );
      else
        fmt::print( "X" );
    }
    fmt::print( "\n" );
  }
}

// Does [r1,r2] overlap with [r3,r4], where the intervals are
// closed.
template<typename T>
bool overlap( T r1, T r2, T r3, T r4 ) {
  return ( r1 >= r3 && r1 <= r4 ) || ( r2 >= r3 && r2 <= r4 );
}

/*
 *  f will take a view and put spacing in all the innards of it,
 *  but not the outter edge.
 *
 *  f( view v ) {
 *    for each child view cv of v:
 *      cv = f( cv )
 *     create hash map(s) to store borders added at this level.
 *    for each child view cv of v:
 *      for each side of cv:
 *        if side is not on edge:
 *          add padding if there is none touching it
 *  }
 *
 *  f( main_view )
 */
void pad_impl( block& b ) {
  for( auto& [_, sub_block] : b.subdivisions )
    pad_impl( sub_block );
  // These ranges are inclusive.
  absl::flat_hash_map<X, Vec<pair<Y, Y>>> vertical_edges;
  absl::flat_hash_map<Y, Vec<pair<X, X>>> horizontal_edges;
  for( auto& [coord, sub_block] : b.subdivisions ) {
    auto rect = Rect::from( coord, sub_block.size );
    // Handle each side of the sub-block so long as it is not
    // touching one of the edges of this block.
    if( rect.top_edge() != 0_y ) {
      // [start, end] inclusive of edge not including corners.
      auto start = rect.left_edge() + 1_w;
      auto end   = rect.right_edge() - 1_w - 1_w;
      if( start < end ) {
        auto& ranges   = horizontal_edges[rect.top_edge()];
        bool  overlaps = false;
        for( auto [x1, x2] : ranges )
          overlaps |= overlap( x1, x2, start, end );
        if( !overlaps ) {
          sub_block.u = true;
          ranges.push_back( {start, end} );
        }
      }
    }
    if( rect.bottom_edge() != 0_y + b.size.h ) {
      // [start, end] inclusive of edge not including corners.
      auto start = rect.left_edge() + 1_w;
      auto end   = rect.right_edge() - 1_w - 1_w;
      if( start < end ) {
        auto& ranges =
            horizontal_edges[rect.bottom_edge() - 1_h];
        bool overlaps = false;
        for( auto [x1, x2] : ranges )
          overlaps |= overlap( x1, x2, start, end );
        if( !overlaps ) {
          sub_block.d = true;
          ranges.push_back( {start, end} );
        }
      }
    }
    if( rect.left_edge() != 0_x ) {
      // [start, end] inclusive of edge not including corners.
      auto start = rect.top_edge() + 1_h;
      auto end   = rect.bottom_edge() - 1_h - 1_h;
      if( start < end ) {
        auto& ranges   = vertical_edges[rect.left_edge()];
        bool  overlaps = false;
        for( auto [x1, x2] : ranges )
          overlaps |= overlap( x1, x2, start, end );
        if( !overlaps ) {
          sub_block.l = true;
          ranges.push_back( {start, end} );
        }
      }
    }
    if( rect.right_edge() != 0_x + b.size.w ) {
      // [start, end] inclusive of edge not including corners.
      auto start = rect.top_edge() + 1_h;
      auto end   = rect.bottom_edge() - 1_h - 1_h;
      if( start < end ) {
        auto& ranges   = vertical_edges[rect.right_edge() - 1_w];
        bool  overlaps = false;
        for( auto [x1, x2] : ranges )
          overlaps |= overlap( x1, x2, start, end );
        if( !overlaps ) {
          sub_block.r = true;
          ranges.push_back( {start, end} );
        }
      }
    }
  }
}

void pad( block& b ) {
  pad_impl( b );
  b.l = b.r = b.u = b.d = true;
}

} // namespace

void test_autopad() {
  /* clang-format off
   *+--------------+-----------------------+------------------------+
   *|              |                       |           3            |
   *|              |                       +------------------------+
   *|              |           0           |                        |
   *|              |                       4                        |
   *|              +-----------------------+                        |
   *|              |                       |           2            |
   *|              |           1           |                        |
   *|              |                       |                        |
   *|              9--------------+--------+------------------------+
   *|        8     |              |                                 |
   *|              |              |                                 |
   *|              |              |                                 |
   *|              |              |                                 |
   *|              |       6      7            5                    |
   *|              |              |                                 |
   *|              |              |                                 |
   *|              |              |                                 |
   *|              |              |                                 |
   *+--------------+--------------+---------------------------------+
   * clang-format on
   */

  auto block0 = block( {25_w, 6_h}, {} );
  auto block1 = block( {25_w, 5_h}, {} );
  auto block2 = block( {26_w, 8_h}, {} );
  auto block3 = block( {26_w, 3_h}, {} );
  auto block4 = block( {50_w, 10_h}, {{{0_x, 0_y}, block0},
                                      {{0_x, 5_y}, block1},
                                      {{24_x, 2_y}, block2},
                                      {{24_x, 0_y}, block3}} );
  auto block5 = block( {35_w, 11_h}, {} );
  auto block6 = block( {16_w, 11_h}, {} );
  auto block7 = block( {50_w, 11_h}, {{{15_x, 0_y}, block5},
                                      {{0_x, 0_y}, block6}} );
  auto block8 = block( {16_w, 20_h}, {} );
  auto block9 = block( {65_w, 20_h}, {{{15_x, 0_y}, block4},
                                      {{15_x, 9_y}, block7},
                                      {{0_x, 0_y}, block8}} );
  fmt::print( "\nBefore:\n\n" );
  print_matrix( block9.to_matrix() );

  pad( block9 );

  fmt::print( "\nAfter:\n\n" );
  print_matrix( block9.to_matrix() );

  fmt::print( "\n" );
}

} // namespace rn::autopad
