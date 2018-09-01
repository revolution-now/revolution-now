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
#include "base-util.hpp"
#include "macros.hpp"
#include "tiles.hpp"
#include "unit.hpp"
#include "world.hpp"

#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace rn {

namespace {

UnitId next_id = 0;

unordered_map<UnitId, Unit> units;

unordered_map<Coord, unordered_set<UnitId>> units_from_coords;
  
unordered_map<g_unit_type, UnitDescriptor, EnumClassHash> unit_desc{
  {g_unit_type::free_colonist, UnitDescriptor{
    /*name=*/"free colonist",
    /*type=*/g_unit_type::free_colonist,
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
  {g_unit_type::caravel, UnitDescriptor{
    /*name=*/"caravel",
    /*type=*/g_unit_type::caravel,
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

g_nation player_nationality() {
  return g_nation::dutch;
}

vector<UnitId> units_all( g_nation nation ) {
  vector<UnitId> res; res.reserve( units.size() );
  for( auto const& p : units )
    if( p.second.nation == nation )
      res.push_back( p.first );
  return res;
}

namespace explicit_types {

  // These are to make the auto-completer happy since it doesn't
  // want to recognize the fully generic templated one.

  OptCRef<unordered_set<UnitId>> get_val_safe(
        unordered_map<Coord,unordered_set<UnitId>> const& m,
        Coord const& k ) {
      auto found = m.find( k );
      if( found == m.end() )
          return std::nullopt;
      return found->second;
  }

  OptRef<Unit> get_val_safe( unordered_map<UnitId,Unit>& m, UnitId k ) {
      auto found = m.find( k );
      if( found == m.end() )
          return std::nullopt;
      return found->second;
  }

}

// need to think about what this API should be.
UnitId create_unit_on_map( g_unit_type type, Y y, X x ) {
  auto const& desc = unit_desc[type];
  units[next_id] = Unit{
    next_id,
    &desc,
    g_unit_orders::none,
    {},
    g_nation::dutch,
  };
  units.at( next_id ).cargo_slots.resize( desc.unit_cargo_slots );
  units_from_coords[Coord{y,x}].insert( next_id );
  return next_id++;
}

Unit const& unit_from_id( UnitId id ) {
  auto res = explicit_types::get_val_safe( units, id );
  ASSERT( res );
  return *res;
}

UnitIdVec units_from_coord( Y y, X x ) {
  auto opt_set = explicit_types::get_val_safe( units_from_coords, Coord{y,x} );
  if( !opt_set ) return {};
  unordered_set<UnitId> const& set = (*opt_set);
  UnitIdVec res; res.reserve( set.size() );
  for( auto id : set )
    res.push_back( id );
  return res;
}

UnitIdVec units_int_rect( Rect const& rect ) {
  UnitIdVec res;
  for( Y i = rect.y; i < rect.y+rect.h; ++i )
    for( X j = rect.x; j < rect.x+rect.w; ++j )
      for( auto id : units_from_coord( i, j ) )
        res.push_back( id );
  return res;
}

} // namespace rn
