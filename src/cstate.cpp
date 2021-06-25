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
#include "colony-mgr.hpp"
#include "error.hpp"
#include "fb.hpp"
#include "id.hpp"
#include "logging.hpp"
#include "lua.hpp"
#include "sg-macros.hpp"

// luapp
#include "luapp/state.hpp"

// base
#include "base/function-ref.hpp"
#include "base/keyval.hpp"

// Flatbuffers
#include "fb/sg-colony_generated.h"

// C++ standard library
#include <unordered_map>

using namespace std;

namespace rn {

DECLARE_SAVEGAME_SERIALIZERS( Colony );

namespace {

/****************************************************************
** Save-Game State
*****************************************************************/
struct SAVEGAME_STRUCT( Colony ) {
  using StorageMap_t = unordered_map<ColonyId, Colony>;

  // Fields that are actually serialized.
  // clang-format off
  SAVEGAME_MEMBERS( Colony,
  ( StorageMap_t, colonies  ));
  // clang-format on

public:
  // Fields that are derived from the serialized fields.

  unordered_map<Coord, ColonyId>  colony_from_coord;
  unordered_map<string, ColonyId> colony_from_name;

private:
  SAVEGAME_FRIENDS( Colony );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).

    // Populate colony_from_*.
    for( auto const& [id, colony] : colonies ) {
      Coord where = colony.location();
      VERIFY_DESERIAL(
          !colony_from_coord.contains( where ),
          fmt::format( "multiples colonies on tile {}.",
                       where ) );
      colony_from_coord[where] = id;
      string name              = colony.name();
      VERIFY_DESERIAL(
          !colony_from_name.contains( name ),
          fmt::format( "multiples colonies have name {}.",
                       name ) );
      colony_from_name[name] = id;
    }

    return valid;
  }
  // Called after all modules are deserialized.
  SAVEGAME_VALIDATE() {
    for( auto const& [id, colony] : colonies )
      HAS_VALUE_OR_RET( check_colony_invariants_safe( id ) );
    return valid;
  }
};
SAVEGAME_IMPL( Colony );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
expect<ColonyId, string> cstate_create_colony(
    e_nation nation, Coord const& where,
    std::string_view name ) {
  if( SG().colony_from_coord.contains( where ) )
    return fmt::format( "square {} already contains a colony.",
                        where );
  if( SG().colony_from_name.contains( string( name ) ) )
    return fmt::format(
        "there is already a colony with name {}.", name );

  auto col_id = next_colony_id();

  Colony colony;
  colony.id_           = col_id;
  colony.nation_       = nation;
  colony.name_         = string( name );
  colony.location_     = where;
  colony.production_   = nothing;
  colony.prod_hammers_ = 0;
  colony.prod_tools_   = 0;
  colony.sentiment_    = 0;

  SG().colony_from_coord[where]         = col_id;
  SG().colony_from_name[string( name )] = col_id;
  SG().colonies[col_id]                 = std::move( colony );
  return col_id;
}

bool colony_exists( ColonyId id ) {
  return SG().colonies.contains( id );
}

Colony& colony_from_id( ColonyId id ) {
  auto it = SG().colonies.find( id );
  CHECK( it != SG().colonies.end(), "colony {} does not exist.",
         id );
  return it->second;
}

vector<ColonyId> colonies_all( maybe<e_nation> n ) {
  vector<ColonyId> res;
  for( auto const& [id, colony] : SG().colonies ) {
    if( !n.has_value() || n == colony.nation() )
      res.push_back( id );
  }
  return res;
}

// Apply a function to all colonies.
void map_colonies( function_ref<void( Colony& )> func ) {
  for( auto& p : SG().colonies ) func( p.second );
}

// Should not be holding any references to the colony after this.
void cstate_destroy_colony( ColonyId id ) {
  Colony& colony = colony_from_id( id );
  CHECK( SG().colony_from_coord.contains( colony.location() ) );
  SG().colony_from_coord.erase( colony.location() );
  CHECK( SG().colony_from_name.contains( colony.name() ) );
  SG().colony_from_name.erase( colony.name() );
  // Should be last.
  SG().colonies.erase( id );
}

maybe<ColonyId> colony_from_coord( Coord const& coord ) {
  return base::lookup( SG().colony_from_coord, coord );
}

maybe<ColonyId> colony_from_name( std::string_view name ) {
  return base::lookup( SG().colony_from_name, string( name ) );
}

vector<ColonyId> colonies_in_rect( Rect const& rect ) {
  vector<ColonyId> res;
  for( auto coord : rect )
    if( auto colony = colony_from_coord( coord ); colony )
      res.push_back( *colony );
  return res;
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

} // namespace

} // namespace rn
