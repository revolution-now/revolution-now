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
#include "colony-mgr.hpp"
#include "colony-view.hpp"
#include "maybe.hpp"
#include "waitable-coro.hpp"
#include "window.hpp"

// Rnl
#include "rnl/ui-enums.hpp"

using namespace std;

namespace rn {

namespace {

valid_or<string> is_valid_colony_name_msg( string_view name ) {
  if( util::strip( name ) != name )
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
  BuildHandler( UnitId unit_id_ ) : unit_id( unit_id_ ) {}

  waitable<bool> confirm() override {
    if( auto valid = unit_can_found_colony( unit_id ); !valid ) {
      switch( valid.error() ) {
        case e_found_colony_err::colony_exists_here:
          co_await ui::message_box(
              "There is already a colony on this "
              "square." );
          co_return false;
        case e_found_colony_err::no_water_colony:
          co_await ui::message_box(
              "Cannot found a colony on water." );
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
        co_await ui::yes_no( "Build colony here?" );
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
      co_await ui::message_box( is_valid.error() );
    }
  }

  waitable<> perform() override {
    colony_id = found_colony_unsafe( unit_id, *colony_name );
    co_return;
  }

  waitable<> post() const override {
    return show_colony_view( colony_id );
  }

  UnitId        unit_id;
  maybe<string> colony_name;
  ColonyId      colony_id;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<OrdersHandler> handle_orders(
    UnitId id, orders::build const& /*build*/ ) {
  return make_unique<BuildHandler>( id );
}

} // namespace rn
