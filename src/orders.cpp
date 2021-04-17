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
#include "orders-build.hpp"
#include "orders-disband.hpp"
#include "orders-fortify.hpp"
#include "orders-move.hpp"
#include "ustate.hpp"
#include "waitable-coro.hpp"

// base
#include "base/lambda.hpp"

// C++ standard library
#include <queue>
#include <unordered_map>

using namespace std;

namespace rn {

namespace {

unordered_map<UnitId, queue<orders_t>> g_orders_queue;

unique_ptr<OrdersHandler> handle_orders( UnitId,
                                         orders::wait const& ) {
  SHOULD_NOT_BE_HERE;
}

unique_ptr<OrdersHandler> handle_orders(
    UnitId, orders::forfeight const& ) {
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

std::unique_ptr<OrdersHandler> orders_handler(
    UnitId id, orders_t const& orders ) {
  auto const& unit = unit_from_id( id );
  // TODO: consolidate these checks
  CHECK( unit.movement_points() > 0 );
  CHECK( !unit.mv_pts_exhausted() );
  CHECK( unit.orders_mean_input_required() );

  return visit( orders, LC( handle_orders( id, _ ) ) );
}

} // namespace rn
