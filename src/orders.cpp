/****************************************************************
**orders.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-27.
*
* Description: Handles the representation and application of
*              the orders that players can give to units.
*
*****************************************************************/
#include "orders.hpp"

// Revolution Now
#include "ownership.hpp"

// base-util
#include "base-util/variant.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

// C++ standard library
#include <queue>

using namespace std;

namespace rn {

namespace {

absl::flat_hash_map<UnitId, queue<PlayerUnitOrders>>
    g_orders_queue;

} // namespace

void push_unit_orders( UnitId                  id,
                       PlayerUnitOrders const& orders ) {
  g_orders_queue[id].push( orders );
}

Opt<PlayerUnitOrders> pop_unit_orders( UnitId id ) {
  Opt<PlayerUnitOrders> res{};
  if( has_key( g_orders_queue, id ) ) {
    auto& q = g_orders_queue[id];
    if( !q.empty() ) {
      res = q.front();
      q.pop();
    }
  }
  return res;
}

vector<UnitId> ProposedOrdersAnalysis::units_to_prioritize()
    const {
  vector<UnitId> res;

  switch_v( result ) {
    case_v( ProposedMetaOrderAnalysisResult ) {}
    case_v( ProposedMoveAnalysisResult ) {
      res = val.to_prioritize;
    }
    case_v( ProposedCombatAnalysisResult ) {
      res = val.to_prioritize;
    }
    case_v( ProposedJobAnalysisResult ) {}
    default_v;
  }

  return res;
}

ProposedOrdersAnalysis analyze_proposed_orders(
    UnitId id, PlayerUnitOrders const& orders ) {
  ProposedOrdersAnalysis analysis;
  analysis.id     = id;
  analysis.orders = orders;

  auto const& unit = unit_from_id( id );
  CHECK( unit.movement_points() > 0 );

  switch_v( orders ) {
    case_v( orders::quit_t ) {}
    case_v( orders::wait_t ) {
      analysis.result = ProposedMetaOrderAnalysisResult{false};
    }
    case_v( orders::forfeight_t ) {
      analysis.result = ProposedMetaOrderAnalysisResult{true};
    }
    case_v_( orders::move, direction ) {
      auto mv_res     = analyze_proposed_move( id, direction );
      analysis.result = mv_res;
      if( mv_res.allowed() ) {
        // move is physically possible, so check attack.
        auto maybe_nation =
            nation_from_coord( mv_res.move_target );
        if( maybe_nation && *maybe_nation != unit.nation() ) {
          analysis.result =
              analyze_proposed_attack( id, direction );
        }
      }
    }
    default_v;
  }

  return analysis;
}

bool confirm_explain_orders(
    ProposedOrdersAnalysis const& analysis ) {
  bool res = false;
  switch_v( analysis.result ) {
    // Orders about orders
    case_v( ProposedMetaOrderAnalysisResult ) { res = true; }
    // If unit is to move on the map
    case_v( ProposedMoveAnalysisResult ) {
      res = confirm_explain_move( val );
    }
    // If this is combat.
    case_v( ProposedCombatAnalysisResult ) {
      res = confirm_explain_attack( val );
    }
    // actions in current tile
    case_v( ProposedJobAnalysisResult ) { res = true; }
    default_v;
  }
  return res;
}

void apply_orders( ProposedOrdersAnalysis const& analysis ) {
  auto  id   = analysis.id;
  auto& unit = unit_from_id( id );

  switch_v( analysis.result ) {
    case_v( ProposedMetaOrderAnalysisResult ) {
      /* Always allowed */
      if( val.mv_points_forfeighted ) unit.forfeight_mv_points();
    }
    case_v( ProposedMoveAnalysisResult ) {
      CHECK( val.allowed() );
      move_unit( val );
    }
    case_v( ProposedCombatAnalysisResult ) {
      CHECK( val.allowed() );
      run_combat( val );
    }
    case_v( ProposedJobAnalysisResult ) {
      CHECK( val.allowed() );
    }
    default_v;
  }
}

} // namespace rn
