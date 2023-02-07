/****************************************************************
**missionary.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-05.
*
* Description: All things related to missionaries.
*
*****************************************************************/
#include "missionary.hpp"

// Revolution Now
#include "alarm.hpp"
#include "colony-buildings.hpp"
#include "colony-mgr.hpp"
#include "irand.hpp"
#include "unit.hpp"

// config
#include "config/missionary.rds.hpp"

// ss
#include "ss/colony.rds.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/unit-type.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
bool can_bless_missionaries( Colony const& colony ) {
  return building_for_slot( colony,
                            e_colony_building_slot::crosses )
      .has_value();
}

bool unit_can_be_blessed( UnitType type ) {
  return is_unit_human( type );
}

void bless_as_missionary( Player const& player, Colony& colony,
                          Unit& unit ) {
  strip_unit_to_base_type( player, unit, colony );
  UNWRAP_CHECK( ut, add_unit_type_modifiers(
                        unit.type_obj(),
                        { e_unit_type_modifier::blessing } ) );
  unit.change_type( player, ut );
}

bool is_missionary( e_unit_type type ) {
  return missionary_type( type ).has_value();
}

maybe<double> probability_dwelling_produces_convert_on_attack(
    SSConst const& ss, Player const& player_attacking,
    DwellingId dwelling_id ) {
  maybe<UnitId> const missionary_id =
      ss.units.missionary_from_dwelling( dwelling_id );
  if( !missionary_id.has_value() ) return nothing;
  Unit const& missionary_unit =
      ss.units.unit_for( *missionary_id );
  if( missionary_unit.nation() != player_attacking.nation )
    return nothing;
  UNWRAP_CHECK( missionary_type,
                missionary_type( missionary_unit.type_obj() ) );
  // TODO: need to account for father Juan de Sepulveda here.
  return config_missionary.type[missionary_type]
      .convert_on_attack.probability;
}

bool should_burn_mission_on_attack(
    IRand& rand, int tribal_alarm_after_attack ) {
  e_alarm_category const category =
      tribe_alarm_category( tribal_alarm_after_attack );
  if( category < e_alarm_category::angry ) return false;
  return rand.bernoulli(
      config_missionary.burn_mission.probability );
}

vector<UnitId> player_missionaries_in_tribe(
    SSConst const& ss, Player const& player,
    e_tribe tribe_type ) {
  UNWRAP_CHECK( dwellings,
                ss.natives.dwellings_for_tribe( tribe_type ) );
  vector<UnitId> res;
  res.reserve( dwellings.size() );
  for( DwellingId const dwelling_id : dwellings ) {
    maybe<UnitId> const missionary =
        ss.units.missionary_from_dwelling( dwelling_id );
    if( !missionary.has_value() ) continue;
    if( ss.units.unit_for( *missionary ).nation() !=
        player.nation )
      continue;
    res.push_back( *missionary );
  }
  // So that unit tests are deterministic. This should cause any
  // performace issues because this function is not called in
  // realtime.
  sort( res.begin(), res.end() );
  return res;
}

e_missionary_reaction tribe_reaction_to_missionary(
    Player const& player, Tribe const& tribe ) {
  TribeRelationship const& relationship =
      tribe.relationship[player.nation];
  int const alarm = relationship.tribal_alarm;
  if( alarm < 25 ) return e_missionary_reaction::curiosity;
  if( alarm < 50 ) return e_missionary_reaction::cautious;
  if( alarm < 75 ) return e_missionary_reaction::offended;
  return e_missionary_reaction::hostility;
}

} // namespace rn
