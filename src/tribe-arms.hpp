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

// Choose if/how to equip a newly-created brave based on the
// availability of muskets and horses in the tribe and return the
// brave type selected as well as any deductions in the tribe's
// stockpiles.
[[nodiscard]] EquippedBrave select_new_brave_equip(
    SSConst const& ss, IRand& rand, Tribe const& tribe );

// Same as above but for a brave that already exists. This will
// not allow any transitions that require discarding arms; e.g.,
// it won't allow an armed_brave to transition to a
// mounted_brave, since that would entail shedding muskets.
//
// NOTE: in the OG there appears to be a probability that an ex-
// isting brave that is stitting over a dwelling and which can be
// equipped will sometimes not be equipped, however that handled
// by the caller of this function. This function assumes that any
// equipping that can be done should be done.
[[nodiscard]] EquippedBrave select_existing_brave_equip(
    SSConst const& ss, IRand& rand, Tribe const& tribe,
    e_native_unit_type type );

// Called each turn to breed horses.
void evolve_tribe_horse_breeding( SSConst const& ss,
                                  Tribe&         tribe );

// Each time a dwelling is destroyed (even the last one) this
// should be called to adjust the tribe's stockpile of muskets
// and horses.
//
// NOTE: this must be called before the dwelling is destroyed.
void adjust_arms_on_dwelling_destruction( SSConst const& ss,
                                          Tribe&         tribe );

// NOTE: this is an expensive call, so should probably not be
// called every frame.
ArmsReportForIndianAdvisorReport tribe_arms_for_advisor_report(
    SSConst const& ss, Tribe const& tribe );

} // namespace rn
