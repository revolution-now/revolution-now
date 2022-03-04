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

// Revolution Now (config)
#include "../config/rcl/units.inl"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/rtable.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

// refl
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
** e_unit_type
*****************************************************************/
LUA_ENUM( unit_type );

/****************************************************************
** e_unit_human
*****************************************************************/
LUA_ENUM( unit_human );

/****************************************************************
** e_unit_type_modifier
*****************************************************************/
LUA_ENUM( unit_type_modifier );

/****************************************************************
** e_unit_activity
*****************************************************************/
LUA_ENUM( unit_activity );

/****************************************************************
** Unit Inventory
*****************************************************************/
namespace {

unordered_map<e_unit_inventory, e_unit_type_modifier>
create_inventory_to_modifier_map(
    EnumMap<e_unit_type_modifier, UnitTypeModifierTraits> const&
        modifier_traits ) {
  unordered_map<e_unit_inventory, e_unit_type_modifier> res;
  for( auto const& [mod, val] : modifier_traits ) {
    auto inventory =
        val.association.get_if<ModifierAssociation::inventory>();
    if( !inventory ) continue;
    bool was_inserted =
        res.try_emplace( inventory->type, mod ).second;
    CHECK( was_inserted );
  }
  return res;
}

} // namespace

maybe<e_unit_type_modifier> inventory_to_modifier(
    e_unit_inventory inv ) {
  static auto const m = [] {
    DCHECK( configs_loaded() );
    return create_inventory_to_modifier_map(
        config_units.composition.modifier_traits );
  }();
  DCHECK( !m.empty() );
  return base::lookup( m, inv );
}

maybe<e_unit_inventory> commodity_to_inventory(
    e_commodity comm ) {
  return refl::enum_from_string<e_unit_inventory>(
      refl::enum_value_name( comm ) );
}

maybe<e_commodity> inventory_to_commodity(
    e_unit_inventory inv_type ) {
  return config_units.composition.inventory_traits[inv_type]
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
UnitTypeAttributes const& unit_attr( e_unit_type type ) {
  UNWRAP_CHECK_MSG(
      desc,
      base::lookup( config_units.composition.unit_types, type ),
      "internal error: unit type {} does not have a type "
      "descriptor.",
      type );
  return desc;
}

bool can_attack( UnitTypeAttributes const& attr ) {
  return attr.attack_points > 0;
}

bool is_military_unit( UnitTypeAttributes const& attr ) {
  return can_attack( attr );
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
  LUA_ADD_MEMBER( human );
  LUA_ADD_MEMBER( visibility );
  LUA_ADD_MEMBER( movement_points );
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

bool is_unit_human( UnitType ut ) {
  e_unit_human res =
      config_units.composition.unit_types[ut.type()].human;
  switch( res ) {
    case e_unit_human::no: return false;
    case e_unit_human::yes: return true;
    case e_unit_human::from_base: {
      res = config_units.composition.unit_types[ut.base_type()]
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

namespace {

e_unit_type expert_for_activity( e_unit_activity activity ) {
  // During config deserialization it should have been verified
  // that there is precisely one expert for each activity.
  for( auto& [type, attr] : config_units.composition.unit_types )
    if( attr.expertise == activity ) return type;
  SHOULD_NOT_BE_HERE;
}

// This will attempt to change only the base type while holding
// the modifier list constant. If a valid unit results, it will
// be returned.
maybe<UnitType> change_base_with_constant_modifiers(
    UnitType ut, e_unit_type new_base_type ) {
  unordered_set<e_unit_type_modifier> const& existing_modifiers =
      ut.unit_type_modifiers();
  // We always allow the independence modifier because this func-
  // tion is just keeping the modifiers constant, so if that mod-
  // ifier was already there then it is allowed.
  return add_unit_type_modifiers(
      UnitType::create( new_base_type ), existing_modifiers );
}

} // namespace

maybe<UnitType> promoted_unit_type( UnitType        ut,
                                    e_unit_activity activity ) {
  if( ut.type() == ut.base_type() ) {
    UNWRAP_RETURN( promo,
                   unit_attr( ut.base_type() ).promotion );
    switch( promo.to_enum() ) {
      using namespace UnitPromotion;
      case e::fixed:
        return UnitType::create( promo.get<fixed>().type );
      case e::occupation:
        // In this case we need to mind the occupation of the
        // unit, but there is no derived type to provide one for
        // us, so we fall back to the `activity` parameter.
        return UnitType::create(
            expert_for_activity( activity ) );
      case e::expertise: {
        // Should have been validated during config reading. This
        // setting is not for base types.
        SHOULD_NOT_BE_HERE;
      }
      case e::modifier: {
        // Not currently used in the unmodded game.
        SHOULD_NOT_BE_HERE;
      }
    }
  }
  // Base type and effective type are different.
  auto& base_promo     = unit_attr( ut.base_type() ).promotion;
  auto& eff_type_promo = unit_attr( ut.type() ).promotion;
  if( !base_promo.has_value() && !eff_type_promo.has_value() )
    return nothing;
  if( !eff_type_promo.has_value() ) {
    // Base type has promotion mode but not effective type. Cur-
    // rently does not happen in unmodded game.
    SHOULD_NOT_BE_HERE;
  }

  if( !base_promo.has_value() ) {
    // Base type has nothing to say about promotion, so it just
    // comes down to the effective type.
    DCHECK( eff_type_promo.has_value() );
    switch( eff_type_promo->to_enum() ) {
      using namespace UnitPromotion;
      case e::fixed:
      case e::occupation:
        // This should not happen because if it happens then that
        // means that the effective type is a base type (because
        // `fixed` and `occupation` are only allowed for base
        // types) which would mean that the base types and effec-
        // tive types should be the same, which is a case that we
        // should have already handled above.
        SHOULD_NOT_BE_HERE;
      case e::expertise: {
        // In this case the derived type is granting expertise,
        // but the base type does not have the `occupation` pro-
        // motion mode (in fact it has no promotion mode set) and
        // so we can't do any promotion here. Not sure if this
        // ever happens in the unmodded game.
        return nothing;
      }
      case e::modifier: {
        // Here, the derived type says "when promoting me, try to
        // add a modifier to my UnitType." An example of this
        // would be a veteran dragoon getting promoted in battle
        // to a continental cavalry.
        auto const&          o = eff_type_promo->get<modifier>();
        e_unit_type_modifier modifier = o.kind;
        return add_unit_type_modifiers( ut, { modifier } );
      }
    }
  }

  DCHECK( base_promo.has_value() );
  DCHECK( eff_type_promo.has_value() );
  // At this point, both base type and derived types specify pro-
  // motion modes and they have different types.
  switch( eff_type_promo->to_enum() ) {
    using namespace UnitPromotion;
    case e::fixed:
    case e::occupation:
      // This should not happen because if it happens then that
      // means that the effective type is a base type (because
      // `fixed` and `occupation` are only allowed for base
      // types) which would mean that the base types and effec-
      // tive types should be the same, which is a case that we
      // should have already handled above.
      SHOULD_NOT_BE_HERE;
    case e::expertise: {
      auto const& o_derived = eff_type_promo->get<expertise>();
      switch( base_promo->to_enum() ) {
        using namespace UnitPromotion;
        case e::fixed: {
          // derived type: expertise
          // base type:    fixed
          //
          // In this case the derived type is granting expertise,
          // but the base type wants to be promoted always to a
          // fixed type, so we will respect that.
          auto const& o = base_promo->get<fixed>();
          return change_base_with_constant_modifiers( ut,
                                                      o.type );
        }
        case e::occupation: {
          // derived type: expertise
          // base type:    occupation
          //
          // In this case we need to mind the occupation of the
          // unit, and the derived type is providing one for us,
          // so we will use the provided one and ignore the `ac-
          // tivity` parameter to this function. So we will use
          // the activity to promote the base type while at-
          // tempting to keep the modifiers the same.
          e_unit_type new_base_type =
              expert_for_activity( o_derived.kind );
          return change_base_with_constant_modifiers(
              ut, new_base_type );
        }
        case e::expertise: {
          // Should have been validated during config reading.
          // This setting is not for base types.
          SHOULD_NOT_BE_HERE;
        }
        case e::modifier: {
          // derived type: expertise
          // base type:    modifier
          //
          // In this case the derived type is granting expertise,
          // but the base type does not have the `occupation`
          // mode set, so it cannot be granted expertise. It
          // would seem a bit strange to grant the type a modi-
          // fier as is being requested by the base type, so we
          // will just do nothing.
          return nothing;
        }
      }
      SHOULD_NOT_BE_HERE;
    }
    case e::modifier: {
      auto const& o_derived = eff_type_promo->get<modifier>();
      // If the derived type is asking to add a modifier, then
      // just attempt it and if it fails, do nothing. In this
      // scenario we just ignore what the base type wants.
      e_unit_type_modifier modifier = o_derived.kind;
      return add_unit_type_modifiers( ut, { modifier } );
    }
  }
}

namespace {

maybe<e_unit_type> cleared_expertise( e_unit_type type ) {
  return unit_attr( type ).cleared_expertise;
}

} // namespace

maybe<UnitType> cleared_expertise( UnitType ut ) {
  // First see if the effective type has a cleared_expertise
  // specification. If so, then give priority to that.
  if( auto type = cleared_expertise( ut.type() ); type )
    return UnitType::create( *type );
  // If not, then try base type.
  UNWRAP_RETURN( new_base_type,
                 cleared_expertise( ut.base_type() ) );
  return change_base_with_constant_modifiers( ut,
                                              new_base_type );
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

/****************************************************************
** UnitCompositionConfig
*****************************************************************/
valid_or<string> UnitCompositionConfig::validate() const {
  auto& m = unit_types;
  // Validation: any unit type that is derived must not itself
  // have modifiers.
  for( auto& [type, type_struct] : m )
    if( type_struct.is_derived &&
        !type_struct.modifiers.empty() )
      return fmt::format(
          "derived type {} cannot have modifiers.", type );

  // Validation: For each unit type, make sure that each modifier
  // has a non-empty set of modifiers.
  for( auto& [type, type_struct] : m )
    for( auto& [mtype, mod_set] : type_struct.modifiers )
      REFL_VALIDATE( !mod_set.empty(),
                     "type `{}' has an empty list of modifiers "
                     "for the modified type `{}'.",
                     type, mtype );

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
        return fmt::format(
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
          return fmt::format(
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
      auto& base_modifiers = type_struct.modifiers;
      bool  found          = false;
      for( auto& [mtype, base_mod_set] : base_modifiers )
        if( base_mod_set == target_modifiers ) found = true;
      // Any type that has an on_death.demoted field should
      // always be demotable regardless of base type (that is a
      // requirement of the game rules).
      if( !found )
        return fmt::format(
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
        return fmt::format(
            "derived type {} must have a value for its "
            "`canonical_base` field.",
            type );
      e_unit_type base_type = *type_struct.canonical_base;
      UNWRAP_CHECK( base_desc, base::lookup( m, base_type ) );
      auto& modifiers = base_desc.modifiers;
      if( !modifiers.contains( type ) )
        return fmt::format(
            "derived type {} lists the {} type as its canonical "
            "base type, but that base type does not have a path "
            "to the type {}.",
            type, base_type, type );
      // We're good.
    } else {
      // Not derived type.
      if( type_struct.canonical_base.has_value() )
        return fmt::format(
            "base type {} must have `null` for its "
            "`canonical_base` field.",
            type );
    }
  }

  // Validate that only base types have human == yes/no and
  // derived types have human == from_base.
  for( auto& [type, type_struct] : m ) {
    if( type_struct.is_derived ) {
      REFL_VALIDATE(
          type_struct.human == e_unit_human::from_base,
          "derived type {} must have `from_base` for its "
          "`human` field.",
          type );
    } else {
      // Not derived type.
      REFL_VALIDATE(
          type_struct.human != e_unit_human::from_base,
          "base type {} must not have `from_base` for its "
          "`human` field.",
          type );
    }
  }

  // Validate that the `expertise` field is only set for base
  // types.
  for( auto& [type, type_struct] : m )
    if( type_struct.is_derived )
      if( type_struct.expertise.has_value() )
        return fmt::format(
            "derived type {} has the`expertise` field set, but "
            "that is only for base types.",
            type );

  // Validation that there is precisely one unit that has
  // expertise in each activity.
  unordered_set<e_unit_activity> expertises;
  for( auto& [type, type_struct] : m ) {
    if( !type_struct.expertise.has_value() ) continue;
    e_unit_activity activity = *type_struct.expertise;
    CHECK(
        !expertises.contains( activity ),
        "there are multiple unit types which have expertise {}.",
        activity );
    expertises.insert( activity );
  }
  for( auto activity : refl::enum_values<e_unit_activity> ) {
    CHECK( expertises.contains( activity ),
           "there is no unit type that has expertise {}.",
           activity );
  }

  // Validation of promotion fields.
  for( auto& [type, type_struct] : m ) {
    if( !type_struct.promotion.has_value() ) continue;
    switch( type_struct.promotion->to_enum() ) {
      using namespace UnitPromotion;
      case e::fixed: {
        // Validation: this must be a base type and the target
        // type of the promotion must be a base type.
        auto const& o_fixed =
            type_struct.promotion->get<fixed>();
        if( type_struct.is_derived )
          return fmt::format(
              "derived type {} cannot have value `fixed` for "
              "its promotion field.",
              type );
        UNWRAP_CHECK( new_type_desc,
                      base::lookup( m, o_fixed.type ) );
        if( new_type_desc.is_derived )
          return fmt::format(
              "type {} has type {} as its fixed promotion "
              "target, but the {} type cannot be a fixed "
              "promotion target because it is not a base "
              "type.",
              type, type, type );
        break;
      }
      case e::occupation: {
        // Validation: this must be a base type.
        if( type_struct.is_derived )
          return fmt::format(
              "derived type {} cannot have value `occupation` "
              "for its promotion field.",
              type );
        break;
      }
      case e::expertise: {
        // Validation: this must be a derived type.
        if( !type_struct.is_derived )
          return fmt::format(
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

  // Get the inventory-to-modifier traits this way because we
  // cannot use config_units.composition.modifier_traits because
  // that structure has not yet been populated at this stage.
  unordered_map<e_unit_inventory, e_unit_type_modifier> const
      inv_to_mod =
          create_inventory_to_modifier_map( modifier_traits );

  // Validate that no base types have inventory_types that in-
  // clude any inventory type that has a modifier association.
  for( auto& [type, type_struct] : m ) {
    if( !type_struct.is_derived ) {
      for( e_unit_inventory inv : type_struct.inventory_types ) {
        maybe<e_unit_type_modifier> mod =
            base::lookup( inv_to_mod, inv );
        REFL_VALIDATE(
            !mod.has_value(),
            "base type `{}' cannot have inventory type `{}' "
            "because it has the modifier assocation `{}'.",
            type, inv, *mod );
      }
    }
  }

  // Validate that, for each base type, the list of modifiers re-
  // quired for each modified type includes the
  // inventory-associated modifier if the modified type includes
  // the modifier-associated inventory type.
  for( auto& [type, type_struct] : m ) {
    if( type_struct.is_derived ) continue;
    for( auto& [mtype, mod_list] : type_struct.modifiers ) {
      UNWRAP_CHECK( mtype_desc, base::lookup( m, mtype ) );
      unordered_set<e_unit_inventory>
          modifier_associated_inventory_types;
      for( e_unit_inventory inv_type :
           mtype_desc.inventory_types ) {
        maybe<e_unit_type_modifier> mod =
            base::lookup( inv_to_mod, inv_type );
        if( !mod.has_value() ) continue;
        // This is a modifier-associated inventory type, so make
        // sure that the modifier is in the list.
        REFL_VALIDATE(
            mod_list.contains( *mod ),
            "base type `{}' has a modifier conversion to "
            "derived type `{}' which in turn has a "
            "modifier-associated inventory type `{}', but the "
            "associated modifier does not appear in the list in "
            "the base type.",
            type, mtype, inv_type );
      }
    }
  }

  return base::valid;
}

} // namespace rn