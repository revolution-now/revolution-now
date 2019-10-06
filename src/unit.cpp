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
#include "errors.hpp"
#include "lua.hpp"

// {fmt}
#include "fmt/format.h"

using namespace std;

namespace rn {

namespace {} // namespace

Unit::Unit( e_nation nation, e_unit_type type )
  : id_( next_unit_id() ),
    type_( type ),
    orders_( e_unit_orders::none ),
    cargo_( unit_desc( type ).cargo_slots ),
    nation_( nation ),
    worth_( nullopt ),
    mv_pts_( unit_desc( type ).movement_points ),
    finished_turn_( false ) {}

// Ideally this should be empty... try to do this with types.
void Unit::check_invariants() const {
  // Check that only treasure units can have a worth.
  switch( type_ ) {
    case +e_unit_type::large_treasure:
    case +e_unit_type::small_treasure:
      CHECK( worth_.has_value() );
      CHECK( *worth_ > 0 );
      break;
    default: //
      CHECK( !worth_.has_value() );
      break;
  };
}

UnitDescriptor const& Unit::desc() const {
  return unit_desc( type_ );
}

// Mark unit as having moved.
void Unit::forfeight_mv_points() {
  // This function doesn't necessarily have to be responsible for
  // checking this, but it may end up catching some problems.
  CHECK( !moved_this_turn() );
  mv_pts_ = 0;
  check_invariants();
}

// Marks unit as not having moved this turn.
void Unit::new_turn() {
  mv_pts_        = desc().movement_points;
  finished_turn_ = false;
  check_invariants();
}

// Marks unit as having finished processing this turn.
void Unit::finish_turn() {
  CHECK( !finished_turn_ );
  finished_turn_ = true;
  check_invariants();
}

void Unit::unfinish_turn() { finished_turn_ = false; }

Opt<Vec<UnitId>> Unit::units_in_cargo() const {
  if( desc().cargo_slots == 0 ) return nullopt;
  return cargo_.items_of_type<UnitId>();
}

// Returns true if the unit's orders are such that the unit may
// physically move this turn, either by way of player input or
// automatically, assuming it has movement points.
bool Unit::orders_mean_move_needed() const {
  return orders_ == e_unit_orders::none;
}

// Returns true if the unit's orders are such that the unit re-
// quires player input this turn, assuming that it has some move-
// ment points.
bool Unit::orders_mean_input_required() const {
  return orders_ == e_unit_orders::none;
}

// Called to consume movement points as a result of a move.
void Unit::consume_mv_points( MovementPoints points ) {
  mv_pts_ -= points;
  CHECK( mv_pts_ >= 0 );
  check_invariants();
}

void Unit::fortify() {
  CHECK( !desc().boat, "Only land units can be fortified." );
  orders_ = e_unit_orders::fortified;
}

void Unit::change_nation( e_nation nation ) {
  // This may be allowed in the future, but for now it is not in-
  // tended to happen, so check for it.
  CHECK(
      cargo_.items_of_type<UnitId>().size() == 0,
      "attempt to change nation of a unit ({}) which contains "
      "other units in its cargo.",
      debug_string( *this ) );

  nation_ = nation;
}

void Unit::change_type( e_unit_type type ) {
  TODO(
      "need to reset unit state to something that is consistent "
      "with the new unit type." );
  CHECK( cargo_.slots_occupied() == 0,
         "cannot change the type of a unit holding cargo." );
  type_ = type;
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
  // Opt<Vec<UnitId>> units_in_cargo() const;

  // Actions.
  u["change_nation"] = &U::change_nation;
  u["change_type"]   = &U::change_type;
  u["sentry"]        = &U::sentry;
  u["fortify"]       = &U::fortify;
  u["clear_orders"]  = &U::clear_orders;
};

} // namespace
