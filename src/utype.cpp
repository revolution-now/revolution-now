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
#include "lua.hpp"

// base
#include "base/keyval.hpp"

// Revolution Now (config)
#include "../config/ucl/units.inl"

using namespace std;

#define LOAD_UNIT_DESC( u )                             \
  desc_[e_unit_type::u] =                               \
      UnitDescriptor{ /*UnitDescriptorBase=*/{},        \
                      config_units.u.name,              \
                      e_unit_type::u,                   \
                      e_tile::u,                        \
                      config_units.u.nat_icon_front,    \
                      config_units.u.nat_icon_position, \
                      config_units.u.ship,              \
                      config_units.u.visibility,        \
                      config_units.u.movement_points,   \
                      config_units.u.attack_points,     \
                      config_units.u.defense_points,    \
                      config_units.u.on_death,          \
                      config_units.u.demoted,           \
                      config_units.u.cargo_slots,       \
                      config_units.u.cargo_slots_occupies };

namespace rn {

namespace {

unordered_map<e_unit_type, UnitDescriptor> const& unit_desc() {
  static auto const desc = [] {
    unordered_map<e_unit_type, UnitDescriptor> desc_;
    LOAD_UNIT_DESC( merchantman );
    LOAD_UNIT_DESC( privateer );
    LOAD_UNIT_DESC( free_colonist );
    LOAD_UNIT_DESC( soldier );
    LOAD_UNIT_DESC( large_treasure );
    LOAD_UNIT_DESC( small_treasure );
    for( auto const& p : desc_ ) p.second.check_invariants();
    return desc_;
  }();
  return desc;
}

} // namespace

UnitDescriptor const& unit_desc( e_unit_type type ) {
  auto maybe_desc = base::lookup( unit_desc(), type );
  CHECK( maybe_desc );
  return *maybe_desc;
}

void UnitDescriptor::check_invariants() const {
  // Check that the `demoted` field should have a value if and
  // only if the unit is to be demoted after losing a battle.
  // FIXME: this check would not be necessary if we could repre-
  // sent the on_death/demoted fields in a single abstract data
  // type.
  switch( on_death ) {
    case e_unit_death::destroy:
    case e_unit_death::naval:
    case e_unit_death::capture:
      CHECK(
          !demoted.has_value(),
          "units of type `{}` are not marked for demotion upon "
          "losing a battle and so should have demoted=null",
          type );
      break;
    case e_unit_death::demote:
    case e_unit_death::maybe_demote:
    case e_unit_death::demote_and_capture:
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
  if( desc.ship ) return res_t::no_bombard;
  return desc.can_attack() ? res_t::attack : res_t::no_attack;
}
BEHAVIOR_IMPL_END()

BEHAVIOR_IMPL_START( land, foreign, colony ) {
  // Possible results: never, attack, trade.
  if( desc.ship ) return res_t::trade;
  if( desc.is_military_unit() ) return res_t::attack;
  return res_t::never;
}
BEHAVIOR_IMPL_END()

// BEHAVIOR_IMPL_START( land, foreign, village ) {
//  // Possible results: unused
//  (void)desc;
//}
// BEHAVIOR_IMPL_END()

BEHAVIOR_IMPL_START( land, neutral, empty ) {
  // Possible results: never, always, unload
  return desc.ship ? res_t::unload : res_t::always;
}
BEHAVIOR_IMPL_END()

BEHAVIOR_IMPL_START( land, friendly, unit ) {
  // Possible results: always, never, unload
  return desc.ship ? res_t::unload : res_t::always;
}
BEHAVIOR_IMPL_END()

BEHAVIOR_IMPL_START( land, friendly, colony ) {
  // Possible results: always
  (void)desc;
  return res_t::always;
}
BEHAVIOR_IMPL_END()

BEHAVIOR_IMPL_START( water, foreign, unit ) {
  // Possible results: nothing, attack, bombard
  if( !desc.ship ) return res_t::no_bombard;
  return desc.can_attack() ? res_t::attack : res_t::no_attack;
}
BEHAVIOR_IMPL_END()

BEHAVIOR_IMPL_START( water, neutral, empty ) {
  // Possible results: never, always
  return desc.ship ? res_t::always : res_t::never;
}
BEHAVIOR_IMPL_END()

BEHAVIOR_IMPL_START( water, friendly, unit ) {
  // Possible results: always, never, move_onto_ship
  if( desc.ship ) return res_t::always;
  return desc.cargo_slots_occupies.has_value()
             ? res_t::move_onto_ship
             : res_t::never;
}
BEHAVIOR_IMPL_END()

} // namespace rn

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace rn {
namespace {

LUA_ENUM( unit_type );

#define RO_FIELD( n ) \
  utype[#n] = sol::readonly( &rn::UnitDescriptor::n )

LUA_STARTUP( sol::state& st ) {
  using UD = ::rn::UnitDescriptor;

  sol::usertype<UD> utype = st.new_usertype<UD>(
      "UnitDescriptor", sol::no_constructor );

  RO_FIELD( name );
  RO_FIELD( type );
  RO_FIELD( tile );
  RO_FIELD( nat_icon_front );
  RO_FIELD( nat_icon_position );
  RO_FIELD( ship );
  RO_FIELD( visibility );
  RO_FIELD( movement_points );
  RO_FIELD( attack_points );
  RO_FIELD( defense_points );
  RO_FIELD( on_death );
  RO_FIELD( demoted );
  RO_FIELD( cargo_slots );
  RO_FIELD( cargo_slots_occupies );
};

} // namespace
} // namespace rn
