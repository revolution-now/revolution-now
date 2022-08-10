/****************************************************************
**promotion.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-10.
*
* Description: All things related to unit type promotion
*              & demotion.
*
*****************************************************************/
#include "promotion.hpp"

// Revolution Now
#include "ustate.hpp"

// config
#include "config/unit-type.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/unit-composer.hpp"
#include "ss/unit.hpp"

using namespace std;

namespace rn {

namespace {

// Gets the one (unique) unit type that is the expert for this
// activity. Note that During config deserialization it should
// have been verified that there is precisely one expert for each
// activity.
e_unit_type expert_for_activity( e_unit_activity activity ) {
  // During config deserialization it should have been verified
  // that there is precisely one expert for each activity.
  for( auto& [type, attr] :
       config_unit_type.composition.unit_types )
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

maybe<e_unit_type> cleared_expertise( e_unit_type type ) {
  return unit_attr( type ).cleared_expertise;
}

// This function will promote a unit given an activity. It will
// promote the unit type given the activity, then preserve the
// inventory. Note that if the unit type is already an expert at
// something other than the activity then this will not promote
// them, since that does not happen in the game normally (there
// may be some cheat/debug features that allow doing that, but
// that logic is kept separate from this). On the other hand, if
// the unit is already an expert at the given activity, then no
// promotion will happen and an error will be returned.
expect<UnitComposition> promoted_from_activity(
    UnitComposition const& comp, e_unit_activity activity ) {
  maybe<UnitType> new_type_obj =
      promoted_unit_type( comp.type_obj(), activity );
  if( !new_type_obj.has_value() )
    return unexpected<UnitComposition>(
        "viable unit type not found" );
  return UnitComposition::create( *new_type_obj,
                                  comp.inventory() );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
bool try_promote_unit_for_current_activity( SSConst const& ss,
                                            Unit& unit ) {
  if( !is_unit_human( unit.type_obj() ) ) return false;
  maybe<e_unit_activity> activity = current_activity_for_unit(
      ss.units, ss.colonies, unit.id() );
  if( !activity.has_value() ) return false;
  if( unit_attr( unit.base_type() ).expertise == *activity )
    return false;
  expect<UnitComposition> promoted =
      promoted_from_activity( unit.composition(), *activity );
  if( !promoted.has_value() ) return false;
  unit.change_type( *promoted );
  return true;
}

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

maybe<e_unit_type> on_capture_demoted_type( UnitType ut ) {
  UNWRAP_RETURN(
      capture_and_demote,
      unit_attr( ut.type() )
          .on_death
          .get_if<UnitDeathAction::capture_and_demote>() );
  return capture_and_demote.type;
}

} // namespace rn
