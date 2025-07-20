/****************************************************************
**ref.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-18.
*
* Description: Handles the REF forces.
*
*****************************************************************/
#pragma once

// rds
#include "ref.rds.hpp"

// Revolution Now
#include "wait.hpp"

// ss
#include "ss/revolution.rds.hpp"

namespace rn {

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct ExpeditionaryForce;
struct IAgent;
struct ILandViewPlane;
struct IMapUpdater;
struct Player;
struct SS;
struct SSConst;
struct TerrainConnectivity;

enum class e_difficulty;
enum class e_expeditionary_force_type;
enum class e_nation;
enum class e_player;
enum class e_unit_type;

/****************************************************************
** Royal Money.
*****************************************************************/
// Called once per turn to evolve the royal money. Note that this
// does not include the adjustments made due to tax revenue,
// which are done immediately when tax revenue is received and
// are handled in their respective modules.
[[nodiscard]] RoyalMoneyChange evolved_royal_money(
    e_difficulty difficulty, int royal_money );

void apply_royal_money_change( Player& player,
                               RoyalMoneyChange const& change );

/****************************************************************
** REF Unit Addition.
*****************************************************************/
[[nodiscard]] e_expeditionary_force_type select_next_ref_type(
    ExpeditionaryForce const& force );

void add_ref_unit( ExpeditionaryForce& force,
                   e_expeditionary_force_type type );

e_unit_type ref_unit_to_unit_type(
    e_expeditionary_force_type type );

wait<> add_ref_unit_ui_seq( IAgent& agent,
                            e_expeditionary_force_type type );

/****************************************************************
** REF Unit Deployment.
*****************************************************************/
std::vector<ColonyId> find_coastal_colonies(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    e_player player );

// Makes an assessment of all the colonist player's colonies and
// assigns each one a strength score.
RefColonySelectionMetrics ref_colony_selection_metrics(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    ColonyId const colony_id );

// A score of nothing means that it is ineligible, which can
// happen for a couple reasons (e.g., no available tiles for
// landing).
base::maybe<int> ref_colony_selection_score(
    RefColonySelectionMetrics const& metrics );

base::maybe<RefColonySelectionMetrics const&>
select_ref_landing_colony(
    std::vector<RefColonyMetricsScored> const& choices
        ATTR_LIFETIMEBOUND );

base::maybe<RefColonyLandingTiles const&>
select_ref_landing_tiles( RefColonySelectionMetrics const&
                              metrics ATTR_LIFETIMEBOUND );

e_ref_landing_formation select_ref_formation(
    RefColonySelectionMetrics const& metrics );

RefLandingForce select_landing_units(
    SSConst const& ss, e_nation nation,
    e_ref_landing_formation formation );

RefLandingPlan make_ref_landing_plan(
    RefColonyLandingTiles const& landing_tiles,
    RefLandingForce const& force );

[[nodiscard]] RefLandingUnits create_ref_landing_units(
    SS& ss, e_nation nation, RefLandingPlan const& plan );

wait<> offboard_ref_units(
    SS& ss, IMapUpdater& map_updater, ILandViewPlane& land_view,
    IAgent& colonial_agent,
    RefLandingUnits const& landing_units );

} // namespace rn
