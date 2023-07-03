/****************************************************************
**ai-native-mind.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-25.
*
* Description: AI for natives.
*
*****************************************************************/
#include "ai-native-mind.hpp"

// Revolution Now
#include "irand.hpp"
#include "rand-enum.hpp"
#include "society.hpp"

// ss
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
AiNativeMind::AiNativeMind( SS& ss, IRand& rand )
  : ss_( ss ), rand_( rand ) {}

// Implement INativeMind.
NativeUnitCommand AiNativeMind::command_for(
    NativeUnitId native_unit_id ) {
  NativeUnitOwnership const& ownership =
      as_const( ss_.units ).ownership_of( native_unit_id );
  auto const& world =
      ownership.get_if<NativeUnitOwnership::world>();
  if( !world.has_value() ) return NativeUnitCommand::forfeight{};
  NativeUnit const& unit = ss_.units.unit_for( native_unit_id );
  Tribe const&      tribe =
      ss_.natives.tribe_for( world->dwelling_id );
  CHECK_GT( unit.movement_points, 0 );
  e_direction const rand_d = pick_one<e_direction>( rand_ );
  Coord const       moved  = world->coord.moved( rand_d );
  if( !ss_.terrain.square_exists( moved ) )
    return NativeUnitCommand::forfeight{};
  MapSquare const& square = ss_.terrain.square_at( moved );
  if( square.surface != e_surface::land )
    return NativeUnitCommand::forfeight{};
  maybe<Society> const society = society_on_square( ss_, moved );
  if( !society.has_value() )
    return NativeUnitCommand::travel{ .direction = rand_d };
  SWITCH( *society ) {
    CASE( native ) {
      if( native.tribe == tribe.type )
        return NativeUnitCommand::travel{ .direction = rand_d };
      return NativeUnitCommand::forfeight{};
    }
    CASE( european ) {
      return NativeUnitCommand::attack{ .direction = rand_d };
    }
    END_CASES;
  }
}

void AiNativeMind::on_attack_colony_finished(
    CombatBraveAttackColony const&,
    BraveAttackColonyEffect const& ) {}

} // namespace rn
