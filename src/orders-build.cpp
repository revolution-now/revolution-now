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
#include "maybe.hpp"
#include "ts.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/ref.hpp"

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

struct BuildHandler : public OrdersHandler {
  BuildHandler( Planes& planes, SS& ss, TS& ts, UnitId unit_id_ )
    : planes_( planes ),
      ss_( ss ),
      ts_( ts ),
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
        case e_found_colony_err::non_human_cannot_found_colony:
          co_await ts_.gui.message_box(
              "Only human units can found colonies." );
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

    ui::e_confirm proceed =
        co_await ts_.gui.yes_no( { .msg = "Build colony here?",
                                   .yes_label = "Yes",
                                   .no_label  = "No" } );
    if( proceed == ui::e_confirm::no ) co_return false;
    while( true ) {
      colony_name = co_await ts_.gui.string_input(
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
    colony_id = found_colony( ss_, ts_, unit_id, *colony_name );
    co_return;
  }

  wait<> post() const override {
    e_colony_abandoned const abandoned =
        co_await show_colony_view(
            planes_, ss_, ts_,
            ss_.colonies.colony_for( colony_id ) );
    if( abandoned == e_colony_abandoned::yes )
      // Nothing special to do here.
      co_return;
  }

  Planes& planes_;
  SS&     ss_;
  TS&     ts_;

  UnitId unit_id;

  maybe<string> colony_name;
  ColonyId      colony_id;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<OrdersHandler> handle_orders( Planes& planes, SS& ss,
                                         TS& ts, UnitId id,
                                         orders::build const& ) {
  return make_unique<BuildHandler>( planes, ss, ts, id );
}

} // namespace rn
