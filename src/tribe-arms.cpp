/****************************************************************
**tribe-arms.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-26.
*
* Description: Manages a tribe's horses and muskets.
*
*****************************************************************/
#include "tribe-arms.hpp"

// config
#include "config/natives.rds.hpp"

// ss
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/tribe.rds.hpp"

using namespace std;

namespace rn {

namespace {

int max_horse_breeding( SSConst const& ss, e_tribe tribe_type ) {
  int const num_dwellings =
      ss.natives.dwellings_for_tribe( tribe_type ).size();
  e_native_level const level =
      config_natives.tribes[tribe_type].level;
  auto const& coefficients =
      config_natives.arms.max_horse_breeding[level];
  // From experiments with the OG, the formula is:
  //
  //   max_value = 50 + N + D*M + A
  //
  // where D is the number of dwellings, and N, M, A are
  // constants that are dependent on the native tech level.
  return 50 + coefficients.N + num_dwellings * coefficients.M +
         coefficients.A;
}

void add_to_horse_breeding( SSConst const& ss,
                            e_tribe tribe_type, int delta,
                            int& horse_breeding ) {
  horse_breeding += delta;
  // Cap it at the max.
  horse_breeding = std::min(
      horse_breeding, max_horse_breeding( ss, tribe_type ) );
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
void retain_horses_from_destroyed_brave( SSConst const& ss,
                                         Tribe&         tribe ) {
  int const delta = 25;
  add_to_horse_breeding( ss, tribe.type, delta,
                         tribe.horse_breeding );
}

void retain_muskets_from_destroyed_brave( Tribe& tribe ) {
  ++tribe.muskets;
}

void gain_horses_from_winning_combat( Tribe& tribe ) {
  ++tribe.horse_herds;
}

} // namespace rn
