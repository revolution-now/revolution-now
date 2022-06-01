/****************************************************************
**orders-build.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-16.
*
* Description: Carries out orders to build a colony
*
*****************************************************************/
#include "orders-build.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "colony-view.hpp"
#include "map-updater.hpp"
#include "maybe.hpp"
#include "window.hpp"

// Rds
#include "ui-enums.rds.hpp"

// base
#include "base/string.hpp"

using namespace std;

namespace rn {

namespace {

valid_or<string> is_valid_colony_name_msg( string_view name ) {
  if( base::trim( name ) != name )
    return invalid<string>(
        "Colony name must not start or end with spaces." );
  auto res = is_valid_new_colony_name( name );
  if( res ) return valid;
  switch( res.error() ) {
    case e_new_colony_name_err::already_exists:
      return invalid<string>(
          "There is already a colony with that name!" );
  }
}

struct BuildHandler : public OrdersHandler {
  BuildHandler( IMapUpdater* map_updater_arg, IGui& gui_arg,
                UnitId unit_id_ )
    : map_updater( map_updater_arg ),
      gui( gui_arg ),
      unit_id( unit_id_ ) {}

  wait<bool> confirm() override {
    if( auto valid = unit_can_found_colony( unit_id ); !valid ) {
      switch( valid.error() ) {
        case e_found_colony_err::colony_exists_here:
          co_await gui.message_box(
              "There is already a colony on this "
              "square." );
          co_return false;
        case e_found_colony_err::too_close_to_colony:
          // TODO: put the name of the adjacent colony here for a
          // better message.
          co_await gui.message_box(
              "Cannot found a colony in a square that is "
              "adjacent to an existing colony." );
          co_return false;
        case e_found_colony_err::no_water_colony:
          co_await gui.message_box(
              "Cannot found a colony on water." );
          co_return false;
        case e_found_colony_err::non_human_cannot_found_colony:
          co_await gui.message_box(
              "Only human units can found colonies." );
          co_return false;
        case e_found_colony_err::ship_cannot_found_colony:
          // No message box here since this should be obvious to
          // the player.
          co_return false;
        case e_found_colony_err::colonist_not_on_map:
          SHOULD_NOT_BE_HERE;
      }
    }

    ui::e_confirm proceed =
        co_await gui.yes_no( { .msg       = "Build colony here?",
                               .yes_label = "Yes",
                               .no_label  = "No" } );
    if( proceed == ui::e_confirm::no ) co_return false;
    while( true ) {
      colony_name = co_await ui::str_input_box(
          "Question",
          "What shall this colony be named, your majesty?",
          /*initial_text=*/colony_name.value_or( "" ) );
      if( !colony_name.has_value() ) co_return false;
      valid_or<string> is_valid =
          is_valid_colony_name_msg( *colony_name );
      if( is_valid ) co_return true;
      co_await gui.message_box( is_valid.error() );
    }
  }

  wait<> perform() override {
    colony_id = found_colony_unsafe( unit_id, *map_updater,
                                     *colony_name );
    co_return;
  }

  wait<> post() const override {
    return show_colony_view( colony_id, *map_updater );
  }

  IMapUpdater*  map_updater;
  IGui&         gui;
  UnitId        unit_id;
  maybe<string> colony_name;
  ColonyId      colony_id;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<OrdersHandler> handle_orders(
    UnitId       id, orders::build const& /*build*/,
    IMapUpdater* map_updater, IGui& gui, Player&,
    TerrainState const&, UnitsState&, SettingsState const& ) {
  return make_unique<BuildHandler>( map_updater, gui, id );
}

} // namespace rn
