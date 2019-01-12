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

/****************************************************************
**Unit Movement Behaviors / Capabilities
*****************************************************************/
#define BEHAVIOR_IMPL_START( c, r, e )       \
  template<>                                 \
  to_behaviors_t<BEHAVIOR_VALUES( c, r, e )> \
  behavior<BEHAVIOR_VALUES( c, r, e )>(      \
      UnitDescriptor const& desc ) {         \
    using res_t = BEHAVIOR_NS( c, r, e )::e_vals;

#define BEHAVIOR_IMPL_END() }

// BEHAVIOR_IMPL_START( land, foreign, unit ) {
//  // Possible results: nothing, attack, bombard
//  (void)desc;
//  return res;
//}
// BEHAVIOR_IMPL_END()

// BEHAVIOR_IMPL_START( land, foreign, colony ) {
//  // Possible results: unused
//  (void)desc;
//  return res;
//}
// BEHAVIOR_IMPL_END()

// BEHAVIOR_IMPL_START( land, foreign, village ) {
//  // Possible results: unused
//  (void)desc;
//  return res;
//}
// BEHAVIOR_IMPL_END()

BEHAVIOR_IMPL_START( land, neutral, empty ) {
  // Possible results: never, always, unload
  return desc.boat ? res_t::unload : res_t::always;
}
BEHAVIOR_IMPL_END()

BEHAVIOR_IMPL_START( land, friendly, unit ) {
  // Possible results: always, never, unload
  return desc.boat ? res_t::unload : res_t::always;
}
BEHAVIOR_IMPL_END()

// BEHAVIOR_IMPL_START( land, friendly, colony ) {
//  // Possible results: always, move_into_dock
//  (void)desc;
//  return res;
//}
// BEHAVIOR_IMPL_END()

// BEHAVIOR_IMPL_START( water, foreign, unit ) {
//  // Possible results: nothing, attack, bombard
//  (void)desc;
//  return res;
//}
// BEHAVIOR_IMPL_END()

BEHAVIOR_IMPL_START( water, neutral, empty ) {
  // Possible results: never, always
  return desc.boat ? res_t::always : res_t::never;
}
BEHAVIOR_IMPL_END()

BEHAVIOR_IMPL_START( water, friendly, unit ) {
  // Possible results: always, never, move_onto_ship
  if( desc.boat ) return res_t::always;
  return desc.cargo_slots_occupies.has_value()
             ? res_t::move_onto_ship
             : res_t::never;
}
BEHAVIOR_IMPL_END()

} // namespace rn
