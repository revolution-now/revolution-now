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
#include "lua.hpp"
#include "ustate.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

Unit::Unit( e_nation nation, e_unit_type type )
  : id_( next_unit_id() ),
    type_( type ),
    orders_( e_unit_orders::none ),
    cargo_( unit_desc( type ).cargo_slots ),
    nation_( nation ),
    worth_( nothing ),
    mv_pts_( unit_desc( type ).movement_points ) {}

valid_deserial_t Unit::check_invariants_safe() const {
  // Check that only treasure units can have a worth.
  switch( type_ ) {
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

UnitDescriptor const& Unit::desc() const {
  return unit_desc( type_ );
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

void Unit::change_type( e_unit_type type ) {
  CHECK( cargo_.slots_occupied() == 0,
         "cannot change the type of a unit holding cargo." );
  // Most attributes remain the same, save for a few.
  type_  = type;
  cargo_ = CargoHold( desc().cargo_slots );
  // FIXME: worth?
  mv_pts_ = std::clamp( mv_pts_, MovementPoints{ 0 },
                        desc().movement_points );
}

string debug_string( Unit const& unit ) {
  return fmt::format( "unit{{id: {}, nation: {}, type: \"{}\"}}",
                      unit.id(), unit.nation(),
                      unit.desc().name );
}

} // namespace rn

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_ENUM( unit_orders );

LUA_STARTUP( sol::state& st ) {
  using U = ::rn::Unit;

  sol::usertype<U> u =
      st.new_usertype<U>( "Unit", sol::no_constructor );

  // Getters.
  u["id"]              = &U::id;
  u["desc"]            = &U::desc;
  u["orders"]          = &U::orders;
  u["nation"]          = &U::nation;
  u["worth"]           = &U::worth;
  u["movement_points"] = &U::movement_points;
  // u["cargo"] = &U::cargo;
  // CargoHold&     cargo() { return cargo_; }
  // maybe<vector<UnitId>> units_in_cargo() const;

  // Actions.
  u["change_nation"] = &U::change_nation;
  u["change_type"]   = &U::change_type;
  u["sentry"]        = &U::sentry;
  u["fortify"]       = &U::fortify;
  u["clear_orders"]  = &U::clear_orders;
};

} // namespace
