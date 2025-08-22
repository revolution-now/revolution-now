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
struct IGui;
struct ILandViewPlane;
struct IMapUpdater;
struct IVisibility;
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
// These are exposed for testing.
namespace detail {

// Makes an assessment of all the colonist player's colonies and
// assigns each one a strength score.
RefColonySelectionMetrics ref_colony_selection_metrics(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    ColonyId const colony_id );

// A score of nothing means that it is ineligible, which can
// happen for a couple reasons (e.g., no available tiles for
// landing). Otherwise, the smaller the score, the more likely
// the colony is to be chosen.
base::maybe<int> ref_colony_selection_score(
    RefColonySelectionMetrics const& metrics );

base::maybe<RefColonySelectionMetrics const&>
select_ref_landing_colony(
    std::vector<RefColonyMetricsScored> const& choices
        ATTR_LIFETIMEBOUND );

// There must be at least one eligible landing site in the sup-
// plied parameter or check fail.
RefColonyLandingTiles select_ref_landing_tiles(
    RefColonySelectionMetrics const& metrics
        ATTR_LIFETIMEBOUND );

void filter_ref_landing_tiles( RefColonyLandingTiles& tiles );

bool is_initial_visit_to_colony(
    SSConst const& ss, RefColonySelectionMetrics const& metrics,
    IVisibility const& ref_viz );

[[nodiscard]] e_ref_manowar_availability
ensure_manowar_availability( SSConst const& ss,
                             e_nation nation );

e_ref_landing_formation select_ref_formation(
    RefColonySelectionMetrics const& metrics,
    bool initial_visit_to_colony );

RefLandingForce allocate_landing_units(
    SSConst const& ss, e_nation nation,
    e_ref_landing_formation formation );

RefLandingPlan make_ref_landing_plan(
    RefColonyLandingTiles const& landing_tiles,
    RefLandingForce const& force );

[[nodiscard]] RefLandingUnits create_ref_landing_units(
    SS& ss, e_nation nation, RefLandingPlan const& plan,
    ColonyId colony_id );

} // namespace detail

/****************************************************************
** REF Deployment Public Methods.
*****************************************************************/
// This is the full routine that creates the deployed REF troops,
// using the other functions in this module, which are exposed in
// the API so that they can be tested.
maybe<RefLandingUnits> produce_REF_landing_units(
    SS& ss, TerrainConnectivity const& connectivity,
    e_nation nation );

wait<> offboard_ref_units(
    SS& ss, IMapUpdater& map_updater, ILandViewPlane& land_view,
    IAgent& colonial_agent,
    RefLandingUnits const& landing_units );

/****************************************************************
** REF Winning/Forfeight.
*****************************************************************/
maybe<e_forfeight_reason> ref_should_forfeight(
    SSConst const& ss, Player const& ref_player );

void do_ref_forfeight( SS& ss, Player& ref_player );

wait<> ref_forfeight_ui_routine( SSConst const& ss, IGui& gui,
                                 Player const& ref_player );

int percent_ref_owned_population( SSConst const& ss,
                                  Player const& ref_player );

maybe<e_ref_win_reason> ref_should_win(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    Player const& ref_player );

void do_ref_win( SS& ss, Player const& ref_player );

wait<> ref_win_ui_routine( SSConst const& ss, IGui& gui,
                           Player const& ref_player,
                           e_ref_win_reason reason );

// Returns the number moved.
[[nodiscard]] int move_ref_harbor_ships_to_stock(
    SS& ss, Player& ref_player );

} // namespace rn
