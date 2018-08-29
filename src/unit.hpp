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

// Static information describing classes of units.  There will be
// one of these for each type of unit.
struct UnitDescriptor {
  char const* name;

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

struct CargoSlot {
  bool is_unit; // determines which of the following are relevant.
  UnitId unit_id;
};

// Mutable.  This holds information about a specific instance
// of a unit that is intrinsic to the unit apart from location.
struct Unit {
  UnitId const id; // universal, unique, non-repeating, non-changing ID

  // A unit can change type, but we cannot change the type
  // information of a unit descriptor itself.
  UnitDescriptor const* type;

  enum class orders {
    none,
    sentry,
    fortified,
    enroute
  };
  orders orders;

  std::vector<CargoSlot> cargo_slots;
};

using UnitRef = std::reference_wrapper<Unit>;
using UnitCRef = std::reference_wrapper<Unit const>;
using OptUnitRef = std::optional<UnitRef>;
using OptUnitCRef = std::optional<UnitCRef>;
using UnitIdVec = std::vector<UnitId>;

Unit& unit_from_id( UnitId id );
UnitIdVec units_from_coord( Y y, X x );
UnitIdVec units_int_rect( Rect rect );

} // namespace rn
