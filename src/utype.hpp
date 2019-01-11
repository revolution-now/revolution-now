/****************************************************************
**utype.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-11.
*
* Description: Unit type descriptors.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "enum.hpp"
#include "geo-types.hpp"
#include "mv-points.hpp"
#include "tiles.hpp"

namespace rn {

enum class ND e_unit_type { free_colonist, soldier, caravel };

// Static information describing classes of units.  There will be
// one of these for each type of unit.
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
struct ND UnitDescriptor {
  std::string name;
  e_unit_type type;

  // Rendering
  g_tile      tile;
  bool        nat_icon_front;
  e_direction nat_icon_position;

  // Movement
  bool     boat;
  int      visibility;
  MvPoints movement_points;

  // Combat
  bool can_attack;
  int  attack_points;
  int  defense_points;

  // Cargo
  int      cargo_slots;
  Opt<int> cargo_slots_occupies; // slots occupied by this unit.
};

UnitDescriptor const& unit_desc( e_unit_type type );

// clang-format off

// This one is only relevant when there is a foreign unit on the
// target square.
enum class e_(terrain_bombardment_foreign_unit,
  nothing,
  attack,
  bombard
);

// This one is only relevant when there is a foreign colony on
// the target square.
enum class e_(terrain_bombardment_foreign_colony,
  unused
);

// This one is only relevant when there is a foreign indian vil-
// lage on the target square.
enum class e_(terrain_bombardment_village,
  unused
);

// This one is for when there there are no units on the target
// square.
enum class e_(terrain_traversal_no_units,
  never_move,
  always_move,
  move_off_ship
);

// This one is for when there is a friendly unit in the target
// square.
enum class e_(terrain_traversal_friendly_unit,
  always_move,
  move_onto_ship,
  move_off_ship,
  move_into_dock
);

// This one is for when there is a friendly colony in the target
// square.
enum class e_(terrain_traversal_friendly_colony,
  always_move,
  move_off_ship,
  move_into_dock
);

// clang-format on

struct ND UnitTerrainAnalysis {
  // e_terrain_bombardment bombardment;
  // e_terrain_traversal   traversal;
};

} // namespace rn
