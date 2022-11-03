/****************************************************************
**natives.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-30.
*
* Description: Top-level save-game state for native tribes and
*              dwellings.
*
*****************************************************************/
#pragma once

// Rds
#include "ss/natives.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct UnitsState;

struct NativesState {
  NativesState();
  bool operator==( NativesState const& ) const = default;

  // Implement refl::WrapsReflected.
  NativesState( wrapped::NativesState&& o );
  wrapped::NativesState const&      refl() const { return o_; }
  static constexpr std::string_view refl_ns   = "rn";
  static constexpr std::string_view refl_name = "NativesState";

  // API. Here we mainly just have the functions that are re-
  // quired to access the state in this class and maintain in-
  // variants. Any more complicated game logic that gets layered
  // on top of these should go elsewhere.

  DwellingId last_dwelling_id() const;

  std::unordered_map<DwellingId, Dwelling> const& all() const;

  std::vector<DwellingId> for_tribe( e_tribe tribe ) const;

  Dwelling const& dwelling_for( DwellingId id ) const;
  Dwelling&       dwelling_for( DwellingId id );

  Coord coord_for( DwellingId id ) const;

  base::maybe<DwellingId> maybe_from_coord(
      Coord const& c ) const;
  DwellingId from_coord( Coord const& c ) const;

  bool exists( DwellingId id ) const;

  // The id of this dwelling must be zero (i.e., you can't select
  // the ID); a new ID will be generated for this unit and re-
  // turned.
  DwellingId add_dwelling( Dwelling&& Dwelling );

  void destroy_dwelling( DwellingId id );

 private:
  [[nodiscard]] DwellingId next_dwelling_id();

  base::valid_or<std::string> validate() const;
  void                        validate_or_die() const;

  // ----- Serializable state.
  wrapped::NativesState o_;

  // ----- Non-serializable (transient) state.
  std::unordered_map<Coord, DwellingId> dwelling_from_coord_;
};

// FIXME: need to move this and merge it with nation_from_coord.
base::maybe<e_tribe> tribe_from_coord(
    UnitsState const& units, NativesState const& natives,
    Coord where );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::NativesState, owned_by_cpp ){};

} // namespace lua
