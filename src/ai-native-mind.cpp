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
#include "tribe-arms.hpp"

// config
#include "config/natives.rds.hpp"

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
AiNativeMind::AiNativeMind( SS& ss, IRand& rand,
                            e_tribe tribe_type )
  : INativeMind( tribe_type ), ss_( ss ), rand_( rand ) {}

wait<> AiNativeMind::message_box( std::string const& ) {
  return {};
}

NativeUnitId AiNativeMind::select_unit(
    set<NativeUnitId> const& units ) {
  CHECK( !units.empty() );
  return *units.begin();
}

// TODO:
//   Recording this so that we don't forget, since it may not be
//   explicitly mentioned in the SG. When the natives are upset,
//   sometimes a brave can approach a wagon train and demand all
//   of a particular good within it. In the OG, there seems to
//   also be a bug where they will ask even if the wagon train is
//   empty. If the good is horses/muskets then probably we should
//   treat this like if they had demanded it from a colony.

// Implement INativeMind.
NativeUnitCommand AiNativeMind::command_for(
    NativeUnitId native_unit_id ) {
  NativeUnitOwnership const& ownership =
      as_const( ss_.units ).ownership_of( native_unit_id );
  NativeUnit const& unit = ss_.units.unit_for( native_unit_id );
  Tribe const&      tribe =
      ss_.natives.tribe_for( ownership.dwelling_id );
  CHECK_GT( unit.movement_points, 0 );

  // If the brave is over its dwelling and the tribe has some
  // muskets/horses then allow the brave to be equipped.
  if( auto const dwelling_id =
          ss_.natives.maybe_dwelling_from_coord(
              ownership.coord );
      dwelling_id.has_value() ) {
    e_tribe const tribe_of_dwelling =
        ss_.natives.tribe_type_for( *dwelling_id );
    // TODO: the brave should only be over it's own dwelling, but
    // we're not ready to enforce that yet with a check fail, so
    // just check if it is the case.
    // CHECK( *dwelling_id == ownership.dwelling_id );
    //   or at least:
    // CHECK( tribe_of_dwelling == tribe.type );
    if( tribe_of_dwelling == tribe.type ) {
      EquippedBrave const equipped =
          select_brave_equip( ss_, rand_, as_const( tribe ) );
      if( equipped.type != unit.type ) {
        // In the OG it appears that, depending on the tech level
        // of the tribe, there is a small probability that a
        // brave will not be equipped when it otherwise could be
        // (lower tech levels have a higher probability of not
        // equipping).
        bool const delay_equipping = rand_.bernoulli(
            config_natives.arms
                .delay_equipping
                    [config_natives.tribes[tribe.type].level]
                .probability );
        if( !delay_equipping )
          return NativeUnitCommand::equip{ .how = equipped };
      }
    }
  }
  e_direction const rand_d = [&] {
    for( e_direction d : refl::enum_values<e_direction> ) {
      Coord const moved = ownership.coord.moved( d );
      if( !ss_.terrain.square_exists( moved ) ) continue;
      if( ss_.colonies.maybe_from_coord( moved ) ) return d;
    }
    for( e_direction d : refl::enum_values<e_direction> ) {
      Coord const moved = ownership.coord.moved( d );
      if( !ss_.terrain.square_exists( moved ) ) continue;
      maybe<Society> const society =
          society_on_square( ss_, moved );
      if( society.has_value() &&
          society->holds<Society::european>() )
        return d;
    }
    return pick_one<e_direction>( rand_ );
  }();
  Coord const moved = ownership.coord.moved( rand_d );
  if( !ss_.terrain.square_exists( moved ) )
    return NativeUnitCommand::forfeight{};
  MapSquare const& square = ss_.terrain.square_at( moved );
  if( square.surface != e_surface::land )
    return NativeUnitCommand::forfeight{};
  if( maybe<Society> const society =
          society_on_square( ss_, moved );
      society.has_value() ) {
    SWITCH( *society ) {
      CASE( native ) {
        if( native.tribe != tribe.type )
          return NativeUnitCommand::forfeight{};
        break;
      }
      CASE( european ) { break; }
    }
  }
  return NativeUnitCommand::move{ .direction = rand_d };
}

void AiNativeMind::on_attack_colony_finished(
    CombatBraveAttackColony const&,
    BraveAttackColonyEffect const& ) {
  // TODO: adjust alarm.
}

void AiNativeMind::on_attack_unit_finished(
    CombatBraveAttackEuro const& ) {
}

} // namespace rn
