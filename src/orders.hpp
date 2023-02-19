/****************************************************************
**orders.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-27.
*
* Description: Handles the representation and application of
*              the orders that players can give to units.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "igui.hpp"
#include "unit-id.hpp"
#include "wait.hpp"

// Rds
#include "orders.rds.hpp"

// C++ standard library
#include <memory>

namespace rn {

struct Player;
struct SS;
struct TS;

void push_unit_orders( UnitId id, orders_t const& orders );
maybe<orders_t> pop_unit_orders( UnitId id );

struct OrdersHandler {
  OrdersHandler()          = default;
  virtual ~OrdersHandler() = default;

  OrdersHandler( OrdersHandler const& )            = delete;
  OrdersHandler& operator=( OrdersHandler const& ) = delete;
  OrdersHandler( OrdersHandler&& )                 = delete;
  OrdersHandler& operator=( OrdersHandler&& )      = delete;

  // Run though the entire sequence of
  wait<OrdersHandlerRunResult> run();

  // This will do a few things:
  //
  //   1. it will perform more thorough checks to see that this
  //      move can be carried out; if not, will return false and
  //      typically show a message to the user.
  //   2. it will ask the user for input and/or confirmation if
  //      necessary, sometimes allowing the user to cancel the
  //      move (in which case it returns false).
  //   3. if the move can proceed, it will return true.
  //
  virtual wait<bool> confirm() = 0;

  // This will be called when `confirm` has returned true to see
  // if the handler wants to delegate to another handler for the
  // remainder. If so, this function will return non-null, then
  // the process will start over again with the new handler.
  virtual std::unique_ptr<OrdersHandler> switch_handler() {
    return nullptr;
  }

  // Animate the orders being carried out, if any. This should be
  // run before `perform`.
  virtual wait<> animate() const { return make_wait<>(); }

  // Perform the orders (i.e., make changes to game state).
  virtual wait<> perform() = 0;

 protected:
  virtual std::vector<UnitId> units_to_prioritize() const {
    return {};
  }
};

std::unique_ptr<OrdersHandler> orders_handler(
    SS& ss, TS& ts, Player& player, UnitId id,
    orders_t const& orders );

} // namespace rn
