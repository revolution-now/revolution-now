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
#include "logger.hpp"
#include "matrix.hpp"

// config
#include "config/ui.rds.hpp"

using namespace std;

namespace rn {

namespace {

// This struct stands in for a ui::View in a data structure of
// recursive block's that mirror the CompositeView structure.
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
      for( Y y = 1; y < 0 + size.h - 1; ++y )
        if( res[y][0] != -1 ) res[y][0]++;
    if( r )
      for( Y y = 1; y < 0 + size.h - 1; ++y )
        if( res[y][0 + size.w - 1] != -1 )
          res[y][0 + size.w - 1]++;
    if( u )
      for( X x = 1; x < 0 + size.w - 1; ++x )
        if( res[0][x] != -1 ) res[0][x]++;
    if( d )
      for( X x = 1; x < 0 + size.w - 1; ++x )
        if( res[0 + size.h - 1][x] != -1 )
          res[0 + size.h - 1][x]++;
    if( l && u ) res[0][0] = -1;
    if( l && d ) res[0 + size.h - 1][0] = -1;
    if( r && u ) res[0][0 + size.w - 1] = -1;
    if( r && d ) res[0 + size.h - 1][0 + size.w - 1] = -1;
    return res;
  }

  int   id{};
  Delta size{};

  // Flags indicating whether padding is needed on which side
  // (left, right, up, down).
  bool l{ false };
  bool r{ false };
  bool u{ false };
  bool d{ false };

  std::vector<PositionedBlock> subdivisions{};
};
NOTHROW_MOVE( block );

void print_matrix( Matrix<int> const& m ) {
  fmt::print( "\n" );
  for( Y y{ 0 }; y < m.rect().bottom_edge(); ++y ) {
    for( X x{ 0 }; x < m.rect().right_edge(); ++x ) {
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
  fmt::print( "\n" );
}

// Does [r1,r2] overlap with [r3,r4], where the intervals are
// closed.
template<typename T>
bool overlap( T r1, T r2, T r3, T r4 ) {
  return ( r1 >= r3 && r1 <= r4 ) || ( r2 >= r3 && r2 <= r4 ) ||
         ( r3 >= r1 && r3 <= r2 ) || ( r4 >= r1 && r4 <= r2 );
}

// f will take a view and put spacing in all the innards of it,
// but not the outter edge. What is non-trivial in this algo-
// rithm is that we cannot just put padding around every block,
// because then there would be too much padding between adjacent
// blocks; instead the padding has to be merged between blocks
// so that we have a fixed amount of space between all UI ele-
// ments regardless of where they are in the block hierarchy.
//
//   f( block b ) {
//     for each child block cb of b:
//       cb = f( cb )
//     Create hash map(s) to store borders added at this level.
//     for each child block cb of b:
//       for each side of cb:
//         if side is not on edge:
//           add padding if there is none touching it
//   }
//
void compute_merged_padding_impl( block& b ) {
  for( auto& [_, sub_block] : b.subdivisions )
    compute_merged_padding_impl( sub_block );
  // These ranges are inclusive.
  unordered_map<X, vector<pair<Y, Y>>> vertical_edges;
  unordered_map<Y, vector<pair<X, X>>> horizontal_edges;
  for( auto& [coord, sub_block] : b.subdivisions ) {
    auto rect = Rect::from( coord, sub_block.size );
    // Handle each side of the sub-block so long as it is not
    // touching one of the edges of this block.
    if( rect.top_edge() != 0 ) {
      // [start, end] inclusive of edge not including corners.
      auto start = rect.left_edge() + 1;
      auto end   = rect.right_edge() - 1 - 1;
      if( start < end ) {
        auto& ranges   = horizontal_edges[rect.top_edge()];
        bool  overlaps = false;
        for( auto [x1, x2] : ranges )
          overlaps |= overlap( x1, x2, start, end );
        if( !overlaps ) {
          sub_block.u = true;
          ranges.push_back( { start, end } );
        }
      }
    }
    if( rect.bottom_edge() != 0 + b.size.h ) {
      // [start, end] inclusive of edge not including corners.
      auto start = rect.left_edge() + 1;
      auto end   = rect.right_edge() - 1 - 1;
      if( start < end ) {
        auto& ranges = horizontal_edges[rect.bottom_edge() - 1];
        bool  overlaps = false;
        for( auto [x1, x2] : ranges )
          overlaps |= overlap( x1, x2, start, end );
        if( !overlaps ) {
          sub_block.d = true;
          ranges.push_back( { start, end } );
        }
      }
    }
    if( rect.left_edge() != 0 ) {
      // [start, end] inclusive of edge not including corners.
      auto start = rect.top_edge() + 1;
      auto end   = rect.bottom_edge() - 1 - 1;
      if( start < end ) {
        auto& ranges   = vertical_edges[rect.left_edge()];
        bool  overlaps = false;
        for( auto [x1, x2] : ranges )
          overlaps |= overlap( x1, x2, start, end );
        if( !overlaps ) {
          sub_block.l = true;
          ranges.push_back( { start, end } );
        }
      }
    }
    if( rect.right_edge() != 0 + b.size.w ) {
      // [start, end] inclusive of edge not including corners.
      auto start = rect.top_edge() + 1;
      auto end   = rect.bottom_edge() - 1 - 1;
      if( start < end ) {
        auto& ranges   = vertical_edges[rect.right_edge() - 1];
        bool  overlaps = false;
        for( auto [x1, x2] : ranges )
          overlaps |= overlap( x1, x2, start, end );
        if( !overlaps ) {
          sub_block.r = true;
          ranges.push_back( { start, end } );
        }
      }
    }
  }
}

void compute_merged_padding( block& b ) {
  compute_merged_padding_impl( b );
  // Do not add padding around the outter most one.
  // b.l = b.r = b.u = b.d = true;
}

void inc_sizes( block& b ) {
  for( auto& sub_b : b.subdivisions ) inc_sizes( sub_b.second );
  b.size += Delta{ .w = 1, .h = 1 };
}

block derive_blocks_impl( ui::View const* view );

block derive_blocks_impl_composite(
    ui::CompositeView const& view ) {
  block res( view.delta(), {} );
  for( int i = 0; i < view.count(); ++i ) {
    auto  p_view    = view.at( i );
    block sub_block = derive_blocks_impl( p_view.view );
    block::PositionedBlock p_block{ view.pos_of( i ),
                                    sub_block };
    res.subdivisions.push_back( p_block );
  }
  return res;
}

block derive_blocks_impl( ui::View const* view ) {
  if( auto maybe_composite_view =
          view->cast_safe<ui::CompositeView>();
      maybe_composite_view.has_value() ) {
    return derive_blocks_impl_composite(
        **maybe_composite_view );
  } else
    return block( view->delta(), {} );
}

// This will traverse the view (tree ) hierarchy and construct a
// block tree structure that mirrors the view structure.
block derive_blocks( ui::View& view ) {
  auto block = derive_blocks_impl( &view );

  // inc_sizes will increment the size of each block by one. This
  // is because the compute_merged_padding algorithm requires
  // that the blocks overlap with right/down adjacent blocks by
  // one square/pixel so that borders overlap and can be merged.
  // In other words, it's just a quirk of the algorithm. For
  // blocks that are touch a right/down edge this might not be
  // necessary, but it doesn't seem to hurt (and is simpler) to
  // just do it for all blocks.
  inc_sizes( block );

  return block;
}

void insert_padding_views_fancy( ui::View& view, block const& b,
                                 int pixels );

void autopad_fancy_impl_composite( ui::CompositeView& view,
                                   block const& b, int pixels ) {
  CHECK( int( b.subdivisions.size() ) == view.count() );
  for( int i = 0; i < view.count(); ++i ) {
    auto& sub_view  = view.mutable_at( i );
    auto& sub_block = b.subdivisions[i].second;
    // Always try padding the sub views themselves, even if this
    // view has can_pad_immediate_children() == false, because
    // that only applies to this level in the hierarchy, not
    // recursively.
    insert_padding_views_fancy( *sub_view, sub_block, pixels );
    if( view.can_pad_immediate_children() ) {
      auto new_sub_view = make_unique<ui::PaddingView>(
          std::move( sub_view ), pixels, sub_block.l,
          sub_block.r, sub_block.u, sub_block.d );
      sub_view = std::move( new_sub_view );
    }
  }
}

void insert_padding_views_fancy( ui::View& view, block const& b,
                                 int pixels ) {
  if( auto maybe_composite_view =
          view.cast_safe<ui::CompositeView>();
      maybe_composite_view.has_value() ) {
    auto& composite_view = **maybe_composite_view;
    CHECK( int( b.subdivisions.size() ) ==
           composite_view.count() );
    autopad_fancy_impl_composite( composite_view, b, pixels );
  } else {
    CHECK( b.subdivisions.size() == 0 );
  }
}

void insert_padding_views_simple( ui::View& view, int pixels );

void autopad_simple_impl_composite( ui::CompositeView& view,
                                    int                pixels ) {
  for( int i = 0; i < view.count(); ++i ) {
    auto& sub_view = view.mutable_at( i );
    // Always try padding the sub views themselves, even if this
    // view has can_pad_immediate_children() == false, because
    // that only applies to this level in the hierarchy, not
    // recursively.
    insert_padding_views_simple( *sub_view, pixels );
    if( view.can_pad_immediate_children() &&
        sub_view->needs_padding() ) {
      // In this one we use half the padding because the assump-
      // tion is that this sub view will be touching another view
      // that also has half padding.
      auto new_sub_view = make_unique<ui::PaddingView>(
          std::move( sub_view ), pixels / 2, true, true, true,
          true );
      sub_view = std::move( new_sub_view );
    }
  }
}

void insert_padding_views_simple( ui::View& view, int pixels ) {
  if( auto maybe_composite_view =
          view.cast_safe<ui::CompositeView>();
      maybe_composite_view.has_value() ) {
    auto& composite_view = **maybe_composite_view;
    autopad_simple_impl_composite( composite_view, pixels );
  }
}

// This will traverse the view (tree) hierarchy and construct a
// block tree structure that mirrors the view structure. Then the
// padding analysis will be performed on the block structure, and
// the results copied into the view structure by inserting
// PaddingView's where needed.
void autopad_fancy( ui::View& view, int pixels ) {
  auto block = derive_blocks( view );

  // Traverse the block structure and enable the l/r/u/d booleans
  // intelligently to add a single line of padding between all
  // elements.
  compute_merged_padding( block );

  // Turn on when debugging.
  // print_matrix( block.to_matrix() );

  insert_padding_views_fancy( view, block, pixels );
}

void autopad_simple( ui::View& view, int pixels ) {
  insert_padding_views_simple( view, pixels );
}

} // namespace

void autopad( ui::View& view, bool use_fancy, int pixels ) {
  if( use_fancy )
    autopad_fancy( view, pixels );
  else
    // TODO: reimplement this with a generic visitor that tra-
    // verses the views.
    autopad_simple( view, pixels );

  if( auto cv = view.cast_safe<ui::CompositeView>();
      cv.has_value() )
    ( *cv )->children_updated();
}

void autopad( ui::View& view, bool use_fancy ) {
  autopad( view, use_fancy, config_ui.window.ui_padding );
}

void test_autopad() {
  /* clang-format off
   *.................................................................
   *...............|.......................|...........3.............
   *...............|.......................+------------------------.
   *...............|...........0...........|.........................
   *...............|.......................4.........................
   *...............+-----------------------+.........................
   *...............|.......................|...........2.............
   *...............|...........1...........|.........................
   *...............|.......................|.........................
   *...............9--------------+--------+------------------------.
   *.........8.....|..............|..................................
   *...............|..............|..................................
   *...............|..............|..................................
   *...............|..............|..................................
   *...............|.......6......7............5.....................
   *...............|..............|..................................
   *...............|..............|..................................
   *...............|..............|..................................
   *...............|..............|..................................
   *.................................................................
   * clang-format on
   */

  auto block0 = block( { .w = 25, .h = 6 }, {} );
  auto block1 = block( { .w = 25, .h = 5 }, {} );
  auto block2 = block( { .w = 26, .h = 8 }, {} );
  auto block3 = block( { .w = 26, .h = 3 }, {} );
  auto block4 = block( { .w = 50, .h = 10 },
                       { { { .x = 0, .y = 0 }, block0 },
                         { { .x = 0, .y = 5 }, block1 },
                         { { .x = 24, .y = 2 }, block2 },
                         { { .x = 24, .y = 0 }, block3 } } );
  auto block5 = block( { .w = 35, .h = 11 }, {} );
  auto block6 = block( { .w = 16, .h = 11 }, {} );
  auto block7 = block( { .w = 50, .h = 11 },
                       { { { .x = 15, .y = 0 }, block5 },
                         { { .x = 0, .y = 0 }, block6 } } );
  auto block8 = block( { .w = 16, .h = 20 }, {} );
  auto block9 = block( { .w = 65, .h = 20 },
                       { { { .x = 15, .y = 0 }, block4 },
                         { { .x = 15, .y = 9 }, block7 },
                         { { .x = 0, .y = 0 }, block8 } } );
  fmt::print( "\nBefore:\n" );
  print_matrix( block9.to_matrix() );

  compute_merged_padding( block9 );

  fmt::print( "After:\n" );
  print_matrix( block9.to_matrix() );
}

} // namespace rn
