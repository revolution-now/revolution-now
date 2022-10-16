/****************************************************************
**orders-disband.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-16.
*
* Description: Carries out orders to disband a unit.
*
*****************************************************************/
#include "orders-disband.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "ts.hpp"
#include "ustate.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/units.hpp"

// Rds
#include "ui-enums.rds.hpp"

using namespace std;

namespace rn {

namespace {

struct DisbandHandler : public OrdersHandler {
  DisbandHandler( UnitId unit_id_, IGui& gui_arg,
                  UnitsState& units_state_arg )
    : unit_id( unit_id_ ),
      gui( gui_arg ),
      units_state( units_state_arg ) {}

  wait<bool> confirm() override {
    auto q = fmt::format(
        "Really disband {}?",
        units_state.unit_for( unit_id ).desc().name );

    maybe<ui::e_confirm> const answer =
        co_await gui.optional_yes_no(
            { .msg = q, .yes_label = "Yes", .no_label = "No" } );
    co_return answer == ui::e_confirm::yes;
  }

  wait<> perform() override {
    units_state.destroy_unit( unit_id );
    co_return;
  }

  UnitId      unit_id;
  IGui&       gui;
  UnitsState& units_state;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<OrdersHandler> handle_orders(
    Planes&, SS& ss, TS& ts, Player&, UnitId id,
    orders::disband const& ) {
  return make_unique<DisbandHandler>( id, ts.gui, ss.units );
}

} // namespace rn
