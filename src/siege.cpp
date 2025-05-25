/****************************************************************
**siege.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-25.
*
* Description: Handles the colony under-siege status.
*
*****************************************************************/
#include "siege.hpp"

// Revolution Now
#include "society.hpp"
#include "unit-classes.hpp"

// ss
#include "ss/colony.rds.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"
#include "unit-type.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::point;
using ::refl::enum_values;

bool can_fight_siege( e_unit_type const type ) {
  if( scout_type( type ).has_value() )
    // Even though scouts can engage in combat, the OG does not
    // consider them a military unit for various purposes, in-
    // cluding when identifying units that can counter a siege.
    return false;
  return is_military_unit( type );
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
bool is_colony_under_siege( SSConst const& ss,
                            Colony const& colony ) {
  int hostile_military_units  = 0;
  int friendly_military_units = 0;
  UNWRAP_CHECK_T( Player const& player,
                  ss.players.players[colony.player] );
  // We count the colony square as well as the surrounding
  // squares.
  for( auto const d : enum_values<e_cdirection> ) {
    point const moved = colony.location.moved( d );
    if( !ss.terrain.square_exists( moved ) ) continue;
    if( ss.terrain.square_at( moved ).surface ==
        e_surface::water )
      // Ships don't count for siege.
      continue;
    auto const society = society_on_square( ss, moved );
    if( !society.has_value() ) continue;
    SWITCH( *society ) {
      CASE( native ) { break; }
      CASE( european ) {
        auto const& generic_unit_ids =
            ss.units.from_coord( moved );
        for( GenericUnitId const id : generic_unit_ids ) {
          Unit const& unit = ss.units.euro_unit_for( id );
          if( !can_fight_siege( unit.type() ) ) continue;
          if( unit.player_type() == colony.player ) {
            ++friendly_military_units;
            continue;
          }
          // Foreign unit.
          e_euro_relationship const relationship =
              player.relationship_with[unit.player_type()];
          switch( relationship ) {
            case e_euro_relationship::not_met: {
              // The OG appears to count this case.
              ++hostile_military_units;
              break;
            }
            case e_euro_relationship::peace:
              break;
            case e_euro_relationship::war: {
              ++hostile_military_units;
              break;
            }
          }
        }
        break;
      }
    }
  }
  return hostile_military_units > friendly_military_units;
}

} // namespace rn
