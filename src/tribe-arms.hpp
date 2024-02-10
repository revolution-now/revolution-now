/****************************************************************
**tribe-arms.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-26.
*
* Description: Manages a tribe's horses and muskets.
*
*****************************************************************/
#pragma once

// rds
#include "tribe-arms.rds.hpp"

namespace rn {

struct Tribe;
struct IRand;
struct SSConst;

/****************************************************************
** Public API.
*****************************************************************/
// Called when a brave with horses is defeated in combat (de-
// stroyed) and the tribe retains the horses. Note: although
// there is some randomness in determining when the tribe retains
// the horses, that is done by the caller ahead of time, and this
// is only called when the result is in the affirmative.
void retain_horses_from_destroyed_brave( SSConst const& ss,
                                         Tribe&         tribe );

// Called when a brave with muskets is defeated in combat (de-
// stroyed) and the tribe retains the muskets. Note: although
// there is some randomness in determining when the tribe retains
// the muskets, that is done by the caller ahead of time, and
// this is only called when the result is in the affirmative.
void retain_muskets_from_destroyed_brave( Tribe& tribe );

// This is called when a brave without horses wins in combat
// against a european unit that loses its horses (or gets de-
// stroyed with horses) in the process. In that situation the
// brave gains the horses (handled elsewhere) and the tribe in-
// creases the number of their herds, which this function does.
void gain_horses_from_winning_combat( Tribe& tribe );

// When a brave raids a colony and steals `quantity` muskets from
// the colony's stock.
void acquire_muskets_from_colony_raid( Tribe& tribe,
                                       int    quantity );

// When a brave raids a colony and steals some quantity of horses
// from the colony's stock. Experiments seem to have indicated
// that the resulting behavior does not depend in any way on the
// quantity stolen, though we still pass it in just to check if
// it is zero.
void acquire_horses_from_colony_raid( SSConst const& ss,
                                      Tribe&         tribe,
                                      int            quantity );

// Choose if/how to equip the brave based on the availability of
// muskets and horses in the tribe and return the brave type se-
// lected as well as any deductions in the tribe's stockpiles.
[[nodiscard]] EquippedBrave select_brave_equip(
    SSConst const& ss, IRand& rand, Tribe const& tribe );

} // namespace rn
