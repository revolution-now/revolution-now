/****************************************************************
**continental.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-21.
*
* Description: Handles promotion of units to continental status.
*
*****************************************************************/
#pragma once

// rds
#include "continental.rds.hpp"

// Revolution Now
#include "wait.hpp"

// ss
#include "ss/colony-id.hpp"

namespace rn {

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct SSConst;
struct SS;
struct TS;
struct IGui;
struct Player;

/****************************************************************
** Functions.
*****************************************************************/
ContinentalPromotion compute_continental_promotion(
    SSConst const& ss, Player const& player,
    ColonyId colony_id );

void do_continental_promotion(
    SS& ss, TS& ts, ContinentalPromotion const& promotion );

// It is the caller's respondibility to make sure that the colony
// is visible on the map.
wait<> continental_promotion_ui_seq(
    SSConst const& ss, IGui& gui,
    ContinentalPromotion const& promotion, ColonyId colony_id );

} // namespace rn
