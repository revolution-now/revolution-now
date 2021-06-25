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

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/rtable.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

// base
#include "base/keyval.hpp"

// Revolution Now (config)
#include "../config/ucl/units.inl"

// C++ standard library
#include <unordered_map>

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
    LOAD_UNIT_DESC( dragoon );
    LOAD_UNIT_DESC( large_treasure );
    LOAD_UNIT_DESC( small_treasure );
    for( auto const& p : desc_ ) p.second.check_invariants();
    return desc_;
  }();
  return desc;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
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
** Lua Bindings
*****************************************************************/
LUA_ENUM( unit_type );
LUA_ENUM( unit_death );

namespace {

// #define RO_FIELD( n )
//   utype[#n] = sol::readonly( &rn::UnitDescriptor::n )
#define RO_FIELD( n ) utype[#n] = &rn::UnitDescriptor::n

LUA_STARTUP( lua::state& st ) {
  using UD = ::rn::UnitDescriptor;

  auto utype = st.usertype.create<UD>();

  // FIXME: these are writable to lua. Find an equivalent of
  // sol::readonly to make them appear constant to luapp.

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
