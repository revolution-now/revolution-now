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
#include "unit-mgr.hpp"

// config
#include "config/natives.hpp"

// ss
#include "ss/native-enums.rds.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/tribe.rds.hpp"
#include "ss/units.hpp"

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

EquippedBrave select_brave_equip_impl( SSConst const& ss,
                                       IRand&         rand,
                                       Tribe const&   tribe,
                                       bool has_muskets,
                                       bool has_horses ) {
  bool const take_muskets =
      !has_muskets && ( tribe.muskets > 0 );
  bool const take_horses =
      !has_horses &&
      ( tribe.horse_breeding >=
        config_natives.arms.internal_horses_per_mounted_brave );

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
      .type          = find_brave( has_muskets || take_muskets,
                                   has_horses || take_horses ),
      .muskets_delta = depletes_muskets ? -1 : 0,
      .horse_breeding_delta =
          depletes_horses
              ? -config_natives.arms
                     .internal_horses_per_mounted_brave
              : 0,
  };
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
void retain_horses_from_destroyed_brave( SSConst const& ss,
                                         Tribe&         tribe ) {
  int const& delta =
      config_natives.arms.internal_horses_per_mounted_brave;
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
  int const& delta =
      config_natives.arms.internal_muskets_per_armed_brave;
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
      config_natives.arms.internal_horses_per_mounted_brave;
  add_to_horse_breeding( ss, tribe.type, delta,
                         tribe.horse_breeding );
}

EquippedBrave select_new_brave_equip( SSConst const& ss,
                                      IRand&         rand,
                                      Tribe const&   tribe ) {
  return select_brave_equip_impl( ss, rand, tribe,
                                  /*has_muskets=*/false,
                                  /*has_horses=*/false );
}

EquippedBrave select_existing_brave_equip(
    SSConst const& ss, IRand& rand, Tribe const& tribe,
    e_native_unit_type type ) {
  bool const has_muskets =
      config_natives.arms
          .equipment[type][e_brave_equipment::muskets];
  bool const has_horses =
      config_natives.arms
          .equipment[type][e_brave_equipment::horses];
  return select_brave_equip_impl( ss, rand, tribe, has_muskets,
                                  has_horses );
}

void evolve_tribe_horse_breeding( SSConst const& ss,
                                  Tribe&         tribe ) {
  int const delta = tribe.horse_herds;
  // Not strictly needed, but just to save time so that we don't
  // need to compute max horse breeding.
  if( delta == 0 ) return;
  add_to_horse_breeding( ss, tribe.type, delta,
                         tribe.horse_breeding );
}

void adjust_arms_on_dwelling_destruction( SSConst const& ss,
                                          Tribe& tribe ) {
  int const num_dwellings =
      ss.natives.dwellings_for_tribe( tribe.type ).size();
  CHECK_GT( num_dwellings, 0 );
  // Note we call this method before the dwelling is destroyed.
  bool const destroying_last_dwelling = ( num_dwellings == 1 );

  auto scale_down = [&]( int& n ) {
    if( n == 0 ) return;
    n = n - n / num_dwellings;
    if( !destroying_last_dwelling ) { CHECK_GE( n, 1 ); }
  };

  // Horses.
  scale_down( tribe.horse_herds );
  scale_down( tribe.horse_breeding );

  // Muskets. In the OG muskets do not seem to be lost when a
  // dwelling is destroyed (even the last one when the tribe is
  // made extinct). Not sure if this is a bug or not. Here we do
  // reproduce it because it is not obvious that it's a bug, and
  // it would have an effect on gameplay if it was decreased.
  // That said, for the sake of tidyness, we'll zero it out if
  // we're destroying the last dwelling, since that has no effect
  // on gameplay.
  if( destroying_last_dwelling ) tribe.muskets = 0;
}

ArmsReportForIndianAdvisorReport tribe_arms_for_advisor_report(
    SSConst const& ss, Tribe const& tribe ) {
  auto count_units_of_type = [&]( e_native_unit_type type ) {
    int count = 0;
    for( auto& [id, p_state] : ss.units.native_all() ) {
      if( tribe_for_unit( ss, p_state->unit ).type !=
          tribe.type )
        continue;
      if( p_state->unit.type != type ) continue;
      ++count;
    }
    return count;
  };

  int const armed_brave_count =
      count_units_of_type( e_native_unit_type::armed_brave );
  int const mounted_brave_count =
      count_units_of_type( e_native_unit_type::mounted_brave );
  int const mounted_warrior_count =
      count_units_of_type( e_native_unit_type::mounted_warrior );

  int const horse_breeding_count =
      tribe.horse_breeding /
      config_natives.arms.internal_horses_per_mounted_brave;

  int const musket_units = tribe.muskets         //
                           + armed_brave_count   //
                           + mounted_warrior_count;
  int const horse_units = tribe.horse_herds      //
                          + horse_breeding_count //
                          + mounted_brave_count  //
                          + mounted_warrior_count;

  return {
      .muskets =
          config_natives.arms.display_muskets_per_armed_brave *
          musket_units,
      .horses =
          config_natives.arms.display_horses_per_mounted_brave *
          horse_units };
}

} // namespace rn
