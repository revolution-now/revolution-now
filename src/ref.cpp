/****************************************************************
**ref.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-18.
*
* Description: Handles the REF forces.
*
*****************************************************************/
#include "ref.hpp"
#include "revolution.rds.hpp"

using namespace std;

// Revolution Now
#include "co-wait.hpp"
#include "ieuro-mind.hpp"

// config
#include "config/revolution.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"

// C++ standard library
#include <ranges>

namespace rg = std::ranges;

namespace rn {

namespace {

using ::refl::enum_map;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
RoyalMoneyChange evolved_royal_money(
    e_difficulty const difficulty, int const royal_money ) {
  RoyalMoneyChange res;
  res.old_value         = royal_money;
  res.per_turn_increase = config_revolution.royal_money
                              .constant_per_turn[difficulty];
  res.new_value = res.old_value + res.per_turn_increase;
  int const threshold =
      config_revolution.royal_money.threshold_for_new_ref;
  if( res.new_value >= threshold ) {
    res.new_value -= threshold;
    res.new_unit_produced = true;
    res.amount_subtracted = threshold;
  }
  // NOTE: the new_value may still be larger than `threshold`
  // here, since we produce at most one new unit per turn.
  return res;
}

void apply_royal_money_change( Player& player,
                               RoyalMoneyChange const& change ) {
  player.royal_money = change.new_value;
}

e_expeditionary_force_type select_next_ref_type(
    ExpeditionaryForce const& force ) {
  using enum e_expeditionary_force_type;
  int const total = force.regular + force.cavalry +
                    force.artillery + force.man_o_war;
  if( total == 0 ) return man_o_war;
  enum_map<e_expeditionary_force_type, double> metrics;
  CHECK_GT( total, 0 );
  // Get a positive metric that measures how much in need one
  // unit type is relative to the others in order for the overall
  // distribution to look like the target.
  auto const metric = [&]( double& dst, int const n,
                           int const target ) {
    CHECK_GE( n, 0 );
    // Should have been verified when loading configs.
    CHECK_GE( target, 0 );
    double const ratio = double( n ) / total;
    dst                = target / ratio;
  };
  metric(
      metrics[regular], force.regular,
      config_revolution.ref_forces.target_ratios.ratio[regular]
          .percent );
  metric(
      metrics[cavalry], force.cavalry,
      config_revolution.ref_forces.target_ratios.ratio[cavalry]
          .percent );
  metric(
      metrics[artillery], force.artillery,
      config_revolution.ref_forces.target_ratios.ratio[artillery]
          .percent );
  metric(
      metrics[man_o_war], force.man_o_war,
      config_revolution.ref_forces.target_ratios.ratio[man_o_war]
          .percent );

  vector<pair<e_expeditionary_force_type, double>>
      sorted_metrics = metrics;
  // Sort largest to smallest.
  rg::stable_sort( sorted_metrics,
                   []( auto const& l, auto const& r ) {
                     return r.second < l.second;
                   } );

  CHECK( !sorted_metrics.empty() );
  auto const& [type, _] = sorted_metrics[0];
  return type;
}

void add_ref_unit( ExpeditionaryForce& force,
                   e_expeditionary_force_type const type ) {
  using enum e_expeditionary_force_type;
  switch( type ) {
    case regular: {
      ++force.regular;
      break;
    }
    case cavalry: {
      ++force.cavalry;
      break;
    }
    case artillery: {
      ++force.artillery;
      break;
    }
    case man_o_war: {
      ++force.man_o_war;
      break;
    }
  }
}

e_unit_type ref_unit_to_unit_type(
    e_expeditionary_force_type const type ) {
  using enum e_expeditionary_force_type;
  switch( type ) {
    case regular:
      return e_unit_type::regular;
    case cavalry:
      return e_unit_type::cavalry;
    case artillery:
      return e_unit_type::artillery;
    case man_o_war:
      return e_unit_type::man_o_war;
  }
}

wait<> add_ref_unit_ui_seq(
    IEuroMind& mind,
    e_expeditionary_force_type const ref_type ) {
  e_unit_type const unit_type =
      ref_unit_to_unit_type( ref_type );
  auto const& plural_name =
      config_unit_type.composition.unit_types[unit_type]
          .name_plural;
  string const msg = format(
      "The King has announced an increase to the Royal military "
      "budget. [{}] have been added to the Royal Expeditionary "
      "Force, causing alarm among colonists.",
      plural_name );
  co_await mind.message_box( "{}", msg );
}

} // namespace rn
