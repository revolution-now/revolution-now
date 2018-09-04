/****************************************************************
* unit.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-28.
*
* Description: Data structure for units.
*
*****************************************************************/
#include "unit.hpp"

#include "base-util.hpp"
#include "macros.hpp"

#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace rn {

namespace {

UnitId next_unit_id = 0;
unordered_map<UnitId, Unit> units;

#if 1
namespace explicit_types {
  // These are to make the auto-completer happy since it doesn't
  // want to recognize the fully generic templated one.
  OptRef<Unit> get_val_safe( unordered_map<UnitId,Unit>& m, UnitId k ) {
      auto found = m.find( k );
      if( found == m.end() )
          return std::nullopt;
      return found->second;
  }
}
#endif

unordered_map<e_unit_type, UnitDescriptor, EnumClassHash> unit_desc{
  {e_unit_type::free_colonist, UnitDescriptor{
    /*name=*/"free colonist",
    /*type=*/e_unit_type::free_colonist,
    /*tile=*/g_tile::free_colonist,
    /*boat=*/false,
    /*visibility=*/1,
    /*movement_points=*/1,
    /*can_attack=*/false,
    /*attack_points=*/0,
    /*defense_points=*/1,
    /*unit_cargo_slots=*/0,
    /*cargo_slots_occupied=*/1
  }},
  {e_unit_type::caravel, UnitDescriptor{
    /*name=*/"caravel",
    /*type=*/e_unit_type::caravel,
    /*tile=*/g_tile::caravel,
    /*boat=*/true,
    /*visibility=*/1,
    /*movement_points=*/4,
    /*can_attack=*/false,
    /*attack_points=*/0,
    /*defense_points=*/2,
    /*unit_cargo_slots=*/4,
    /*cargo_slots_occupied=*/-1
  }},
};

} // namespace

Unit& Unit::create( e_nation nation, e_unit_type type ) {
  Unit unit( nation, type );
  auto id = unit.id_;
  // To avoid requirement of operator[] that we have a default
  // constructor on Unit.
  units.emplace( id, move( unit ) );
  return units.find( id )->second;
}

Unit::Unit( e_nation nation, e_unit_type type ) {
  id_ = next_unit_id++;
  desc_ = &unit_desc[type];
  orders_ = e_unit_orders::none;
  cargo_slots_.resize( desc_->unit_cargo_slots );
  nation_ = nation;
  movement_points_ = desc_->movement_points;
  finished_turn_ = false;
}

// Ideally this should be empty... try to do this with types.
void Unit::check_invariants() const { }

// Mark unit as having moved.
void Unit::forfeight_mv_points() {
  // This function doesn't necessarily have to be responsible for
  // checking this, but it may end up catching some problems.
  ASSERT( !moved_this_turn() );
  movement_points_ = 0;
  check_invariants();
}

// Marks unit as not having moved this turn.
void Unit::new_turn() {
  movement_points_ = desc_->movement_points;
  finished_turn_ = false;
  check_invariants();
}

// Marks unit as having finished processing this turn.
void Unit::finish_turn() {
  ASSERT( !finished_turn_ );
  finished_turn_ = true;
  check_invariants();
}

// Returns true if the unit's orders are such that the unit may
// physically move this turn, either by way of player input or
// automatically, assuming it has movement points.
bool Unit::orders_mean_move_needed() const {
  return orders_ == e_unit_orders::none ||
         orders_ == e_unit_orders::enroute;
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
  ASSERT( movement_points_ >= 0 );
  check_invariants();
}

UnitIdVec units_all( optional<e_nation> nation ) {
  vector<UnitId> res; res.reserve( units.size() );
  if( nation ) {
    for( auto const& p : units )
      if( *nation == p.second.nation() )
        res.push_back( p.first );
  } else {
    for( auto const& p : units )
      res.push_back( p.first );
  }
  return res;
}

Unit& unit_from_id( UnitId id ) {
  auto res = explicit_types::get_val_safe( units, id );
  ASSERT( res );
  return *res;
}

// Apply a function to all units. The function may mutate the
// units.
void map_units( function<void( Unit& )> func ) {
  for( auto& p : units )
    func( p.second );
}

} // namespace rn
