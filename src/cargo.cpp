/****************************************************************
**cargo.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-08.
*
* Description: Handles the cargo that a unit may carry.
*
*****************************************************************/
#include "cargo.hpp"

#include "macros.hpp"
#include "ownership.hpp"

#include <iostream>

using namespace std;

namespace rn {

namespace {


  
} // namespace

CargoHold::~CargoHold() {
  if( !items_.empty() )
    WARNING() << "CargoHold destroyed with " << items_.size()
              << " remaining items!";
}

int CargoHold::slots_occupied() const {
  int total = 0;
  for( auto const& cargo : items_ ) {
    int occupied;
    // TODO: better visitation mechanism needed here.
    if( holds_alternative<UnitId>( cargo ) ) {
      UnitId id = get<UnitId>( cargo );
      occupied = unit_from_id( id ).desc().cargo_slots_occupies;
    } else {
      occupied = 1;
      DIE( "should not be here" );
    }
    total += occupied;
  }
  return total;
}

int CargoHold::slots_remaining() const {
  return slots_total() - slots_occupied();
}

int CargoHold::slots_total() const {
  return slots_;
}

bool CargoHold::fits( Cargo cargo ) const {
  CHECK( holds_alternative<UnitId>( cargo ) );
  UnitId id = get<UnitId>( cargo );
  auto occupied = unit_from_id( id ).desc().cargo_slots_occupies;
  return slots_remaining() >= occupied;
}

void CargoHold::add( Cargo cargo ) {
  if( holds_alternative<UnitId>( cargo ) ) {
    UnitId id = get<UnitId>( cargo );
    // Make sure that the unit is not already in this cargo.
    CHECK( rn::count( items_, Cargo{id} ) == 0 );
    CHECK( fits( id ) );
    items_.push_back( id );
  } else {
    DIE( "should not be here" );
  }
}

void CargoHold::remove( Cargo cargo ) {
  auto it = find( items_.begin(), items_.end(), cargo );
  CHECK( it != items_.end() );
  items_.erase( it );
}

} // namespace rn
