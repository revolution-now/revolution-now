/****************************************************************
**command-build.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-16.
*
* Description: Carries out commands to build a colony
*
*****************************************************************/
#include "command-build.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "colony-view.hpp"
#include "connectivity.hpp"
#include "maybe.hpp"
#include "minds.hpp"
#include "ts.hpp"
#include "woodcut.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// Rds
#include "ui-enums.rds.hpp"

// base
#include "base/string.hpp"

using namespace std;

namespace rn {

namespace {

valid_or<string> is_valid_colony_name_msg(
    ColoniesState const& colonies_state, string_view name ) {
  if( base::trim( name ) != name )
    return invalid<string>(
        "Colony name must not start or end with spaces." );
  auto res = is_valid_new_colony_name( colonies_state, name );
  if( res ) return valid;
  switch( res.error() ) {
    case e_new_colony_name_err::already_exists:
      return invalid<string>(
          "There is already a colony with that name!" );
  }
}

struct BuildHandler : public CommandHandler {
  BuildHandler( SS& ss, TS& ts, Player& player, UnitId unit_id_ )
    : ss_( ss ),
      ts_( ts ),
      player_( player ),
      euro_mind_( ts.euro_minds[player.nation] ),
      unit_id( unit_id_ ) {}

  wait<bool> confirm() override {
    if( auto valid =
            unit_can_found_colony( SSConst( ss_ ), unit_id );
        !valid ) {
      switch( valid.error() ) {
        case e_found_colony_err::colony_exists_here:
          co_await ts_.gui.message_box(
              "There is already a colony on this "
              "square." );
          co_return false;
        case e_found_colony_err::too_close_to_colony:
          // TODO: put the name of the adjacent colony here for a
          // better message.
          co_await ts_.gui.message_box(
              "Cannot found a colony in a square that is "
              "adjacent to an existing colony." );
          co_return false;
        case e_found_colony_err::no_water_colony:
          co_await ts_.gui.message_box(
              "Cannot found a colony on water." );
          co_return false;
        case e_found_colony_err::no_mountain_colony:
          co_await ts_.gui.message_box(
              "Cannot found a colony on mountains." );
          co_return false;
        case e_found_colony_err::
            non_colonist_cannot_found_colony:
          co_await ts_.gui.message_box(
              "Only colonist units can found colonies." );
          co_return false;
        case e_found_colony_err::native_convert_cannot_found:
          co_await ts_.gui.message_box(
              "Native converts cannot found new colonies." );
          co_return false;
        case e_found_colony_err::unit_cannot_found:
          co_await ts_.gui.message_box(
              "This unit cannot found new colonies." );
          co_return false;
        case e_found_colony_err::ship_cannot_found_colony:
          // No message box here since this should be obvious to
          // the player.
          co_return false;
        case e_found_colony_err::colonist_not_on_map:
          SHOULD_NOT_BE_HERE;
      }
    }

    Coord const location = ss_.units.coord_for( unit_id );
    if( !colony_has_ocean_access( ss_, ts_.connectivity,
                                  location ) ) {
      YesNoConfig const config{
          .msg =
              "Your Excellency, this square does not have "
              "[ocean access].  This means that we will not be "
              "able to access it by ship and thus we will have "
              "to build a wagon train to transport goods to and "
              "from it.",
          .yes_label =
              "Yes, that is exactly what I had in mind.",
          .no_label       = "Nevermind, I forgot about that.",
          .no_comes_first = true };
      maybe<ui::e_confirm> const answer =
          co_await ts_.gui.optional_yes_no( config );
      if( answer != ui::e_confirm::yes ) co_return false;
    }

    while( true ) {
      colony_name = co_await ts_.gui.optional_string_input(
          { .msg =
                "What shall this colony be named, your majesty?",
            .initial_text = colony_name.value_or( "" ) } );
      if( !colony_name.has_value() ) co_return false;
      valid_or<string> is_valid =
          is_valid_colony_name_msg( ss_.colonies, *colony_name );
      if( is_valid ) co_return true;
      co_await ts_.gui.message_box( is_valid.error() );
    }
  }

  wait<> perform() override {
    co_await show_woodcut_if_needed(
        player_, euro_mind_, e_woodcut::building_first_colony );
    colony_id =
        found_colony( ss_, ts_, player_, unit_id, *colony_name );
    e_colony_abandoned const abandoned =
        co_await ts_.colony_viewer.show( ts_, colony_id );
    if( abandoned == e_colony_abandoned::yes )
      // Nothing special to do here.
      co_return;
    co_return;
  }

  SS&        ss_;
  TS&        ts_;
  Player&    player_;
  IEuroMind& euro_mind_;

  UnitId unit_id;

  maybe<string> colony_name;
  ColonyId      colony_id;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<CommandHandler> handle_command(
    SS& ss, TS& ts, Player& player, UnitId id,
    command::build const& ) {
  return make_unique<BuildHandler>( ss, ts, player, id );
}

} // namespace rn
