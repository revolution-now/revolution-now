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

// C++ standard library
#include <queue>
#include <unordered_map>

using namespace std;

namespace rn {

namespace {

unordered_map<UnitId, queue<orders_t>> g_orders_queue;

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

} // namespace rn
