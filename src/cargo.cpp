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

#include "errors.hpp"
#include "logging.hpp"
#include "ownership.hpp"
#include "util.hpp"

#include <iostream>

using namespace std;

namespace rn {

namespace {} // namespace

CargoHold::~CargoHold() {
  if( !items_.empty() )
    logger->warn( "CargoHold destroyed with {} remaining items.",
                  items_.size() );
}

int CargoHold::slots_occupied() const {
  int total = 0;
  for( auto const& cargo : items_ ) {
    int occupied{0};
    // TODO: better visitation mechanism needed here.
    if( holds<UnitId>( cargo ) ) {
      UnitId id = get<UnitId>( cargo );
      auto   occupied_ =
          unit_from_id( id ).desc().cargo_slots_occupies;
      CHECK( occupied_.has_value() );
      occupied = *occupied_;
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

int CargoHold::slots_total() const { return slots_; }

bool CargoHold::fits( Cargo const& cargo ) const {
  CHECK( holds<UnitId>( cargo ) );
  UnitId id     = get<UnitId>( cargo );
  auto occupied = unit_from_id( id ).desc().cargo_slots_occupies;
  return slots_remaining() >= occupied;
}

void CargoHold::add( Cargo const& cargo ) {
  if( holds<UnitId>( cargo ) ) {
    UnitId id = get<UnitId>( cargo );
    // Make sure that the unit is not already in this cargo.
    CHECK( rn::count( items_, Cargo{id} ) == 0 );
    CHECK( fits( id ) );
    items_.emplace_back( id );
  } else {
    DIE( "should not be here" );
  }
}

void CargoHold::remove( Cargo const& cargo ) {
  auto it = find( items_.begin(), items_.end(), cargo );
  CHECK( it != items_.end() );
  items_.erase( it );
}

} // namespace rn
