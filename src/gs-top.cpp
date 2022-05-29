/****************************************************************
**gs-top.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-14.
*
* Description: Top-level struct representing the data that is
*              saved when a game is saved.
*
*****************************************************************/
#include "gs-top.hpp"

// Revolution Now
#include "gs-settings.hpp"
#include "lua.hpp"
#include "map-square.hpp"

// luapp
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

// ColoniesState & UnitsState
valid_or<string> validate_interaction(
    ColoniesState const& colonies, UnitsState const& units ) {
  for( auto const& [colony_id, colony] : colonies.all() ) {
    // All units owned by colony are colony's units.
    for( UnitId unit_id : units.from_colony( colony_id ) ) {
      REFL_VALIDATE(
          colony.units_jobs().contains( unit_id ),
          "unit {} owned by colony is not in colony {}.",
          debug_string( units.unit_for( unit_id ) ),
          colony.debug_string() );
    }

    // All colony's units are of same nation.
    for( auto const& p : colony.units_jobs() ) {
      auto unit_nation = units.unit_for( p.first ).nation();
      REFL_VALIDATE( colony.nation() == unit_nation,
                     "Colony {} has nation {} but contains a "
                     "unit that has nation {}.",
                     colony.id(), colony.nation(), unit_nation );
    }

    // All colony's units owned by colony.
    for( auto const& p : colony.units_jobs() ) {
      auto unit_id = p.first;
      REFL_VALIDATE(
          units.state_of( unit_id ).ownership.to_enum() ==
              UnitOwnership::e::colony,
          "{} in Colony {} is not owned by colony.",
          debug_string( units.unit_for( unit_id ) ),
          colony.debug_string() );
    }
  }
  return base::valid;
}

// ColoniesState & TerrainState
valid_or<string> validate_interaction(
    ColoniesState const& colonies,
    TerrainState const&  terrain ) {
  for( auto const& [colony_id, colony] : colonies.all() ) {
    // Colony is on land.
    REFL_VALIDATE(
        is_land( terrain.world_map()[colony.location()] ),
        "Colony {} is not on land.", colony.debug_string() );
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

valid_or<string> wrapped::TopLevelState::validate() const {
  HAS_VALUE_OR_RET( validate_interaction( colonies, units ) );
  HAS_VALUE_OR_RET(
      validate_interaction( colonies, zzz_terrain ) );
  return valid;
}

valid_or<string> TopLevelState::validate() const {
  // First validate reflected part.
  HAS_VALUE_OR_RET( o_.validate() );
  // Now validate transient state.
  // n/a.
  return valid;
}

void TopLevelState::validate_or_die() const {
  CHECK_HAS_VALUE( validate() );
}

TopLevelState::TopLevelState( wrapped::TopLevelState&& o )
  : o_( std::move( o ) ) {
  // Populate any transient fields.
  o_.land_view.viewport.set_max_viewable_size_tiles(
      o_.zzz_terrain.world_map().size() );
}

TopLevelState::TopLevelState()
  : TopLevelState( wrapped::TopLevelState{} ) {
  validate_or_die();
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// TopLevelState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::TopLevelState;
  auto u  = st.usertype.create<U>();

  // u["version"] = []( U& obj ) -> decltype( auto ) {
  //   return obj.version();
  // };

  u["settings"] = []( U& obj ) -> decltype( auto ) {
    return obj.settings();
  };

  // u["events"] = []( U& obj ) -> decltype( auto ) {
  //   return obj.events();
  // };

  // u["units"] = []( U& obj ) -> decltype( auto ) {
  //   return obj.units();
  // };

  // u["players"] = []( U& obj ) -> decltype( auto ) {
  //   return obj.players();
  // };

  // u["turn"] = []( U& obj ) -> decltype( auto ) {
  //   return obj.turn();
  // };

  // u["colonies"] = []( U& obj ) -> decltype( auto ) {
  //   return obj.colonies();
  // };

  // u["land_view"] = []( U& obj ) -> decltype( auto ) {
  //   return obj.land_view();
  // };

  u["terrain"] = []( U& obj ) -> decltype( auto ) {
    return obj.terrain();
  };
};

} // namespace

} // namespace rn
