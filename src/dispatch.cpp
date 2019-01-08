/****************************************************************
**dispatch.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-08.
*
* Description: Dispatches player orders to the right handler.
*
*****************************************************************/
#include "dispatch.hpp"

// Revolution Now
#include "ownership.hpp"

// base-util
#include "base-util/misc.hpp"
#include "base-util/variant.hpp"

using namespace std;

namespace rn {

namespace {

template<typename T>
auto try_dispatch( UnitId id, Orders const& orders ) {
  return T::analyze( id, orders );
}

} // namespace

Opt<OrdersAnalysisDispatch> dispatch_orders(
    UnitId id, Orders const& orders ) {
  Opt<OrdersAnalysisDispatch> maybe_analysis;

  auto const& unit = unit_from_id( id );
  CHECK( unit.movement_points() > 0 );

  if( ( maybe_analysis =
            try_dispatch<MetaAnalysis>( id, orders ) ) )
    return maybe_analysis;

  if( ( maybe_analysis =
            try_dispatch<TravelAnalysis>( id, orders ) ) )
    return maybe_analysis;

  // case_v_( orders::move, direction ) {
  //  auto mv_res     = analyze_proposed_move( id, direction );
  //  analysis.result = mv_res;
  //  if( mv_res.allowed() ) {
  //    // move is physically possible, so check attack.
  //    auto maybe_nation =
  //        nation_from_coord( mv_res.move_target );
  //    if( maybe_nation && *maybe_nation != unit.nation() ) {
  //      analysis.result =
  //          analyze_proposed_attack( id, direction );
  //    }
  //  }
  //}

  return nullopt;
}

bool confirm_explain( OrdersAnalysisDispatch const& analysis ) {
  return util::visit( analysis, L( _.confirm_explain() ) );
}

void affect_orders( OrdersAnalysisDispatch const& analysis ) {
  util::visit( analysis, L( _.affect_orders() ) );
}

vector<UnitId> units_to_prioritize(
    OrdersAnalysisDispatch const& analysis ) {
  return util::visit( analysis, L( _.units_to_prioritize ) );
}

} // namespace rn
