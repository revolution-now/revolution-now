/****************************************************************
**native-turn.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-25.
*
* Description: Implements the logic needed to run a native
*              tribe's turn independent of any particular AI
*              model.  It just enforces game rules.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "wait.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct Colony;
struct NativeUnit;
struct SS;
struct TS;

enum class e_tribe;

/****************************************************************
** INativesTurnDeps.
*****************************************************************/
// The dependencies representing complex actions are injected in
// order to make unit testing easier.
struct INativesTurnDeps {
  virtual ~INativesTurnDeps() = default;

  // Dependencies. NOTE: Parameters need to be mock friendly.

  virtual wait<> raid_unit( SS* ss, TS* ts, NativeUnit& attacker,
                            Coord dst ) const = 0;

  virtual wait<> raid_colony( SS* ss, TS* ts,
                              NativeUnit& attacker,
                              Colony&     colony ) const = 0;

  virtual void evolve_tribe_common(
      SS* ss, e_tribe tribe_type ) const = 0;

  virtual void evolve_dwellings_for_tribe(
      SS* ss, TS* ts, e_tribe tribe_type ) const = 0;
};

/****************************************************************
** Public API.
*****************************************************************/
wait<> natives_turn( SS& ss, TS& ts,
                     maybe<INativesTurnDeps const&> = {} );

} // namespace rn
