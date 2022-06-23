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

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

constexpr int kFirstColonyId = 1;

} // namespace

/****************************************************************
** wrapped::ColoniesState
*****************************************************************/
valid_or<string> wrapped::ColoniesState::validate() const {
  unordered_set<Coord>  used_coords;
  unordered_set<string> used_names;

  ColonyId max_id{ -1 };

  for( auto const& [id, colony] : colonies ) {
    max_id = std::max( max_id, id );

    // Each colony has a unique location.
    Coord where = colony.location();
    REFL_VALIDATE( !used_coords.contains( where ),
                   "multiples colonies on tile {}.", where );
    used_coords.insert( where );

    // Each colony has a unique name.
    string const& name = colony.name();
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

  return base::valid;
}

/****************************************************************
** ColoniesState
*****************************************************************/
valid_or<std::string> ColoniesState::validate() const {
  HAS_VALUE_OR_RET( o_.validate() );

  // Colony location matches coord.
  for( auto const& [colony_id, colony] : o_.colonies ) {
    Coord const&    coord = colony.location();
    maybe<ColonyId> actual_colony_id =
        base::lookup( colony_from_coord_, coord );
    REFL_VALIDATE(
        actual_colony_id == colony_id,
        "Inconsistent colony map coordinate ({}) for colony {}.",
        coord, colony_id );
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
    colony_from_coord_[colony.location()] = id;

  // Populate colony_from_name_.
  for( auto const& [id, colony] : o_.colonies )
    colony_from_name_[colony.name()] = id;
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
  return col;
}

Colony& ColoniesState::colony_for( ColonyId id ) {
  UNWRAP_CHECK_MSG( col, base::lookup( o_.colonies, id ),
                    "colony {} does not exist.", id );
  return col;
}

Coord ColoniesState::coord_for( ColonyId id ) const {
  return colony_for( id ).location();
}

ColonyId ColoniesState::add_colony( Colony&& colony ) {
  CHECK( colony.id() == ColonyId{ 0 },
         "colony ID must be zero when creating colony." );
  ColonyId id  = next_colony_id();
  colony.o_.id = id;
  CHECK( !colony_from_coord_.contains( colony.location() ) );
  CHECK( !colony_from_name_.contains( colony.name() ) );
  colony_from_coord_[colony.location()] = id;
  colony_from_name_[colony.name()]      = id;
  // Must be last to avoid use-after-move.
  CHECK( !o_.colonies.contains( id ) );
  o_.colonies[id] = std::move( colony );
  return id;
}

void ColoniesState::destroy_colony( ColonyId id ) {
  Colony& colony = colony_for( id );
  CHECK( colony_from_coord_.contains( colony.location() ) );
  colony_from_coord_.erase( colony.location() );
  CHECK( colony_from_name_.contains( colony.name() ),
         "colony_from_name_ does not contain '{}'.",
         colony.name() );
  colony_from_name_.erase( colony.name() );
  // Should be last so above reference doesn't dangle.
  o_.colonies.erase( id );
}

ColonyId ColoniesState::next_colony_id() {
  return ColonyId{ o_.next_colony_id++ };
}

ColonyId ColoniesState::last_colony_id() const {
  CHECK( o_.next_colony_id > 0, "no colonies yet created." );
  return ColonyId{ o_.next_colony_id };
}

maybe<ColonyId> ColoniesState::maybe_from_coord(
    Coord const& coord ) const {
  return base::lookup( colony_from_coord_, coord );
}

ColonyId ColoniesState::from_coord( Coord const& coord ) const {
  UNWRAP_CHECK( id, maybe_from_coord( coord ) );
  return id;
}

maybe<ColonyId> ColoniesState::maybe_from_name(
    string_view name ) const {
  return base::lookup( colony_from_name_, string( name ) );
}

} // namespace rn
