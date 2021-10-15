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
#include "fb.hpp"
#include "id.hpp"
#include "lua-enum.hpp"
#include "mv-points.hpp"
#include "nation.hpp"
#include "unit-composer.hpp"
#include "util.hpp"
#include "utype.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// Rds
#include "rds/unit.hpp"

// Flatbuffers
#include "fb/unit_generated.h"

namespace rn {

// Mutable.  This holds information about a specific instance
// of a unit that is intrinsic to the unit apart from location.
// We don't allow copying (since their should never be two unit
// objects alive with the same ID) but moving is fine.
class ND Unit {
public:
  Unit()  = default; // for serialization framework.
  ~Unit() = default;

  MOVABLE_ONLY( Unit );

  bool operator==( Unit const& ) const = default;

  /************************* Getters ***************************/

  UnitId                    id() const { return id_; }
  UnitTypeAttributes const& desc() const;
  // FIXME: luapp can only take this as non-const....
  UnitTypeAttributes& desc_non_const() const;
  e_unit_orders       orders() const { return orders_; }
  CargoHold const&    cargo() const { return cargo_; }
  // Allow non-const access to cargo since the CargoHold class
  // itself should enforce all invariants and interacting with it
  // doesn't really depend on any private Unit data.
  CargoHold& cargo() { return cargo_; }
  e_nation   nation() const { return nation_; }
  NationDesc nation_desc() const {
    return nation_obj( nation_ );
  }
  MovementPoints movement_points() const { return mv_pts_; }
  e_unit_type    base_type() const {
    return composition_.base_type();
  }
  e_unit_type type() const { return composition_.type(); }
  UnitType type_obj() const { return composition_.type_obj(); }
  UnitComposition const& composition() const {
    return composition_;
  }

  /************************* Setters ***************************/

  // This would be used when e.g. a colonist is captured and
  // changes nations.
  void change_nation( e_nation nation );

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
  bool mv_pts_exhausted() const { return mv_pts_ == 0; }
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

  /************************* Orders ****************************/

  // Returns true if the unit's orders are other than `none`.
  bool has_orders() const;
  // Marks unit as not having moved this turn.
  void new_turn();
  // Mark a unit as sentry.
  void sentry() { orders_ = e_unit_orders::sentry; }
  // Mark a unit as fortified (non-ships only).
  void fortify();
  // Clear a unit's orders (they will then wait for orders).
  void clear_orders() { orders_ = e_unit_orders::none; }

  valid_deserial_t check_invariants_safe() const;

  /********************** Type Changing ************************/

  // Take by value because we will move it out.
  void change_type( UnitComposition new_comp );

  // Will check-fail if the unit cannot be demoted.
  void demote_from_lost_battle();

  maybe<e_unit_type> demoted_type() const;

  // Can unit receive commodity, and if so how many and what unit
  // type will it become? This will not mutate the unit in any
  // way. If you want to affect the change, then you have to look
  // at the results, pick one that you want, and then call
  // change_type with the UnitComposition that it contains.
  std::vector<UnitTransformationFromCommodityResult>
  with_commodity_added( Commodity const& commodity ) const;

private:
  friend UnitId create_unit( e_nation        nation,
                             UnitComposition type );
  friend UnitId create_unit( e_nation nation, UnitType type );

  Unit( e_nation nation, UnitComposition type );

  void check_invariants() const;

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, Unit,
  ( UnitId,           id_            ),
  ( UnitComposition,  composition_   ),
  ( e_unit_orders,    orders_        ),
  ( CargoHold,        cargo_         ),
  ( e_nation,         nation_        ),
  ( MovementPoints,   mv_pts_        ));
  // clang-format on
};
NOTHROW_MOVE( Unit );

std::string debug_string( Unit const& unit );

LUA_ENUM_DECL( unit_orders );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::Unit, owned_by_cpp ){};
}
