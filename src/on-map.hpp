/****************************************************************
**on-map.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-18.
*
* Description: Handles actions that need to be take in response
*              to a unit appearing on a map square (after
*              creation or moving).
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "unit-deleted.hpp"
#include "wait.hpp"

// ss
#include "ss/dwelling-id.hpp"
#include "ss/unit-id.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct EuroUnitOwnershipChangeTo;
struct SS;
struct TS;

// Some of the methods in this struct are private because they
// are not supposed to be called by normal game code. Instead,
// the top-level unit_ownership_change* family of methods should
// be used.
struct UnitOnMapMover {
  static void native_unit_to_map_non_interactive(
      SS& ss, NativeUnitId id, Coord dst_tile );

  static wait<> native_unit_to_map_interactive( SS& ss, TS& ts,
                                                NativeUnitId id,
                                                Coord dst_tile );

 private:
  static void to_map_non_interactive( SS& ss, TS& ts,
                                      UnitId unit_id,
                                      Coord  tile );

  static wait<maybe<UnitDeleted>> to_map_interactive(
      SS& ss, TS& ts, UnitId unit_id, Coord tile );

  // Friends.
  friend struct UnitOwnershipChanger;
  friend struct TestingOnlyUnitOnMapMover;
};

} // namespace rn
