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

struct Planes;
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

  struct RunResult {
    // Was the order carried out or not.
    bool order_was_run = false;
    // Regardless of the value of `order_run`, was there a corou-
    // tine suspension during the process (other than during an
    // animation)? This is basically used to infer whether the
    // user was prompted for anything during the process, which
    // is useful to know in order for the caller to have a pol-
    // ished user interface.
    bool suspended = false;
  };

  // Run though the entire sequence of
  wait<RunResult> run();

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

  // Animate the orders being carried out, if any. This should be
  // run before `perform`.
  virtual wait<> animate() const { return make_wait<>(); }

  // Perform the orders (i.e., make changes to game state).
  virtual wait<> perform() = 0;

  // Perform any actions that should be done after affecting the
  // orders. For example, after founding a colony, this will open
  // the colony view.
  virtual wait<> post() const { return make_wait<>(); }

  // Any units that need to be prioritized (in the sense of
  // asking for orders) after this order has been carried out. An
  // example of this would be after units make landfall from a
  // ship, it is natural for them to ask for orders right away.
  virtual std::vector<UnitId> units_to_prioritize() const {
    return {};
  }
};

std::unique_ptr<OrdersHandler> orders_handler(
    Planes& planes, SS& ss, TS& ts, Player& player, UnitId id,
    orders_t const& orders );

} // namespace rn
