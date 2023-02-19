/****************************************************************
**orders.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-27.
*
* Description: Handles the representation and application of
*              the orders that players can give to units.
*
*****************************************************************/
#include "orders.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "co-wait.hpp"
#include "orders-build.hpp"
#include "orders-disband.hpp"
#include "orders-dump.hpp"
#include "orders-fortify.hpp"
#include "orders-move.hpp"
#include "orders-plow.hpp"
#include "orders-road.hpp"
#include "unit-mgr.hpp"
#include "variant.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/units.hpp"

// base
#include "base/lambda.hpp"

// C++ standard library
#include <queue>
#include <unordered_map>

using namespace std;

namespace rn {

namespace {

unordered_map<UnitId, queue<orders_t>> g_orders_queue;

unique_ptr<OrdersHandler> handle_orders( SS&, TS&, Player&,
                                         UnitId,
                                         orders::wait const& ) {
  SHOULD_NOT_BE_HERE;
}

unique_ptr<OrdersHandler> handle_orders(
    SS&, TS&, Player&, UnitId, orders::forfeight const& ) {
  SHOULD_NOT_BE_HERE;
}

} // namespace

void push_unit_orders( UnitId id, orders_t const& orders ) {
  g_orders_queue[id].push( orders );
}

maybe<orders_t> pop_unit_orders( UnitId id ) {
  maybe<orders_t> res{};
  if( g_orders_queue.contains( id ) ) {
    auto& q = g_orders_queue[id];
    if( !q.empty() ) {
      res = q.front();
      q.pop();
    }
  }
  return res;
}

unique_ptr<OrdersHandler> orders_handler(
    SS& ss, TS& ts, Player& player, UnitId id,
    orders_t const& orders ) {
  CHECK( !ss.units.unit_for( id ).mv_pts_exhausted() );
  return visit( orders.as_base(),
                LC( handle_orders( ss, ts, player, id, _ ) ) );
}

wait<OrdersHandlerRunResult> OrdersHandler::run() {
  OrdersHandlerRunResult res{ .order_was_run       = false,
                              .units_to_prioritize = {} };
  if( bool confirmed = co_await confirm(); !confirmed )
    co_return res;

  if( unique_ptr<OrdersHandler> delegate = switch_handler();
      delegate != nullptr )
    co_return co_await delegate->run();

  res.order_was_run = true;

  // Orders can be carried out.
  co_await animate();
  co_await perform();

  res.units_to_prioritize = units_to_prioritize();
  co_return res;
}

} // namespace rn
