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

// Rds
#include "rds/helper/rcl.hpp"

// Rcl
#include "rcl/ext-base.hpp"
#include "rcl/ext-builtin.hpp"
#include "rcl/ext-std.hpp"

// base
#include "base/keyval.hpp"

// Revolution Now (config)
#include "../config/rcl/units.inl"

// C++ standard library
#include <unordered_set>

using namespace std;

namespace rn {

/****************************************************************
** e_unit_type
*****************************************************************/
LUA_ENUM( unit_type );

/****************************************************************
** e_unit_type_modifier
*****************************************************************/
LUA_ENUM( unit_type_modifier );

/****************************************************************
** e_unit_activity
*****************************************************************/
LUA_ENUM( unit_activity );

/****************************************************************
** UnitDeathAction
*****************************************************************/
namespace UnitDeathAction {

// FIXME: Have RDS implement this automatically. It requires
// first giving RDS support for reflected structs, then for re-
// flected variants, then write generic convert_to implementa-
// tions for the reflected structs and reflected variants.
rcl::convert_err<UnitDeathAction_t> convert_to(
    rcl::value const& v, rcl::tag<UnitDeathAction_t> ) {
  constexpr string_view kTypeName          = "UnitDeathAction_t";
  constexpr int         kNumFieldsExpected = 1;
  base::maybe<std::unique_ptr<rcl::table> const&> mtbl =
      v.get_if<std::unique_ptr<rcl::table>>();
  if( !mtbl )
    return rcl::error( fmt::format(
        "cannot produce a {} from type {}.", kTypeName,
        rcl::name_of( rcl::type_of( v ) ) ) );
  DCHECK( *mtbl != nullptr );
  rcl::table const& tbl = **mtbl;
  if( tbl.size() != kNumFieldsExpected )
    return rcl::error(
        fmt::format( "table must have precisely {} field for "
                     "conversion to {}.",
                     kNumFieldsExpected, kTypeName ) );
  auto& [key, val] = *tbl.begin();
  if( rcl::type_of( val ) != rcl::type::table )
    return rcl::error( fmt::format(
        "variant alternative field must have type table." ) );
  UNWRAP_CHECK( uptr_alternative,
                val.get_if<unique_ptr<rcl::table>>() );
  CHECK( uptr_alternative );
  rcl::table const& alternative = *uptr_alternative;
  if( key == "destroy" ) return UnitDeathAction::destroy{};
  if( key == "capture" ) return UnitDeathAction::capture{};
  if( key == "naval" ) return UnitDeathAction::naval{};
  if( key == "demote" ) {
    if( alternative.size() != 1 ||
        !alternative.has_key( "lose" ) )
      return rcl::error(
          fmt::format( "{} variant alternative field {} must "
                       "have precisely one field named {}.",
                       kTypeName, "demote", "lose" ) );
    UNWRAP_RETURN(
        lose,
        rcl::convert_to<unordered_set<e_unit_type_modifier>>(
            alternative["lose"] ) );
    return UnitDeathAction::demote{ .lose = std::move( lose ) };
  }
  return rcl::error(
      fmt::format( "config field of type {} has invalid variant "
                   "alternative type name: {}.",
                   kTypeName, key ) );
}

} // namespace UnitDeathAction

/****************************************************************
** UnitPromotion
*****************************************************************/
namespace UnitPromotion {

// FIXME: Have RDS implement this automatically. It requires
// first giving RDS support for reflected structs, then for re-
// flected variants, then write generic convert_to implementa-
// tions for the reflected structs and reflected variants.
rcl::convert_err<UnitPromotion_t> convert_to(
    rcl::value const& v, rcl::tag<UnitPromotion_t> ) {
  constexpr string_view kTypeName          = "UnitPromotion_t";
  constexpr int         kNumFieldsExpected = 1;
  base::maybe<std::unique_ptr<rcl::table> const&> mtbl =
      v.get_if<std::unique_ptr<rcl::table>>();
  if( !mtbl )
    return rcl::error( fmt::format(
        "cannot produce a {} from type {}.", kTypeName,
        rcl::name_of( rcl::type_of( v ) ) ) );
  DCHECK( *mtbl != nullptr );
  rcl::table const& tbl = **mtbl;
  if( tbl.size() != kNumFieldsExpected )
    return rcl::error(
        fmt::format( "table must have precisely {} field for "
                     "conversion to {}.",
                     kNumFieldsExpected, kTypeName ) );
  auto& [key, val] = *tbl.begin();
  if( rcl::type_of( val ) != rcl::type::table )
    return rcl::error( fmt::format(
        "variant alternative field must have type table." ) );
  UNWRAP_CHECK( uptr_alternative,
                val.get_if<unique_ptr<rcl::table>>() );
  CHECK( uptr_alternative );
  rcl::table const& alternative = *uptr_alternative;

  if( key == "fixed" ) {
    if( alternative.size() != 1 ||
        !alternative.has_key( "type" ) )
      return rcl::error(
          fmt::format( "{} variant alternative field {} must "
                       "have precisely one field named {}.",
                       kTypeName, "fixed", "type" ) );
    UNWRAP_RETURN( type, rcl::convert_to<e_unit_type>(
                             alternative["type"] ) );
    return UnitPromotion::fixed{ .type = std::move( type ) };
  }

  if( key == "occupation" ) return UnitPromotion::occupation{};

  if( key == "expertise" ) {
    if( alternative.size() != 1 ||
        !alternative.has_key( "kind" ) )
      return rcl::error(
          fmt::format( "{} variant alternative field {} must "
                       "have precisely one field named {}.",
                       kTypeName, "expertise", "kind" ) );
    UNWRAP_RETURN( kind, rcl::convert_to<e_unit_activity>(
                             alternative["kind"] ) );
    return UnitPromotion::expertise{ .kind = std::move( kind ) };
  }

  if( key == "modifier" ) {
    if( alternative.size() != 1 ||
        !alternative.has_key( "kind" ) )
      return rcl::error(
          fmt::format( "{} variant alternative field {} must "
                       "have precisely one field named {}.",
                       kTypeName, "modifier", "kind" ) );
    UNWRAP_RETURN( kind, rcl::convert_to<e_unit_type_modifier>(
                             alternative["kind"] ) );
    return UnitPromotion::modifier{ .kind = std::move( kind ) };
  }

  return rcl::error(
      fmt::format( "config field of type {} has invalid variant "
                   "alternative type name: {}.",
                   kTypeName, key ) );
}

} // namespace UnitPromotion

/****************************************************************
** UnitTypeAttributes
*****************************************************************/
UnitTypeAttributes const& unit_attr( e_unit_type type ) {
  UNWRAP_CHECK_MSG(
      desc, base::lookup( config_units.unit_types.map, type ),
      "internal error: unit type {} does not have a type "
      "descriptor.",
      type );
  return desc;
}

// Rcl
rcl::convert_err<UnitTypeAttributes> convert_to(
    rcl::value const& v, rcl::tag<UnitTypeAttributes> ) {
  (void)rcl::convert_to<UnitDeathAction_t>( v );
  constexpr string_view kTypeName = "UnitTypeAttributes";
  constexpr int         kNumFieldsExpected = 16;
  base::maybe<std::unique_ptr<rcl::table> const&> mtbl =
      v.get_if<std::unique_ptr<rcl::table>>();
  if( !mtbl )
    return rcl::error( fmt::format(
        "cannot produce a {} from type {}.", kTypeName,
        rcl::name_of( rcl::type_of( v ) ) ) );
  DCHECK( *mtbl != nullptr );
  rcl::table const& tbl = **mtbl;
  if( tbl.size() != kNumFieldsExpected )
    return rcl::error(
        fmt::format( "table must have precisely {} fields for "
                     "conversion to {}.",
                     kNumFieldsExpected, kTypeName ) );
  UnitTypeAttributes res;

  // clang-format off
  EVAL( PP_MAP_SEMI( RCL_CONVERT_FIELD,
    name,
    tile,
    nat_icon_front,
    nat_icon_position,
    ship,
    visibility,
    movement_points,
    attack_points,
    defense_points,
    cargo_slots,
    cargo_slots_occupies,
    on_death,
    canonical_base,
    expertise,
    promotion,
    modifiers
  ) );
  // clang-format on

  return res;
}

// Lua
namespace {

LUA_STARTUP( lua::state& st ) {
  using UD = ::rn::UnitTypeAttributes;

  auto ut = st.usertype.create<UD>();

  // FIXME(maybe): these are writable to lua. Find an equivalent
  // of sol::readonly to make them appear constant to luapp.

  LUA_ADD_MEMBER( name );
  LUA_ADD_MEMBER( tile );
  LUA_ADD_MEMBER( nat_icon_front );
  LUA_ADD_MEMBER( nat_icon_position );
  LUA_ADD_MEMBER( ship );
  LUA_ADD_MEMBER( visibility );
  LUA_ADD_MEMBER( movement_points );
  LUA_ADD_MEMBER( attack_points );
  LUA_ADD_MEMBER( defense_points );
  LUA_ADD_MEMBER( cargo_slots );
  LUA_ADD_MEMBER( cargo_slots_occupies );
  LUA_ADD_MEMBER( canonical_base );
  LUA_ADD_MEMBER( expertise );
  LUA_ADD_MEMBER( type );
  LUA_ADD_MEMBER( is_derived );
  LUA_ADD_MEMBER( can_attack );
  LUA_ADD_MEMBER( is_military_unit );

  // FIXME: Figure out how to deal with C++ variants in Lua.
  // LUA_ADD_MEMBER( on_death );
  // LUA_ADD_MEMBER( promotion );
  // LUA_ADD_MEMBER( modifiers );
};

} // namespace

/****************************************************************
** UnitAttributesMap
*****************************************************************/
rcl::convert_err<UnitAttributesMap> convert_to(
    rcl::value const& v, rcl::tag<UnitAttributesMap> ) {
  UNWRAP_RETURN( m,
                 rcl::convert_to<UnitAttributesMap::Map>( v ) );
  // Populate derived fields.
  unordered_set<e_unit_type> derived_types;
  for( auto& [type, type_struct] : m ) {
    type_struct.type = type;
    // Any type that can be obtained by modifying this one is a
    // derived type.
    for( auto& modifier : type_struct.modifiers )
      derived_types.insert( modifier.first );
  }
  for( auto& [type, type_struct] : m )
    type_struct.is_derived = derived_types.contains( type );

  // Validation: any unit type that is derived must not itself
  // have modifiers.
  for( auto& [type, type_struct] : m )
    if( type_struct.is_derived &&
        !type_struct.modifiers.empty() )
      return rcl::error(
          "derived type {} cannot have modifiers.", type );

  // Validation: For each unit type, make sure that each modifier
  // has a unique set of modifiers.
  for( auto& [type, type_struct] : m ) {
    // Unfortunately it seems that unordered_set does not support
    // nesting like this.
    set<set<e_unit_type_modifier>> seen;
    for( auto& [mtype, mod_set] : type_struct.modifiers ) {
      set<e_unit_type_modifier> uset( mod_set.begin(),
                                      mod_set.end() );
      if( seen.contains( uset ) )
        return rcl::error(
            "unit type {} contains a duplicate set of "
            "modifiers.",
            type );
      seen.insert( uset );
    }
  }

  // NOTE: do not use unit_attr() here to access unit properties
  // since it queries a structure that will not yet exist at this
  // point. Instead, us `m` as is done below.

  // Validation (should be done after populating all derived
  // fields). This verifies that any unit that has an
  // on_death.demote field can be demoted regardless of its base
  // type. In other words, any base type that can be modified to
  // another unit X must also provide a valid modifier for when X
  // loses its on-death-lost modifiers.
  for( auto& [type, type_struct] : m ) {
    UNWRAP_CHECK( type_desc, base::lookup( m, type ) );
    for( auto& [mtype, mod_list] : type_struct.modifiers ) {
      UNWRAP_CHECK( mtype_desc, base::lookup( m, mtype ) );
      auto demote =
          mtype_desc.on_death.get_if<UnitDeathAction::demote>();
      if( !demote ) continue;
      // Sanity check: make sure that the modifiers that the
      // derived type is supposed to be losing (to affect
      // demotion) are present in the current modifiers list.
      for( e_unit_type_modifier mod : demote->lose ) {
        if( !mod_list.contains( mod ) )
          return rcl::error(
              "unit type {} is supposed to lose modifier {} "
              "upon demotion, but its base type ({}) modifier "
              "list does not contain that modifier.",
              type, mod, mtype );
      }
      unordered_set<e_unit_type_modifier> target_modifiers =
          mod_list;
      for( e_unit_type_modifier mod : demote->lose )
        target_modifiers.erase( mod );
      if( target_modifiers.empty() )
        // In this case we've lost all modifiers, so we would
        // just become the base type, which is always allowed.
        continue;
      auto& base_modifiers = type_desc.modifiers;
      bool  found          = false;
      for( auto& [mtype, base_mod_set] : base_modifiers )
        if( base_mod_set == target_modifiers ) found = true;
      // Any type that has an on_death.demoted field should
      // always be demotable regardless of base type (that is a
      // requirement of the game rules).
      if( !found )
        return rcl::error(
            "cannot find a new type to which to demote the type "
            "{} (with base type {}).",
            mtype, type );
    }
  }

  // Validate that canonical_base is populated iff the type is a
  // derived type, and that the base type that it refers to has a
  // path to the derived type.
  for( auto& [type, type_struct] : m ) {
    if( type_struct.is_derived ) {
      if( !type_struct.canonical_base.has_value() )
        return rcl::error(
            "derived type {} must have a value for its "
            "`canonical_base` field.",
            type );
      e_unit_type base_type = *type_struct.canonical_base;
      UNWRAP_CHECK( base_desc, base::lookup( m, base_type ) );
      auto& modifiers = base_desc.modifiers;
      if( !modifiers.contains( type ) )
        return rcl::error(
            "derived type {} lists the {} type as its canonical "
            "base type, but that base type does not have a path "
            "to the type {}.",
            type, base_type, type );
      // We're good.
    } else {
      // Not derived type.
      if( type_struct.canonical_base.has_value() )
        return rcl::error(
            "base type {} must have `null` for its "
            "`canonical_base` field.",
            type );
    }
  }

  // Validate that the `expertise` field is only set for base
  // types.
  for( auto& [type, type_struct] : m )
    if( type_struct.is_derived )
      if( type_struct.expertise.has_value() )
        return rcl::error(
            "derived type {} has the`expertise` field set, but "
            "that is only for base types.",
            type );

  // Validation of promotion fields.
  for( auto& [type, type_struct] : m ) {
    if( !type_struct.promotion.has_value() ) continue;
    switch( type_struct.promotion->to_enum() ) {
      using namespace UnitPromotion;
      case e::fixed: {
        // Validation: this must be a base type and the target
        // type of the promotion must be a base type.
        auto const& o = type_struct.promotion->get<fixed>();
        if( type_struct.is_derived )
          return rcl::error(
              "derived type {} cannot have value `fixed` for "
              "its promotion field.",
              type );
        UNWRAP_CHECK( new_type_desc, base::lookup( m, o.type ) );
        if( new_type_desc.is_derived )
          return rcl::error(
              "type {} has type {} as its fixed promotion "
              "target, but the {} type cannot be a fixed "
              "promotion target because it is not a base "
              "type.",
              type, o.type, o.type );
        break;
      }
      case e::occupation: {
        // Validation: this must be a base type.
        if( type_struct.is_derived )
          return rcl::error(
              "derived type {} cannot have value `occupation` "
              "for its promotion field.",
              type );
        break;
      }
      case e::expertise: {
        // Validation: this must be a derived type.
        if( !type_struct.is_derived )
          return rcl::error(
              "base type {} cannot have value `occupation` for "
              "its promotion field.",
              type );
        break;
      }
      case e::modifier: {
        // This is allowed for either base types or derived types
        // and moreover it is not required that the base type be
        // able to accept the modifier (in that case no promotion
        // happens), so there isn't really any validation that we
        // need to do here.
        break;
      }
    }
  }

  return UnitAttributesMap{ .map = std::move( m ) };
}

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
** Commodity to Modifier Conversion
*****************************************************************/
maybe<UnitTypeModifierFromCommodity>
convert_commodity_to_modifier( Commodity const& comm ) {
  switch( comm.type ) {
    case e_commodity::horses: {
      constexpr int needed = 50;
      if( comm.quantity >= needed )
        return UnitTypeModifierFromCommodity{
            .modifier           = e_unit_type_modifier::horses,
            .comm_quantity_used = needed,
        };
      return nothing;
    }
    case e_commodity::muskets: {
      constexpr int needed = 50;
      if( comm.quantity >= needed )
        return UnitTypeModifierFromCommodity{
            .modifier           = e_unit_type_modifier::muskets,
            .comm_quantity_used = needed,
        };
      return nothing;
    }
    case e_commodity::tools: {
      constexpr int multiple     = 20;
      constexpr int max_possible = 100;
      static_assert( max_possible % multiple == 0 );
      if( comm.quantity < multiple ) return nothing;
      int adjusted_quantity =
          std::min( comm.quantity, max_possible );
      int residual = adjusted_quantity % multiple;
      return UnitTypeModifierFromCommodity{
          .modifier           = e_unit_type_modifier::tools,
          .comm_quantity_used = adjusted_quantity - residual,
      };
      return nothing;
    }
    default: return nothing;
  }
}

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

valid_deserial_t UnitType::check_invariants_safe() const {
  if( !unit_type_modifier_path_exists( base_type_, type_ ) )
    return invalid_deserial(
        fmt::format( "no unit type modifier path exists between "
                     "unit types {} and {}.",
                     base_type_, type_ ) );
  return valid;
}

void UnitType::check_invariants_or_die() const {
  CHECK_HAS_VALUE( check_invariants_safe() );
}

UnitType::UnitType( e_unit_type base_type, e_unit_type type )
  : base_type_( base_type ), type_( type ) {
  check_invariants_or_die();
}

UnitType::UnitType()
  : UnitType( e_unit_type::free_colonist,
              e_unit_type::free_colonist ) {
  check_invariants_or_die();
}

UnitTypeAttributes const& unit_attr( UnitType type ) {
  return unit_attr( type.type() );
}

namespace {

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

} // namespace

maybe<UnitType> on_death_demoted_type( UnitType ut ) {
  unordered_set<e_unit_type_modifier> const& current_modifiers =
      ut.unit_type_modifiers();
  UNWRAP_RETURN(
      demote, unit_attr( ut.type() )
                  .on_death.get_if<UnitDeathAction::demote>() );
  unordered_set<e_unit_type_modifier> target_modifiers =
      current_modifiers;
  for( e_unit_type_modifier mod : demote.lose )
    target_modifiers.erase( mod );
  // Any type that has an on_death.demoted field should always be
  // demotable regardless of base type (that is a requirement of
  // the game rules). This should have been validated while dese-
  // rializing the unit descriptor configs, and so we should
  // never check-fail here in practice.
  UNWRAP_CHECK( res, find_unit_type_modifiers(
                         ut.base_type(), target_modifiers ) );
  return res;
}

std::unordered_set<e_unit_type_modifier> const&
UnitType::unit_type_modifiers() {
  UNWRAP_CHECK(
      res, unit_type_modifiers_for_path( base_type_, type_ ) );
  return res;
}

maybe<UnitType> add_unit_type_modifiers(
    UnitType                                    ut,
    std::initializer_list<e_unit_type_modifier> modifiers ) {
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

// Lua
namespace {

LUA_STARTUP( lua::state& st ) {
  auto utype = st.usertype.create<UnitType>();

  utype["base_type"] = &UnitType::base_type;
  utype["type"]      = &UnitType::type;

  lua::table utype_tbl =
      lua::table::create_or_get( st["utype"] );

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