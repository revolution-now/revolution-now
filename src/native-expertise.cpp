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
#include "rand-enum.hpp"
#include "ts.hpp"

// config
#include "config/natives.rds.hpp"

// ss
#include "ss/colony-enums.rds.hpp"
#include "ss/dwelling.hpp"
#include "ss/native-enums.hpp"
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
    refl::enum_map<e_native_skill, int>& out, int& total_squares,
    int& num_forest_squares ) {
  refl::enum_map<e_native_skill, int> res;
  for( e_outdoor_job job : refl::enum_values<e_outdoor_job> ) {
    e_unit_activity const activity =
        activity_for_outdoor_job( job );
    maybe<e_native_skill> const skill =
        native_skill_for_activity( activity );
    if( !skill.has_value() )
      // In practice (with OG rules) this should only be lumber.
      continue;
    // Compute land production as if by a free_colonist belonging
    // to a player that has no founding fathers.
    out[*skill] += production_on_square(
        job, ss.terrain,
        /*fathers=*/{}, e_unit_type::free_colonist, square );
  }
  ++total_squares;
  if( ss.terrain.square_at( square ).overlay ==
      e_land_overlay::forest )
    ++num_forest_squares;
}

void add_outdoor_weights_around_square(
    SSConst const& ss, Coord start,
    refl::enum_map<e_native_skill, int>& out, int& total_squares,
    int& num_forest_squares ) {
  //    x x x
  //  x x x x x
  //  x x c x x
  //  x x x x x
  //    x x x
  Rect const r = Rect::from( start, Delta{ .w = 1, .h = 1 } )
                     .with_border_added( 2 );
  int count = 0; // for sanity checking.
  for( Rect sub_rect : gfx::subrects( r ) ) {
    Coord const coord = sub_rect.upper_left();
    Delta const delta = coord - start;
    // Remove corners.
    if( abs( delta.w ) + abs( delta.h ) >= 4 ) continue;
    if( ss.terrain.square_exists( coord ) )
      add_outdoor_weights_for_square(
          ss, coord, out, total_squares, num_forest_squares );
    ++count;
  }
  CHECK( count == 21 );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
refl::enum_map<e_native_skill, int> dwelling_expertise_weights(
    SSConst const& ss, Dwelling const& dwelling ) {
  refl::enum_map<e_native_skill, int> weights;
  int                                 num_forest_squares = 0;
  int                                 total_squares      = 0;
  add_outdoor_weights_around_square( ss, dwelling.location,
                                     weights, total_squares,
                                     num_forest_squares );
  CHECK_GT( total_squares, 0 );
  double const forest_fraction =
      double( num_forest_squares ) / total_squares;
  e_tribe const        tribe = dwelling.tribe;
  e_native_level const level =
      config_natives.tribes[tribe].level;

  int total_weights = 0;
  for( e_native_skill skill :
       refl::enum_values<e_native_skill> ) {
    CHECK_GE( weights[skill], 0 );
    total_weights += weights[skill];
  }
  if( total_weights == 0 )
    // Make sure that at least one weight is non-zero so that we
    // can always choose something. This should never really
    // happen in practice; it is just defensive.
    weights[e_native_skill::farming] = 1;

  // Add the seasoned scout skill.
  CHECK( weights[e_native_skill::scouting] == 0 );
  weights[e_native_skill::scouting] = lround(
      total_weights * forest_fraction *
      config_natives.dwelling_expertise.seasoned_scout_scale );

  // Add the fur trader skill.
  CHECK( weights[e_native_skill::fur_trading] == 0 );
  weights[e_native_skill::fur_trading] = lround(
      weights[e_native_skill::fur_trapping] *
      config_natives.dwelling_expertise.fur_trader_fraction );

  // Finally, do the post-scaling.
  for( e_native_skill skill : refl::enum_values<e_native_skill> )
    weights[skill] *= lround( config_natives.dwelling_expertise
                                  .scale_factors[level][skill] );

  // Recompute total weights after scaling.
  total_weights = 0;
  for( e_native_skill skill :
       refl::enum_values<e_native_skill> ) {
    CHECK_GE( weights[skill], 0 );
    total_weights += weights[skill];
  }
  if( total_weights == 0 )
    // Make sure that at least one weight is non-zero so that we
    // can always choose something. This should never really
    // happen in practice; it is just defensive.
    weights[e_native_skill::farming] = 1;

  return weights;
}

e_native_skill select_expertise_for_dwelling(
    TS& ts, refl::enum_map<e_native_skill, int> weights ) {
  return pick_from_weighted_enum_values( ts.rand, weights );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( select_expertise_for_dwelling, e_native_skill,
        Dwelling& dwelling ) {
  SS&                                 ss = st["SS"].as<SS&>();
  TS&                                 ts = st["TS"].as<TS&>();
  refl::enum_map<e_native_skill, int> weights =
      dwelling_expertise_weights( ss, dwelling );
  return select_expertise_for_dwelling( ts, weights );
}

} // namespace

} // namespace rn
