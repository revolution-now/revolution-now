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

#include "aliases.hpp"
#include "cargo.hpp"
#include "id.hpp"
#include "mv-points.hpp"
#include "nation.hpp"
#include "tiles.hpp"
#include "util.hpp"

#include "base-util/non-copyable.hpp"

#include <functional>
#include <optional>
#include <vector>

namespace rn {

enum class ND e_unit_type { free_colonist, soldier, caravel };

// Static information describing classes of units.  There will be
// one of these for each type of unit.
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
struct ND UnitDescriptor {
  std::string name;
  e_unit_type type;

  // Rendering
  g_tile      tile;
  bool        nat_icon_front;
  e_direction nat_icon_position;

  // Movement
  bool     boat;
  int      visibility;
  MvPoints movement_points;

  // Combat
  bool can_attack;
  int  attack_points;
  int  defense_points;

  // Cargo
  int      cargo_slots;
  Opt<int> cargo_slots_occupies; // slots occupied by this unit.
};

// Mutable.  This holds information about a specific instance
// of a unit that is intrinsic to the unit apart from location.
// We don't allow copying (since their should never be two unit
// objects alive with the same ID) but moving is fine.
class ND Unit : public util::movable_only {
public:
  ~Unit()             = default;
  Unit( Unit const& ) = delete;
  Unit( Unit&& )      = default;

  Unit& operator=( Unit const& ) = delete;
  Unit& operator=( Unit&& ) = delete;

  /************************** Enums ****************************/

  enum class ND e_orders {
    none,
    sentry, // includes units on ships
    fortified,
  };

  /************************* Getters ***************************/

  UnitId                id() const { return id_; }
  UnitDescriptor const& desc() const { return *desc_; }
  e_orders              orders() const { return orders_; }
  CargoHold const&      cargo() const { return cargo_; }
  // Allow non-const access to cargo since the CargoHold class
  // itself should enforce all invariants and interacting with it
  // doesn't really depend on any private Unit data.
  CargoHold&     cargo() { return cargo_; }
  e_nation       nation() const { return nation_; }
  MovementPoints movement_points() const {
    return movement_points_;
  }

  /************************ Functions **************************/

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
  bool moved_this_turn() const { return movement_points_ == 0; }
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
  void sentry() { orders_ = e_orders::sentry; }
  // Mark a unit as fortified (non-ships only).
  void fortify();
  // Clear a unit's orders (they will then wait for orders).
  void clear_orders() { orders_ = e_orders::none; }

private:
  friend Unit& create_unit( e_nation nation, e_unit_type type );
  Unit( e_nation nation, e_unit_type type );

  void check_invariants() const;

  // universal, unique, non-repeating, non-changing ID
  UnitId id_;
  // A unit can change type, but we cannot change the type
  // information of a unit descriptor itself.
  UnitDescriptor const* desc_;
  e_orders              orders_;
  CargoHold             cargo_;
  e_nation              nation_;
  // Movement points left this turn.
  MovementPoints movement_points_;
  bool           finished_turn_;
};

std::string debug_string( Unit const& unit );

} // namespace rn
