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

// luapp
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

} // namespace

/****************************************************************
** wrapped::NativesState
*****************************************************************/
base::valid_or<string> wrapped::NativesState::validate() const {
  unordered_set<Coord> used_coords;

  DwellingId max_id{ -1 };

  for( auto const& [id, dwelling] : dwellings ) {
    max_id = std::max( max_id, id );

    // Each dwelling has a unique location.
    Coord where = dwelling.location;
    REFL_VALIDATE( !used_coords.contains( where ),
                   "multiples dwellings on tile {}.", where );
    used_coords.insert( where );
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
  for( auto const& [dwelling_id, dwelling] : o_.dwellings ) {
    Coord const&            coord = dwelling.location;
    base::maybe<DwellingId> actual_dwelling_id =
        base::lookup( dwelling_from_coord_, coord );
    REFL_VALIDATE( actual_dwelling_id == dwelling_id,
                   "Inconsistent dwelling map coordinate ({}) "
                   "for dwelling {}.",
                   coord, dwelling_id );
  }

  return base::valid;
}

void NativesState::validate_or_die() const {
  CHECK_HAS_VALUE( validate() );
}

NativesState::NativesState( wrapped::NativesState&& o )
  : o_( std::move( o ) ) {
  // Populate dwelling_from_coord_.
  for( auto const& [id, dwelling] : o_.dwellings )
    dwelling_from_coord_[dwelling.location] = id;
}

NativesState::NativesState()
  : NativesState( wrapped::NativesState{
        .next_dwelling_id = kFirstDwellingId,
        .dwellings        = {} } ) {
  validate_or_die();
}

unordered_map<DwellingId, Dwelling> const& NativesState::all()
    const {
  return o_.dwellings;
}

Dwelling const& NativesState::dwelling_for(
    DwellingId id ) const {
  UNWRAP_CHECK_MSG( col, base::lookup( o_.dwellings, id ),
                    "dwelling {} does not exist.", id );
  return col;
}

Dwelling& NativesState::dwelling_for( DwellingId id ) {
  UNWRAP_CHECK_MSG( col, base::lookup( o_.dwellings, id ),
                    "dwelling {} does not exist.", id );
  return col;
}

Coord NativesState::coord_for( DwellingId id ) const {
  return dwelling_for( id ).location;
}

vector<DwellingId> NativesState::for_tribe(
    e_tribe tribe ) const {
  vector<DwellingId> res;
  res.reserve( o_.dwellings.size() );
  for( auto const& [id, dwelling] : o_.dwellings )
    if( dwelling.tribe == tribe ) res.push_back( id );
  return res;
}

DwellingId NativesState::add_dwelling( Dwelling&& dwelling ) {
  CHECK( dwelling.id == DwellingId{ 0 },
         "dwelling ID must be zero when creating dwelling." );
  DwellingId id = next_dwelling_id();
  dwelling.id   = id;
  CHECK( !dwelling_from_coord_.contains( dwelling.location ) );
  dwelling_from_coord_[dwelling.location] = id;
  // Must be last to avoid use-after-move.
  CHECK( !o_.dwellings.contains( id ) );
  o_.dwellings[id] = std::move( dwelling );
  return id;
}

void NativesState::destroy_dwelling( DwellingId id ) {
  Dwelling& dwelling = dwelling_for( id );
  CHECK( dwelling_from_coord_.contains( dwelling.location ) );
  dwelling_from_coord_.erase( dwelling.location );
  // Should be last so above reference doesn't dangle.
  o_.dwellings.erase( id );
}

DwellingId NativesState::next_dwelling_id() {
  return DwellingId{ o_.next_dwelling_id++ };
}

DwellingId NativesState::last_dwelling_id() const {
  CHECK( o_.next_dwelling_id > 0, "no dwellings yet created." );
  return DwellingId{ o_.next_dwelling_id - 1 };
}

base::maybe<DwellingId> NativesState::maybe_from_coord(
    Coord const& coord ) const {
  return base::lookup( dwelling_from_coord_, coord );
}

DwellingId NativesState::from_coord( Coord const& coord ) const {
  UNWRAP_CHECK( id, maybe_from_coord( coord ) );
  return id;
}

bool NativesState::exists( DwellingId id ) const {
  return o_.dwellings.contains( id );
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
  u["exists"]           = &U::exists;
  u["dwelling_for_id"]  = [&]( U&         o,
                              DwellingId id ) -> Dwelling& {
    LUA_CHECK( st, o.exists( id ), "dwelling {} does not exist.",
                id );
    return o.dwelling_for( id );
  };
};

} // namespace

} // namespace rn
