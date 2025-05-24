/****************************************************************
**intervention.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-11.
*
* Description: Implements the intervention force.
*
*****************************************************************/
#pragma once

// rds
#include "intervention.rds.hpp"

// Revolution Now
#include "wait.hpp"

// ss
#include "ss/unit-id.hpp"

// base
#include "base/maybe.hpp"

namespace rn {

struct Colony;
struct IGui;
struct IRand;
struct Player;
struct SettingsState;
struct SS;
struct SSConst;
struct TerrainConnectivity;
struct TS;

enum class e_nation;

/****************************************************************
** Intervention Force.
*****************************************************************/
int bells_required_for_intervention(
    SettingsState const& settings );

wait<> intervention_forces_intro_ui_seq( SSConst const& ss,
                                         IGui& gui,
                                         e_nation receiving,
                                         e_nation intervening );

e_nation select_nation_for_intervention( e_nation for_nation );

bool should_trigger_intervention( SSConst const& ss,
                                  Player const& for_player );

void trigger_intervention( Player& player );

// Called once when the necessary bells have been accumulated for
// dispatching the intervention force.
wait<> intervention_forces_triggered_ui_seq(
    SSConst const& ss, IGui& gui, e_nation receiving,
    e_nation intervening );

// This will return nothing if there is no ship available, other-
// wise will return a value, which could have all zeroes, in
// which case a ship just comes empty.
maybe<InterventionLandUnits> pick_forces_to_deploy(
    Player const& player );

// May fail if there are no port colonies, or the port colonies
// have no adjacent ocean tiles with sealane access that are not
// blocked by REF units.
maybe<InterventionDeployTile> find_intervention_deploy_tile(
    SSConst const& ss, IRand& rand,
    TerrainConnectivity const& connectivity,
    Player const& player );

// This just creates the units, but does not move them. Returns
// the ID of the ship. It will also deduct the units from the
// player's intervention force, which must have sufficient num-
// bers to support the units passed in, otherwise check fail.
UnitId deploy_intervention_forces(
    SS& ss, TS& ts, InterventionDeployTile const& location,
    InterventionLandUnits const& forces );

// Called on each turn where some intervention units arrive in
// the new world.
wait<> intervention_forces_deployed_ui_seq(
    TS& ts, Colony const& colony, e_nation intervening );

// This doesn't move them, just animates them moving.
wait<> animate_move_intervention_units_into_colony(
    SS& ss, TS& ts, UnitId ship_id, Colony const& colony );

// This one will actually move them.
void move_intervention_units_into_colony( SS& ss, TS& ts,
                                          UnitId ship_id,
                                          Colony const& colony );

} // namespace rn
