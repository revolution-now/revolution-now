/****************************************************************
**utype.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-11.
*
* Description: Unit type descriptors.
*
*****************************************************************/
#include "utype.hpp"

// Revolution Now
#include "config-files.hpp"
#include "util.hpp"

// C++ standard library
#include <unordered_map>

using namespace std;

#define LOAD_UNIT_DESC( __name )                               \
  {                                                            \
    e_unit_type::__name, UnitDescriptor {                      \
      units.__name.name, e_unit_type::__name, g_tile::__name,  \
          units.__name.nat_icon_front,                         \
          units.__name.nat_icon_position, units.__name.boat,   \
          units.__name.visibility,                             \
          units.__name.movement_points,                        \
          units.__name.can_attack, units.__name.attack_points, \
          units.__name.defense_points,                         \
          units.__name.cargo_slots,                            \
          units.__name.cargo_slots_occupies                    \
    }                                                          \
  }

namespace rn {

namespace {

unordered_map<e_unit_type, UnitDescriptor, EnumClassHash> const&
unit_desc() {
  auto const& units = config_units;

  static unordered_map<e_unit_type, UnitDescriptor,
                       EnumClassHash> const desc{
      LOAD_UNIT_DESC( free_colonist ),
      LOAD_UNIT_DESC( soldier ),
      LOAD_UNIT_DESC( caravel ),
  };
  return desc;
}

} // namespace

UnitDescriptor const& unit_desc( e_unit_type type ) {
  return val_or_die( unit_desc(), type );
}

} // namespace rn
