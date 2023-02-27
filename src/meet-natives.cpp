/****************************************************************
**meet-natives.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-09.
*
* Description: Handles the sequence of events that happen when
*              first encountering a native tribe.
*
*****************************************************************/
#include "meet-natives.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "igui.hpp"
#include "logger.hpp"
#include "native-owned.hpp"
#include "society.hpp"
#include "ts.hpp"
#include "woodcut.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/natives.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/tribe.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"

using namespace std;

namespace rn {

namespace {

MeetTribe check_meet_tribe_single( SSConst const& ss,
                                   Player const&  player,
                                   e_tribe        tribe ) {
  // Compute the land "occupied" by the player that will be
  // awarded to them by this tribe.
  lg.debug( "meeting the {} tribe.", tribe );

  // 1. Compute all land square occupied by the player, meaning
  // the squares containing colonies and the outdoor workers in
  // those colonies.
  unordered_set<Coord>   land_occupied;
  vector<ColonyId> const colonies =
      ss.colonies.for_nation( player.nation );
  for( ColonyId colony_id : colonies ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    Coord const   home   = colony.location;
    land_occupied.insert( home );
    for( auto& [direction, outdoor_unit] :
         colony.outdoor_jobs ) {
      if( !outdoor_unit.has_value() ) continue;
      Coord const outdoor = home.moved( direction );
      land_occupied.insert( outdoor );
    }
  }

  // 2. Get all dwellings for this tribe.
  UNWRAP_CHECK( dwellings,
                ss.natives.dwellings_for_tribe( tribe ) );

  // 3. For each occupied square, see if it is owned by one of
  // the above dwellings.
  unordered_set<Coord> land_awarded;
  for( Coord occupied : land_occupied ) {
    maybe<DwellingId> const owning_dwelling_id =
        is_land_native_owned_after_meeting_without_colonies(
            ss, player, occupied );
    if( !owning_dwelling_id.has_value() ) continue;
    if( !dwellings.contains( *owning_dwelling_id ) ) continue;
    // The square is owned by natives of this tribe, so award it
    // to the player.
    land_awarded.insert( occupied );
  }
  vector<Coord> sorted_land_awarded( land_awarded.begin(),
                                     land_awarded.end() );
  sort( sorted_land_awarded.begin(), sorted_land_awarded.end() );

  int const num_dwellings = dwellings.size();
  return MeetTribe{
      .is_first      = false, // filled out later.
      .tribe         = tribe,
      .num_dwellings = num_dwellings,
      .land_awarded  = std::move( sorted_land_awarded ) };
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
vector<MeetTribe> check_meet_tribes( SSConst const& ss,
                                     Player const&  player,
                                     Coord          coord ) {
  vector<MeetTribe> res;
  MapSquare const&  square = ss.terrain.square_at( coord );
  if( square.surface == e_surface::water )
    // Cannot make an initial encounter with the natives from a
    // water square.
    return res;
  unordered_set<e_tribe> met;
  for( e_direction d : refl::enum_values<e_direction> ) {
    Coord const moved = coord.moved( d );
    if( !ss.terrain.square_exists( moved ) ) continue;
    maybe<Society> const society =
        society_on_square( ss, moved );
    if( !society.has_value() ) continue;
    maybe<Society::native const&> native =
        society->get_if<Society::native>();
    if( !native.has_value() ) continue;
    if( met.contains( native->tribe ) ) continue;
    if( ss.natives.tribe_for( native->tribe )
            .relationship[player.nation]
            .encountered )
      continue;
    // We're meeting a new tribe.
    met.insert( native->tribe );
    res.push_back(
        check_meet_tribe_single( ss, player, native->tribe ) );
  }

  if( res.empty() ) return res;

  // Determines if this is the very first tribe we're meeting. We
  // do this last in order to avoid running this every time a
  // unit moves to a new square, since it will yield "no" the
  // vast majority of the time.
  for( e_tribe tribe : refl::enum_values<e_tribe> ) {
    if( !ss.natives.tribe_exists( tribe ) ) continue;
    Tribe const& tribe_obj = ss.natives.tribe_for( tribe );
    if( tribe_obj.relationship[player.nation].encountered )
      // We've already met at least one tribe, so we're done.
      return res;
  }
  CHECK( !res.empty() );
  res[0].is_first = true;
  return res;
}

wait<e_declare_war_on_natives> perform_meet_tribe_ui_sequence(
    TS& ts, Player& player, MeetTribe const& meet_tribe ) {
  co_await display_woodcut_if_needed(
      ts, player, e_woodcut::meeting_the_natives );
  if( meet_tribe.tribe == e_tribe::inca )
    co_await display_woodcut_if_needed(
        ts, player, e_woodcut::meeting_the_inca_nation );
  else if( meet_tribe.tribe == e_tribe::aztec )
    co_await display_woodcut_if_needed(
        ts, player, e_woodcut::meeting_the_aztec_empire );

  auto const& tribe_conf =
      config_natives.tribes[meet_tribe.tribe];
  ui::e_confirm accept_peace =
      co_await ts.gui.required_yes_no( YesNoConfig{
          .msg = fmt::format(
              "The [{}] tribe is a celebrated nation of "
              "[{} {}].  In honor of our glorious future "
              "together we will generously give you all of the "
              "land that your colonies now occupy. Will you "
              "accept our peace treaty and agree to live in "
              "harmony with us?",
              tribe_conf.name_singular, meet_tribe.num_dwellings,
              meet_tribe.num_dwellings > 1
                  ? config_natives
                        .dwelling_types[tribe_conf.level]
                        .name_plural
                  : config_natives
                        .dwelling_types[tribe_conf.level]
                        .name_singular ),
          .yes_label      = "Yes",
          .no_label       = "No",
          .no_comes_first = false } );
  switch( accept_peace ) {
    case ui::e_confirm::no: {
      co_await ts.gui.message_box(
          "In that case the mighty [{}] will drive you "
          "into oblivion. Prepare for WAR!",
          tribe_conf.name_singular );
      co_return e_declare_war_on_natives::yes;
    }
    case ui::e_confirm::yes:
      break;
  }

  co_await ts.gui.message_box(
      "Let us smoke a peace pipe to celebrate our purpetual "
      "friendship with the [{}].",
      config_nation.nations[player.nation].display_name );

  co_await ts.gui.message_box(
      "We hope that you will send us your colonists and "
      "[Wagon Trains] to share knowledge and to trade." );

  co_return e_declare_war_on_natives::no;
}

void perform_meet_tribe( SS& ss, Player const& player,
                         MeetTribe const&         meet_tribe,
                         e_declare_war_on_natives declare_war ) {
  Tribe& tribe = ss.natives.tribe_for( meet_tribe.tribe );

  // Create the relationship object.
  CHECK( !tribe.relationship[player.nation].encountered );
  tribe.relationship[player.nation] = TribeRelationship{
      .encountered = true,
      .at_war = ( declare_war == e_declare_war_on_natives::yes ),
      .tribal_alarm =
          config_natives.alarm
              .starting_tribal_alarm[meet_tribe.tribe] };

  // Award player any land they "occupy" that is owned by this
  // tribe. Note that if the player has Peter Minuit then this
  // should be an empty list.
  if( player.fathers.has[e_founding_father::peter_minuit] ) {
    CHECK( meet_tribe.land_awarded.empty() );
  }
  for( Coord to_award : meet_tribe.land_awarded ) {
    // We use this long version of the function to check here be-
    // cause 1) we can assume that there already is a relation-
    // ship (because we just created one above), and 2) because
    // some of the owned land squares we get in this function
    // might contain colonies, and so if we were to call the
    // normal `is_land_native_owned` it would report those as not
    // being owned. It does this in order to support the behavior
    // of the OG where it will not ask the player to acquire na-
    // tive land to build a colony there, and so colony squares
    // retain their native land ownership, just that it is ig-
    // nored. For that reason, the `is_land_native_owned` will
    // ignore native owned markers on squares with colonies. But
    // that is not what we want here for this sanity check, which
    // tests that we only receive squares that are actually owned
    // by the natives from their point of view.
    CHECK( is_land_native_owned_after_meeting_without_colonies(
               ss, player, to_award ),
           "square {} was supposed to be owned by the {} tribe "
           "but isn't owned at all.",
           to_award, meet_tribe.tribe );
    ss.natives.mark_land_unowned( to_award );
  }
}

} // namespace rn
