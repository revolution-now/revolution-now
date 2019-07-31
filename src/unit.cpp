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

// {fmt}
#include "fmt/format.h"

using namespace std;

namespace rn {

namespace {} // namespace

Unit::Unit( e_nation nation, e_unit_type type )
  : id_( next_unit_id() ),
    desc_( &unit_desc( type ) ),
    orders_( e_unit_orders::none ),
    cargo_( desc_->cargo_slots ),
    nation_( nation ),
    movement_points_( desc_->movement_points ),
    finished_turn_( false ) {}

// Ideally this should be empty... try to do this with types.
void Unit::check_invariants() const {}

// Mark unit as having moved.
void Unit::forfeight_mv_points() {
  // This function doesn't necessarily have to be responsible for
  // checking this, but it may end up catching some problems.
  CHECK( !moved_this_turn() );
  movement_points_ = 0;
  check_invariants();
}

// Marks unit as not having moved this turn.
void Unit::new_turn() {
  movement_points_ = desc_->movement_points;
  finished_turn_   = false;
  check_invariants();
}

// Marks unit as having finished processing this turn.
void Unit::finish_turn() {
  CHECK( !finished_turn_ );
  finished_turn_ = true;
  check_invariants();
}

void Unit::unfinish_turn() { finished_turn_ = false; }

Opt<Vec<CRef<UnitId>>> Unit::units_in_cargo() const {
  if( desc_->cargo_slots == 0 ) return nullopt;
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
  movement_points_ -= points;
  CHECK( movement_points_ >= 0 );
  check_invariants();
}

void Unit::fortify() {
  CHECK( !desc().boat );
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
  CHECK( cargo_.slots_total() == 0,
         "what does it mean to change "
         "the type of a unit with cargo slots?" );

  desc_ = &unit_desc( type );
}

string debug_string( Unit const& unit ) {
  return fmt::format( "unit{{id: {}, nation: {}, type: \"{}\"}}",
                      unit.id(), unit.nation(),
                      unit.desc().name );
}

} // namespace rn
