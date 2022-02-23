/****************************************************************
**cstate.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-20.
*
* Description: Holds the Colony objects and tracks them.
*
*****************************************************************/
#include "cstate.hpp"

// Revolution Now
#include "colony-id.hpp"
#include "colony-mgr.hpp"
#include "error.hpp"
#include "game-state.hpp"
#include "gs-colonies.hpp"
#include "logger.hpp"
#include "lua.hpp"

// luapp
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/function-ref.hpp"
#include "base/keyval.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <unordered_map>

using namespace std;

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
ColonyId create_colony( e_nation nation, Coord const& where,
                        std::string_view name ) {
  ColoniesState& cols_state = GameState::colonies();
  CHECK( !cols_state.maybe_from_coord( where ).has_value(),
         "square {} already contains a colony.", where );
  CHECK( !cols_state.maybe_from_name( name ).has_value(),
         "there is already a colony with name {}.", name );

  wrapped::Colony refl_colony{
      .id           = ColonyId{ 0 },
      .nation       = nation,
      .name         = string( name ),
      .location     = where,
      .commodities  = {},
      .units        = {},
      .buildings    = {},
      .production   = nothing,
      .prod_hammers = 0,
      .prod_tools   = 0,
      .sentiment    = 0,
  };
  return cols_state.add_colony(
      Colony( std::move( refl_colony ) ) );
}

bool colony_exists( ColonyId id ) {
  ColoniesState const& cols_state = GameState::colonies();
  return cols_state.all().contains( id );
}

Colony& colony_from_id( ColonyId id ) {
  ColoniesState& cols_state = GameState::colonies();
  return cols_state.colony_for( id );
}

vector<ColonyId> colonies_all( e_nation n ) {
  ColoniesState&   cols_state = GameState::colonies();
  vector<ColonyId> res;
  for( auto const& [id, colony] : cols_state.all() ) {
    if( n == colony.nation() ) //
      res.push_back( id );
  }
  return res;
}

vector<ColonyId> colonies_all() {
  ColoniesState&   cols_state = GameState::colonies();
  vector<ColonyId> res;
  for( auto const& [id, colony] : cols_state.all() )
    res.push_back( id );
  return res;
}

// Apply a function to all colonies.
void map_colonies( base::function_ref<void( Colony& )> func ) {
  ColoniesState& cols_state = GameState::colonies();
  for( auto& [id, colony] : cols_state.all() )
    func( cols_state.colony_for( id ) );
}

maybe<ColonyId> colony_from_coord( Coord const& coord ) {
  return GameState::colonies().maybe_from_coord( coord );
}

maybe<ColonyId> colony_from_name( std::string_view name ) {
  return GameState::colonies().maybe_from_name( name );
}

vector<ColonyId> colonies_in_rect( Rect const& rect ) {
  return GameState::colonies().from_rect( rect );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( colony_from_id, Colony&, ColonyId id ) {
  lua::cthread L = lua_global_state().thread.main().cthread();
  if( !colony_exists( id ) )
    lua::throw_lua_error( L, "colony {} does not exist.", id );
  return colony_from_id( id );
}

// TODO: move this?
LUA_FN( last_colony_id, ColonyId ) {
  ColoniesState& cols_state = GameState::colonies();
  return cols_state.last_colony_id();
}

} // namespace

} // namespace rn
