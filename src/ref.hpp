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
struct IEuroMind;
struct Player;
struct SSConst;

enum class e_expeditionary_force_type;
enum class e_unit_type;

/****************************************************************
** Public API
*****************************************************************/
// Called once per turn to evolve the royal money. Note that this
// does not include the adjustments made due to tax revenue,
// which are done immediately when tax revenue is received and
// are handled in their respective modules.
[[nodiscard]] RoyalMoneyChange evolved_royal_money(
    SSConst const& ss, Player const& player );

void apply_royal_money_change( Player& player,
                               RoyalMoneyChange const& change );

[[nodiscard]] e_expeditionary_force_type select_next_ref_type(
    ExpeditionaryForce const& force );

void add_ref_unit( ExpeditionaryForce& force,
                   e_expeditionary_force_type type );

e_unit_type ref_unit_to_unit_type(
    e_expeditionary_force_type type );

wait<> add_ref_unit_ui_seq( IEuroMind& mind,
                            e_expeditionary_force_type type );

} // namespace rn
