/****************************************************************
**unit-type.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-11.
*
* Description: Unit type descriptors.
*
*****************************************************************/
#include "unit-type.hpp"

// config
#include "config/unit-type.hpp"

// ss
#include "ss/player.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/rtable.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"
#include "luapp/usertype.hpp"

// refl
#include "refl/enum-map.hpp"
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <set>
#include <unordered_set>

using namespace std;

namespace rn {

/****************************************************************
** Unit Inventory
*****************************************************************/
namespace {} // namespace

maybe<e_unit_inventory> commodity_to_inventory(
    e_commodity comm ) {
  return refl::enum_from_string<e_unit_inventory>(
      refl::enum_value_name( comm ) );
}

maybe<e_commodity> inventory_to_commodity(
    e_unit_inventory inv_type ) {
  return config_unit_type.composition.inventory_traits[inv_type]
      .commodity;
}

valid_or<string> UnitInventoryTraits::validate() const {
  REFL_VALIDATE( min_quantity >= 0,
                 "inventory traits min quantity must be > 0." );
  REFL_VALIDATE( min_quantity <= max_quantity,
                 "inventory traits min quantity must be <= than "
                 "max quantity." );
  REFL_VALIDATE( min_quantity <= default_quantity,
                 "inventory traits min quantity must be <= than "
                 "default quantity." );
  REFL_VALIDATE( default_quantity <= max_quantity,
                 "inventory traits default quantity must be <= "
                 "than max quantity." );
  REFL_VALIDATE( multiple > 0,
                 "inventory traits multiple must be > 0." );
  REFL_VALIDATE( min_quantity % multiple == 0,
                 "inventory traits multiple must divide evenly "
                 "into the min quantity." );
  REFL_VALIDATE( max_quantity % multiple == 0,
                 "inventory traits multiple must divide evenly "
                 "into the max quantity." );
  REFL_VALIDATE( default_quantity % multiple == 0,
                 "inventory traits multiple must divide evenly "
                 "into the default quantity." );
  return base::valid;
}

/****************************************************************
** UnitTypeAttributes
*****************************************************************/
bool can_attack( e_unit_type type ) {
  return unit_attr( type ).attack_points > 0;
}

bool is_military_unit( e_unit_type type ) {
  return can_attack( type );
}

MvPoints movement_points( Player const& player,
                          e_unit_type   type ) {
  UnitTypeAttributes const& attr = unit_attr( type );
  if( attr.ship &&
      player.fathers.has[e_founding_father::ferdinand_magellan] )
    return attr.base_movement_points + MvPoints( 1 );
  return attr.base_movement_points;
}

// Lua
namespace {

LUA_STARTUP( lua::state& st ) {
  using UD = ::rn::UnitTypeAttributes;

  auto ut = st.usertype.create<UD>();

  // FIXME(maybe): these are writable to lua. Find an equivalent
  // of sol::readonly to make them appear constant to luapp.

  LUA_ADD_MEMBER( name );
  // LUA_ADD_MEMBER( tile );
  LUA_ADD_MEMBER( nat_icon_front );
  LUA_ADD_MEMBER( nat_icon_position );
  LUA_ADD_MEMBER( ship );
  LUA_ADD_MEMBER( human );
  LUA_ADD_MEMBER( visibility );
  // Should not be accessing this in Lua since it would probably
  // indicate a bug. Intead Lua should call the movement_points
  // function defined below since it accounts for bonuses.
  // LUA_ADD_MEMBER( base_movement_points );
  LUA_ADD_MEMBER( attack_points );
  LUA_ADD_MEMBER( defense_points );
  LUA_ADD_MEMBER( cargo_slots );
  LUA_ADD_MEMBER( cargo_slots_occupies );
  LUA_ADD_MEMBER( canonical_base );
  LUA_ADD_MEMBER( expertise );
  LUA_ADD_MEMBER( cleared_expertise );
  LUA_ADD_MEMBER( type );
  LUA_ADD_MEMBER( is_derived );

  // FIXME: Figure out how to deal with C++ variants in Lua.
  // LUA_ADD_MEMBER( on_death );
  // LUA_ADD_MEMBER( promotion );
  // LUA_ADD_MEMBER( modifiers );
};

LUA_AUTO_FN( "movement_points", movement_points );

} // namespace

/****************************************************************
** Unit Type Modifier Inspection / Updating.
*****************************************************************/
namespace {

bool is_base_unit_type( e_unit_type type ) {
  return !unit_attr( type ).is_derived;
}

maybe<std::unordered_set<e_unit_type_modifier> const&>
unit_type_modifiers_for_path( e_unit_type base_type,
                              e_unit_type type ) {
  if( !is_base_unit_type( base_type ) ) return nothing;
  static std::unordered_set<e_unit_type_modifier> const empty;
  if( type == base_type ) return empty;
  if( auto const& modifiers =
          base::lookup( unit_attr( base_type ).modifiers, type );
      modifiers )
    return modifiers;
  return nothing;
}

bool unit_type_modifier_path_exists( e_unit_type base_type,
                                     e_unit_type type ) {
  return unit_type_modifiers_for_path( base_type, type )
      .has_value();
}

} // namespace

/****************************************************************
** UnitType
*****************************************************************/
maybe<UnitType> UnitType::create( e_unit_type type,
                                  e_unit_type base_type ) {
  maybe<UnitType> res;
  if( unit_type_modifier_path_exists( base_type, type ) )
    res = UnitType( base_type, type );
  return res;
}

UnitType UnitType::create( e_unit_type type ) {
  UnitTypeAttributes const& desc = unit_attr( type );
  if( desc.is_derived ) {
    UNWRAP_CHECK( base_type, desc.canonical_base );
    UNWRAP_CHECK( res, create( type, base_type ) );
    return res;
  } else {
    UNWRAP_CHECK( res, create( type, type ) );
    return res;
  }
}

valid_or<string> wrapped::UnitType::validate() const {
  REFL_VALIDATE(
      unit_type_modifier_path_exists( base_type, type ),
      "no unit type modifier path exists between unit types {} "
      "and {}.",
      base_type, type );
  return valid;
}

void UnitType::check_invariants_or_die() const {
  CHECK_HAS_VALUE( o_.validate() );
}

UnitType::UnitType( e_unit_type base_type, e_unit_type type )
  : o_( wrapped::UnitType{ .base_type = base_type,
                           .type      = type } ) {
  check_invariants_or_die();
}

UnitType::UnitType()
  : UnitType( e_unit_type::free_colonist,
              e_unit_type::free_colonist ) {}

UnitTypeAttributes const& unit_attr( UnitType type ) {
  return unit_attr( type.type() );
}

maybe<UnitType> find_unit_type_modifiers(
    e_unit_type                                base_type,
    unordered_set<e_unit_type_modifier> const& modifiers ) {
  if( modifiers.empty() ) return UnitType::create( base_type );
  auto& base_modifiers = unit_attr( base_type ).modifiers;
  for( auto& [mtype, base_mod_set] : base_modifiers )
    if( base_mod_set == modifiers )
      // It's ok to take the first one we find, since validation
      // should have ensured that this set of modifiers is
      // unique.
      return UnitType::create( mtype, base_type );
  return nothing;
}

bool is_unit_human( UnitType ut ) {
  e_unit_human res =
      config_unit_type.composition.unit_types[ut.type()].human;
  switch( res ) {
    case e_unit_human::no: return false;
    case e_unit_human::yes: return true;
    case e_unit_human::from_base: {
      res =
          config_unit_type.composition.unit_types[ut.base_type()]
              .human;
      switch( res ) {
        case e_unit_human::no: return false;
        case e_unit_human::yes: return true;
        case e_unit_human::from_base: {
          SHOULD_NOT_BE_HERE;
        }
      }
    }
  }
}

bool can_unit_found( UnitType ut ) {
  e_unit_can_found_colony res =
      config_unit_type.composition.unit_types[ut.type()]
          .can_found;
  switch( res ) {
    case e_unit_can_found_colony::no: return false;
    case e_unit_can_found_colony::yes: return true;
    case e_unit_can_found_colony::from_base: {
      res =
          config_unit_type.composition.unit_types[ut.base_type()]
              .can_found;
      switch( res ) {
        case e_unit_can_found_colony::no: return false;
        case e_unit_can_found_colony::yes: return true;
        case e_unit_can_found_colony::from_base: {
          SHOULD_NOT_BE_HERE;
        }
      }
    }
  }
}

std::unordered_set<e_unit_type_modifier> const&
UnitType::unit_type_modifiers() {
  UNWRAP_CHECK( res, unit_type_modifiers_for_path( o_.base_type,
                                                   o_.type ) );
  return res;
}

maybe<UnitType> add_unit_type_modifiers(
    UnitType                                   ut,
    unordered_set<e_unit_type_modifier> const& modifiers ) {
  unordered_set<e_unit_type_modifier> const& current_modifiers =
      ut.unit_type_modifiers();
  for( e_unit_type_modifier mod : modifiers )
    if( current_modifiers.contains( mod ) )
      // Game rules dictate that a modifier is just a boolean, so
      // if the unit already posesses it then it cannot accept it
      // again, and so this request will be denied.
      return nothing;
  auto target_modifiers = current_modifiers;
  for( e_unit_type_modifier mod : modifiers )
    target_modifiers.insert( mod );
  return find_unit_type_modifiers( ut.base_type(),
                                   target_modifiers );
}

maybe<UnitType> rm_unit_type_modifiers(
    UnitType                                        ut,
    std::unordered_set<e_unit_type_modifier> const& modifiers ) {
  unordered_set<e_unit_type_modifier> const& current_modifiers =
      ut.unit_type_modifiers();
  for( e_unit_type_modifier mod : modifiers )
    if( !current_modifiers.contains( mod ) )
      // Unit must have all modifiers.
      return nothing;
  auto target_modifiers = current_modifiers;
  for( e_unit_type_modifier mod : modifiers )
    target_modifiers.erase( mod );
  return find_unit_type_modifiers( ut.base_type(),
                                   target_modifiers );
}

// Lua
namespace {

LUA_STARTUP( lua::state& st ) {
  auto unit_type = st.usertype.create<UnitType>();

  unit_type["base_type"] = &UnitType::base_type;
  unit_type["type"]      = &UnitType::type;

  lua::table utype_tbl =
      lua::table::create_or_get( st["unit_type"] );

  lua::table tbl_UnitType =
      lua::table::create_or_get( utype_tbl["UnitType"] );

  tbl_UnitType["create_with_base"] =
      [&]( e_unit_type type,
           e_unit_type base_type ) -> UnitType {
    maybe<UnitType> ut = UnitType::create( type, base_type );
    LUA_CHECK( st, ut.has_value(),
               "failed to create UnitType with type={} and "
               "base_type={}.",
               type, base_type );
    return *ut;
  };

  tbl_UnitType["create"] = [&]( e_unit_type type ) -> UnitType {
    return UnitType::create( type );
  };
};

} // namespace

} // namespace rn