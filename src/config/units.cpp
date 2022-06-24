/****************************************************************
**units.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-24.
*
* Description: Configuration for unit types.
*
*****************************************************************/
#include "units.hpp"

// refl
#include "refl/ext.hpp"

using namespace std;

namespace rn {

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
    if( activity == e_unit_activity::teaching )
      // This is a hack to avoid the fact that we don't have a
      // unit with the expertise of teaching. Apparently the
      // original game had an expert teacher (and it can still be
      // seen in the original game by entering cheat mode and
      // promoting a unit that is teaching), but it seems that it
      // was removed before the game was shipped. It is not clear
      // what an expert teacher would have done.
      continue;
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
