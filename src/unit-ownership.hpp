/****************************************************************
**unit-ownership.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-26.
*
* Description: Handles transitions between different unit
*              ownership states for high level game logic.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "unit-deleted.hpp"
#include "wait.hpp"

// ss
#include "ss/dwelling-id.hpp"
#include "ss/unit-id.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct Colony;
struct ColonyJob;
struct Player;
struct PortStatus;
struct SS;
struct TS;
struct Unit;

/****************************************************************
** UnitOwnershipChanger
*****************************************************************/
// All normal game code should use these methods whenever a
// unit's ownership is changed directly. It will ensure that the
// state is transitioned in a way that does not break any invari-
// ants. That said, it won't always be the thing you want to call
// for upholding game rules; there are sometimes higher-level
// functions that you'll want to call that will do other things
// along with change the state.
//
// Where present, the interactive version should be used where
// possible/necessary.
//
// Note that, although there is a method in the API to make a
// unit free, it is never necessary to call that before calling
// another API method; it will be done automatically.
struct UnitOwnershipChanger {
  UnitOwnershipChanger( SS& ss, UnitId unit_id );

  // Free. This needs to be followed up with either some new own-
  // ership assignment or destruction.
  void change_to_free() const;

  // Destroy. No need to free the unit before doing this.
  void destroy() const;

  // Map.
  wait<maybe<UnitDeleted>> change_to_map( TS&   ts,
                                          Coord target ) const;

  void change_to_map_non_interactive( TS&   ts,
                                      Coord target ) const;

  // If the unit is on the map it will remove it from the map and
  // then re-place it on the map at the same location. This is
  // used by code that wants to run through the actions that are
  // normally performed when a unit is placed on a new map
  // square, even though they may not be moving.
  void reinstate_on_map_if_on_map( TS& ts ) const;

  // Cargo.
  void change_to_cargo( UnitId new_holder,
                        int    starting_slot ) const;

  // Colony.
  void change_to_colony( TS& ts, Colony& colony,
                         ColonyJob const& job ) const;

  // Any of the harbor-related status. Normal game code probably
  // won't want to use this method, since there are higher-level
  // methods in the harbor-units module that will determine what
  // the parameters of this method should be in a given situa-
  // tion, including looking at the previous state of the unit.
  void change_to_harbor( PortStatus const& port_status,
                         maybe<Coord>      sailed_from ) const;

  // Dwelling (for missionaries).
  void change_to_dwelling( DwellingId dwelling_id ) const;

 private:
  SS&          ss_;
  Unit&        unit_;
  Player&      player_;
  UnitId const unit_id_ = {};
};

/****************************************************************
** NativeOwnershipChanger
*****************************************************************/
struct NativeUnitOwnershipChanger {
  NativeUnitOwnershipChanger( SS& ss, NativeUnitId unit_id );

  void destroy() const;

 private:
  SS&                ss_;
  NativeUnitId const unit_id_ = {};
};

} // namespace rn
