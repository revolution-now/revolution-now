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

  std::unordered_map<DwellingId, Dwelling> const& dwellings_all()
      const;

  std::vector<DwellingId> dwellings_for_tribe(
      e_tribe tribe ) const;

  Dwelling const& dwelling_for( DwellingId id ) const;
  Dwelling&       dwelling_for( DwellingId id );

  Coord coord_for( DwellingId id ) const;

  base::maybe<DwellingId> maybe_dwelling_from_coord(
      Coord const& c ) const;
  DwellingId dwelling_from_coord( Coord const& c ) const;

  bool dwelling_exists( DwellingId id ) const;

  // The id of this dwelling must be zero (i.e., you can't select
  // the ID); a new ID will be generated for this unit and re-
  // turned.
  DwellingId add_dwelling( Dwelling&& Dwelling );

  // NOTE: this should not be called directly since it will not
  // do associated cleanup such as deleting (or at least disown-
  // ing) native units that are owned by this dwelling.
  void destroy_dwelling( DwellingId id );

  // ------------------------------------------------------------
  // Owned Land
  // ------------------------------------------------------------

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
  is_land_native_owned_after_meeting( SSConst const& ss,
                                      Player const&  player,
                                      Coord          coord );

 public:
  void mark_land_owned( DwellingId dwelling_id, Coord where );

  void mark_land_unowned( Coord where );

 private:
  [[nodiscard]] DwellingId next_dwelling_id();

  base::valid_or<std::string> validate() const;
  void                        validate_or_die() const;

  // ----- Serializable state.
  wrapped::NativesState o_;

  // ----- Non-serializable (transient) state.
  std::unordered_map<Coord, DwellingId> dwelling_from_coord_;
};

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::NativesState, owned_by_cpp ){};

} // namespace lua
