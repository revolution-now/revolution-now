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
#include "ustate.hpp"
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

unique_ptr<OrdersHandler> handle_orders( Planes&, SS&, TS&,
                                         Player&, UnitId,
                                         orders::wait const& ) {
  SHOULD_NOT_BE_HERE;
}

unique_ptr<OrdersHandler> handle_orders(
    Planes&, SS&, TS&, Player&, UnitId,
    orders::forfeight const& ) {
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
    Planes& planes, SS& ss, TS& ts, Player& player, UnitId id,
    orders_t const& orders ) {
  CHECK( !ss.units.unit_for( id ).mv_pts_exhausted() );
  return visit( orders, LC( handle_orders( planes, ss, ts,
                                           player, id, _ ) ) );
}

wait<OrdersHandler::RunResult> OrdersHandler::run() {
  RunResult res{ .order_was_run       = false,
                 .suspended           = false,
                 .units_to_prioritize = {} };

  // Run the given coroutine, await its result, and return it,
  // but run it under a detect that can detect if it suspended in
  // the process, and record that before returning the result.
  auto record_suspend = [&]<typename T>( wait<T> w ) -> wait<T> {
    auto info = co_await co::detect_suspend( std::move( w ) );
    res.suspended |= info.suspended;
    if constexpr( !is_same_v<T, monostate> )
      co_return std::move( info.result );
  };

  bool confirmed = co_await record_suspend( confirm() );
  if( !confirmed ) co_return res;

  unique_ptr<OrdersHandler> delegate = switch_handler();
  if( delegate != nullptr ) co_return co_await delegate->run();

  res.order_was_run = true;

  // Orders can be carried out. We don't care about the sus-
  // pending here, because what we're really interested in is
  // whether the user was prompted for something; animation al-
  // ways suspends.
  co_await animate();

  co_await record_suspend( perform() );
  co_await record_suspend( post() );

  res.units_to_prioritize = units_to_prioritize();
  co_return res;
}

} // namespace rn
