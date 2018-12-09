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

#include "config-files.hpp"
#include "errors.hpp"
#include "util.hpp"

#include <unordered_map>

using namespace std;

#define LOAD_UNIT_DESC( __name )                               \
  {                                                            \
    e_unit_type::__name, UnitDescriptor {                      \
      units.__name.name, e_unit_type::__name, g_tile::__name,  \
          units.__name.boat, units.__name.visibility,          \
          units.__name.movement_points,                        \
          units.__name.can_attack, units.__name.attack_points, \
          units.__name.defense_points,                         \
          units.__name.cargo_slots,                            \
          units.__name.cargo_slots_occupies                    \
    }                                                          \
  }

namespace rn {

namespace {

unordered_map<e_unit_type, UnitDescriptor, EnumClassHash> const&
unit_desc() {
  auto const& units = config_units;

  static unordered_map<e_unit_type, UnitDescriptor,
                       EnumClassHash> const desc{
      LOAD_UNIT_DESC( free_colonist ),
      LOAD_UNIT_DESC( caravel ),
  };
  return desc;
}

} // namespace

Unit::Unit( e_nation nation, e_unit_type type )
  : id_( next_unit_id() ),
    desc_( &get_val_or_die( unit_desc(), type ) ),
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

void Unit::unfinish_turn() {
  CHECK( !moved_this_turn() );
  finished_turn_ = false;
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

} // namespace rn
