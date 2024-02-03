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

namespace rn {

struct Tribe;
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

} // namespace rn
