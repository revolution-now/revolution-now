/****************************************************************
**native-expertise.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-12.
*
* Description: Selects a teaching expertise for a native
*              dwelling.
*
*****************************************************************/
#include "native-expertise.hpp"

// Revolution Now
#include "irand.hpp"
#include "land-production.hpp"
#include "promotion.hpp"
#include "rand-enum.hpp"
#include "ts.hpp"

// config
#include "config/natives.rds.hpp"

// ss
#include "ss/colony-enums.rds.hpp"
#include "ss/dwelling.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/register.hpp"

// gfx
#include "gfx/coord.hpp"
#include "gfx/iter.hpp"

using namespace std;

namespace rn {

void linker_dont_discard_module_native_expertise();
void linker_dont_discard_module_native_expertise() {}

namespace {

void add_outdoor_weights_for_square(
    SSConst const& ss, Coord square,
    refl::enum_map<e_unit_activity, int>& out ) {
  refl::enum_map<e_unit_activity, int> res;
  for( e_outdoor_job job : refl::enum_values<e_outdoor_job> ) {
    e_unit_activity const activity =
        activity_for_outdoor_job( job );
    e_unit_type const expert_type =
        expert_for_activity( activity );
    // Compute land production as if by an expert belonging to a
    // player that has no founding fathers. We use the expert be-
    // cause that does two things:
    //   1. It allows us to get some silver on mountains tiles
    //      which we otherwise wouldn't because a free colonist
    //      cannot produce any silver on mountain tiles without
    //      resources (which are rare).
    //   2. It tends to weight non-food items higher than food,
    //      which is good, because every square produces some
    //      food, so food will have a high weight already.
    out[activity] += production_on_square( job, ss.terrain,
                                           /*fathers=*/{},
                                           expert_type, square );
  }
}

void add_outdoor_weights_around_square(
    SSConst const& ss, Coord start,
    refl::enum_map<e_unit_activity, int>& out ) {
  Rect const r = Rect::from( start, Delta{ .w = 1, .h = 1 } )
                     .with_border_added( 2 );
  for( Rect sub_rect : gfx::subrects( r ) ) {
    Coord const coord = sub_rect.upper_left();
    if( ss.terrain.square_exists( coord ) )
      add_outdoor_weights_for_square( ss, coord, out );
  }
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
refl::enum_map<e_unit_activity, int> dwelling_expertise_weights(
    SSConst const& ss, Dwelling const& dwelling ) {
  refl::enum_map<e_unit_activity, int> weights;
  add_outdoor_weights_around_square( ss, dwelling.location,
                                     weights );
  e_tribe const            tribe = dwelling.tribe;
  e_native_civilized const civilized =
      config_natives.tribes[tribe].civilized;

  for( e_unit_activity act :
       refl::enum_values<e_unit_activity> ) {
    maybe<e_native_civilized> const minimum_level =
        config_natives.dwelling_expertise.minimum_level[act];
    if( !minimum_level.has_value() ||
        civilized < *minimum_level ) {
      weights[act] = 0;
      continue;
    }
    weights[act] *= lround(
        config_natives.dwelling_expertise.scale_factors[act] );
  }

  int total_weights = 0;
  for( e_unit_activity act :
       refl::enum_values<e_unit_activity> ) {
    CHECK_GE( weights[act], 0 );
    total_weights += weights[act];
  }
  if( total_weights == 0 )
    // Make sure that at least one weight is non-zero so that we
    // can always choose something. This should never really
    // happen in practice; it is just defensive.
    weights[e_unit_activity::farming] = 1;

  // Add the seasoned scout skill.
  CHECK( weights[e_unit_activity::scouting] == 0 );
  weights[e_unit_activity::scouting] =
      lround( total_weights * config_natives.dwelling_expertise
                                  .seasoned_scout_fraction );

  // Add the fur trader skill.
  CHECK( weights[e_unit_activity::fur_trading] == 0 );
  weights[e_unit_activity::fur_trading] = lround(
      weights[e_unit_activity::fur_trapping] *
      config_natives.dwelling_expertise.fur_trader_fraction );

  // Finally, do the post-scaling.
  for( e_unit_activity act : refl::enum_values<e_unit_activity> )
    weights[act] *= lround(
        config_natives.dwelling_expertise.scale_factors[act] );

  // Recompute total weights after scaling.
  total_weights = 0;
  for( e_unit_activity act :
       refl::enum_values<e_unit_activity> ) {
    CHECK_GE( weights[act], 0 );
    total_weights += weights[act];
  }
  if( total_weights == 0 )
    // Make sure that at least one weight is non-zero so that we
    // can always choose something. This should never really
    // happen in practice; it is just defensive.
    weights[e_unit_activity::farming] = 1;

  return weights;
}

e_unit_activity select_expertise_for_dwelling(
    TS& ts, refl::enum_map<e_unit_activity, int> weights ) {
  return pick_from_weighted_enum_values( ts.rand, weights );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( select_expertise_for_dwelling, e_unit_activity,
        Dwelling& dwelling ) {
  SS&                                  ss = st["SS"].as<SS&>();
  TS&                                  ts = st["TS"].as<TS&>();
  refl::enum_map<e_unit_activity, int> weights =
      dwelling_expertise_weights( ss, dwelling );
  return select_expertise_for_dwelling( ts, weights );
}

} // namespace

} // namespace rn
