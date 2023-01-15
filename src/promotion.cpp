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
#include "colony-buildings.hpp"
#include "irand.hpp"
#include "land-production.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"

// config
#include "config/colony.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colony.rds.hpp"
#include "ss/native-enums.hpp"
#include "ss/ref.hpp"
#include "ss/unit-composer.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"

using namespace std;

namespace rn {

namespace {

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

// This promotes a unit type only, meaning that it ignores inven-
// tory. If the promotion is possible then either the base type
// or derived type (or both) may change. The `activity` parameter
// may or may not be used depending on the unit type. The logic
// behind this function is a bit complicated; see the comments in
// the Rds definition for UnitPromotion as well as the function
// implementation for more info.
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

} // namespace

/****************************************************************
** Public API
*****************************************************************/
e_unit_type expert_for_activity( e_unit_activity activity ) {
  // During config deserialization it should have been verified
  // that there is precisely one expert for each activity.
  for( auto& [type, attr] :
       config_unit_type.composition.unit_types )
    if( attr.expertise == activity ) return type;
  FATAL( "expert unit type for activity {} not found.",
         activity );
}

bool try_promote_unit_for_current_activity( SSConst const& ss,
                                            Player const& player,
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
  unit.change_type( player, *promoted );
  return true;
}

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

maybe<UnitComposition> promoted_by_natives(
    UnitComposition const& comp, e_native_skill skill ) {
  e_unit_activity const activity =
      activity_for_native_skill( skill );
  maybe<e_unit_activity> const expertise =
      unit_attr( comp.base_type() ).expertise;
  if( expertise.has_value() ) return nothing;
  if( comp.type_obj().type() != comp.type_obj().base_type() ) {
    // This will attempt to preserve the unit type modifiers and
    // inventory and only promote the base type, so that e.g.
    // when a pioneer enters a native village and learns a skill
    // it will continue to be a pioneer with the same number of
    // tools, but its base type will be an expert. Or a regular
    // scout might become a seasoned scout.
    e_unit_type const new_base_type =
        expert_for_activity( activity );
    unordered_set<e_unit_type_modifier> const& modifiers =
        comp.type_obj().unit_type_modifiers();
    UNWRAP_RETURN(
        with_modifiers,
        add_unit_type_modifiers(
            UnitType::create( new_base_type ), modifiers ) );
    expect<UnitComposition> const res = UnitComposition::create(
        with_modifiers, comp.inventory() );
    if( res.has_value() ) return *res;
    return nothing;
  } else {
    e_unit_type const new_unit_type =
        expert_for_activity( activity );
    expect<UnitComposition> const res = UnitComposition::create(
        UnitType::create( new_unit_type ), comp.inventory() );
    if( res.has_value() ) return *res;
    return nothing;
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

std::vector<OnTheJobPromotionResult>
workers_to_promote_for_on_the_job_training(
    SSConst const& ss, TS& ts, Colony const& colony ) {
  std::vector<OnTheJobPromotionResult> res;

  // First the indoor workers. Note that in the original game
  // only outdoor workers can get promoted. We support both here,
  // but configure it to be compatible with config settings.
  for( auto const& [job, units] : colony.indoor_jobs ) {
    if( !config_colony.on_the_job_training
             .eligible_indoor_jobs[job] )
      continue;
    for( UnitId const unit_id : units ) {
      e_unit_type const unit_type =
          ss.units.unit_for( unit_id ).type();
      maybe<double> const probability = base::lookup(
          config_colony.on_the_job_training.probabilities,
          unit_type );
      if( !probability.has_value() )
        // This unit type is not eligible for promotion, typi-
        // cally because they are either already an expert at
        // something or they are a native convert.
        continue;
      // At this point we know that the unit type and its current
      // job are both such that unit is eligible for promotion.
      // Now roll the dice to see if it happens.
      DCHECK( *probability >= 0 && *probability <= 1.0 );
      if( !ts.rand.bernoulli( *probability ) )
        // Sorry, maybe next time.
        continue;
      // The unit is getting promoted! Now find what type. The
      // idea is that they get promoted immediately to the unit
      // type that is an expert at what they are currently doing.
      e_unit_activity const activity =
          activity_for_indoor_job( job );
      e_unit_type const promoted_type =
          expert_for_activity( activity );
      res.push_back( OnTheJobPromotionResult{
          .unit_id = unit_id, .promoted_to = promoted_type } );
    }
  }

  // Next the outdoor workers.
  for( auto const& [direction, outdoor_unit] :
       colony.outdoor_jobs ) {
    if( !outdoor_unit.has_value() )
      // No unit working on this square.
      continue;
    auto const [unit_id, job] = *outdoor_unit;
    if( !config_colony.on_the_job_training
             .eligible_outdoor_jobs[job] )
      continue;
    e_unit_type const unit_type =
        ss.units.unit_for( unit_id ).type();
    maybe<double> const probability = base::lookup(
        config_colony.on_the_job_training.probabilities,
        unit_type );
    if( !probability.has_value() )
      // This unit type is not eligible for promotion, typically
      // because they are either already an expert at something
      // or they are a native convert.
      continue;
    // At this point we know that the unit type and its current
    // job are both such that unit is eligible for promotion. Now
    // roll the dice to see if it happens.
    DCHECK( *probability >= 0 && *probability <= 1.0 );
    if( !ts.rand.bernoulli( *probability ) )
      // Sorry, maybe next time.
      continue;
    // The unit is getting promoted! Now find what type. The idea
    // is that they get promoted immediately to the unit type
    // that is an expert at what they are currently doing.
    e_unit_activity const activity =
        activity_for_outdoor_job( job );
    e_unit_type const promoted_type =
        expert_for_activity( activity );
    res.push_back( OnTheJobPromotionResult{
        .unit_id = unit_id, .promoted_to = promoted_type } );
  }

  return res;
}

} // namespace rn
