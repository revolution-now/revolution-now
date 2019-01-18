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

// Abseil
#include "absl/container/flat_hash_map.h"

using namespace std;

#define LOAD_UNIT_DESC( __name )                              \
  {                                                           \
    e_unit_type::__name, UnitDescriptor {                     \
      units.__name.name, e_unit_type::__name, g_tile::__name, \
          units.__name.nat_icon_front,                        \
          units.__name.nat_icon_position, units.__name.boat,  \
          units.__name.visibility,                            \
          units.__name.movement_points,                       \
          units.__name.attack_points,                         \
          units.__name.defense_points, units.__name.on_death, \
          units.__name.demoted, units.__name.cargo_slots,     \
          units.__name.cargo_slots_occupies                   \
    }                                                         \
  }

namespace rn {

namespace {

absl::flat_hash_map<e_unit_type, UnitDescriptor> const&
unit_desc() {
  auto const& units = config_units;

  static auto const desc = [] {
    absl::flat_hash_map<e_unit_type, UnitDescriptor> desc_{
        LOAD_UNIT_DESC( free_colonist ),
        LOAD_UNIT_DESC( soldier ),
        LOAD_UNIT_DESC( caravel ),
    };
    for( auto const& p : desc_ ) p.second.check_invariants();
    return desc_;
  }();
  return desc;
}

} // namespace

UnitDescriptor const& unit_desc( e_unit_type type ) {
  return val_or_die( unit_desc(), type );
}

void UnitDescriptor::check_invariants() const {
  // Check that the `demoted` field should have a value if and
  // only if the unit is to be demoted after losing a battle.
  // FIXME: this check would not be necessary if we could repre-
  // sent the on_death/demoted fields in a single abstract data
  // type.
  switch( on_death ) {
    case +e_unit_death::destroy:
    case +e_unit_death::naval:
    case +e_unit_death::capture:
      CHECK(
          !demoted.has_value(),
          "units of type `{}` are not marked for demotion upon "
          "losing a battle and so should have demoted=null",
          type );
      break;
    case +e_unit_death::demote:
    case +e_unit_death::maybe_demote:
    case +e_unit_death::demote_and_capture:
      CHECK( demoted.has_value(),
             "units of type `{}` are marked for demotion upon "
             "losing a battle and so must have a valid demoted "
             "field",
             type );
      break;
  }
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

BEHAVIOR_IMPL_START( land, foreign, unit ) {
  // Possible results: nothing, attack, bombard
  if( desc.boat ) return res_t::no_bombard;
  return desc.can_attack() ? res_t::attack : res_t::no_attack;
}
BEHAVIOR_IMPL_END()

// BEHAVIOR_IMPL_START( land, foreign, colony ) {
//  // Possible results: unused
//  (void)desc;
//}
// BEHAVIOR_IMPL_END()

// BEHAVIOR_IMPL_START( land, foreign, village ) {
//  // Possible results: unused
//  (void)desc;
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
//}
// BEHAVIOR_IMPL_END()

BEHAVIOR_IMPL_START( water, foreign, unit ) {
  // Possible results: nothing, attack, bombard
  if( !desc.boat ) return res_t::no_bombard;
  return desc.can_attack() ? res_t::attack : res_t::no_attack;
}
BEHAVIOR_IMPL_END()

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
