/****************************************************************
**natives.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-30.
*
* Description: Top-level save-game state for native tribes and
*              dwellings.
*
*****************************************************************/
#include "natives.hpp"

// ss
#include "ss/dwelling.hpp"
#include "ss/tribe.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

constexpr int kFirstDwellingId = 1;

using ::base::maybe;
using ::base::nothing;

} // namespace

/****************************************************************
** wrapped::NativesState
*****************************************************************/
base::valid_or<string> wrapped::NativesState::validate() const {
  unordered_set<Coord> used_coords;

  for( e_tribe tribe : refl::enum_values<e_tribe> ) {
    // Tribe type is consistent.
    if( !tribes[tribe].has_value() ) continue;
    REFL_VALIDATE( tribes[tribe]->type == tribe,
                   "tribe object for tribe {} has an "
                   "inconsistent tribe type ({}).",
                   tribe, tribes[tribe]->type );
  }

  DwellingId max_id{ -1 };

  for( auto const& [id, state] : dwellings ) {
    max_id = std::max( max_id, id );

    // Each dwelling has a unique location.
    Coord where = state.ownership.location;
    REFL_VALIDATE( !used_coords.contains( where ),
                   "multiples dwellings on tile {}.", where );
    used_coords.insert( where );

    // Dwelling's tribe exists.
    REFL_VALIDATE( tribes[state.ownership.tribe].has_value(),
                   "dwelling {} is part of tribe {} but that "
                   "tribe does not exist.",
                   id, state.ownership.tribe );
  }

  // next dwelling ID is larger than any used dwelling ID.
  REFL_VALIDATE( DwellingId{ next_dwelling_id } > max_id,
                 "next_dwelling_id ({}) must be larger than any "
                 "dwelling ID in use (max found is {}).",
                 next_dwelling_id, max_id );

  return base::valid;
}

/****************************************************************
** NativesState
*****************************************************************/
base::valid_or<std::string> NativesState::validate() const {
  HAS_VALUE_OR_RET( o_.validate() );

  // Dwelling location matches coord.
  for( auto const& [dwelling_id, state] : o_.dwellings ) {
    Coord const&            coord = state.ownership.location;
    base::maybe<DwellingId> actual_dwelling_id =
        base::lookup( dwelling_from_coord_, coord );
    REFL_VALIDATE( actual_dwelling_id == dwelling_id,
                   "Inconsistent dwelling map coordinate ({}) "
                   "for dwelling {}.",
                   coord, dwelling_id );
  }

  // Validate dwellings_from_tribe_.
  for( auto const& [tribe, dwellings] : dwellings_from_tribe_ ) {
    // tribe exists.
    REFL_VALIDATE( o_.tribes[tribe].has_value(),
                   "tribe {}  does not exist.", tribe );
  }

  return base::valid;
}

void NativesState::validate_or_die() const {
  CHECK_HAS_VALUE( validate() );
}

NativesState::NativesState( wrapped::NativesState&& o )
  : o_( std::move( o ) ) {
  // Populate dwelling_from_coord_.
  for( auto const& [id, state] : o_.dwellings )
    dwelling_from_coord_[state.ownership.location] = id;

  // Populate dwellings_from_tribe_;
  for( auto const& [id, state] : o_.dwellings ) {
    Dwelling const& dwelling = state.dwelling;
    e_tribe const   tribe    = state.ownership.tribe;
    CHECK(
        !dwellings_from_tribe_[tribe].contains( dwelling.id ) );
    dwellings_from_tribe_[tribe].insert( dwelling.id );
  }
}

NativesState::NativesState()
  : NativesState( wrapped::NativesState{
        .next_dwelling_id = kFirstDwellingId,
        .dwellings        = {} } ) {
  validate_or_die();
}

bool NativesState::tribe_exists( e_tribe tribe ) const {
  return o_.tribes[tribe].has_value();
}

Tribe& NativesState::tribe_for( e_tribe tribe ) {
  UNWRAP_CHECK_MSG( res, o_.tribes[tribe],
                    "the {} tribe does not exist in this game.",
                    tribe );
  return res;
}

Tribe const& NativesState::tribe_for( e_tribe tribe ) const {
  UNWRAP_CHECK_MSG( res, o_.tribes[tribe],
                    "the {} tribe does not exist in this game.",
                    tribe );
  return res;
}

Tribe& NativesState::create_or_add_tribe( e_tribe tribe ) {
  CHECK( dwellings_from_tribe_.contains( tribe ) ==
         o_.tribes[tribe].has_value() );
  if( o_.tribes[tribe].has_value() ) return *o_.tribes[tribe];
  Tribe& obj                   = o_.tribes[tribe].emplace();
  obj.type                     = tribe;
  dwellings_from_tribe_[tribe] = {};
  return obj;
}

unordered_map<DwellingId, DwellingState> const&
NativesState::dwellings_all() const {
  return o_.dwellings;
}

Dwelling const& NativesState::dwelling_for(
    DwellingId id ) const {
  return state_for( id ).dwelling;
}

Dwelling& NativesState::dwelling_for( DwellingId id ) {
  return state_for( id ).dwelling;
}

DwellingState const& NativesState::state_for(
    DwellingId id ) const {
  UNWRAP_CHECK_MSG( res, base::lookup( o_.dwellings, id ),
                    "dwelling {} does not exist.", id );
  return res;
}

DwellingState& NativesState::state_for( DwellingId id ) {
  UNWRAP_CHECK_MSG( res, base::lookup( o_.dwellings, id ),
                    "dwelling {} does not exist.", id );
  return res;
}

DwellingOwnership const& NativesState::ownership_for(
    DwellingId id ) const {
  return state_for( id ).ownership;
}

DwellingOwnership& NativesState::ownership_for( DwellingId id ) {
  return state_for( id ).ownership;
}

Coord NativesState::coord_for( DwellingId id ) const {
  return ownership_for( id ).location;
}

Tribe const& NativesState::tribe_for( DwellingId id ) const {
  return tribe_for( ownership_for( id ).tribe );
}

Tribe& NativesState::tribe_for( DwellingId id ) {
  return tribe_for( ownership_for( id ).tribe );
}

base::maybe<std::unordered_set<DwellingId> const&>
NativesState::dwellings_for_tribe( e_tribe tribe ) const {
  return base::lookup( dwellings_from_tribe_, tribe );
}

DwellingId NativesState::add_dwelling( e_tribe    tribe,
                                       Coord      location,
                                       Dwelling&& dwelling ) {
  CHECK( tribe_exists( tribe ), "tribe {} does not exist.",
         tribe );
  CHECK( dwelling.id == DwellingId{ 0 },
         "dwelling ID must be zero when creating dwelling." );
  DwellingId const id = next_dwelling_id();
  dwelling.id         = id;
  CHECK( !dwelling_from_coord_.contains( location ) );
  dwelling_from_coord_[location] = id;
  CHECK( dwellings_from_tribe_.contains( tribe ) );
  CHECK( !dwellings_from_tribe_[tribe].contains( id ) );
  dwellings_from_tribe_[tribe].insert( id );
  // Must be last to avoid use-after-move.
  CHECK( !o_.dwellings.contains( id ) );
  o_.dwellings[id] = {
      .dwelling  = std::move( dwelling ),
      .ownership = DwellingOwnership{ .location = location,
                                      .tribe    = tribe } };
  return id;
}

void NativesState::destroy_dwelling( DwellingId id ) {
  DwellingState&     state     = state_for( id );
  DwellingOwnership& ownership = state.ownership;
  CHECK( dwelling_from_coord_.contains( ownership.location ) );
  dwelling_from_coord_.erase( ownership.location );
  Dwelling&     dwelling = dwelling_for( id );
  e_tribe const tribe    = tribe_for( dwelling.id ).type;
  CHECK( tribe_exists( tribe ), "the {} tribe does not exist.",
         tribe );
  CHECK( dwellings_from_tribe_.contains( tribe ) );
  CHECK( dwellings_from_tribe_[tribe].contains( dwelling.id ) );
  // Note that even if this is the last dwelling in the tribe we
  // don't erase the tribe from the map, we just let it be empty.
  dwellings_from_tribe_[tribe].erase( dwelling.id );
  // Should be last so above reference doesn't dangle.
  o_.dwellings.erase( id );
}

void NativesState::destroy_tribe_last_step( e_tribe tribe ) {
  dwellings_from_tribe_.erase( tribe );
  o_.tribes[tribe].reset();
}

DwellingId NativesState::next_dwelling_id() {
  return DwellingId{ o_.next_dwelling_id++ };
}

DwellingId NativesState::last_dwelling_id() const {
  CHECK( o_.next_dwelling_id > 0, "no dwellings yet created." );
  return DwellingId{ o_.next_dwelling_id - 1 };
}

base::maybe<DwellingId> NativesState::maybe_dwelling_from_coord(
    Coord const& coord ) const {
  return base::lookup( dwelling_from_coord_, coord );
}

DwellingId NativesState::dwelling_from_coord(
    Coord const& coord ) const {
  UNWRAP_CHECK( id, maybe_dwelling_from_coord( coord ) );
  return id;
}

bool NativesState::dwelling_exists( DwellingId id ) const {
  return o_.dwellings.contains( id );
}

unordered_map<Coord, DwellingId>&
NativesState::owned_land_without_minuit() {
  return o_.owned_land_without_minuit;
}

unordered_map<Coord, DwellingId> const&
NativesState::owned_land_without_minuit() const {
  return o_.owned_land_without_minuit;
}

std::unordered_map<Coord, DwellingId> const&
NativesState::testing_only_owned_land_without_minuit() const {
  return o_.owned_land_without_minuit;
}

void NativesState::mark_land_owned( DwellingId dwelling_id,
                                    Coord      where ) {
  o_.owned_land_without_minuit[where] = dwelling_id;
}

void NativesState::mark_land_unowned( Coord where ) {
  auto it = o_.owned_land_without_minuit.find( where );
  if( it == o_.owned_land_without_minuit.end() ) return;
  o_.owned_land_without_minuit.erase( it );
}

void NativesState::mark_land_unowned_for_dwellings(
    unordered_set<DwellingId> const& dwelling_ids ) {
  erase_if( o_.owned_land_without_minuit,
            [&]( auto const& item ) {
              auto const& [coord, dwelling_id] = item;
              return dwelling_ids.contains( dwelling_id );
            } );
}

void NativesState::mark_land_unowned_for_tribe( e_tribe tribe ) {
  maybe<std::unordered_set<DwellingId> const&> dwelling_ids =
      dwellings_for_tribe( tribe );
  CHECK( dwelling_ids.has_value() );
  mark_land_unowned_for_dwellings( *dwelling_ids );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// NativesState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::NativesState;
  auto u  = st.usertype.create<U>();

  u["last_dwelling_id"] = &U::last_dwelling_id;
  u["dwelling_exists"]  = &U::dwelling_exists;
  u["dwelling_for_id"]  = [&]( U&         o,
                              DwellingId id ) -> Dwelling& {
    LUA_CHECK( st, o.dwelling_exists( id ),
                "dwelling {} does not exist.", id );
    return o.dwelling_for( id );
  };
  u["tribe_for_dwelling"] = []( U& o, DwellingId id ) {
    return o.ownership_for( id ).tribe;
  };
  u["coord_for_dwelling"] = []( U& o, DwellingId id ) {
    return o.ownership_for( id ).location;
  };
  u["has_dwelling_on_square"] =
      []( U& o, Coord where ) -> maybe<Dwelling&> {
    maybe<DwellingId> const dwelling_id =
        o.maybe_dwelling_from_coord( where );
    if( !dwelling_id.has_value() ) return nothing;
    return o.dwelling_for( *dwelling_id );
  };
  // FIXME: temporary; need to move this somewhere else which can
  // create the dwelling properly with all of the fields initial-
  // ized to the correct values.
  u["new_dwelling"] = [&]( U& o, e_tribe tribe,
                           Coord where ) -> Dwelling& {
    Dwelling dwelling;
    o.create_or_add_tribe( tribe );
    DwellingId const id =
        o.add_dwelling( tribe, where, std::move( dwelling ) );
    return o.dwelling_for( id );
  };
  u["create_or_add_tribe"] = &U::create_or_add_tribe;
  u["mark_land_owned"]     = &U::mark_land_owned;
};

} // namespace

} // namespace rn
