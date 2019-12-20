/****************************************************************
**colony-state.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-20.
*
* Description: Holds the Colony objects and tracks them.
*
*****************************************************************/
#include "colony-state.hpp"

// Revolution Now
#include "aliases.hpp"
#include "errors.hpp"
#include "fb.hpp"
#include "id.hpp"
#include "logging.hpp"
#include "lua.hpp"

// Flatbuffers
#include "fb/sg-colony_generated.h"

// base-util
#include "base-util/keyval.hpp"

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

  unordered_map<Coord, ColonyId> colony_from_coord;

private:
  SAVEGAME_FRIENDS( Colony );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).

    // TODO

    return xp_success_t{};
  }
};
SAVEGAME_IMPL( Colony );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
expect<ColonyId> create_colony( e_nation         nation,
                                Coord const&     where,
                                std::string_view name ) {
  if( bu::has_key( SG().colony_from_coord, where ) )
    return UNEXPECTED( "square {} already contains a colony.",
                       where );

  Colony colony;
  colony.id_           = next_colony_id();
  colony.nation_       = nation;
  colony.name_         = string( name );
  colony.location_     = where;
  colony.production_   = nullopt;
  colony.prod_hammers_ = 0;
  colony.prod_tools_   = 0;
  colony.sentiment_    = 0;

  SG().colony_from_coord[where] = colony.id_;
  SG().colonies[colony.id_]     = std::move( colony );
  return colony.id_;
}

bool colony_exists( ColonyId id ) {
  return bu::has_key( SG().colonies, id ).has_value();
}

Colony& colony_from_id( ColonyId id ) {
  auto it = SG().colonies.find( id );
  CHECK( it != SG().colonies.end() );
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
void map_colonies( tl::function_ref<void( Colony& )> func ) {
  for( auto& p : SG().colonies ) func( p.second );
}

// Should not be holding any references to the colony after this.
void destroy_colony( ColonyId ) { NOT_IMPLEMENTED; }

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( colony_from_id, Colony const&, ColonyId id ) {
  return colony_from_id( id );
}

} // namespace

} // namespace rn
