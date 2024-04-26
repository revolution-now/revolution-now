/****************************************************************
**depletion.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-03-29.
*
* Description: Handles depletion of prime resources.
*
*****************************************************************/
#include "depletion.hpp"

// Revolution Now
#include "co-maybe.hpp"
#include "imap-updater.hpp"
#include "irand.hpp"
#include "map-square.hpp"

// config
#include "config/production.rds.hpp"

// ss
#include "ss/colony.rds.hpp"
#include "ss/difficulty.rds.hpp"
#include "ss/map.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"

// gfx
#include "gfx/iter.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "terrain-enums.rds.hpp"

using namespace std;

// NOTE: See doc/depletion.txt for an explanation of the me-
// chanics of silver/minerals depletion in both the OG and NG.

namespace rn {

namespace {

using ::base::lookup;

double p_bump( e_difficulty difficulty ) {
  double const D = static_cast<int>( difficulty );
  return 1 - 1 / ( 2 + D );
}

maybe<e_natural_resource> depleted(
    e_natural_resource resource ) {
  switch( resource ) {
    case e_natural_resource::silver:
      return e_natural_resource::silver_depleted;
    default:
      return nothing;
  }
}

// This is a maybe coroutine.
maybe<DepletionEvent> advance_tile_depletion_state(
    SSConst const& ss, IRand& rand, ResourceDepletion& depletion,
    Colony const& colony, e_direction d ) {
  auto const& conf =
      config_production.outdoor_production.depletion;
  Coord const tile = colony.location.moved( d );
  auto const& square =
      co_await ss.terrain.maybe_square_at( tile );
  auto const  resource = co_await effective_resource( square );
  auto const& outdoor_unit = co_await colony.outdoor_jobs[d];
  e_outdoor_job const job  = outdoor_unit.job;
  auto const&         counter_bump = conf.counter_bump;
  auto& by_resource = co_await lookup( counter_bump, job );
  auto& bump        = co_await lookup( by_resource, resource );
  if( bump == 0 ) co_await nothing;
  e_difficulty const difficulty = ss.settings.difficulty;
  co_await rand.bernoulli( p_bump( difficulty ) );
  auto& counter = depletion.counters[tile];
  counter += bump;
  if( counter < conf.counter_limit ) co_await nothing;
  depletion.counters.erase( tile );
  co_return DepletionEvent{
      .tile          = tile,
      .resource_from = resource,
      .resource_to   = depleted( resource ) };
}

bool square_allows_depletion_counter( MapSquare const& square ) {
  UNWRAP_RETURN_FALSE( rsrc, effective_resource( square ) );
  using E = e_natural_resource;
  return ( rsrc == E::minerals ) || ( rsrc == E::silver );
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
vector<DepletionEvent> advance_depletion_state(
    SS& ss, IRand& rand, Colony const& colony ) {
  vector<DepletionEvent> res;
  for( e_direction const d : refl::enum_values<e_direction> )
    if( auto event = advance_tile_depletion_state(
            ss, rand, ss.map.depletion, colony, d );
        event.has_value() )
      res.push_back( *event );
  return res;
}

void update_depleted_tiles(
    IMapUpdater&                       map_updater,
    std::vector<DepletionEvent> const& events ) {
  for( DepletionEvent const& event : events ) {
    auto mutator = [&]( MapSquare& square ) {
      if( square.overlay == e_land_overlay::forest ) {
        CHECK_EQ( square.forest_resource, event.resource_from );
        square.forest_resource = event.resource_to;
      } else {
        CHECK_EQ( square.ground_resource, event.resource_from );
        square.ground_resource = event.resource_to;
      }
    };
    map_updater.modify_map_square( event.tile, mutator );
  }
}

void remove_depletion_counter_if_needed( SS& ss, Coord tile ) {
  if( !square_allows_depletion_counter(
          ss.terrain.square_at( tile ) ) )
    ss.map.depletion.counters.erase( tile );
}

void remove_depletion_counters_where_needed( SS& ss ) {
  Rect const rect = ss.terrain.world_rect_tiles();
  for( gfx::point const p : gfx::rect_iterator( rect ) )
    remove_depletion_counter_if_needed( ss,
                                        Coord::from_gfx( p ) );
}

} // namespace rn
