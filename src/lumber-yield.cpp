/****************************************************************
**lumber-yield.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-20.
*
* Description: Lumber yields from pioneers clearing forests.
*
*****************************************************************/
#include "lumber-yield.hpp"

// Revolution Now
#include "colony-buildings.hpp"
#include "map-search.hpp"
#include "map-square.hpp"

// config
#include "config/command.rds.hpp"
#include "config/production.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

// base
#include "base/range-lite.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::generator;

// Yields a finite stream of friendly colonies spiraling outward
// from the starting point that are within a radius of 3.5 to the
// start.
generator<ColonyId> close_friendly_colonies(
    SSConst const& ss, Player const& player,
    gfx::point const start ) {
  generator<gfx::point> points =
      outward_spiral_search_existing( ss, start );
  // If we search a 7x7 grid (49) tiles then we should cover all
  // of the ones that are within a 3.5 pythagorean distance to
  // the starting square.
  int  kMaxSquaresToSearch = 49;
  auto rng = base::rl::all( points ).take( kMaxSquaresToSearch );

  for( gfx::point p : rng ) {
    Coord const square = Coord::from_gfx( p );
    CHECK( ss.terrain.square_exists( square ) );
    gfx::size const delta = square.to_gfx() - start;
    double const    distance =
        std::sqrt( delta.w * delta.w + delta.h * delta.h );
    if( distance > 3.5 ) continue;
    // Is there a friendly colony there.
    maybe<ColonyId> const colony_id =
        ss.colonies.maybe_from_coord( square );
    if( !colony_id.has_value() ) continue;
    if( ss.colonies.colony_for( *colony_id ).nation !=
        player.nation )
      continue;
    co_yield *colony_id;
  }
}

LumberYield yield_for_colony( SSConst const& ss,
                              ColonyId colony_id, Coord plow_loc,
                              e_unit_type pioneer_type ) {
  Colony const& colony = ss.colonies.colony_for( colony_id );
  // First base yield.
  int base_yield = 0;
  // Just in case in the future we add a building higher than the
  // lumber mill.
  if( colony_has_building_level(
          colony, e_colony_building::lumber_mill ) ) {
    e_terrain const terrain_type =
        effective_terrain( ss.terrain.square_at( plow_loc ) );
    int const tile_yield = config_production.outdoor_production
                               .jobs[e_outdoor_job::lumber]
                               .base_productions[terrain_type];
    base_yield = ( config_command.lumber_yield.tile_yield_extra +
                   tile_yield ) *
                 config_command.lumber_yield.multiplier;
  } else {
    base_yield =
        config_command.lumber_yield.base_yield_no_lumber_mill;
  }

  // Now total yield.
  int total_yield = base_yield;
  if( pioneer_type == e_unit_type::hardy_pioneer )
    total_yield *= 2;

  // Now net yield.
  int const amount_that_can_fit =
      std::max( colony_warehouse_capacity( colony ) -
                    colony.commodities[e_commodity::lumber],
                0 );
  auto res = LumberYield{
      .colony_id   = colony_id,
      .total_yield = total_yield,
      .yield_to_add_to_colony =
          std::min( total_yield, amount_that_can_fit ),
  };
  return res;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
// Will find all of the friendly colonies that exist in the
// vicinity and return the yield that they'd receive. The caller
// can then pick the one with the highest yield.
vector<LumberYield> lumber_yields( SSConst const& ss,
                                   Player const&  player,
                                   Coord          loc,
                                   e_unit_type pioneer_type ) {
  vector<LumberYield> res;
  for( ColonyId colony_id :
       close_friendly_colonies( ss, player, loc ) )
    res.push_back(
        yield_for_colony( ss, colony_id, loc, pioneer_type ) );
  return res;
}

maybe<LumberYield> best_lumber_yield(
    vector<LumberYield> const& yields ) {
  maybe<LumberYield> res;
  for( LumberYield const& yield : yields ) {
    if( yield.yield_to_add_to_colony == 0 ) continue;
    if( !res.has_value() ) res = yield;
    // By using > and not >= we will prefer earlier colonies in
    // the list (all else being equal), which is important be-
    // cause they will generally be closer in distance.
    if( yield.yield_to_add_to_colony >
        res->yield_to_add_to_colony )
      res = yield;
  }
  return res;
}

} // namespace rn
