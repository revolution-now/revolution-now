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
#include "errors.hpp"
#include "fb.hpp"
#include "id.hpp"
#include "logging.hpp"
#include "lua.hpp"
#include "ranges.hpp"

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
      UNXP_CHECK( !bu::has_key( colony_from_coord, where ),
                  "multiples colonies on tile {}.", where );
      colony_from_coord[where] = id;
      string name              = colony.name();
      UNXP_CHECK( !bu::has_key( colony_from_name, name ),
                  "multiples colonies have name {}.", name );
      colony_from_name[name] = id;
    }

    return xp_success_t{};
  }
  // Called after all modules are deserialized.
  SAVEGAME_VALIDATE() { return xp_success_t{}; }
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
  if( bu::has_key( SG().colony_from_name, string( name ) ) )
    return UNEXPECTED( "there is already a colony with name {}.",
                       name );

  auto col_id = next_colony_id();

  Colony colony;
  colony.id_           = col_id;
  colony.nation_       = nation;
  colony.name_         = string( name );
  colony.location_     = where;
  colony.production_   = nullopt;
  colony.prod_hammers_ = 0;
  colony.prod_tools_   = 0;
  colony.sentiment_    = 0;

  SG().colony_from_coord[where]         = col_id;
  SG().colony_from_name[string( name )] = col_id;
  SG().colonies[col_id]                 = std::move( colony );
  return col_id;
}

bool colony_exists( ColonyId id ) {
  return bu::has_key( SG().colonies, id ).has_value();
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
void map_colonies( tl::function_ref<void( Colony& )> func ) {
  for( auto& p : SG().colonies ) func( p.second );
}

// Should not be holding any references to the colony after this.
void destroy_colony( ColonyId id ) {
  Colony& colony = colony_from_id( id );
  CHECK(
      bu::has_key( SG().colony_from_coord, colony.location() ) );
  SG().colony_from_coord.erase( colony.location() );
  CHECK( bu::has_key( SG().colony_from_name, colony.name() ) );
  SG().colony_from_name.erase( colony.name() );
  // Should be last.
  SG().colonies.erase( id );
}

Opt<ColonyId> colony_from_coord( Coord const& coord ) {
  return bu::val_safe( SG().colony_from_coord, coord );
}

Opt<ColonyId> colony_from_name( std::string_view name ) {
  return bu::val_safe( SG().colony_from_name, string( name ) );
}

Vec<ColonyId> colonies_in_rect( Rect const& rect ) {
  return rect                                 //
         | rv::transform( colony_from_coord ) //
         | cat_opts;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( colony_from_id, Colony const&, ColonyId id ) {
  return colony_from_id( id );
}

// FIXME: temporary; this function should not be called directly
// by users since it does not fully initialize a colony into a
// valid state.
LUA_FN( create_colony, ColonyId, e_nation nation, Coord where,
        std::string const& name ) {
  ASSIGN_CHECK_XP( id, create_colony( nation, where, name ) );
  lg.info( "created a colony on {}.", where );
  return id;
}

} // namespace

} // namespace rn
