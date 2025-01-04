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

using ::refl::enum_count;

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
// TODO:
//   When a brave is next to a colony and makes some kind of in-
//   teraction, it seems to be randomly decided what it will be,
//   including whether it is good or bad (e.g. could be a gift or
//   could be demanding reparations). It is probably chosen ran-
//   domly on the fly according to the sentiments of the natives.
// TODO:
//   When a tribe has muskets and/or horses it will summon free
//   braves (who are lacking thoes things) to their dwelling to
//   pick some up. We need to figure out the mechanics of this.

// Implement INativeMind.
NativeUnitCommand AiNativeMind::command_for(
    NativeUnitId native_unit_id ) {
  NativeUnitOwnership const& ownership =
      as_const( ss_.units ).ownership_of( native_unit_id );
  NativeUnit const& unit = ss_.units.unit_for( native_unit_id );
  Tribe const& tribe =
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
      EquippedBrave const equipped = select_existing_brave_equip(
          ss_, rand_, as_const( tribe ), unit.type );
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

  // TODO: in the OG braves never seem to end up adjacent to
  // braves (or dwellings) of another tribe unless they are
  // forced to by the positioning of other units or LCRs around
  // them. The ai movement weights in the debug info flags seem
  // to allow them to move adjacent, so maybe there must be some
  // exclusion mechanism beyond that. It is not statistical --
  // this has been observed for hundreds of turns. The times when
  // it happens seem to be when all of the squares that the unit
  // could move to would be either invalid or adjacent to another
  // tribe and so it has no choice.
  //
  // Some experiments have shown that a brave will not move onto
  // a tile that is labeled as the land of another tribe, even if
  // there are no other braves on the map. We need to better un-
  // derstand how the game initially assigns those tiles (it may
  // not; it may just let them get populated as the braves move).
  // So a brave probably factors in both distance to its dwelling
  // and the land ownership.
  //
  // The reason that this matters is because when one brave moves
  // adjacent to a brave of another tribe, they immediately trade
  // horses, which can have a significant impact on the game.
  // Note that the code that initiates the inter-tribe trade when
  // a brave moves next to another tribe is handled not in this
  // AI module but in the code that moves the brave to a new
  // tile, since it is a deterministic part of the game that
  // doesn't need any AI.
  vector<e_direction> available_d;
  available_d.reserve( enum_count<e_direction> );
  for( e_direction const d : refl::enum_values<e_direction> ) {
    Coord const moved = ownership.coord.moved( d );
    if( !ss_.terrain.square_exists( moved ) ) continue;
    MapSquare const& square = ss_.terrain.square_at( moved );
    if( square.surface != e_surface::land ) continue;
    if( maybe<Society> const society =
            society_on_square( ss_, moved );
        society.has_value() ) {
      SWITCH( *society ) {
        CASE( native ) {
          if( native.tribe != tribe.type ) continue;
          break;
        }
        CASE( european ) { break; }
      }
    }
    available_d.push_back( d );
  }
  if( available_d.empty() )
    return NativeUnitCommand::forfeight{};

  e_direction const rand_d = rand_.pick_one( available_d );
  Coord const moved        = ownership.coord.moved( rand_d );
  if( maybe<Society> const society =
          society_on_square( ss_, moved );
      society.has_value() ) {
    SWITCH( *society ) {
      CASE( native ) { break; }
      CASE( european ) {
        if( rand_.bernoulli( .2 ) )
          // TODO: temporary.
          return NativeUnitCommand::talk{ .direction = rand_d };
        break;
      }
    }
  }
  return NativeUnitCommand::move{ .direction = rand_d };
}

void AiNativeMind::on_attack_colony_finished(
    CombatBraveAttackColony const&,
    BraveAttackColonyEffect const& ) {
  // TODO: adjust alarm.
  //
  // NOTE: Some experiments have indicated that when the colony
  // is burned tribal alarm goes down by half.
}

void AiNativeMind::on_attack_unit_finished(
    CombatBraveAttackEuro const& ) {
  // TODO: adjust alarm.
}

} // namespace rn
