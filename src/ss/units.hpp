/****************************************************************
**units.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-13.
*
* Description: Unit-related save-game state.
*
*****************************************************************/
#pragma once

// Rd
#include "ss/units.rds.hpp"

// Revolution Now
#include "ss/colony-id.hpp"
#include "ss/colony.hpp"
#include "ss/dwelling-id.hpp"
#include "ss/unit-id.hpp"

// gfx
#include "gfx/coord.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// C++ standard library
#include <unordered_map>
#include <unordered_set>

namespace rn {

struct Colony;
struct ColonyJob;
struct EuroUnitOwnershipChangeTo;
struct Player;
struct SS;

struct UnitsState {
  UnitsState();
  // We don't default this because we don't want to compare the
  // caches (at the time of writing some of them contain pointers
  // whose values shouldn't be compared from instance to in-
  // stance).
  bool operator==( UnitsState const& ) const;

  // Implement refl::WrapsReflected.
  UnitsState( wrapped::UnitsState&& o );
  wrapped::UnitsState const&        refl() const { return o_; }
  static constexpr std::string_view refl_ns   = "rn";
  static constexpr std::string_view refl_name = "UnitsState";

  // API. Here we mainly just have the functions that are re-
  // quired to access the state in this class and maintain in-
  // variants. Any more complicated game logic that gets layered
  // on top of these should go elsewhere.

  GenericUnitId last_unit_id() const;

  std::unordered_map<GenericUnitId, UnitState> const& all()
      const;
  std::unordered_map<UnitId, EuroUnitState const*> const&
  euro_all() const;
  std::unordered_map<NativeUnitId, NativeUnitState const*> const&
  native_all() const;

  // Is this a European or native unit.
  e_unit_kind unit_kind( GenericUnitId id ) const;

  // Verify that the id represents a unit of the given kind and
  // return that unit's casted ID.
  UnitId       check_euro_unit( GenericUnitId id ) const;
  NativeUnitId check_native_unit( GenericUnitId id ) const;

  // For these we allow non-const access to publicly. The unit
  // must exist.
  Unit const&       unit_for( UnitId id ) const;
  Unit&             unit_for( UnitId id );
  NativeUnit const& unit_for( NativeUnitId id ) const;
  NativeUnit&       unit_for( NativeUnitId id );
  // These will check if the unit is in fact of the requested
  // type and if so return it (otherwise check fail). This is for
  // convenience so that the caller doesn't have to convert IDs
  // when they know what type of unit it is.
  Unit const&       euro_unit_for( GenericUnitId id ) const;
  Unit&             euro_unit_for( GenericUnitId id );
  NativeUnit const& native_unit_for( GenericUnitId id ) const;
  NativeUnit&       native_unit_for( GenericUnitId id );

  // Unit must exist.
  UnitState const&           state_of( GenericUnitId id ) const;
  EuroUnitState const&       state_of( UnitId id ) const;
  NativeUnitState const&     state_of( NativeUnitId id ) const;
  UnitOwnership const&       ownership_of( UnitId id ) const;
  NativeUnitOwnership const& ownership_of(
      NativeUnitId id ) const;

  maybe<Coord> maybe_coord_for( UnitId id ) const;
  Coord        coord_for( UnitId id ) const;
  maybe<Coord> maybe_coord_for( NativeUnitId id ) const;
  Coord        coord_for( NativeUnitId id ) const;
  maybe<Coord> maybe_coord_for( GenericUnitId id ) const;
  Coord        coord_for( GenericUnitId id ) const;

  maybe<UnitId> maybe_holder_of( UnitId id ) const;
  UnitId        holder_of( UnitId id ) const;

  // Check-fail if the unit is a free unit.
  DwellingId dwelling_for( NativeUnitId id ) const;

  // If the unit is a missionary and it is inside a dwelling then
  // this will return the dwelling id.
  maybe<DwellingId> maybe_dwelling_for_missionary(
      UnitId id ) const;

  // We allow non-const access to the harbor view state because
  // changing it will not affect the invariants of this class.
  maybe<UnitHarborViewState&> maybe_harbor_view_state_of(
      UnitId id );
  maybe<UnitHarborViewState const&> maybe_harbor_view_state_of(
      UnitId id ) const;

  UnitHarborViewState& harbor_view_state_of( UnitId id );

  std::unordered_set<GenericUnitId> const& from_coord(
      Coord const& c ) const;

  // Note this returns only units that are working in the colony,
  // not units that are on the map at the location of the colony.
  std::unordered_set<UnitId> const& from_colony(
      Colony const& colony ) const;

  // If there is a mission in the dwelling then this will return
  // the unit id of the missionary that is inside. Note that this
  // missionary is not considered to be on the map; it is owned
  // by the dwelling.
  maybe<UnitId> missionary_from_dwelling(
      DwellingId dwelling_id ) const;

  // This will return the unit ID of the brave that is on the map
  // that is associated with this dwelling, if any. In the OG
  // there is at most one brave on the map per dwelling, but here
  // we return a set because 1) sometimes we create a second
  // (temporary) brave associated with the dwelling to act as the
  // visual target to an attack, and also for future flexibility.
  std::unordered_set<NativeUnitId> const& braves_for_dwelling(
      DwellingId dwelling_id ) const;

  // The id of this unit must be zero (i.e., you can't select the
  // ID); a new ID will be generated for this unit and returned.
  [[nodiscard]] UnitId       add_unit( Unit&& unit );
  [[nodiscard]] NativeUnitId add_unit( NativeUnit&& unit );

  // Should not be holding any references to the unit after this.
  void destroy_unit( NativeUnitId id );

 public:
  // This should probably only be used in unit tests. Returns
  // false if the unit currently exists; returns true if the unit
  // existed but has since been deleted; check-fails if the id
  // doesn't correspond to a unit that ever existed. Note that
  // this does not use the deleted-units cache, it uses the
  // next_unit_id, and so should be ok to call across saves. That
  // said, normal game code probably shouldn't need to call this.
  bool exists( GenericUnitId id ) const;
  bool exists( UnitId id ) const;
  bool exists( NativeUnitId id ) const;

 private:
  // ------------------------------------------------------------
  // State Changes.
  // ------------------------------------------------------------
  // These are all private and must be called via the friend
  // functions below in order to ensure that the proper cleanup
  // is done and invariants are maintained.

  friend void destroy_unit( SS& ss, UnitId id );

  friend void unit_ownership_change_non_interactive(
      SS& ss, UnitId id, EuroUnitOwnershipChangeTo const& info );

  friend struct UnitOnMapMover;
  friend class UnitHarborMover;

  friend void move_unit_to_colony( UnitsState&      units_state,
                                   Player const&    player,
                                   Colony&          colony,
                                   UnitId           unit_id,
                                   ColonyJob const& job );

  void destroy_unit( UnitId id );

  void change_to_map( UnitId id, Coord target );
  void change_to_map( NativeUnitId id, Coord target,
                      DwellingId dwelling_id );

  void change_to_colony( UnitId id, ColonyId col_id );

  // Will start at the starting slot and rotate right trying to
  // find a place where the unit can fit.
  void change_to_cargo_somewhere( UnitId new_holder, UnitId held,
                                  int starting_slot = 0 );

  // Not really used.
  void change_to_cargo( UnitId new_holder, UnitId held,
                        int slot );

  void change_to_harbor_view( UnitId              id,
                              UnitHarborViewState info );

  void change_to_dwelling( UnitId     unit_id,
                           DwellingId dwelling_id );

  // This will erase any ownership that is had over the given
  // unit and mark it as free. The unit must soon be assigned a
  // new owernership in order to uphold invariants. This function
  // should rarely be called.
  void disown_unit( UnitId id );
  void disown_unit( NativeUnitId id );

 private:
  [[nodiscard]] GenericUnitId next_unit_id();

  UnitState&           state_of( GenericUnitId id );
  EuroUnitState&       state_of( UnitId id );
  NativeUnitState&     state_of( NativeUnitId id );
  UnitOwnership&       ownership_of( UnitId id );
  NativeUnitOwnership& ownership_of( NativeUnitId id );

  valid_or<std::string> validate() const;
  void                  validate_or_die() const;

  // ----- Serializable state.
  wrapped::UnitsState o_;

  // ----- Non-serializable (transient) state.

  // Holds deleted units for debugging purposes (they will never
  // be resurrected and their IDs will never be reused). Holding
  // the IDs here is technically redundant, but this is on pur-
  // pose in the hope that it might catch a bug.
  std::unordered_set<GenericUnitId> deleted_;

  // For units that are on (owned by) the world (map).
  std::unordered_map<Coord, std::unordered_set<GenericUnitId>>
      units_from_coords_;

  // For units that are held in a colony.
  std::unordered_map<ColonyId, std::unordered_set<UnitId>>
      worker_units_from_colony_;

  // For units that are held in a dwelling as missionaries.
  std::unordered_map<DwellingId, UnitId> missionary_in_dwelling_;

  // For native units (braves) that are on the map but that are
  // also associated with a dwelling. We can map native unit ->
  // dwelling by way of the native unit state, and so this map-
  // ping allows us to go the other way efficiently. Note that
  // sometimes a dwelling may not have any braves (e.g. if one is
  // destroyed in battle and not yet regenerated); in that case
  // the dwelling will not have an entry here. Also, it may some-
  // times have more than one brave e.g. when we create a tempo-
  // rary brave to act as the target of an attack on a dwelling
  // for visual effect.
  std::unordered_map<DwellingId,
                     std::unordered_set<NativeUnitId>>
      braves_for_dwelling_;

  // All units of a given kind. The pointers will always be
  // non-null if an element exists in the map.
  std::unordered_map<UnitId, EuroUnitState const*> euro_units_;
  std::unordered_map<NativeUnitId, NativeUnitState const*>
      native_units_;
};

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::UnitsState, owned_by_cpp ){};
}
