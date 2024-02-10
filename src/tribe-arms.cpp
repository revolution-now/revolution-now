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

// Revolution Now
#include "irand.hpp"

// config
#include "config/natives.hpp"

// ss
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
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
  int const& delta =
      config_natives.arms.horses_per_mounted_brave;
  add_to_horse_breeding( ss, tribe.type, delta,
                         tribe.horse_breeding );
}

void retain_muskets_from_destroyed_brave( Tribe& tribe ) {
  ++tribe.muskets;
}

void gain_horses_from_winning_combat( Tribe& tribe ) {
  ++tribe.horse_herds;
}

void acquire_muskets_from_colony_raid( Tribe& tribe,
                                       int    quantity ) {
  if( quantity == 0 ) return;
  int const& delta = config_natives.arms.muskets_per_armed_brave;
  // This should also have been valided by the config validator.
  CHECK_GT( delta, 0 );
  tribe.muskets += clamp( quantity / delta, 1, 2 );
}

void acquire_horses_from_colony_raid( SSConst const& ss,
                                      Tribe&         tribe,
                                      int            quantity ) {
  if( quantity == 0 ) return;
  ++tribe.horse_herds;
  int const& delta =
      config_natives.arms.horses_per_mounted_brave;
  add_to_horse_breeding( ss, tribe.type, delta,
                         tribe.horse_breeding );
}

EquippedBrave select_brave_equip( SSConst const& ss, IRand& rand,
                                  Tribe const& tribe ) {
  bool const take_muskets = ( tribe.muskets > 0 );
  bool const take_horses =
      ( tribe.horse_breeding >=
        config_natives.arms.horses_per_mounted_brave );

  bool const depletes_muskets = [&] {
    return take_muskets &&
           rand.bernoulli(
               config_natives.arms
                   .musket_depletion[ss.settings.difficulty]
                   .probability );
  }();
  // In the OG this always happens. This is likely because a
  // tribe can't really lose horses once they have them, so
  // therefore there is no need to throttle the depletion. They
  // can't lose them because the source of horse breeding is the
  // horse_herds field, which can't go completely to zero unless
  // the tribe is completely destroyed.
  bool const depletes_horses = take_horses;

  return EquippedBrave{
      .type          = find_brave( take_muskets, take_horses ),
      .muskets_delta = depletes_muskets ? -1 : 0,
      .horse_breeding_delta =
          depletes_horses
              ? -config_natives.arms.horses_per_mounted_brave
              : 0,
  };
}

} // namespace rn
