/****************************************************************
**alarm.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-07.
*
* Description: Things related to alarm level of tribes and
*              dwellings.
*
*****************************************************************/
#include "alarm.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/tribe.rds.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
int effective_dwelling_alarm( SSConst const&  ss,
                              Dwelling const& dwelling,
                              e_nation        nation ) {
  Tribe const& tribe = ss.natives.tribe_for( dwelling.tribe );
  if( !tribe.relationship[nation].has_value() ) return 0;
  int const tribal_alarm =
      tribe.relationship[nation]->tribal_alarm;
  CHECK_GE( tribal_alarm, 0 );
  CHECK_LT( tribal_alarm, 100 );
  int const dwelling_only_alarm =
      dwelling.relationship[nation].dwelling_only_alarm;
  CHECK_GE( dwelling_only_alarm, 0 );
  CHECK_LT( dwelling_only_alarm, 100 );
  // The alarm A is given by:
  //
  //   A = 1-(1-x)*(1-y) then rescaled to be [0,100).
  //
  // This formula guarantees that:
  //
  //   A >= x  and  A >= y
  //
  // so i.e. the effective alarm is always at least as large as
  // the individual dwelling-only alarm and tribal alarm.
  //
  int const effective_alarm = lround(
      ( 1.0 - ( 1.0 - tribal_alarm / 100.0 ) *
                  ( 1.0 - dwelling_only_alarm / 100.0 ) ) *
      100.0 );
  return clamp( effective_alarm, 0, 99 );
}

e_enter_dwelling_reaction reaction_for_dwelling(
    SSConst const& ss, Player const& player, Tribe const& tribe,
    Dwelling const& dwelling ) {
  if( !tribe.relationship[player.nation].has_value() )
    // Not yet made contact.
    return e_enter_dwelling_reaction::wave_happily;
  UNWRAP_CHECK( relationship,
                tribe.relationship[player.nation] );
  if( relationship.at_war )
    return e_enter_dwelling_reaction::scalps_and_war_drums;
  int const effective_alarm =
      effective_dwelling_alarm( ss, dwelling, player.nation );
  CHECK_GE( effective_alarm, 0 );
  CHECK_LT( effective_alarm, 100 );
  // The below assumes there are five elements in the enum; if
  // that ever changes then the calculation must be redone.
  static_assert( refl::enum_count<e_enter_dwelling_reaction> ==
                 5 );
  // 100/20 == 5
  int const category = effective_alarm / 20;
  return static_cast<e_enter_dwelling_reaction>( category );
}

} // namespace rn
