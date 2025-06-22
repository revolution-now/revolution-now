/****************************************************************
**continental.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-21.
*
* Description: Handles promotion of units to continental status.
*
*****************************************************************/
#include "continental.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "igui.hpp"
#include "promotion.hpp"
#include "sons-of-liberty.hpp"
#include "unit-mgr.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// base
#include "base/conv.hpp"
#include "base/string.hpp"

using namespace std;

namespace rn {

// The formula the OG appears to use for the number of units to
// promote in a colony is:
//
//   min( floor( P*max( SoL% - 50, 0 )*2/100 ), floor( P/2 ) )
//
// It will prioritize promoting veteran dragoons, then if those
// run out it will go to veteran soldiers.
//
// There is no randomness in the number of units or the types of
// units that are promoted.
ContinentalPromotion compute_continental_promotion(
    SSConst const& ss, Player const& player,
    ColonyId const colony_id ) {
  ContinentalPromotion res;
  Colony const& colony = ss.colonies.colony_for( colony_id );
  ColonySonsOfLiberty const sol =
      compute_colony_sons_of_liberty( player, colony );
  int const population   = colony_population( colony );
  int const max_promoted = std::min(
      int( floor( population *
                  std::max( sol.sol_integral_percent - 50, 0 ) *
                  2 / 100.0 ) ),
      int( floor( population / 2.0 ) ) );
  // NOTE: Any units in the cargo of ships should already have
  // been unloaded when the declaration was done. But just in
  // case, we will use the recursive one anyway.
  vector<UnitId> units = euro_units_from_coord_recursive(
      ss.units, colony.location );
  auto const can_be_promoted = []( e_unit_type const type ) {
    auto const& promotion =
        config_unit_type.composition.unit_types[type].promotion;
    auto const modifier =
        promotion.get_if<UnitPromotion::modifier>();
    if( !modifier.has_value() ) return false;
    if( modifier->kind != e_unit_type_modifier::independence )
      return false;
    return true;
  };
  erase_if( units, [&]( UnitId const unit_id ) {
    Unit const& unit = ss.units.unit_for( unit_id );
    return !can_be_promoted( unit.type() );
  } );
  sort( units.begin(), units.end(),
        [&]( UnitId const l, UnitId const r ) {
          Unit const& l_unit = ss.units.unit_for( l );
          Unit const& r_unit = ss.units.unit_for( r );
          return l_unit.desc().combat > r_unit.desc().combat;
        } );
  int const num_will_promote =
      std::min( max_promoted, int( units.size() ) );
  CHECK_LE( num_will_promote, ssize( units ) );
  units.resize( num_will_promote );
  sort( units.begin(), units.end() );
  res.units = std::move( units );
  return res;
}

void do_continental_promotion(
    SS& ss, TS& ts, ContinentalPromotion const& promotion ) {
  for( UnitId const unit_id : promotion.units ) {
    Unit& unit = ss.units.unit_for( unit_id );
    // NOTE: for the units that we're promoting, the activity
    // won't be used, since we've already filtered only on units
    // that have a promotion mode where they are given an inde-
    // pendence modifier.
    UNWRAP_CHECK_T(
        auto const promoted_comp,
        promoted_from_activity( unit.composition(),
                                e_unit_activity{} ) );
    change_unit_type( ss, ts, unit, promoted_comp );
  }
}

wait<> continental_promotion_ui_seq(
    SSConst const& ss, IGui& gui,
    ContinentalPromotion const& promotion,
    ColonyId const colony_id ) {
  vector<UnitId> const& units = promotion.units;
  if( units.empty() ) co_return;
  int const count      = ssize( units );
  Colony const& colony = ss.colonies.colony_for( colony_id );
  string const unit_str =
      ( count == 1 ) ? "unit has" : "units have";
  string const msg = format(
      "The colony of [{}] has mobilized its Continental Army! "
      "[{}] veteran {} been promoted to [Continental] status.",
      colony.name,
      base::capitalize_initials(
          base::int_to_string_literary( count ) ),
      unit_str );
  co_await gui.message_box( "{}", msg );
}

} // namespace rn
