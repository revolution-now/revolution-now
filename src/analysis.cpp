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
#include "cstate.hpp"
#include "ustate.hpp"
#include "variant.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

maybe<MetaAnalysis> MetaAnalysis::analyze_( UnitId   id,
                                          orders_t orders ) {
  if( holds<orders::wait>( orders ) )
    return MetaAnalysis( id, orders,
                         /*mv_points_forfeighted=*/false );
  if( holds<orders::forfeight>( orders ) )
    return MetaAnalysis( id, orders,
                         /*mv_points_forfeighted=*/true );
  return nothing;
}

void MetaAnalysis::affect_orders_() const {
  if( mv_points_forfeighted )
    unit_from_id( id ).forfeight_mv_points();
}

maybe<e_nation> nation_from_coord( Coord coord ) {
  if( auto maybe_colony_id = colony_from_coord( coord );
      maybe_colony_id )
    return colony_from_id( *maybe_colony_id ).nation();

  auto const& units = units_from_coord( coord );
  if( units.empty() ) return nothing;
  e_nation first = unit_from_id( *units.begin() ).nation();
  for( auto const& id : units ) {
    (void)id; // for release builds.
    DCHECK( first == unit_from_id( id ).nation() );
  }
  return first;
}

} // namespace rn
