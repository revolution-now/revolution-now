/****************************************************************
* unit.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-28.
*
* Description: Data structure for units.
*
*****************************************************************/
#pragma once

#include "base-util.hpp"
#include "tiles.hpp"

#include <functional>
#include <optional>
#include <vector>

namespace rn {

enum class g_unit_type {
  free_colonist,
  caravel
};

// Static information describing classes of units.  There will be
// one of these for each type of unit.
struct UnitDescriptor {
  char const* name;
  g_unit_type type;

  // Rendering
  g_tile tile;

  // Movement
  bool boat;
  int visibility;
  int movement_points;

  // Combat
  bool can_attack;
  int attack_points;
  int defense_points;

  // Cargo
  int unit_cargo_slots;
  int cargo_slots_occupied;
};

using UnitId = int;

struct Cargo {
  bool is_unit; // determines which of the following are relevant.
  UnitId unit_id;
  /* more to come */
};

enum class g_unit_orders {
  none,
  sentry,
  fortified,
  enroute
};

enum class g_nation {
  dutch
};

// Mutable.  This holds information about a specific instance
// of a unit that is intrinsic to the unit apart from location.
// We don't allow copying (since their should never be two unit
// objects alive with the same ID) but moving is fine.
struct Unit {
  // universal, unique, non-repeating, non-changing ID
  UnitId id;
  // A unit can change type, but we cannot change the type
  // information of a unit descriptor itself.
  UnitDescriptor const* desc;
  g_unit_orders orders;
  std::vector<std::optional<Cargo>> cargo_slots;
  g_nation nation;
};

using UnitIdVec = std::vector<UnitId>;

// Not safe, probably temporary.
UnitId create_unit_on_map( g_unit_type type, Y y, X x );

Unit const& unit_from_id( UnitId id );
UnitIdVec units_from_coord( Y y, X x );
UnitIdVec units_int_rect( Rect const& rect );

std::vector<UnitId> units_all( g_nation nation );

g_nation player_nationality();

} // namespace rn
