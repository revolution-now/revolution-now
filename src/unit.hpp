/****************************************************************
**unit.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-28.
*
* Description: Data structure for units.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "cargo.hpp"
#include "enum.hpp"
#include "fb.hpp"
#include "id.hpp"
#include "mv-points.hpp"
#include "nation.hpp"
#include "util.hpp"
#include "utype.hpp"

// Flatbuffers
#include "fb/unit_generated.h"

// base-util
#include "base-util/non-copyable.hpp"

namespace rn {

enum class e_( unit_orders, //
               none,        //
               sentry,      // includes units on ships
               fortified    //
);
SERIALIZABLE_ENUM( e_unit_orders );

// Mutable.  This holds information about a specific instance
// of a unit that is intrinsic to the unit apart from location.
// We don't allow copying (since their should never be two unit
// objects alive with the same ID) but moving is fine.
class ND Unit : public util::movable_only {
public:
  Unit()              = default; // for serialization framework.
  ~Unit()             = default;
  Unit( Unit const& ) = delete;
  Unit( Unit&& )      = default;

  Unit& operator=( Unit const& ) = delete;
  Unit& operator=( Unit&& ) = default;

  /************************* Getters ***************************/

  UnitId                id() const { return id_; }
  UnitDescriptor const& desc() const;
  e_unit_orders         orders() const { return orders_; }
  CargoHold const&      cargo() const { return cargo_; }
  // Allow non-const access to cargo since the CargoHold class
  // itself should enforce all invariants and interacting with it
  // doesn't really depend on any private Unit data.
  CargoHold&     cargo() { return cargo_; }
  e_nation       nation() const { return nation_; }
  Opt<int>       worth() const { return worth_; }
  MovementPoints movement_points() const { return mv_pts_; }

  /************************* Setters ***************************/
  // This would be used when e.g. a colonist is captured and
  // changes nations.
  void change_nation( e_nation nation );

  // This would be used when e.g. a unit gets demoted in combat
  // or promoted.
  void change_type( e_unit_type type );

  /************************ Functions **************************/

  // Returns nullopt if this unit cannot hold cargo. If it can
  // hold cargo then returns the list of units it holds, which
  // may be empty. To emphasize, nullopt will only be returned
  // when this unit is unable to hold cargo.
  Opt<Vec<UnitId>> units_in_cargo() const;

  // Has the unit been fully processed this turn. This concept is
  // distinct from that of having used all movement points. For
  // example, a unit that is sentry'd on a ship will be marked as
  // having finished its turn when it comes up in the queue, but
  // its movement points will not be exhausted. This is because
  // the player might want to wake the unit up by making
  // landfall, and it should not have to wait til the next turn
  // to respond, since it technically hasn't yet moved this turn.
  bool finished_turn() const { return finished_turn_; }
  // If the unit has physically moved this turn. This concept is
  // dinstict from whether the unit has been evolved this turn,
  // since not all units need to physically move or take orders
  // each turn (i.e., pioneer building).
  bool moved_this_turn() const { return mv_pts_ == 0; }
  // Returns true if the unit's orders are such that the unit may
  // physically move this turn, either by way of player input or
  // automatically, assuming it has movement points.
  bool orders_mean_move_needed() const;
  // Returns true if the unit's orders are such that the unit re-
  // quires player input this turn, assuming that it has some
  // movement points.
  bool orders_mean_input_required() const;
  // Gives up all movement points this turn and marks unit as
  // having moved. This can be called when the player directly
  // issues the "pass" command, or if e.g. a unit waiting for or-
  // ders is added to a colony, or if a unit waiting for orders
  // boards a ship.
  void forfeight_mv_points();
  // Marks unit as not having moved this turn.
  void new_turn();
  // Marks unit as having finished processing this turn.
  void finish_turn();
  // One can call this to put units back into consideration for
  // movement. There is no harm on calling this on a unit that
  // has already fully moved (and has no movement points
  // remaining) since it will then just be passed over again in
  // the orders loop and its turn will again be marked as
  // finished.
  void unfinish_turn();
  // Called to consume movement points as a result of a move.
  void consume_mv_points( MovementPoints points );
  // Mark a unit as sentry.
  void sentry() { orders_ = e_unit_orders::sentry; }
  // Mark a unit as fortified (non-ships only).
  void fortify();
  // Clear a unit's orders (they will then wait for orders).
  void clear_orders() { orders_ = e_unit_orders::none; }

  expect<> check_invariants_safe() const;

private:
  friend Unit& create_unit( e_nation nation, e_unit_type type );

  Unit( e_nation nation, e_unit_type type );

  void check_invariants() const;

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( Unit,
  ( UnitId,         id             ),
  ( e_unit_type,    type           ),
  ( e_unit_orders,  orders         ),
  ( CargoHold,      cargo          ),
  ( e_nation,       nation         ),
  ( Opt<int>,       worth          ),
  ( MovementPoints, mv_pts         ),
  ( bool,           finished_turn  ));
  // clang-format on
};
NOTHROW_MOVE( Unit );

std::string debug_string( Unit const& unit );

} // namespace rn
