/****************************************************************
**unsentry.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-26.
*
* Description: Handles logic related to unsentrying sentried
*              units.
*
*****************************************************************/
#include "unsentry.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

// Get all units in the eight squares that surround coord.
bool has_surrounding_foreign_unit( SSConst const& ss,
                                   e_nation       nation,
                                   Coord          coord ) {
  for( e_direction d : refl::enum_values<e_direction> ) {
    Coord const moved = coord.moved( d );
    for( GenericUnitId id : ss.units.from_coord( moved ) ) {
      if( ss.units.unit_kind( id ) != e_unit_kind::euro )
        return true;
      Unit const& euro_unit = ss.units.euro_unit_for( id );
      if( euro_unit.nation() != nation ) return true;
    }
  }
  return false;
}

// See if `unit` needs to be unsentry'd due to surrounding for-
// eign units.
void unsentry_unit_if_needed( SS& ss, Unit& unit ) {
  if( !unit.orders().holds<unit_orders::sentry>() ) return;
  // Don't use the "indirect" version here because we don't want
  // to e.g. wake up units that are sentry'd on ships.
  maybe<Coord> loc = ss.units.maybe_coord_for( unit.id() );
  if( !loc.has_value() ) return;
  if( has_surrounding_foreign_unit( ss, unit.nation(), *loc ) ) {
    unit.clear_orders();
    return;
  }
}

// Get all euro units in the eight squares that surround coord.
vector<Unit*> surrounding_sentried_euro_units(
    SS& ss, Coord const& coord,
    maybe<e_nation> exclude = nothing ) {
  vector<Unit*> res;
  for( e_direction const d : refl::enum_values<e_direction> ) {
    Coord const moved = coord.moved( d );
    for( GenericUnitId const generic_id :
         ss.units.from_coord( moved ) ) {
      if( ss.units.unit_kind( generic_id ) != e_unit_kind::euro )
        continue;
      Unit& unit = ss.units.euro_unit_for( generic_id );
      if( !unit.orders().holds<unit_orders::sentry>() ) continue;
      if( unit.nation() == exclude ) continue;
      res.push_back( &unit );
    }
  }
  return res;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void unsentry_units_next_to_foreign_units(
    SS& ss, e_nation nation_to_unsentry ) {
  for( auto& p : ss.units.euro_all() ) {
    Unit& unit = ss.units.unit_for( p.first );
    if( unit.nation() != nation_to_unsentry ) continue;
    unsentry_unit_if_needed( ss, unit );
  }
}

// Given a euro unit that is on the map, this will unsenty any
// foreign sentried units immediately adjacent to it.
void unsentry_foreign_units_next_to_euro_unit(
    SS& ss, Unit const& src_unit ) {
  Coord const src_loc = ss.units.coord_for( src_unit.id() );
  vector<Unit*> const units = surrounding_sentried_euro_units(
      ss, src_loc, /*exclude=*/src_unit.nation() );
  for( Unit* p_unit : units ) p_unit->clear_orders();
}

// Given the coordinate of a unit, this will unsenty any euro
// sentried units immediately adjacent to it.
void unsentry_units_next_to_tile( SS& ss, Coord coord ) {
  vector<Unit*> const units =
      surrounding_sentried_euro_units( ss, coord );
  for( Unit* p_unit : units ) p_unit->clear_orders();
}

} // namespace rn
