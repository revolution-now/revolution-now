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

// luapp
#include "luapp/ext-userdata.hpp"

// Rds
#include "unit.rds.hpp"

namespace rn {

struct Coord;
struct SS;
struct TS;
struct UnitTypeAttributes;
struct UnitsState;

// Mutable.  This holds information about a specific instance
// of a unit that is intrinsic to the unit apart from location.
// We don't allow copying (since their should never be two unit
// objects alive with the same ID) but moving is fine.
struct ND Unit {
  // This is provided for the serialization framework; a
  // default-constructed object will likely not be valid.
  Unit() = default;

  bool operator==( Unit const& ) const = default;

  /************************* Getters ***************************/

  [[nodiscard]] UnitId id() const { return o_.id; }
  UnitTypeAttributes const& desc() const;
  // FIXME: luapp can only take this as non-const....
  UnitTypeAttributes& desc_non_const() const;
  bool is_colonist() const;
  unit_orders const& orders() const { return o_.orders; }
  unit_orders& orders() { return o_.orders; }
  CargoHold const& cargo() const { return o_.cargo; }
  // Allow non-const access to cargo since the CargoHold class
  // itself should enforce all invariants and interacting with it
  // doesn't really depend on any private Unit data.
  CargoHold& cargo() { return o_.cargo; }
  e_player player_type() const { return o_.player_type; }
  MovementPoints movement_points() const { return o_.mv_pts; }
  e_unit_type base_type() const {
    return o_.composition.base_type();
  }
  e_unit_type type() const { return o_.composition.type(); }
  UnitType type_obj() const { return o_.composition.type_obj(); }
  UnitComposition const& composition() const {
    return o_.composition;
  }

  /************************** Cargo ****************************/

  // Returns nothing if this unit cannot hold cargo. If it can
  // hold cargo then returns the list of units it holds, which
  // may be empty. To emphasize, nothing will only be returned
  // when this unit is unable to hold cargo.
  maybe<std::vector<UnitId>> units_in_cargo() const;

  /********************* Movement Points ***********************/

  // Movement points indicate whether a unit has physically moved
  // this turn and cannot move any further, but are also used to
  // indicate (e.g. for units that are not on the map or for
  // units that are performing a job such as building a road)
  // that the unit has already been evolved this turn. Using
  // movement points to represent the latter is done for two rea-
  // sons:
  //
  //   1. It is convenient because we don't need an additional
  //      piece of state that would have to be then kept in sync
  //      with other things.
  //   2. It automatically allows us to enforce common-sense game
  //      rules such as one which says that "if a unit expended
  //      effort building a road in a given turn, then the player
  //      should not be able to clear their orders and move them
  //      in that same turn," as violations of such rules could
  //      somehow allow cheating.
  //
  // So in other words, all units start their turn with movement
  // points greater than zero, and all units finish their turn
  // with movement points equal to zero.
  bool mv_pts_exhausted() const { return o_.mv_pts == 0; }
  // Gives up all movement points this turn. This neither can nor
  // should ever be reversed. This can be called in many dif-
  // ferent circumstances, namely whenever we want to end a
  // unit's turn and/or ensure that they don't move any longer.
  void forfeight_mv_points();
  // Called to consume movement points as a result of a physical
  // move on land. Anything else that consumes movement points
  // would just consume all of them (by just calling
  // forfeight_mv_points) to mark the unit as having finished
  // their turn.
  void consume_mv_points( MovementPoints points );
  // Will be true at the start of a turn or when a unit has not
  // yet moved.
  bool has_full_mv_points() const;

  /************************* Orders ****************************/

  // Returns true if the unit's orders are other than `none`.
  bool has_orders() const;
  // Marks unit as not having moved this turn.
  void new_turn( Player const& player );
  // Mark a unit as sentry.
  void sentry() { o_.orders = unit_orders::sentry{}; }
  // Mark a unit as fortifying.
  void start_fortify();
  // Mark a unit as fully fortified. This happens after one turn
  // of beying in the "fortifying" state.
  void fortify();
  // Clear a unit's orders (they will then wait for orders).
  void clear_orders() { o_.orders = unit_orders::none{}; }

  /******************* Type/Player Changing ********************/

 private:
  // Should not call this directly, instead should use
  // change_unit_type.
  void change_type( Player const& player,
                    UnitComposition new_comp );

  friend void change_unit_type(
      SS& ss, TS& ts, Unit& unit,
      UnitComposition const& new_comp );

  // This would be used when e.g. a colonist is captured and
  // changes nations.
  void change_player( UnitsState& units_state, e_player player );

  friend void change_unit_player( SS& ss, TS& ts, Unit& unit,
                                  e_player new_player );
  friend void change_unit_player_and_move( SS& ss, TS& ts,
                                           Unit& unit,
                                           e_player new_player,
                                           Coord target );

 public:
  maybe<e_unit_type> demoted_type() const;

  // Implement refl::WrapsReflected.
  Unit( wrapped::Unit&& o ) : o_( std::move( o ) ) {}
  wrapped::Unit const& refl() const { return o_; }
  static constexpr std::string_view refl_ns   = "rn";
  static constexpr std::string_view refl_name = "Unit";

 private:
  friend struct UnitsState;

  Unit( e_player player, UnitComposition type );

 private:
  wrapped::Unit o_;
};
NOTHROW_MOVE( Unit );

std::string debug_string( Unit const& unit );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::Unit, owned_by_cpp ){};
}
