/****************************************************************
**native-owned.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-13.
*
* Description: Handles things related to native-owned land.
*
*****************************************************************/
#include "native-owned.hpp"

// Revolution Now
#include "map-square.hpp"

// config
#include "config/natives.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"

// base
#include "base/keyval.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
maybe<DwellingId> is_land_native_owned_after_meeting(
    SSConst const& ss, Player const& player, Coord coord ) {
  if( player.fathers.has[e_founding_father::peter_minuit] )
    // Effectively no native land ownership when we have Minuit.
    return nothing;
  return base::lookup( ss.natives.owned_land_without_minuit(),
                       coord );
}

maybe<DwellingId> is_land_native_owned( SSConst const& ss,
                                        Player const&  player,
                                        Coord          coord ) {
  maybe<DwellingId> const dwelling_id =
      is_land_native_owned_after_meeting( ss, player, coord );
  if( !dwelling_id.has_value() ) return nothing;
  Dwelling const& dwelling =
      ss.natives.dwelling_for( *dwelling_id );
  Tribe const& tribe = ss.natives.tribe_for( dwelling.tribe );
  if( !tribe.relationship[player.nation].has_value() )
    return nothing;
  // Note that we don't check the "at war" status here, because
  // the OG keeps the ownership of the land even when at war with
  // the tribe, but it simply doesn't try to stop the player when
  // they try to use it.
  return *dwelling_id;
}

refl::enum_map<e_direction, maybe<DwellingId>>
native_owned_land_around_square( SSConst const& ss,
                                 Player const&  player,
                                 Coord          loc ) {
  refl::enum_map<e_direction, maybe<DwellingId>> res;
  for( e_direction d : refl::enum_values<e_direction> )
    res[d] = is_land_native_owned( ss, player, loc.moved( d ) );
  return res;
}

maybe<LandPrice> price_for_native_owned_land(
    SSConst const& ss, Player const& player, Coord coord ) {
  CHECK( ss.terrain.square_exists( coord ) );
  UNWRAP_RETURN( dwelling_id,
                 is_land_native_owned( ss, player, coord ) );
  Dwelling const& dwelling_obj =
      ss.natives.dwelling_for( dwelling_id );
  if( dwelling_obj.location == coord )
    // It is not specified whether the map generator will mark
    // the tile under a dwelling as owned, but in any case the
    // game should not be asking for the price of that tile while
    // there is still a dwelling there.
    return nothing;
  e_tribe const tribe =
      ss.natives.dwelling_for( dwelling_id ).tribe;
  LandPrice            res{ .owner = tribe };
  e_native_level const level =
      config_natives.tribes[tribe].level;
  Tribe const& tribe_obj = ss.natives.tribe_for( tribe );
  UNWRAP_RETURN( relationship,
                 tribe_obj.relationship[player.nation] );
  if( relationship.at_war ) return nothing;
  int const num_colonies =
      ss.colonies.for_nation( player.nation ).size();
  auto&        conf = config_natives.land_prices;
  double const base_price =
      conf.anchor_price +
      ( std::min( num_colonies,
                  conf.max_colonies_for_increase ) /
        2 ) *
          conf.increment_per_two_colonies +
      static_cast<int>( level ) *
          conf.increment_per_tribe_level +
      static_cast<int>( ss.settings.difficulty ) *
          conf.increment_per_difficulty_level;

  double price = base_price;
  // Apply prime resource modifier.
  MapSquare const& square = ss.terrain.square_at( coord );
  if( effective_resource( square ).has_value() )
    price *= conf.bonus_prime_resource;

  // Apply capital modifier.
  if( dwelling_obj.is_capital ) price *= conf.bonus_capital;

  // Apply tribal anger modifier. Note that we use 100 here even
  // though the upper bound of tribal alarm is 99.
  int const anger_bucket_size =
      100 / ( conf.tribal_anger_max_n + 1 );
  CHECK_GE( anger_bucket_size, 0 );
  CHECK_LE( anger_bucket_size, 100 );
  int const anger_bucket =
      relationship.tribal_alarm / anger_bucket_size;
  CHECK_GE( anger_bucket, 0 );
  CHECK_LE( anger_bucket, conf.tribal_anger_max_n ); // 3 in OG.
  price *= ( 1 + anger_bucket );

  // Apply distance modifier to dwelling.
  Coord const dwelling_square = dwelling_obj.location;
  // This is a distance indicator that is used to compute the
  // price decrease with distance. It is not an exact measure of
  // distance, but instead is calculated in a way so as to match
  // the OG price falloff with distance.
  CHECK( dwelling_square != coord ); // checked above.
  Delta const delta      = ( dwelling_square - coord ).abs();
  int const   rect_level = std::max( delta.w, delta.h ) - 1;
  CHECK_GE( rect_level, 0 );
  price *= pow( conf.distance_exponential, rect_level );

  res.price = lround( floor( price ) );
  return res;
}

} // namespace rn
