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

using namespace std;

namespace rn {

namespace {} // namespace

vector<UnitId> ProposedOrdersAnalysis::units_to_prioritize()
    const {
  vector<UnitId> res;

  switch_v( result ) {
    case_v( ProposedMetaOrderAnalysisResult ) {}
    case_v( ProposedMoveAnalysisResult ) {
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
  analysis.orders = orders;

  switch_v( orders ) {
    case_v( orders::quit_t ) {}
    case_v( orders::wait_t ) {
      analysis.result = ProposedMetaOrderAnalysisResult{false};
    }
    case_v( orders::forfeight_t ) {
      analysis.result = ProposedMetaOrderAnalysisResult{true};
    }
    case_v_( orders::move, direction ) {
      analysis.result = analyze_proposed_move( id, direction );
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
    // actions in current tile
    case_v( ProposedJobAnalysisResult ) { res = true; }
    default_v;
  }
  return res;
}

void apply_orders( UnitId                        id,
                   ProposedOrdersAnalysis const& analysis ) {
  auto& unit = unit_from_id( id );

  switch_v( analysis.result ) {
    case_v( ProposedMetaOrderAnalysisResult ) {
      /* Always allowed */
      if( val.mv_points_forfeighted ) unit.forfeight_mv_points();
    }
    case_v( ProposedMoveAnalysisResult ) {
      CHECK( val.allowed() );
      move_unit( id, val );
    }
    case_v( ProposedJobAnalysisResult ) {
      CHECK( val.allowed() );
    }
    default_v;
  }
}

} // namespace rn
