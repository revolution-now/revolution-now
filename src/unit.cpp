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
#include "plow.hpp"
#include "road.hpp"
#include "ustate.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/rtable.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

base::valid_or<string> wrapped::Unit::validate() const {
  REFL_VALIDATE( cargo.slots_total() ==
                     unit_attr( composition.type() ).cargo_slots,
                 "inconsistent number of cargo slots" );
  return valid;
}

UnitTypeAttributes const& Unit::desc() const {
  return unit_attr( type() );
}

// FIXME: luapp can only take this as non-const....
UnitTypeAttributes& Unit::desc_non_const() const {
  return const_cast<UnitTypeAttributes&>( unit_attr( type() ) );
}

bool Unit::is_human() const {
  return is_unit_human( o_.composition.type_obj() );
}

// Mark unit as having moved.
void Unit::forfeight_mv_points() {
  o_.mv_pts = 0;
  CHECK_HAS_VALUE( o_.validate() );
}

// Marks unit as not having moved this turn.
void Unit::new_turn() {
  o_.mv_pts = desc().movement_points;
  CHECK_HAS_VALUE( o_.validate() );
}

maybe<vector<UnitId>> Unit::units_in_cargo() const {
  if( desc().cargo_slots == 0 ) return nothing;
  vector<UnitId> res;
  for( Cargo::unit const& u :
       o_.cargo.items_of_type<Cargo::unit>() )
    res.push_back( u.id );
  return res;
}

bool Unit::has_orders() const {
  return o_.orders != e_unit_orders::none;
}

// Called to consume movement points as a result of a move.
void Unit::consume_mv_points( MovementPoints points ) {
  o_.mv_pts -= points;
  CHECK( o_.mv_pts >= 0 );
  CHECK_HAS_VALUE( o_.validate() );
}

void Unit::set_turns_worked( int turns ) {
  CHECK( type() == e_unit_type::pioneer ||
         type() == e_unit_type::hardy_pioneer );
  CHECK( turns >= 0 );
  o_.turns_worked = turns;
}

void Unit::fortify() {
  CHECK( !desc().ship, "Only land units can be fortified." );
  o_.orders = e_unit_orders::fortified;
}

void Unit::change_nation( e_nation nation ) {
  // This could happen if we capture a colony containing a ship
  // that itself has units in its cargo.
  for( Cargo::unit u : o_.cargo.items_of_type<Cargo::unit>() )
    unit_from_id( u.id ).change_nation( nation );

  o_.nation = nation;
}

void Unit::change_type( UnitComposition new_comp ) {
  UnitType const& new_type = new_comp.type_obj();
  CHECK( o_.cargo.slots_occupied() == 0,
         "cannot change the type of a unit holding cargo." );
  CHECK( unit_attr( type() ).ship ==
             unit_attr( new_type.type() ).ship,
         "cannot change a ship to a non-ship or vice versa." );
  // Most attributes remain the same, save for a few.
  UnitTypeAttributes const& new_desc = unit_attr( new_type );
  UnitTypeAttributes const& old_desc = desc();
  o_.cargo = CargoHold( new_desc.cargo_slots );

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
  MovementPoints used = old_desc.movement_points - o_.mv_pts;

  o_.mv_pts = std::max( MovementPoints{ 0 },
                        new_desc.movement_points - used );

  // This should be done last.
  o_.composition = std::move( new_comp );
}

string debug_string( Unit const& unit ) {
  return fmt::format(
      "unit{{id={},nation={},type={},points={}}}", unit.id(),
      unit.nation(), unit.desc().name, unit.movement_points() );
}

void Unit::demote_from_lost_battle() {
  UNWRAP_CHECK( new_type, on_death_demoted_type( type_obj() ) );
  UNWRAP_CHECK( new_comp,
                o_.composition.with_new_type( new_type ) );
  change_type( std::move( new_comp ) );
}

UnitTransformationResult Unit::strip_to_base_type() {
  UnitTransformationResult res =
      rn::strip_to_base_type( o_.composition );
  change_type( res.new_comp );
  return res;
}

maybe<e_unit_type> Unit::demoted_type() const {
  UNWRAP_RETURN( demoted, on_death_demoted_type( type_obj() ) );
  return demoted.type();
}

vector<UnitTransformationFromCommodityResult>
Unit::with_commodity_added( Commodity const& commodity ) const {
  return unit_receive_commodity( o_.composition, commodity );
}

vector<UnitTransformationFromCommodityResult>
Unit::with_commodity_removed(
    Commodity const& commodity ) const {
  return unit_lose_commodity( o_.composition, commodity );
}

void Unit::build_road() {
  CHECK( can_build_road( *this ) );
  o_.orders = e_unit_orders::road;
}

void Unit::plow() {
  CHECK( can_plow( *this ) );
  o_.orders = e_unit_orders::plow;
}

void Unit::consume_20_tools() {
  vector<UnitTransformationFromCommodityResult> results =
      with_commodity_removed( Commodity{
          .type = e_commodity::tools, .quantity = 20 } );
  vector<UnitTransformationFromCommodityResult> valid_results;
  for( auto const& result : results ) {
    // It would be e.g. -80 because one valid transformation is
    // that we could subtract more than 20 tools, give the
    // blessing mod, and turn the unit into a missionary. But we
    // are not looking for that here.
    if( result.quantity_used != -20 ) continue;
    if( result.modifier_deltas.empty() ||
        result.modifier_deltas ==
            unordered_map<e_unit_type_modifier,
                          e_unit_type_modifier_delta>{
                { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del } } ) {
      valid_results.push_back( result );
    }
  }
  CHECK( valid_results.size() == 1,
         "could not find viable target unit after tools "
         "removed. results: {}",
         results );
  // This won't always change the type; e.g. it might just re-
  // place the type with the same type but with fewer tools in
  // the inventory.
  change_type( valid_results[0].new_comp );
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
  u["desc"]        = &U::desc_non_const;
  u["type_obj"]    = &U::type_obj;
  u["composition"] = []( Unit& unit ) {
    return unit.composition();
  };
  u["orders"]          = &U::orders;
  u["nation"]          = &U::nation;
  u["movement_points"] = &U::movement_points;

  // Actions.
  u["change_nation"] = &U::change_nation;
  u["change_type"]   = &U::change_type;
  u["sentry"]        = &U::sentry;
  u["build_road"]    = &U::build_road;
  u["plow"]          = &U::plow;
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
