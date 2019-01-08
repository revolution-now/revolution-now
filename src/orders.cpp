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
#include "util.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

// C++ standard library
#include <queue>

using namespace std;

namespace rn {

namespace {

absl::flat_hash_map<UnitId, queue<Orders>> g_orders_queue;

} // namespace

void push_unit_orders( UnitId id, Orders const& orders ) {
  g_orders_queue[id].push( orders );
}

Opt<Orders> pop_unit_orders( UnitId id ) {
  Opt<Orders> res{};
  if( has_key( g_orders_queue, id ) ) {
    auto& q = g_orders_queue[id];
    if( !q.empty() ) {
      res = q.front();
      q.pop();
    }
  }
  return res;
}

} // namespace rn
