/****************************************************************
**promotion.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-10.
*
* Description: All things related to unit type promotion
*              & demotion.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// ss
#include "ss/unit-type.hpp"

namespace rn {

struct SSConst;
struct Unit;

// This is the function that top-level game logic should call
// whenever it is determined that a unit is up for promotion (the
// it is ok if the unit cannot be promoted further). This will
// find the unit's current activity and promote it along those
// lines if a promotion target exists. It will return true if a
// promotion was possible (and was thus made). If the unit has no
// activity the no promotion will be made. Technically, in that
// case, we could still theoretically promote e.g. an indentured
// servant to a free colonist, but the game will never do that
// (outside of cheat mode) when the unit has no activity, so we
// don't do that. Also, note that if the unit is an expert at
// something other than the given activity then no promotion will
// happen.
//
// TODO: promote veterans to continentals after independence.
//
bool try_promote_unit_for_current_activity( SSConst const& ss,
                                            Unit& unit );

// This promotes a unit. If the promotion is possible then either
// the base type or derived type (or both) may change. The `ac-
// tivity` parameter may or may not be used depending on the unit
// type. The logic behind this function is a bit complicated; see
// the comments in the Rds definition for UnitPromotion as well
// as the function implementation for more info.
//
// This may be a bit expensive to call; it should not be called
// on every frame or on every unit in a given turn. It should
// only be called when we know that we want to try to promote a
// unit, which should not happen that often. It is ok to call it
// on the order of once per battle, although that probably won't
// happen since the probability of promotion in a battle is not
// large.
//
// NOTE: this function should not be called directly to promote a
// unit in this fashion because it will not take into account
// unit inventory. Search for the other functions that call this
// one.
maybe<UnitType> promoted_unit_type( UnitType        ut,
                                    e_unit_activity activity );

// Will attempt to clear the expertise (if any) of the base type
// while holding any modifiers constant. Though if the derived
// type specifies a cleared_expertise target then that will be
// respected without regard for the base type: that target will
// be created with its default base type and returned.
maybe<UnitType> cleared_expertise( UnitType ut );

// This will return nothing if the unit does not have an
// on_death.demoted property, otherwise it will return the new
// UnitType representing the demoted unit, which is guaranteed by
// the game rules (and validation performed during deserializa-
// tion of the unit descriptor configs) to exist regardless of
// base type.
maybe<UnitType> on_death_demoted_type( UnitType ut );

// For units that get demoted upon capture (e.g.
// veteran_colonist) this will return that demoted type.
maybe<e_unit_type> on_capture_demoted_type( UnitType ut );

} // namespace rn
