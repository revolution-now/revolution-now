/****************************************************************
**colonies.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-13.
*
* Description: Colony-related save-game state.
*
*****************************************************************/
#include "colonies.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <unordered_map>

using namespace std;

namespace rn {

/****************************************************************
** wrapped::ColoniesState
*****************************************************************/
base::valid_or<string> wrapped::ColoniesState::validate() const {
  unordered_set<Coord>  used_coords;
  unordered_set<string> used_names;

  auto const kFirstColonyId = rn::ColoniesState::kFirstColonyId;

  REFL_VALIDATE( next_colony_id >= kFirstColonyId,
                 "next_colony_id must be >= {}.",
                 kFirstColonyId );

  ColonyId max_id{ -1 };

  for( auto const& [id, colony] : colonies ) {
    max_id = std::max( max_id, id );

    // Each colony has a unique location.
    Coord where = colony.location;
    REFL_VALIDATE( !used_coords.contains( where ),
                   "multiples colonies on tile {}.", where );
    used_coords.insert( where );

    // Each colony has a unique name.
    string const& name = colony.name;
    REFL_VALIDATE( !used_names.contains( name ),
                   "multiples colonies have the name \"{}\".",
                   name );
    used_names.insert( name );
  }

  // next colony ID is larger than any used colony ID.
  REFL_VALIDATE( ColonyId{ next_colony_id } > max_id,
                 "next_colony_id ({}) must be larger than any "
                 "colony ID in use (max found is {}).",
                 next_colony_id, max_id );

  // Validate that there is no square where workers from multiple
  // colonies are working.
  unordered_map<Coord, string> have_workers;
  for( auto const& [id, colony] : colonies ) {
    Coord const loc = colony.location;
    for( auto const& [d, outdoor_unit] : colony.outdoor_jobs ) {
      if( !outdoor_unit.has_value() ) continue;
      Coord const worked = loc.moved( d );
      REFL_VALIDATE(
          !have_workers.contains( worked ),
          "square {} is being worked by colonists from both the "
          "{} colony and the {} colony.",
          worked, colony.name, have_workers[worked] );
      have_workers[worked] = colony.name;
    }
  }

  return base::valid;
}

/****************************************************************
** ColoniesState
*****************************************************************/
base::valid_or<std::string> ColoniesState::validate() const {
  HAS_VALUE_OR_RET( o_.validate() );

  for( auto const& [colony_id, colony] : o_.colonies ) {
    // Consistency of IDs.
    REFL_VALIDATE( colony.id == colony_id,
                   "Inconsistent colony IDs: {} != {}",
                   colony.id, colony_id );

    // Validity of IDs.
    REFL_VALIDATE( colony.id > 0,
                   "Real colony IDs must be positive numbers, "
                   "but found one with value {}.",
                   colony.id );

    // Colony location matches coord.
    Coord const&          coord = colony.location;
    base::maybe<ColonyId> actual_colony_id =
        base::lookup( colony_from_coord_, coord );
    REFL_VALIDATE(
        actual_colony_id == colony_id,
        "Inconsistent colony map coordinate ({}) for colony {}.",
        coord, colony_id );

    // Colonies are real.
    REFL_VALIDATE( !colony.frozen.has_value(),
                   "Real colonies must not have frozen info but "
                   "the colony with id={} does.",
                   colony_id );
  }

  return base::valid;
}

void ColoniesState::validate_or_die() const {
  CHECK_HAS_VALUE( validate() );
}

ColoniesState::ColoniesState( wrapped::ColoniesState&& o )
  : o_( std::move( o ) ) {
  // Populate colony_from_coord_.
  for( auto const& [id, colony] : o_.colonies )
    colony_from_coord_[colony.location] = id;
}

ColoniesState::ColoniesState()
  : ColoniesState( wrapped::ColoniesState{
        .next_colony_id = kFirstColonyId, .colonies = {} } ) {
  validate_or_die();
}

unordered_map<ColonyId, Colony> const& ColoniesState::all()
    const {
  return o_.colonies;
}

Colony const& ColoniesState::colony_for( ColonyId id ) const {
  UNWRAP_CHECK_MSG( col, base::lookup( o_.colonies, id ),
                    "colony {} does not exist.", id );
  CHECK( !col.frozen.has_value() );
  return col;
}

Colony& ColoniesState::colony_for( ColonyId id ) {
  UNWRAP_CHECK_MSG( col, base::lookup( o_.colonies, id ),
                    "colony {} does not exist.", id );
  CHECK( !col.frozen.has_value() );
  return col;
}

Coord ColoniesState::coord_for( ColonyId id ) const {
  return colony_for( id ).location;
}

vector<ColonyId> ColoniesState::for_nation(
    e_nation nation ) const {
  vector<ColonyId> res;
  res.reserve( o_.colonies.size() );
  for( auto const& [id, colony] : o_.colonies )
    if( colony.nation == nation ) res.push_back( id );
  return res;
}

ColonyId ColoniesState::add_colony( Colony&& colony ) {
  CHECK( colony.id == ColonyId{ 0 },
         "colony ID must be zero when creating colony." );
  ColonyId const id = next_colony_id();
  colony.id         = id;
  CHECK( !colony_from_coord_.contains( colony.location ) );
  colony_from_coord_[colony.location] = id;
  // Must be last to avoid use-after-move.
  CHECK( !o_.colonies.contains( id ) );
  o_.colonies[id] = std::move( colony );
  return id;
}

void ColoniesState::destroy_colony( ColonyId id ) {
  Colony& colony = colony_for( id );
  CHECK( colony_from_coord_.contains( colony.location ) );
  colony_from_coord_.erase( colony.location );
  // Should be last so above reference doesn't dangle.
  o_.colonies.erase( id );
}

ColonyId ColoniesState::next_colony_id() {
  return ColonyId{ o_.next_colony_id++ };
}

base::maybe<ColonyId> ColoniesState::last_colony_id() const {
  CHECK(
      o_.next_colony_id >= kFirstColonyId,
      "invalid value of next_colony_id: ", o_.next_colony_id );
  if( o_.next_colony_id == kFirstColonyId ) return base::nothing;
  return ColonyId{ o_.next_colony_id - 1 };
}

base::maybe<ColonyId> ColoniesState::maybe_from_coord(
    Coord const& coord ) const {
  return base::lookup( colony_from_coord_, coord );
}

ColonyId ColoniesState::from_coord( Coord const& coord ) const {
  UNWRAP_CHECK( id, maybe_from_coord( coord ) );
  return id;
}

base::maybe<ColonyId> ColoniesState::maybe_from_name(
    string_view name ) const {
  // NOTE: We used to have a hash map caching the names of the
  // colonies for faster lookup, but that caused issues because
  // it is possible for the user to directly change the name of a
  // colony (colony.name) when then makes it inconsistent with
  // what is in the cache.
  for( auto const& [colony_id, colony] : o_.colonies )
    if( colony.name == name ) return colony_id;
  return base::nothing;
}

bool ColoniesState::exists( ColonyId id ) const {
  return o_.colonies.contains( id );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// ColoniesState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::ColoniesState;
  auto u  = st.usertype.create<U>();

  u["last_colony_id"] = &U::last_colony_id;
  u["exists"]         = &U::exists;
  u["colony_for_id"]  = [&]( U& o, ColonyId id ) -> Colony& {
    LUA_CHECK( st, o.exists( id ), "colony {} does not exist.",
                id );
    return o.colony_for( id );
  };
};

} // namespace

} // namespace rn
