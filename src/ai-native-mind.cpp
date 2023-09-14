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
#include "ss/colonies.hpp"
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

// TODO:
//   Recording this so that we don't forget, since it may not be
//   explicitly mentioned in the SG. When the natives are upset,
//   sometimes a brave can approach a wagon train and demand all
//   of a particular good within it. In the OG, there seems to
//   also be a bug where they will ask even if the wagon train is
//   empty.

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
  e_direction const rand_d = [&] {
    for( e_direction d : refl::enum_values<e_direction> ) {
      Coord const moved = world->coord.moved( d );
      if( !ss_.terrain.square_exists( moved ) ) continue;
      if( ss_.colonies.maybe_from_coord( moved ) ) return d;
    }
    for( e_direction d : refl::enum_values<e_direction> ) {
      Coord const moved = world->coord.moved( d );
      if( !ss_.terrain.square_exists( moved ) ) continue;
      maybe<Society> const society =
          society_on_square( ss_, moved );
      if( society.has_value() &&
          society->holds<Society::european>() )
        return d;
    }
    return pick_one<e_direction>( rand_ );
  }();
  Coord const moved = world->coord.moved( rand_d );
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
  }
}

void AiNativeMind::on_attack_colony_finished(
    CombatBraveAttackColony const&,
    BraveAttackColonyEffect const& ) {
  // TODO: adjust alarm.
}

} // namespace rn
