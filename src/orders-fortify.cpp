/****************************************************************
**orders-fortify.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-16.
*
* Description: Carries out orders to fortify or sentry a unit.
*
*****************************************************************/
#include "orders-fortify.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "ts.hpp"
#include "ustate.hpp"
#include "window.hpp"

using namespace std;

namespace rn {

namespace {

struct FortifyHandler : public OrdersHandler {
  FortifyHandler( UnitId unit_id_, IGui& gui_arg )
    : unit_id( unit_id_ ), gui( gui_arg ) {}

  wait<bool> confirm() override {
    if( is_unit_onboard( unit_id ) ) {
      co_await gui.message_box(
          "Cannot fortify as cargo of another unit." );
      co_return false;
    }
    co_return true;
  }

  wait<> perform() override {
    Unit& unit = unit_from_id( unit_id );
    // Note that this will forfeight movement points, since the
    // original game appears to end a unit's turn when the F
    // order is given (as well as the following turn when it is
    // transitioned to "fortified").
    unit.start_fortify();
    co_return;
  }

  UnitId unit_id;
  IGui&  gui;
};

struct SentryHandler : public OrdersHandler {
  SentryHandler( UnitId unit_id_, IGui& gui_arg )
    : unit_id( unit_id_ ), gui( gui_arg ) {}

  wait<bool> confirm() override { co_return true; }

  wait<> perform() override {
    unit_from_id( unit_id ).sentry();
    co_return;
  }

  UnitId unit_id;
  IGui&  gui;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<OrdersHandler> handle_orders(
    SS&, TS& ts, UnitId id, orders::fortify const& ) {
  return make_unique<FortifyHandler>( id, ts.gui );
}

unique_ptr<OrdersHandler> handle_orders(
    SS&, TS& ts, UnitId id, orders::sentry const& ) {
  return make_unique<SentryHandler>( id, ts.gui );
}

} // namespace rn
