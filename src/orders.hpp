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
#include "fmt-helper.hpp"
#include "geo-types.hpp"
#include "job.hpp"
#include "movement.hpp"
#include "unit.hpp"

// C++ standard library
#include <variant>
#include <vector>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Definitions
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Order:  A general term describing what the player proposes
 *         that a unit do when the unit asks the player.  This
 *         could include "wait", "goto tile X", "move to tile X",
 *         "forfeight movement points", "plow square", "build
 *         colony".  Orders only represent what the player has
 *         proposed, and so it makes sense to talk about orders
 *         that are not permitted.  For example, the player
 *         might give orders to a unit to "move left", but doing
 *         so might cause a land unit to go into a sea square
 *         without a ship, which would not be allowed.  In that
 *         case, the orders are not allowed.
 *
 * Order Types
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Move:   This is an order wherein the player instructs a unit
 *         to move to an adjacent square (whether or not it
 *         would be allowed and whatever the consequences). For
 *         example, a unit being told to move on land is a Move
 *         order and also an attack order is a Move order, even
 *         though it wouldn't cause the unit to change squares
 *         (even if it wins).
 *
 * Job:    This is an order that instructs a unit to perform
 *         some action that does not require moving or impinging
 *         upon another square.  Examples of this are "build
 *         colony", "plow square", "fortify", etc.  It is implied
 *         that such an order will consume the remainder of the
 *         unit's movement points this turn.
 *
 * Meta:   These are orders concerning orders.  For example, an
 *         order of "wait" means to ask for orders again later
 *         in the turn.  An order of "forfeight" means that the
 *         unit wishes to give up the remainder of its turn (
 *         i.e., not ask for orders this t urn).  An order of
 *         "goto" means that the unit should start receiving
 *         automatic orders for movement.
 */

#define ORDER_MONOSTATE( __o )    \
  namespace rn::orders {          \
  struct __o##_t {};              \
  inline constexpr __o##_t __o{}; \
  }                               \
  DEFINE_FORMAT_( rn::orders::__o##_t, "{}", TO_STRING( __o ) );

// Define monostate orders with a single macro statement.

ORDER_MONOSTATE( quit ); // temporary until we can quit properly
ORDER_MONOSTATE( wait );
ORDER_MONOSTATE( forfeight );

// Define multi-state orders with the struct and formatting spec.

namespace rn::orders {

struct move {
  e_direction d;
};

} // namespace rn::orders

DEFINE_FORMAT( rn::orders::move, "move{{{{{}}}}}", o.d );

namespace rn {

// These are always allowed.
struct ProposedMetaOrderAnalysisResult {
  // Will execution of this order require the unit to forfeight
  // its remaining movement points this turn (no matter how many
  // it may have).
  bool mv_points_forfeighted;
};

// This algebraic data type is a representation of the orders
// that a player can attempt to give to a unit. This means that
// the orders so described may be invalid on a particular unit at
// a particular time. Roughly speaking, it captures the player
// input when the player is asked to give orders to a unit,
// without regard to the consequences of those orders. NOTE:
// should not have any duplicate types.
using PlayerUnitOrders = std::variant<
    // TODO: this is temporary until we can quit properly
    orders::quit_t,
    // defer to later in same turn
    orders::wait_t,
    // forfeight remainder of turn
    orders::forfeight_t,
    // moving on the map
    orders::move>;

void push_unit_orders( UnitId                  id,
                       PlayerUnitOrders const& orders );

Opt<PlayerUnitOrders> pop_unit_orders( UnitId id );

using ProposedOrdersAnalysisResult = std::variant<
    // Orders about orders
    ProposedMetaOrderAnalysisResult,
    // If unit is to move on the map
    ProposedMoveAnalysisResult,
    // actions in current tile
    ProposedJobAnalysisResult>;

struct ProposedOrdersAnalysis {
  PlayerUnitOrders             orders;
  ProposedOrdersAnalysisResult result;

  // Units that should be prioritized in the queue of units
  // waiting for orders this turn as a result of applying the
  // orders.
  std::vector<UnitId> units_to_prioritize() const;
};

ProposedOrdersAnalysis analyze_proposed_orders(
    UnitId id, PlayerUnitOrders const& orders );

// Checks that the orders are possible (if not, returns false)
// and, if so, will check the type of orders and determine
// whether the player needs to be asked for any kind of
// confirmation. In addition, if the orders are not allowed,
// the player may be given an explantation as to why.
bool confirm_explain_orders(
    ProposedOrdersAnalysis const& analysis );

void apply_orders( UnitId                        id,
                   ProposedOrdersAnalysis const& analysis );

} // namespace rn
