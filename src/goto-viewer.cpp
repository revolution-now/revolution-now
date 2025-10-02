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
#include "roles.hpp"
#include "society.hpp"
#include "visibility.hpp"

// config
#include "config/command.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/nation.hpp"
#include "ss/ref.hpp"
#include "ss/unit.hpp"

// rds
#include "rds/switch-macro.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::maybe;
using ::gfx::point;
using ::gfx::size;

maybe<e_player> viz_player( SSConst const& ss,
                            Unit const& unit ) {
  // In the OG this is true, in the NG it defaults to false.
  if( config_command.go_to.omniscient_path_finding )
    return nothing;
  if( auto const viewer =
          player_for_role( ss, e_player_role::viewer );
      !viewer.has_value() )
    // If the entire map is currently visible then we allow the
    // unit to use that, regardless of player.
    return viewer;
  // The entire map is not visible, so use the one of the unit
  // that is actually moving.
  return unit.player_type();
}

} // namespace

/****************************************************************
** GotoMapViewer
*****************************************************************/
GotoMapViewer::GotoMapViewer( SSConst const& ss,
                              Unit const& unit )
  : ss_( ss ),
    unit_( unit ),
    viz_( create_visibility_for( ss, viz_player( ss, unit ) ) ),
    is_ship_( unit.desc().ship ) {
  CHECK( viz_ ); // should never fail.
}

GotoMapViewer::~GotoMapViewer() = default;

bool GotoMapViewer::can_enter_tile( point const tile ) const {
  if( !viz_->on_map( tile ) ) return false;
  switch( viz_->visible( tile ) ) {
    case e_tile_visibility::hidden:
      return true;
    case e_tile_visibility::fogged:
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
  if( auto const society = society_on_square( ss_, tile );
      society.has_value() ) {
    SWITCH( *society ) {
      CASE( european ) {
        if( european.player != unit_.player_type() )
          return false;
        break;
      }
      CASE( native ) { return false; }
    }
  }
  MapSquare const& square = viz_->square_at( tile );
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
  size const map_size = viz_->rect_tiles().delta();
  if( p.x == 0 ) return pacific;
  if( p.x == map_size.w - 1 ) return atlantic;
  return none;
}

maybe<bool> GotoMapViewer::is_sea_lane(
    point const tile ) const {
  switch( viz_->visible( tile ) ) {
    using enum e_tile_visibility;
    case e_tile_visibility::hidden:
      return nothing;
    case e_tile_visibility::fogged:
    case e_tile_visibility::clear:
      return viz_->square_at( tile ).sea_lane;
  }
}

maybe<bool> GotoMapViewer::has_lcr( point const tile ) const {
  switch( viz_->visible( tile ) ) {
    using enum e_tile_visibility;
    case e_tile_visibility::hidden:
      return nothing;
    case e_tile_visibility::fogged:
    case e_tile_visibility::clear:
      return viz_->square_at( tile ).lost_city_rumor;
  }
}

MovementPoints GotoMapViewer::movement_points_required(
    point const src, e_direction const direction ) const {
  auto const src_square = viz_->visible_square_at( src );
  auto const dst_square =
      viz_->visible_square_at( src.moved( direction ) );
  if( !src_square.has_value() || !dst_square.has_value() )
    // If either src or dst tiles are hidden then just assume a
    // cost of one to traverse them. It seems likely that in
    // practice, if either of them are hidden, then at least the
    // dst tile will be hidden, so there isn't really much better
    // that we can do here.
    return MovementPoints( 1 );
  MovementPoints const uncapped = ::rn::movement_points_required(
      *src_square, *dst_square, direction );
  return std::min( unit_.desc().base_movement_points, uncapped );
}

} // namespace rn
