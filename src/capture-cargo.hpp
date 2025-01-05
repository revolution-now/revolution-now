/****************************************************************
**capture-cargo.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-03.
*
* Description: Implements the game mechanic where a ship captures
*              the cargo of a foreign ship defeated in battle.
*
*****************************************************************/
#pragma once

// rds
#include "capture-cargo.rds.hpp"

// Revolution Now
#include "wait.hpp"

// ss
#include "ss/unit-id.hpp"

namespace rn {

struct IGui;
struct SSConst;
struct CargoHold;
struct Player;
struct Unit;

/****************************************************************
** Public API
*****************************************************************/
[[nodiscard]] CapturableCargo capturable_cargo_items(
    SSConst const& ss, CargoHold const& src,
    CargoHold const& dst );

// This should not be called directly by normal game code; in-
// stead the corresponding method on IEuroMind should be used,
// which will in turn call this method as implementation.
wait<CapturableCargoItems> select_items_to_capture_ui(
    SSConst const& ss, IGui& gui, UnitId src, UnitId dst,
    CapturableCargo const& capturable );

// This should not be called directly by normal game code; in-
// stead the corresponding method on IEuroMind should be used,
// which will in turn call this method as implementation.
wait<> notify_captured_cargo_human( IGui& gui,
                                    Player const& src_player,
                                    Player const& dst_player,
                                    Unit const& dst_unit,
                                    Commodity const& stolen );

void transfer_capturable_cargo_items(
    SSConst const& ss, CapturableCargoItems const& capturable,
    CargoHold& src, CargoHold& dst );

} // namespace rn
