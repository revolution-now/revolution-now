/****************************************************************
**root.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-14.
*
* Description: Top-level struct representing the data that is
*              saved when a game is saved.
*
*****************************************************************/
#include "root.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/land-view.hpp"
#include "ss/map.hpp"
#include "ss/natives.hpp"
#include "ss/players.hpp"
#include "ss/settings.hpp"
#include "ss/terrain.hpp"
#include "ss/turn.hpp"

// luapp
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

// ColoniesState & PlayersState
valid_or<string> validate_interaction(
    ColoniesState const& colonies,
    PlayersState const& players ) {
  // All colonies have a player that has a Player object.
  for( auto const& [colony_id, colony] : colonies.all() ) {
    REFL_VALIDATE( players.players[colony.player].has_value(),
                   "Colony {} has player {} but there is no "
                   "Player object for it.",
                   colony.name, colony.player );
  }
  return base::valid;
}

// UnitsState & PlayersState
valid_or<string> validate_interaction(
    UnitsState const& units, PlayersState const& players ) {
  // All European units have a player that has a Player object.
  for( auto const& [generic_id, state] : units.all() ) {
    if( units.unit_kind( generic_id ) != e_unit_kind::euro )
      continue;
    Unit const& unit = units.euro_unit_for( generic_id );
    REFL_VALIDATE(
        players.players[unit.player_type()].has_value(),
        "Unit {} has player {} but there is no "
        "Player object for it.",
        unit.id(), unit.player_type() );
  }
  return base::valid;
}

// ColoniesState & UnitsState
valid_or<string> validate_interaction(
    ColoniesState const& colonies, UnitsState const& units ) {
  for( auto const& [colony_id, colony] : colonies.all() ) {
    vector<UnitId> const colony_units =
        colony_units_all( colony );
    // All units owned by colony are colony's units.
    for( UnitId unit_id : units.from_colony( colony ) ) {
      REFL_VALIDATE(
          find( colony_units.begin(), colony_units.end(),
                unit_id ) != colony_units.end(),
          "unit {} owned by colony is not in colony {}.",
          debug_string( units.unit_for( unit_id ) ),
          colony.name );
    }

    // All colony's units are of same player.
    for( UnitId unit_id : colony_units ) {
      auto unit_player = units.unit_for( unit_id ).player_type();
      REFL_VALIDATE( colony.player == unit_player,
                     "Colony {} has player {} but contains a "
                     "unit that has player {}.",
                     colony.id, colony.player, unit_player );
    }

    // All colony's units owned by colony.
    for( UnitId unit_id : colony_units ) {
      REFL_VALIDATE(
          units.state_of( unit_id ).ownership.to_enum() ==
              UnitOwnership::e::colony,
          "{} in Colony {} is not owned by colony.",
          debug_string( units.unit_for( unit_id ) ),
          colony.name );
    }
  }
  return base::valid;
}

// ColoniesState & TerrainState
valid_or<string> validate_interaction(
    ColoniesState const& colonies,
    TerrainState const& terrain ) {
  for( auto const& [colony_id, colony] : colonies.all() ) {
    // Colony is on land.
    REFL_VALIDATE(
        terrain.world_map()[colony.location].surface ==
            e_surface::land,
        "Colony {} is not on land.", colony.name );
  }
  return base::valid;
}

// NativesState & TerrainState
valid_or<string> validate_interaction(
    NativesState const& natives_state,
    TerrainState const& terrain ) {
  for( auto const& [dwelling_id, state] :
       natives_state.dwellings_all() ) {
    // Dwelling is on land.
    REFL_VALIDATE(
        terrain.world_map()[state.ownership.location].surface ==
            e_surface::land,
        "Dwelling {} is not on land.", dwelling_id );
  }
  return base::valid;
}

// MapState & TerrainState
valid_or<string> validate_interaction(
    MapState const& map, TerrainState const& terrain_class ) {
  auto& terrain = terrain_class.refl();
  // Check that resource depletion counters are present iff the
  // tile has a (non-depleted) prime silver or minerals resource
  // on it.
  for( auto& [tile, counter] : map.depletion.counters ) {
    MapSquare const& square = terrain.real_terrain.map[tile];
    auto const resource =
        ( square.overlay == e_land_overlay::forest )
            ? square.forest_resource
            : square.ground_resource;
    REFL_VALIDATE(
        resource.has_value(),
        "tile {} has an entry in the depletion counters map but "
        "has no prime resource on it.",
        tile );
    REFL_VALIDATE(
        resource == e_natural_resource::minerals ||
            resource == e_natural_resource::silver,
        "tile {} has an entry in the depletion counters map but "
        "has no prime minerals resource and no (undepleted) "
        "prime silver resource.",
        tile );
    REFL_VALIDATE( counter >= 0,
                   "tile {} has a negative depletion counter.",
                   tile );
  }

  return base::valid;
}

} // namespace

valid_or<string> FormatVersion::validate() const {
  REFL_VALIDATE( major >= 0,
                 "major version number must be >= 0." );
  REFL_VALIDATE( minor >= 0,
                 "minor version number must be >= 0." );
  REFL_VALIDATE( patch >= 0,
                 "patch version number must be >= 0." );
  return valid;
}

valid_or<string> RootState::validate() const {
  HAS_VALUE_OR_RET( validate_interaction( colonies, units ) );
  HAS_VALUE_OR_RET( validate_interaction( colonies, players ) );
  HAS_VALUE_OR_RET( validate_interaction( units, players ) );
  HAS_VALUE_OR_RET(
      validate_interaction( colonies, zzz_terrain ) );
  HAS_VALUE_OR_RET(
      validate_interaction( natives, zzz_terrain ) );
  HAS_VALUE_OR_RET( validate_interaction( map, zzz_terrain ) );
  return valid;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// RootState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::RootState;
  auto u  = st.usertype.create<U>();

  // u["version"] = &U::version;
  u["settings"] = &U::settings;
  // u["events"] = &U::events;
  u["units"]     = &U::units;
  u["players"]   = &U::players;
  u["turn"]      = &U::turn;
  u["colonies"]  = &U::colonies;
  u["natives"]   = &U::natives;
  u["land_view"] = &U::land_view;
  u["map"]       = &U::map;
  u["terrain"]   = &U::zzz_terrain;
};

} // namespace

} // namespace rn
