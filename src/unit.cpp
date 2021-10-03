/****************************************************************
**unit.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-28.
*
* Description: Data structure for units.
*
*****************************************************************/
#include "unit.hpp"

// Revolution Now
#include "error.hpp"
#include "game-state.hpp"
#include "lua.hpp"
#include "ustate.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/rtable.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

Unit::Unit( e_nation nation, UnitType type )
  : id_( next_unit_id() ),
    type_( type ),
    orders_( e_unit_orders::none ),
    cargo_( unit_attr( type.type() ).cargo_slots ),
    nation_( nation ),
    worth_( nothing ),
    mv_pts_( unit_attr( type.type() ).movement_points ) {}

valid_deserial_t Unit::check_invariants_safe() const {
  // Check that only treasure units can have a worth.
  switch( type_.type() ) {
    case e_unit_type::large_treasure:
    case e_unit_type::small_treasure:
      VERIFY_DESERIAL( worth_.has_value(),
                       "Treasure trains must have a `worth`." );
      break;
    default: //
      VERIFY_DESERIAL(
          !worth_.has_value(),
          "Non-treasure trains must not have a `worth`." );
      break;
  };
  VERIFY_DESERIAL( cargo().slots_total() == desc().cargo_slots,
                   "inconsistent number of cargo slots" );
  return valid;
}

UnitTypeAttributes const& Unit::desc() const {
  return unit_attr( type_.type() );
}

// FIXME: luapp can only take this as non-const....
UnitTypeAttributes& Unit::desc_non_const() const {
  return const_cast<UnitTypeAttributes&>(
      unit_attr( type_.type() ) );
}

// Mark unit as having moved.
void Unit::forfeight_mv_points() {
  mv_pts_ = 0;
  CHECK_HAS_VALUE( check_invariants_safe() );
}

// Marks unit as not having moved this turn.
void Unit::new_turn() {
  mv_pts_ = desc().movement_points;
  CHECK_HAS_VALUE( check_invariants_safe() );
}

maybe<vector<UnitId>> Unit::units_in_cargo() const {
  if( desc().cargo_slots == 0 ) return nothing;
  return cargo_.items_of_type<UnitId>();
}

bool Unit::has_orders() const {
  return orders_ != e_unit_orders::none;
}

// Called to consume movement points as a result of a move.
void Unit::consume_mv_points( MovementPoints points ) {
  mv_pts_ -= points;
  CHECK( mv_pts_ >= 0 );
  CHECK_HAS_VALUE( check_invariants_safe() );
}

void Unit::fortify() {
  CHECK( !desc().ship, "Only land units can be fortified." );
  orders_ = e_unit_orders::fortified;
}

void Unit::change_nation( e_nation nation ) {
  // This could happen if we capture a colony containing a ship
  // that itself has units in its cargo.
  for( UnitId id : cargo_.items_of_type<UnitId>() )
    unit_from_id( id ).change_nation( nation );

  nation_ = nation;
}

void Unit::change_type( UnitType const& new_type ) {
  CHECK( cargo_.slots_occupied() == 0,
         "cannot change the type of a unit holding cargo." );
  CHECK( unit_attr( type_.type() ).ship ==
             unit_attr( new_type.type() ).ship,
         "cannot change a ship to a non-ship or vice versa." );
  // Most attributes remain the same, save for a few.
  UnitTypeAttributes const& new_desc = unit_attr( new_type );
  UnitTypeAttributes const& old_desc = desc();
  // FIXME: worth?
  cargo_ = CargoHold( new_desc.cargo_slots );

  // For movement points, just subtract the number of movement
  // points that they have already used from the new unit types
  // quota. If the result is less than zero then the unit will
  // effectively just end its turn. This way, if a unit has not
  // moved at all, it will get all of the new units movement
  // points. At the same time, it will prevent a kind of
  // "perpetual-motion" cheating, an example of which would be
  // that you move a scout into a colony, then remove its horses,
  // then give it horses again, hoping that it will then have a
  // full four movement points. This will cause those used move-
  // ment points to persist across type changes.
  MovementPoints used = old_desc.movement_points - mv_pts_;

  mv_pts_ = std::max( MovementPoints{ 0 },
                      new_desc.movement_points - used );

  // This should be done last.
  type_ = new_type;
}

string debug_string( Unit const& unit ) {
  return fmt::format( "unit{{id: {}, nation: {}, type: \"{}\"}}",
                      unit.id(), unit.nation(),
                      unit.desc().name );
}

void Unit::demote_from_lost_battle() {
  UNWRAP_CHECK( new_type, on_death_demoted_type( type_ ) );
  change_type( new_type );
}

maybe<e_unit_type> Unit::demoted_type() const {
  UNWRAP_RETURN( demoted, on_death_demoted_type( type_ ) );
  return demoted.type();
}

maybe<UnitType> Unit::can_receive_modifiers(
    std::initializer_list<e_unit_type_modifier> modifiers )
    const {
  return add_unit_type_modifiers(
      type_, modifiers,
      /*allow_independence=*/post_independence() );
}

void Unit::receive_modifiers(
    std::initializer_list<e_unit_type_modifier> modifiers ) {
  UNWRAP_CHECK( new_unit_type,
                can_receive_modifiers( modifiers ) );
  change_type( new_unit_type );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
LUA_ENUM( unit_orders );

namespace {

LUA_STARTUP( lua::state& st ) {
  using U = ::rn::Unit;

  auto u = st.usertype.create<rn::Unit>();

  // Getters.
  u["id"] = &U::id;
  // FIXME: luapp can only take this as non-const...
  u["desc"]            = &U::desc_non_const;
  u["orders"]          = &U::orders;
  u["nation"]          = &U::nation;
  u["worth"]           = &U::worth;
  u["movement_points"] = &U::movement_points;

  // Actions.
  u["change_nation"] = &U::change_nation;
  u["change_type"]   = &U::change_type;
  u["sentry"]        = &U::sentry;
  u["fortify"]       = &U::fortify;
  u["clear_orders"]  = &U::clear_orders;

  u[lua::metatable_key]["__tostring"] = []( U const& u ) {
    return fmt::format( "{} {} (id={})",
                        u.nation_desc().adjective, u.desc().name,
                        u.id() );
  };
};

} // namespace
} // namespace rn
