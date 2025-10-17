/****************************************************************
**goto-viewer.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-27.
*
* Description: Implementations of IGotoMapViewer.
*
*****************************************************************/
#include "goto-viewer.hpp"

// Revolution Now
#include "map-square.hpp"
#include "pacific.hpp"
#include "society.hpp"
#include "visibility.hpp"

// config
#include "config/unit-type.hpp"

// ss
#include "ss/ref.hpp"

// rds
#include "rds/switch-macro.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::maybe;
using ::gfx::point;
using ::gfx::size;

} // namespace

/****************************************************************
** GotoMapViewer
*****************************************************************/
GotoMapViewer::GotoMapViewer( SSConst const& ss,
                              IVisibility const& viz,
                              e_player const player_type,
                              e_unit_type const unit_type )
  : ss_( ss ),
    viz_( viz ),
    player_type_( player_type ),
    unit_type_( unit_type ),
    is_ship_( unit_attr( unit_type ).ship ) {}

bool GotoMapViewer::can_enter_tile( point const tile ) const {
  if( !viz_.on_map( tile ) ) return false;
  switch( viz_.visible( tile ) ) {
    case e_tile_visibility::hidden:
      return true;
    case e_tile_visibility::fogged:
      break;
    case e_tile_visibility::clear:
      break;
  }
  // NOTE: we don't check for colonies here, even friendly ones,
  // because we don't want ships to be able to enter them along
  // their path; we only want ships to enter them when they are
  // the destination, which will be allowed because the path
  // finding algos will always allow the destination tile to be
  // visited (or at least attempted to be visited), that way the
  // player can use the goto action to e.g. attack a dwelling,
  // trade with a foreign colony, etc. That said, land units will
  // always be able to enter friendly colonies because they are
  // on land, and we don't exclude them.
  if( auto const society = society_on_real_square( ss_, tile );
      society.has_value() ) {
    SWITCH( *society ) {
      CASE( european ) {
        if( european.player != player_type_ ) return false;
        break;
      }
      CASE( native ) { return false; }
    }
  }
  MapSquare const& square = viz_.square_at( tile );
  switch( square.surface ) {
    case e_surface::land:
      return !is_ship_;
    case e_surface::water:
      return is_ship_;
  }
}

e_map_side GotoMapViewer::map_side( point const tile ) const {
  using enum e_map_side;
  return is_atlantic_side_of_map( ss_.terrain, tile ) ? atlantic
                                                      : pacific;
}

e_map_side_edge GotoMapViewer::is_on_map_side_edge(
    point const p ) const {
  using enum e_map_side_edge;
  size const map_size = viz_.rect_tiles().delta();
  if( p.x == 0 ) return pacific;
  if( p.x == map_size.w - 1 ) return atlantic;
  return none;
}

maybe<bool> GotoMapViewer::is_sea_lane(
    point const tile ) const {
  switch( viz_.visible( tile ) ) {
    using enum e_tile_visibility;
    case e_tile_visibility::hidden:
      return nothing;
    case e_tile_visibility::fogged:
    case e_tile_visibility::clear:
      return viz_.square_at( tile ).sea_lane;
  }
}

maybe<bool> GotoMapViewer::has_lcr( point const tile ) const {
  switch( viz_.visible( tile ) ) {
    using enum e_tile_visibility;
    case e_tile_visibility::hidden:
      return nothing;
    case e_tile_visibility::fogged:
    case e_tile_visibility::clear:
      return viz_.square_at( tile ).lost_city_rumor;
  }
}

maybe<MovementPoints> GotoMapViewer::movement_points_required(
    point const src, e_direction const direction ) const {
  auto const src_square = viz_.visible_square_at( src );
  auto const dst_square =
      viz_.visible_square_at( src.moved( direction ) );
  if( !src_square.has_value() || !dst_square.has_value() )
    // Can't compute it.
    return nothing;
  MovementPoints const uncapped = ::rn::movement_points_required(
      *src_square, *dst_square, direction );
  return std::min( unit_attr( unit_type_ ).base_movement_points,
                   uncapped );
}

MovementPoints GotoMapViewer::minimum_heuristic_tile_cost()
    const {
  // This is used to compute the heuristic cost of moving between
  // two adjacent tiles in the heuristic function for the A*
  // algo. In order for the heuristic function to satisfy the
  // property of Admissibility (required in order for optimal
  // path discovery to work properly given that different tiles
  // have different weights), this value should never overesti-
  // mate the true optimal cost of traveling between two tiles,
  // meaning that it must always return the best case scenario.
  // For a land unit, that means assuming that the tiles are tra-
  // versible and are joined by a road. For a ship, it is just
  // one movement point always.
  //
  // Even though statistically it might be more accurate to as-
  // sume a cost of 1 or 2 movement points (3 and 6 atoms, re-
  // spectively) to move between tiles we are ignorant of the
  // terrain type, that would break the Admissibility property
  // which would then break guarantees of the A* algo in finding
  // the optimal path given that the costs of tiles are
  // non-uniform (due to differing terrain types, roads, etc.).
  return is_ship_ ? MovementPoints( 1 ) : MovementPoints::_1_3();
}

} // namespace rn
