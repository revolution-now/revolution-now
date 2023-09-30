/****************************************************************
**unit-ownership.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-26.
*
* Description: Handles transitions between different unit
*              ownership states for high level game logic.
*
*****************************************************************/
#include "unit-ownership.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "harbor-units.hpp"
#include "on-map.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/scope-exit.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

/****************************************************************
** UnitOwnershipChanger
*****************************************************************/
UnitOwnershipChanger::UnitOwnershipChanger( SS&    ss,
                                            UnitId unit_id )
  : ss_( ss ),
    unit_( ss.units.unit_for( unit_id ) ),
    player_( player_for_nation_or_die( ss_.players,
                                       unit_.nation() ) ),
    unit_id_( unit_id ) {}

void UnitOwnershipChanger::change_to_free() const {
  auto& ownership =
      as_const( ss_.units ).ownership_of( unit_id_ );
  // For most ownership types, the low-level ss/units module
  // takes care of associated cleanup since it is cleanup of
  // state held within that module. But for some there is some
  // higher level cleanup that has to be done first.
  SWITCH( ownership ) {
    CASE( free ) {
      // This is the one case where we can return, because we are
      // already at the goal state.
      return;
    }
    CASE( cargo ) { break; }
    CASE( dwelling ) { break; }
    CASE( harbor ) { break; }
    CASE( world ) { break; }
    CASE( colony ) {
      remove_unit_from_colony_obj_low_level(
          ss_, ss_.colonies.colony_for( colony.id ), unit_id_ );
      break;
    }
  }
  // Should be last.
  ss_.units.disown_unit( unit_id_ );
}

void UnitOwnershipChanger::destroy() const {
  change_to_free();
  // Recursively destroy any units in the cargo. We must get the
  // list of units to destroy first because we don't want to de-
  // stroy a cargo unit while iterating over the cargo.
  vector<UnitId> const cargo_units_to_destroy = [&, this] {
    vector<UnitId> res;
    for( auto const& cargo_unit :
         unit_.cargo().items_of_type<Cargo::unit>() )
      res.push_back( cargo_unit.id );
    return res;
  }();
  for( UnitId const to_destroy : cargo_units_to_destroy )
    UnitOwnershipChanger( ss_, to_destroy ).destroy();
  ss_.units.destroy_unit( unit_id_ );
}

wait<maybe<UnitDeleted>> UnitOwnershipChanger::change_to_map(
    TS& ts, Coord target ) const {
  change_to_free();
  co_return co_await UnitOnMapMover::to_map_interactive(
      ss_, ts, unit_id_, target );
}

void UnitOwnershipChanger::change_to_map_non_interactive(
    TS& ts, Coord target ) const {
  change_to_free();
  UnitOnMapMover::to_map_non_interactive( ss_, ts, unit_id_,
                                          target );
}

void UnitOwnershipChanger::reinstate_on_map_if_on_map(
    TS& ts ) const {
  auto& ownership =
      as_const( ss_.units ).ownership_of( unit_id_ );
  if( auto world = ownership.get_if<UnitOwnership::world>();
      world.has_value() )
    change_to_map_non_interactive( ts, world->coord );
}

void UnitOwnershipChanger::change_to_cargo(
    UnitId new_holder, int starting_slot ) const {
  change_to_free();
  CargoHold& cargo = ss_.units.unit_for( new_holder ).cargo();
  for( int i = starting_slot;
       i < starting_slot + cargo.slots_total(); ++i )
    if( int const modded = i % cargo.slots_total(); cargo.fits(
            ss_.units, Cargo::unit{ unit_id_ }, modded ) )
      return ss_.units.change_to_cargo( new_holder, unit_id_,
                                        modded );
  FATAL( "{} cannot be placed in {}'s cargo: {}", unit_id_,
         new_holder, cargo );
}

void UnitOwnershipChanger::change_to_colony(
    TS& ts, Colony& colony, ColonyJob const& job ) const {
  change_to_free();
  add_unit_to_colony_obj_low_level( ss_, ts, colony, unit_,
                                    job );
  // Low-level method.
  ss_.units.change_to_colony( unit_id_, colony.id );
}

void UnitOwnershipChanger::change_to_harbor(
    PortStatus const& port_status,
    maybe<Coord>      sailed_from ) const {
  change_to_free();
  ss_.units.change_to_harbor_view( unit_id_, port_status,
                                   sailed_from );
}

void UnitOwnershipChanger::change_to_dwelling(
    DwellingId dwelling_id ) const {
  change_to_free();
  ss_.units.change_to_dwelling( unit_id_, dwelling_id );
}

/****************************************************************
** NativeOwnershipChanger
*****************************************************************/
NativeUnitOwnershipChanger::NativeUnitOwnershipChanger(
    SS& ss, NativeUnitId unit_id )
  : ss_( ss ), unit_id_( unit_id ) {}

void NativeUnitOwnershipChanger::destroy() const {
  ss_.units.destroy_unit( unit_id_ );
}

} // namespace rn
