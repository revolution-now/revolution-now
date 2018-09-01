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

// For units that are on (owned by) the map.
unordered_map<Coord, unordered_set<UnitId>> units_from_coords;
unordered_map<UnitId, Coord> coords_from_unit;
  
#if 1
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

  OptCoord get_val_safe(
        unordered_map<UnitId, Coord> const& m, UnitId k ) {
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
#endif

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

Unit& unit_from_id_mutable( UnitId id ) {
  auto res = explicit_types::get_val_safe( units, id );
  ASSERT( res );
  return *res;
}

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
  coords_from_unit[next_id] = Coord{y,x};
  return next_id++;
}

Unit const& unit_from_id( UnitId id ) {
  return unit_from_id_mutable( id );
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

OptCoord coords_for_unit( UnitId id ) {
  return explicit_types::get_val_safe( coords_from_unit, id );
}

// This function will allow the move by default, and so it is
// the burden of the logic in this function to find every possible
// way that the move is *not* allowed and to flag it if that is
// the case.
UnitMoveDesc move_consequences( UnitId id, Y y_target, X x_target ) {
  auto& unit = unit_from_id( id );
  auto& square = square_at( y_target, x_target );

  if( unit.desc->boat && square.land ) {
    return {{y_target, x_target}, false, k_unit_mv_desc::land_forbidden};
  }
  if( !unit.desc->boat && !square.land ) {
    return {{y_target, x_target}, false, k_unit_mv_desc::water_forbidden};
  }
  return {{y_target, x_target}, true, k_unit_mv_desc::none};
}

} // namespace rn
