/****************************************************************
**analysis.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-08.
*
* Description: Analyzes player orders on units.
*
*****************************************************************/
#include "analysis.hpp"

// Revolution Now
#include "aliases.hpp"
#include "ownership.hpp"

// base-util
#include "base-util/variant.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

Opt<MetaAnalysis> MetaAnalysis::analyze_( UnitId id,
                                          Orders orders ) {
  if( util::holds<orders::wait_t>( orders ) )
    return MetaAnalysis( id, orders,
                         /*mv_points_forfeighted=*/false );
  if( util::holds<orders::forfeight_t>( orders ) )
    return MetaAnalysis( id, orders,
                         /*mv_points_forfeighted=*/true );
  return nullopt;
}

void MetaAnalysis::affect_orders_() const {
  if( mv_points_forfeighted )
    unit_from_id( id ).forfeight_mv_points();
}

} // namespace rn
