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
#include "error.hpp"
#include "game-state.hpp"
#include "lua.hpp"

// game-state
#include "gs/colonies.hpp"

// luapp
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
bool colony_exists( ColonyId id ) {
  ColoniesState const& colonies_state = GameState::colonies();
  return colony_exists( colonies_state, id );
}

bool colony_exists( ColoniesState const& colonies_state,
                    ColonyId             id ) {
  return colonies_state.all().contains( id );
}

Colony& colony_from_id( ColonyId id ) {
  ColoniesState& cols_state = GameState::colonies();
  return cols_state.colony_for( id );
}

maybe<ColonyId> colony_from_coord( Coord coord ) {
  return GameState::colonies().maybe_from_coord( coord );
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
