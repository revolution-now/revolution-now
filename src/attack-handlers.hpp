/****************************************************************
**attack-handlers.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-01.
*
* Description: Orders handlers for attacking.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// ss
#include "ss/dwelling-id.hpp"
#include "ss/unit-id.hpp"

namespace rn {

struct Colony;
struct OrdersHandler;
struct Player;
struct SS;
struct TS;

/****************************************************************
** Public API
*****************************************************************/
// This will handle the case when a european unit is attacking
// another european unit on land.
std::unique_ptr<OrdersHandler> attack_euro_land_handler(
    SS& ss, TS& ts, Player& player, UnitId attacker_id,
    UnitId defender_id );

// Handles when one ship attacks another at sea.
std::unique_ptr<OrdersHandler> naval_battle_handler(
    SS& ss, TS& ts, Player& player, UnitId attacker_id,
    UnitId defender_id );

// This handles the special case that a european unit is at-
// tacking a colony that does not have any military units at the
// gate (and so a capture is being attempted).
std::unique_ptr<OrdersHandler> attack_colony_undefended_handler(
    SS& ss, TS& ts, Player& player, UnitId attacker_id,
    UnitId defender_id, Colony& colony );

// When a european unit attacks a brave (on land).
std::unique_ptr<OrdersHandler> attack_native_unit_handler(
    SS& ss, TS& ts, Player& player, UnitId attacker_id,
    NativeUnitId defender_id );

// When a european unit attacks a dwelling (on land).
std::unique_ptr<OrdersHandler> attack_dwelling_handler(
    SS& ss, TS& ts, Player& player, UnitId attacker_id,
    DwellingId dwelling_id );

} // namespace rn
