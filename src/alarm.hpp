/****************************************************************
**alarm.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-07.
*
* Description: Things related to alarm level of tribes and
*              dwellings.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "alarm.rds.hpp"

// ss
#include "ss/nation.rds.hpp"
#include "ss/native-enums.rds.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct Dwelling;
struct Player;
struct SSConst;
struct Tribe;
struct TribeRelationship;

// Combines the dwelling-level and tribal alarm to produce an ef-
// fective alarm for a particular dwelling. This is used when a
// single dwelling needs to decide how to act/react to the
// player.
int effective_dwelling_alarm( SSConst const&  ss,
                              Dwelling const& dwelling,
                              e_nation        nation );

// Determines how a dwelling is described to react to the player
// upon entering the native dwelling. It is based on the effec-
// tive alarm as well as whether the tribe is at war with the
// player. This doesn't really have any effect on gameplay, it is
// just used to improve dialog messages when visiting a native
// dwelling.
e_enter_dwelling_reaction reaction_for_dwelling(
    SSConst const& ss, Player const& player, Tribe const& tribe,
    Dwelling const& dwelling );

// Called when the player steals land from the natives by any
// means.
void increase_tribal_alarm_from_land_grab(
    SSConst const& ss, Player const& player,
    TribeRelationship& relationship, Coord tile );

// This is used when either attacking a brave out in the open or
// attacking a dwelling that has a brave sitting on top of it.
// Note that the capital status of the dwelling that the brave
// belongs to will affect the tribal alarm increase, as usual in
// the OG where tribal alarm affects are amplified when made by
// way of the capital.
void increase_tribal_alarm_from_attacking_brave(
    SSConst const& ss, Dwelling const& dwelling,
    TribeRelationship& relationship );

// This is used when attacking a dwelling that does not have a
// brave sitting on top of it.
void increase_tribal_alarm_from_attacking_dwelling(
    SSConst const& ss, Dwelling const& dwelling,
    TribeRelationship& relationship );

// Attempts to set the tribal alarm at "content" (note: this is
// not quite as good as "happy"). However, some tribes may have a
// non-zero minimum tribal alarm, in which case this will go as
// close to "content" as the tribe config allows.
void set_tribal_alarm_to_content_if_possible(
    e_tribe tribe, int& tribal_alarm );

// Puts the tribal alarm into a bucket; this is useful for se-
// lecting a tribe's behavior in a simple way based on alarm.
e_alarm_category tribe_alarm_category( int tribal_alarm );

} // namespace rn
