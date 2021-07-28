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
#include "coord.hpp"
#include "fb.hpp"
#include "lua-enum.hpp"
#include "mv-points.hpp"
#include "tiles.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// Rds
#include "rds/utype.hpp"

// Flatbuffers
#include "fb/utype_generated.h"

namespace rn {

// We need this for some weird reason -- if we dont' have it and
// we put the MOVABLE_ONLY in the UnitDescriptor class then we
// lose the aggregate constructor that we need in the cpp file.
// This allows us to be movable only but retain the aggregate
// constructor.  FIXME: this may be fixed with C++20.
struct UnitDescriptorBase {
  UnitDescriptorBase() = default;
  MOVABLE_ONLY( UnitDescriptorBase );
};

// Static information describing classes of units. There will be
// one of these for each type of unit.
struct ND UnitDescriptor : public UnitDescriptorBase {
  std::string name{};
  e_unit_type type{};

  // Rendering
  e_tile      tile{};
  bool        nat_icon_front{};
  e_direction nat_icon_position{};

  // Movement
  bool     ship{};
  int      visibility{};
  MvPoints movement_points{};

  // Combat
  int  attack_points{};
  int  defense_points{};
  bool can_attack() const { return attack_points > 0; }
  bool is_military_unit() const { return can_attack(); }

  // FIXME: ideally these should be represented as an algebraic
  // data type in the config (and it should support loading
  // those). That way we would not have to do a runtime check
  // that `demoted` is set only if `on_death` has certain values.
  // When the unit loses a battle, what should happen?
  e_unit_death on_death{};
  // If the unit is to be demoted, what unit should it become?
  maybe<e_unit_type> demoted;

  // Cargo
  int cargo_slots{};
  // Slots occupied by this unit.
  maybe<int> cargo_slots_occupies{};

  void check_invariants() const;
};
NOTHROW_MOVE( UnitDescriptor );

UnitDescriptor const& unit_desc( e_unit_type type );

LUA_ENUM_DECL( unit_type );
LUA_ENUM_DECL( unit_death );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::UnitDescriptor, owned_by_cpp ){};
}
