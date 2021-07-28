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
#include "co-waitable.hpp"
#include "fmt-helper.hpp"
#include "ustate.hpp"
#include "window.hpp"

// Rds
#include "rds/ui-enums.hpp"

using namespace std;

namespace rn {

namespace {

struct DisbandHandler : public OrdersHandler {
  DisbandHandler( UnitId unit_id_ ) : unit_id( unit_id_ ) {}

  waitable<bool> confirm() override {
    auto q = fmt::format( "Really disband {}?",
                          unit_from_id( unit_id ).desc().name );

    ui::e_confirm answer = co_await ui::yes_no( q );
    co_return answer == ui::e_confirm::yes;
  }

  waitable<> perform() override {
    destroy_unit( unit_id );
    co_return;
  }

  UnitId unit_id;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
std::unique_ptr<OrdersHandler> handle_orders(
    UnitId id, orders::disband const& ) {
  return make_unique<DisbandHandler>( id );
}

} // namespace rn
