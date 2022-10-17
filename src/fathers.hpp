/****************************************************************
**fathers.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-26.
*
* Description: Api for querying properties of founding fathers.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

// ss
#include "ss/fathers.rds.hpp"

// C++ standard library
#include <string_view>
#include <vector>

namespace rn {

struct Player;
struct SS;
struct SSConst;
struct TS;

/****************************************************************
** e_founding_father
*****************************************************************/
std::string_view founding_father_name(
    e_founding_father father );

/****************************************************************
** e_founding_father_type
*****************************************************************/
e_founding_father_type founding_father_type(
    e_founding_father father );

std::vector<e_founding_father> const& founding_fathers_for_type(
    e_founding_father_type type );

std::string_view founding_father_type_name(
    e_founding_father_type type );

/****************************************************************
** Father Selection
*****************************************************************/
// Does the player have all founding fathers?
bool has_all_fathers( Player const& player );

// This will return the total number of bells that are needed to
// get the next father, not accounting for the bells that the
// player already has. Will return nothing if all fathers have
// been obtained.
maybe<int> bells_needed_for_next_father( SSConst const& ss,
                                         Player const&  player );

// If the player has some bells and is not currently working to-
// ward a founding father then this will pop up a menu allowing
// (requiring) the player to choose a founding father. Before it
// does so it will re-populate the pool of fathers. If there are
// no fathers left then it will do nothing.
wait<> pick_founding_father_if_needed( SSConst const& ss, TS& ts,
                                       Player& player );

// This is called once per turn to check if we need to choose a
// new founding father and/or if we've obtained one. If we've ob-
// tained one then it will be awarded to the player and also will
// be returned. This will not play the continental congress
// cut-scene animation though when a new father is obtained; that
// should be done by the caller if a father is returned.
maybe<e_founding_father> check_founding_fathers(
    SSConst const& ss, Player& player );

// Cuts to a view of the Continental Congress hall and animates
// the appearance of the new founding father, as in the OG.
wait<> play_new_father_cut_scene( TS& ts, Player const& player,
                                  e_founding_father new_father );

// This should be called just after a founding father is ob-
// tained. It's purpose is to affect any one-time changes that
// are supposed to happen as a result of obtaining that father
// (some founding fathers have one-time effects while others' ef-
// fects are ongoing; this function is for the former).
void on_father_received( SS& ss, TS& ts, Player& player,
                         e_founding_father father );

} // namespace rn
