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
#include "aliases.hpp"
#include "colony-mgr.hpp"
#include "errors.hpp"
#include "fb.hpp"
#include "id.hpp"
#include "logging.hpp"
#include "lua.hpp"
#include "ranges.hpp"

// base
#include "base/function-ref.hpp"
#include "base/keyval.hpp"

// Flatbuffers
#include "fb/sg-colony_generated.h"

// C++ standard library
#include <unordered_map>

using namespace std;

namespace rn {

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
      UNXP_CHECK( !colony_from_coord.contains( where ),
                  "multiples colonies on tile {}.", where );
      colony_from_coord[where] = id;
      string name              = colony.name();
      UNXP_CHECK( !colony_from_name.contains( name ),
                  "multiples colonies have name {}.", name );
      colony_from_name[name] = id;
    }

    return xp_success_t{};
  }
  // Called after all modules are deserialized.
  SAVEGAME_VALIDATE() {
    for( auto const& [id, colony] : colonies )
      XP_OR_RETURN_( check_colony_invariants_safe( id ) );
    return xp_success_t{};
  }
};
SAVEGAME_IMPL( Colony );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
expect<ColonyId> cstate_create_colony( e_nation         nation,
                                       Coord const&     where,
                                       std::string_view name ) {
  if( SG().colony_from_coord.contains( where ) )
    return UNEXPECTED( "square {} already contains a colony.",
                       where );
  if( SG().colony_from_name.contains( string( name ) ) )
    return UNEXPECTED( "there is already a colony with name {}.",
                       name );

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

Vec<ColonyId> colonies_all( Opt<e_nation> n ) {
  Vec<ColonyId> res;
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

Opt<ColonyId> colony_from_coord( Coord const& coord ) {
  return base::lookup( SG().colony_from_coord, coord );
}

Opt<ColonyId> colony_from_name( std::string_view name ) {
  return base::lookup( SG().colony_from_name, string( name ) );
}

Vec<ColonyId> colonies_in_rect( Rect const& rect ) {
  return rg::to<Vec<ColonyId>>(
      rect | rv::transform( colony_from_coord ) | cat_opts );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( colony_from_id, Colony const&, ColonyId id ) {
  return colony_from_id( id );
}

} // namespace

} // namespace rn
