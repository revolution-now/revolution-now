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

// C++ standard library
#include <unordered_set>

namespace rn {

struct Player;
struct SSConst;
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

  // ------------------------------------------------------------
  // Tribes
  // ------------------------------------------------------------
  bool tribe_exists( e_tribe tribe ) const;

  // Tribe must exist or check fail.
  Tribe&       tribe_for( e_tribe tribe );
  Tribe const& tribe_for( e_tribe tribe ) const;

  Tribe& create_or_add_tribe( e_tribe tribe );

  // ------------------------------------------------------------
  // Dwellings
  // ------------------------------------------------------------
  DwellingId last_dwelling_id() const;

  std::unordered_map<DwellingId, DwellingState> const&
  dwellings_all() const;

  // If the tribe exists then this will yield the dwelling ids.
  base::maybe<std::unordered_set<DwellingId> const&>
  dwellings_for_tribe( e_tribe tribe ) const;

  Dwelling const& dwelling_for( DwellingId id ) const;
  Dwelling&       dwelling_for( DwellingId id );

  DwellingState const& state_for( DwellingId id ) const;
  DwellingState&       state_for( DwellingId id );

  DwellingOwnership const& ownership_for( DwellingId id ) const;
  DwellingOwnership&       ownership_for( DwellingId id );

  Coord        coord_for( DwellingId id ) const;
  Tribe const& tribe_for( DwellingId id ) const;
  Tribe&       tribe_for( DwellingId id );

  base::maybe<DwellingId> maybe_dwelling_from_coord(
      Coord const& c ) const;
  DwellingId dwelling_from_coord( Coord const& c ) const;

  bool dwelling_exists( DwellingId id ) const;

  // The id of this dwelling must be zero (i.e., you can't select
  // the ID); a new ID will be generated for this unit and re-
  // turned.
  DwellingId add_dwelling( e_tribe tribe, Coord location,
                           Dwelling&& Dwelling );

  // NOTE: this should not be called directly since it will not
  // do associated cleanup such as deleting native units that are
  // owned by this dwelling, land owned by the dwelling, or mis-
  // sionaries owned by the dwelling.
  void destroy_dwelling( DwellingId id );

  // NOTE: this should not be called by normal game code, since
  // it literally does none of the things required to delete the
  // tribe except delete the tribe object. Other things that must
  // be done first are: all free braves must be deleted, all land
  // owned by the dwellings of the tribe must be deleted, all
  // missionaries in all dwellings of the tribe must be deleted,
  // and all dwellings must be deleted, in that order. Then fi-
  // nally this can be called.
  void destroy_tribe_last_step( e_tribe tribe );

  // ------------------------------------------------------------
  // Owned Land
  // ------------------------------------------------------------
  // Use this for testing only; normal game code should be
  // reading this by way of the API in the native-owned module,
  // since there is subtelty involved in interpreting the re-
  // sults.
  std::unordered_map<Coord, DwellingId> const&
  testing_only_owned_land_without_minuit() const;

 private:
  // NOTE: Normal game logic should not be calling these methods
  // directly since they don't take into account Peter Minuit.
  // that whether the player has Peter Minuit, in which case
  // there is effectively no land ownership by the natives from
  // the perspective of that player.
  std::unordered_map<Coord, DwellingId>&
  owned_land_without_minuit();

  std::unordered_map<Coord, DwellingId> const&
  owned_land_without_minuit() const;

  friend base::maybe<DwellingId>
  is_land_native_owned_after_meeting_without_colonies(
      SSConst const& ss, Player const& player, Coord coord );

 public:
  void mark_land_owned( DwellingId dwelling_id, Coord where );

  void mark_land_unowned( Coord where );

  // NOTE: that these are expensive calls because they have to
  // iterate over all map squares to find the ones owned by the
  // dwellings/tribe in question. For that reason, if you need to
  // do this for multiple dwellings, it is good to batch them to-
  // gether into one call rather than calling this multiple
  // times.
  void mark_land_unowned_for_dwellings(
      std::unordered_set<DwellingId> const& dwelling_ids );
  void mark_land_unowned_for_tribe( e_tribe tribe );

 private:
  [[nodiscard]] DwellingId next_dwelling_id();

  base::valid_or<std::string> validate() const;
  void                        validate_or_die() const;

  // ----- Serializable state.
  wrapped::NativesState o_;

  // ----- Non-serializable (transient) state.
  std::unordered_map<Coord, DwellingId> dwelling_from_coord_;

  std::unordered_map<e_tribe, std::unordered_set<DwellingId>>
      dwellings_from_tribe_;
};

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::NativesState, owned_by_cpp ){};

} // namespace lua
