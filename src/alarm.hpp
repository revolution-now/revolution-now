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

void increase_tribal_alarm_from_attacking_brave(
    TribeRelationship& relationship );

} // namespace rn
