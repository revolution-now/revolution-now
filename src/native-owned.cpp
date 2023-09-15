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
#include "alarm.hpp"
#include "co-wait.hpp"
#include "igui.hpp"
#include "map-square.hpp"
#include "ts.hpp"

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
maybe<DwellingId>
is_land_native_owned_after_meeting_without_colonies(
    SSConst const& ss, Player const& player, Coord coord ) {
  if( player.fathers.has[e_founding_father::peter_minuit] )
    // Effectively no native land ownership when we have Minuit.
    return nothing;
  return base::lookup( ss.natives.owned_land_without_minuit(),
                       coord );
}

maybe<DwellingId> is_land_native_owned_after_meeting(
    SSConst const& ss, Player const& player, Coord coord ) {
  maybe<DwellingId> const dwelling_id =
      is_land_native_owned_after_meeting_without_colonies(
          ss, player, coord );
  if( !dwelling_id.has_value() ) return nothing;
  // If there is a friendly colony on the square then the square
  // is effectively not owned. The OG does not require taking or
  // paying for land when founding a colony, but it also will not
  // remove the ownership (otherwise the player could just found
  // a bunch of temporary colonies to remove native owned land).
  if( ss.colonies.maybe_from_coord( coord ).has_value() )
    return nothing;
  return *dwelling_id;
}

maybe<DwellingId> is_land_native_owned( SSConst const& ss,
                                        Player const&  player,
                                        Coord          coord ) {
  maybe<DwellingId> const dwelling_id =
      is_land_native_owned_after_meeting( ss, player, coord );
  if( !dwelling_id.has_value() ) return nothing;
  Dwelling const& dwelling =
      ss.natives.dwelling_for( *dwelling_id );
  Tribe const& tribe = ss.natives.tribe_for( dwelling.id );
  if( !tribe.relationship[player.nation].encountered )
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
  Coord const location = ss.natives.coord_for( dwelling_id );
  if( location == coord )
    // It is not specified whether the map generator will mark
    // the tile under a dwelling as owned, but in any case the
    // game should not be asking for the price of that tile while
    // there is still a dwelling there.
    return nothing;
  e_tribe const tribe = ss.natives.tribe_for( dwelling_id ).type;
  LandPrice     res{ .owner = tribe };
  e_native_level const level =
      config_natives.tribes[tribe].level;
  Tribe const& tribe_obj = ss.natives.tribe_for( tribe );
  TribeRelationship const& relationship =
      tribe_obj.relationship[player.nation];
  int const num_colonies =
      ss.colonies.for_nation( player.nation ).size();
  auto&        conf = config_natives.land_prices;
  double const base_price =
      conf.anchor_price +
      ( std::min( num_colonies,
                  conf.max_colonies_for_increase ) /
        2 ) *
          conf.increment_per_two_colonies +
      relationship.land_squares_paid_for *
          conf.increment_per_paid_land_square +
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
  Coord const dwelling_square = location;
  // This is a distance indicator that is used to compute the
  // price decrease with distance. It is not an exact measure of
  // distance, but instead is calculated in a way so as to match
  // the OG price falloff with distance.
  CHECK( dwelling_square != coord ); // checked above.
  int const rect_level =
      dwelling_square.concentric_square_distance( coord ) - 1;
  CHECK_GE( rect_level, 0 );
  price *= pow( conf.distance_exponential, rect_level );

  res.price = lround( floor( price ) );
  return res;
}

wait<base::NoDiscard<bool>> prompt_player_for_taking_native_land(
    SS& ss, TS& ts, Player& player, Coord tile,
    e_native_land_grab_type context ) {
  UNWRAP_CHECK(
      price, price_for_native_owned_land( ss, player, tile ) );
  Tribe&             tribe = ss.natives.tribe_for( price.owner );
  TribeRelationship& relationship =
      tribe.relationship[player.nation];
  if( relationship.at_war ) {
    // In the case of being at war with the tribe, the OG still
    // shows the totem poles, but just allows the player to place
    // units anyway without prompting.
    increase_tribal_alarm_from_land_grab( ss, player,
                                          relationship, tile );
    ss.natives.mark_land_unowned( tile );
    co_return true;
  }

  EnumChoiceConfig                                  config;
  refl::enum_map<e_native_land_grab_result, string> names;
  refl::enum_map<e_native_land_grab_result, bool>   disabled;

  names[e_native_land_grab_result::cancel] =
      "Very well, we will respect your wishes.";
  names[e_native_land_grab_result::pay] = fmt::format(
      "We offer you [{}] in compensation for this land.",
      price.price );
  if( price.price > player.money )
    disabled[e_native_land_grab_result::pay] = true;

  switch( context ) {
    case e_native_land_grab_type::in_colony:
      config.msg = fmt::format(
          "You are trespassing on [{}] land.  Please leave "
          "promptly.",
          config_natives.tribes[tribe.type].name_possessive );
      names[e_native_land_grab_result::take] =
          "You are mistaken... this is OUR land now!";
      break;
    case e_native_land_grab_type::found_colony:
      config.msg = fmt::format(
          "You are trespassing on [{}] land.  Please leave "
          "promptly.",
          config_natives.tribes[tribe.type].name_possessive );
      names[e_native_land_grab_result::take] =
          "You are mistaken... this is OUR land now!";
      break;
    case e_native_land_grab_type::clear_forest:
      config.msg = fmt::format(
          "These [forests] are a vital part of the "
          "[{}] way of life.  Please do not disturb them.",
          config_natives.tribes[tribe.type].name_singular );
      names[e_native_land_grab_result::cancel] =
          "We will respect your wishes and not disturb these "
          "forests.";
      names[e_native_land_grab_result::take] =
          "Feel free to watch and enjoy the sounds of these "
          "trees falling!";
      break;
    case e_native_land_grab_type::irrigate:
      config.msg = fmt::format(
          "These grounds help to sustain the life and spirit of "
          "the [{}] people. Please do not disturb them.",
          config_natives.tribes[tribe.type].name_singular );
      names[e_native_land_grab_result::cancel] =
          "We will respect your wishes and not irrigate this "
          "land.";
      names[e_native_land_grab_result::take] =
          "You are mistaken... this is OUR land now!";
      break;
    case e_native_land_grab_type::build_road:
      config.msg = fmt::format(
          "Carving a [road] through this land will do "
          "likewise through the hearts of the [{}] people "
          "who occupy them. Please do not disturb them.",
          config_natives.tribes[tribe.type].name_singular );
      names[e_native_land_grab_result::take] =
          "You are mistaken... this is OUR land now!";
      break;
  }

  maybe<e_native_land_grab_result> const response =
      co_await ts.gui
          .optional_enum_choice<e_native_land_grab_result>(
              config, names, disabled );
  if( !response.has_value() ) co_return false;

  switch( *response ) {
    case e_native_land_grab_result::cancel:
      co_return false;
    case e_native_land_grab_result::pay:
      player.money -= price.price;
      CHECK_GE( player.money, 0 );
      // When paying the natives for the land it won't increase
      // tribal alarm, but it will increase this counter which
      // will cause the subsequent price to go up.
      ++relationship.land_squares_paid_for;
      ss.natives.mark_land_unowned( tile );
      co_return true;
    case e_native_land_grab_result::take:
      increase_tribal_alarm_from_land_grab( ss, player,
                                            relationship, tile );
      ss.natives.mark_land_unowned( tile );
      co_return true;
  }
}

} // namespace rn
