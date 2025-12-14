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
#include "iagent.hpp"
#include "imap-updater.hpp"
#include "maybe.hpp"
#include "terrain-mgr.hpp"
#include "ts.hpp"
#include "woodcut.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// Rds
#include "ui-enums.rds.hpp"

using namespace std;

namespace rn {

namespace {

struct BuildHandler : public CommandHandler {
  BuildHandler( SS& ss, TS& ts, IAgent& agent, Player& player,
                UnitId unit_id_ )
    : ss_( ss ),
      ts_( ts ),
      player_( player ),
      agent_( agent ),
      unit_id( unit_id_ ) {}

  wait<bool> confirm() override {
    if( auto valid =
            unit_can_found_colony( SSConst( ss_ ), unit_id );
        !valid ) {
      switch( valid.error() ) {
        case e_found_colony_err::colony_exists_here:
          co_await agent_.message_box(
              "There is already a colony on this "
              "square." );
          co_return false;
        case e_found_colony_err::too_close_to_colony:
          // TODO: put the name of the adjacent colony here for a
          // better message.
          co_await agent_.message_box(
              "Cannot found a colony in a square that is "
              "adjacent to an existing colony." );
          co_return false;
        case e_found_colony_err::no_water_colony:
          co_await agent_.message_box(
              "Cannot found a colony on water." );
          co_return false;
        case e_found_colony_err::no_mountain_colony:
          co_await agent_.message_box(
              "Cannot found a colony on mountains." );
          co_return false;
        case e_found_colony_err::
            non_colonist_cannot_found_colony:
          co_await agent_.message_box(
              "Only colonist units can found colonies." );
          co_return false;
        case e_found_colony_err::native_convert_cannot_found:
          co_await agent_.message_box(
              "Native converts cannot found new colonies." );
          co_return false;
        case e_found_colony_err::unit_cannot_found:
          co_await agent_.message_box(
              "This unit cannot found new colonies." );
          co_return false;
        case e_found_colony_err::ship_cannot_found_colony:
          co_await agent_.message_box(
              "Colonies cannot be built by ships." );
          co_return false;
        case e_found_colony_err::war_of_independence:
          co_await agent_.message_box(
              "Colonies cannot be founded during the War of "
              "Independence." );
          co_return false;
        case e_found_colony_err::colonist_not_on_map:
          SHOULD_NOT_BE_HERE;
        case e_found_colony_err::no_island_colony:
          co_await agent_.message_box(
              "Colonies must have at least one adjacent land "
              "tile." );
          co_return false;
      }
    }

    Coord const location = ss_.units.coord_for( unit_id );
    if( !colony_has_ocean_access(
            ss_, ts_.map_updater().connectivity(), location ) ) {
      maybe<ui::e_confirm> const answer =
          co_await agent_.confirm_build_inland_colony();
      if( answer != ui::e_confirm::yes ) co_return false;
    }

    // NOTE: this typically gets intercepted above by the
    // no_island_colony condition, but we should still warn the
    // player just in case they enable that feature, or if we de-
    // cide to enable it by default in the future.
    if( is_island( ss_, location ) ) {
      maybe<ui::e_confirm> const answer =
          co_await agent_.confirm_build_island_colony();
      if( answer != ui::e_confirm::yes ) co_return false;
    }

    // The human agent will loop here if the player types an in-
    // valid name, but the AI player only gets one chance.
    colony_name = co_await agent_.name_colony();
    if( !colony_name.has_value() ) co_return false;
    co_return is_valid_new_colony_name( ss_.colonies,
                                        *colony_name )
        .valid();
  }

  wait<> perform() override {
    co_await show_woodcut_if_needed(
        player_, agent_, e_woodcut::building_first_colony );
    colony_id =
        found_colony( ss_, ts_, player_, unit_id, *colony_name );
    e_colony_abandoned const abandoned =
        co_await ts_.colony_viewer.show( ts_, colony_id );
    if( abandoned == e_colony_abandoned::yes )
      // Nothing special to do here.
      co_return;
    co_return;
  }

  SS& ss_;
  TS& ts_;
  Player& player_;
  IAgent& agent_;

  UnitId unit_id;

  maybe<string> colony_name;
  ColonyId colony_id;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<CommandHandler> handle_command(
    IEngine&, SS& ss, TS& ts, IAgent& agent, Player& player,
    UnitId id, command::build const& ) {
  return make_unique<BuildHandler>( ss, ts, agent, player, id );
}

} // namespace rn
